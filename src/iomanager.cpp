#include <unistd.h>
#include <sys/epoll.h>
#include <fcntl.h>

#include "macro.h"
#include "iomanager.h"
#include "log.h"

namespace MNSER {

static MNSER::Logger::ptr g_logger = MS_LOG_NAME("system");

enum EpollCtlOp {
};

static std::ostream& operator<< (std::ostream& os, const EpollCtlOp& op) {
    switch((int)op) {
#define XX(ctl) \
        case ctl: \
            return os << #ctl;
        XX(EPOLL_CTL_ADD);
        XX(EPOLL_CTL_MOD);
        XX(EPOLL_CTL_DEL);
        default:
            return os << (int)op;
    }
#undef XX
}

static std::ostream& operator<< (std::ostream& os, EPOLL_EVENTS events) {
    if(!events) {
        return os << "0";
    }
    bool first = true;
#define XX(E) \
    if(events & E) { \
        if(!first) { \
            os << "|"; \
        } \
        os << #E; \
        first = false; \
    }
    XX(EPOLLIN);
    XX(EPOLLPRI);
    XX(EPOLLOUT);
    XX(EPOLLRDNORM);
    XX(EPOLLRDBAND);
    XX(EPOLLWRNORM);
    XX(EPOLLWRBAND);
    XX(EPOLLMSG);
    XX(EPOLLERR);
    XX(EPOLLHUP);
    XX(EPOLLRDHUP);
    XX(EPOLLONESHOT);
    XX(EPOLLET);
#undef XX
    return os;
}

// 获取事件上下文
IOManager::FdContext::EventContext& IOManager::FdContext::getContext(IOManager::Event event) {
	// 放回事件中包含的上下文
    switch(event) {
        case IOManager::READ:
            return read;
        case IOManager::WRITE:
            return write;
        default:
            MS_ASSERT2(false, "getContext");
    }
    throw std::invalid_argument("getContext invalid event");
}
		
// 重置事件上下位
void IOManager::FdContext::resetContext(IOManager::FdContext::EventContext& ctx) {
    ctx.scheduler = nullptr;
    ctx.fiber.reset();
    ctx.func = nullptr;
}

// 触发事件
void IOManager::FdContext::triggerEvent(Event event) {
    MS_ASSERT(curEvents & event);  // 首先应该满足现在上下文中有这个事件
    curEvents = (Event)(curEvents & ~event);  // 这个事件要处理了，所以从当前事件中提取出去
    EventContext& ctx = getContext(event);
    if(ctx.func) {
        ctx.scheduler->schedule(&ctx.func);
    } else {
        ctx.scheduler->schedule(&ctx.fiber);
    }
    ctx.scheduler = nullptr;
    return;
}

IOManager::IOManager(size_t n_threads, bool use_caller, const std::string& name)
	:Scheduler(n_threads, use_caller, name) {
    m_epfd = epoll_create(5000);  // 创建 epoll
    MS_ASSERT(m_epfd > 0);

    int rt = pipe(m_tickleFds);
    MS_ASSERT(!rt);

    epoll_event event;
    memset(&event, 0, sizeof(epoll_event));
    event.events = EPOLLIN | EPOLLET;  // 读 边缘触发
    event.data.fd = m_tickleFds[0];

    rt = fcntl(m_tickleFds[0], F_SETFL, O_NONBLOCK);
    MS_ASSERT(!rt);

    rt = epoll_ctl(m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event);  // m_tickleFds[0] 设置成读管道的接口，那么 m_tickleFds[1] 将会设置成写端口
    MS_ASSERT(!rt);

    contextResize(32);

    start(); // 初始化直接开始
}

IOManager::~IOManager() {
	// 析构思路：先停止调度器，然后关闭文件描述符，析构空间
    stop();  
    close(m_epfd);
    close(m_tickleFds[0]);
    close(m_tickleFds[1]);

    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(m_fdContexts[i]) {
            delete m_fdContexts[i];
        }
    }
}

// 添加事件 fd 描述符句柄，event 事件类型，cb 回调函数，成功返回0失败返回-1
int IOManager::addEvent(int fd, Event event, std::function<void()> cb) {
    FdContext* fd_ctx = nullptr; // 指向需要追加事件的描述符上下文
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() > fd) {
        fd_ctx = m_fdContexts[fd];
        lock.unlock();
    } else {
        lock.unlock();
        RWMutexType::WriteLock lock2(m_mutex);
        contextResize(fd * 1.5);
        fd_ctx = m_fdContexts[fd];
    }

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(MS_UNLIKELY(fd_ctx->curEvents & event)) { // 原来应该没有这个事件
        MS_LOG_ERROR(g_logger) << "addEvent assert fd=" << fd
                    << " event=" << (EPOLL_EVENTS)event
                    << " fd_ctx.event=" << (EPOLL_EVENTS)fd_ctx->curEvents;
        MS_ASSERT(!(fd_ctx->curEvents & event));
    }

    int op = fd_ctx->curEvents ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
    epoll_event epevent;
    epevent.events = EPOLLET | fd_ctx->curEvents | event;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        MS_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ") fd_ctx->events="
            << (EPOLL_EVENTS)fd_ctx->curEvents;
        return -1;
    }

    ++m_pendingEventCount;
    fd_ctx->curEvents = (Event)(fd_ctx->curEvents | event);
	// 这里用的是 fd_ctx 的成员函数，那么返回的就是 fd_ctx 的read或者write
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event); 
	// 新加进来的事件，这三个应该都是空的
    MS_ASSERT(!event_ctx.scheduler
                && !event_ctx.fiber
                && !event_ctx.func);

    event_ctx.scheduler = Scheduler::GetThis(); // 事件调度器，就使用当下这个调度器
	// 如果设置了回调函数，那么就给事件设置上，如果没有，就设置协程为当前协程
    if(cb) {
        event_ctx.func.swap(cb);
    } else {
        event_ctx.fiber = Fiber::GetThis();
        MS_ASSERT2(event_ctx.fiber->getState() == Fiber::EXEC
                      ,"state=" << event_ctx.fiber->getState());
    }
    return 0;
}

// 删除事件，fd 描述符句柄，event 事件类型
bool IOManager::delEvent(int fd, Event event) {
	// 先取出 FdContext
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
	// 删除事件，原来的事件中应该有这个事件
    if(MS_UNLIKELY(!(fd_ctx->curEvents & event))) {
        return false;
    }

    Event new_events = (Event)(fd_ctx->curEvents & ~event); // 删除事件
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        MS_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    --m_pendingEventCount; // 待做事件减一
    fd_ctx->curEvents = new_events;
    FdContext::EventContext& event_ctx = fd_ctx->getContext(event);  // 取出原来事件
    fd_ctx->resetContext(event_ctx); // 清楚事件的内容
    return true;
}

// 取消事件，fd 描述符句柄，event 事件类型
bool IOManager::cancelEvent(int fd, Event event) {
	// 取 FdContext
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(MS_UNLIKELY(!(fd_ctx->curEvents & event))) {
        return false;
    }

    Event new_events = (Event)(fd_ctx->curEvents & ~event);
    int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = EPOLLET | new_events;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        MS_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    fd_ctx->triggerEvent(event); // 重新触发事件
    --m_pendingEventCount;
    return true;
}

// 取消所有事件，fd 描述符句柄
bool IOManager::cancelAll(int fd) {
    RWMutexType::ReadLock lock(m_mutex);
    if((int)m_fdContexts.size() <= fd) {
        return false;
    }
    FdContext* fd_ctx = m_fdContexts[fd];
    lock.unlock();

    FdContext::MutexType::Lock lock2(fd_ctx->mutex);
    if(!fd_ctx->curEvents) {
        return false;
    }
	// 直接清楚所有事件就行
    int op = EPOLL_CTL_DEL;
    epoll_event epevent;
    epevent.events = 0;
    epevent.data.ptr = fd_ctx;

    int rt = epoll_ctl(m_epfd, op, fd, &epevent);
    if(rt) {
        MS_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
            << (EpollCtlOp)op << ", " << fd << ", " << (EPOLL_EVENTS)epevent.events << "):"
            << rt << " (" << errno << ") (" << strerror(errno) << ")";
        return false;
    }

    if(fd_ctx->curEvents & READ) {
        fd_ctx->triggerEvent(IOManager::READ);
        --m_pendingEventCount;
    }
    if(fd_ctx->curEvents & WRITE) {
        fd_ctx->triggerEvent(IOManager::WRITE);
        --m_pendingEventCount;
    }

    MS_ASSERT(fd_ctx->curEvents == 0);  // 事件清除完毕
    return true;
}

// 返回当前指向的 IOManager
IOManager* IOManager::GetThis() {
    return dynamic_cast<IOManager*>(Scheduler::GetThis());
}

void IOManager::tickle() {
    if(!hasIdleThreads()) {
        return;
    }
    int rt = write(m_tickleFds[1], "T", 1);
    MS_ASSERT(rt == 1);  // 向管道中写一个字母，表示需要处理了
}

bool IOManager::stopping() {
    uint64_t timeout = 0;
    return stopping(timeout);
}

void IOManager::idle() {
    MS_LOG_DEBUG(g_logger) << "iomanager idle";
    const uint64_t MAX_EVENTS = 256;  // 最大等待事件
    epoll_event* events = new epoll_event[MAX_EVENTS](); // 加 () 表示调用构造函数
    std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr){
        delete[] ptr;
    });

    while(true) {
        uint64_t next_timeout = 0;
        if(MS_UNLIKELY(stopping(next_timeout))) {  // 如果停止了 就break
            MS_LOG_INFO(g_logger) << "name=" << getName()
                                     << " idle stopping exit";
            break;
        }
		//std::cout << "### next_timeout= " << next_timeout << std::endl;
        int rt = 0;
        do {
            static const int MAX_TIMEOUT = 3000;
            if(next_timeout != ~0ull) {
                next_timeout = (int)next_timeout > MAX_TIMEOUT
                                ? MAX_TIMEOUT : next_timeout;
            } else {
                next_timeout = MAX_TIMEOUT;
            }
            rt = epoll_wait(m_epfd, events, MAX_EVENTS, (int)next_timeout);
            if(rt < 0 && errno == EINTR) {
            } else {
                break;
            }
        } while(true);

        std::vector<std::function<void()> > cbs;
        listExpiredCb(cbs);  // 展示需要定时器回调的函数列表
		//std::cout << "cbs.size() = " << cbs.size() << " rt = " << rt << " next_timeout = " << next_timeout << std::endl;

        if(!cbs.empty()) {
            schedule(cbs.begin(), cbs.end());
            cbs.clear();
        }

        for(int i = 0; i < rt; ++i) {
            epoll_event& event = events[i];
            if(event.data.fd == m_tickleFds[0]) {
                uint8_t dummy[256];
                while(read(m_tickleFds[0], dummy, sizeof(dummy)) > 0);
                continue;
            }

            FdContext* fd_ctx = (FdContext*)event.data.ptr; // 取出事件
            FdContext::MutexType::Lock lock(fd_ctx->mutex);
            if(event.events & (EPOLLERR | EPOLLHUP)) {
                event.events |= (EPOLLIN | EPOLLOUT) & fd_ctx->curEvents;
            }
            int real_events = NONE;
            if(event.events & EPOLLIN) {
                real_events |= READ;
            }
            if(event.events & EPOLLOUT) {
                real_events |= WRITE;
            }

            if((fd_ctx->curEvents & real_events) == NONE) {
                continue;
            }

            int left_events = (fd_ctx->curEvents & ~real_events);
            int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
            event.events = EPOLLET | left_events;

            int rt2 = epoll_ctl(m_epfd, op, fd_ctx->fd, &event);
            if(rt2) {
                MS_LOG_ERROR(g_logger) << "epoll_ctl(" << m_epfd << ", "
                    << (EpollCtlOp)op << ", " << fd_ctx->fd << ", " << (EPOLL_EVENTS)event.events << "):"
                    << rt2 << " (" << errno << ") (" << strerror(errno) << ")";
                continue;
            }

            if(real_events & READ) {
                fd_ctx->triggerEvent(READ);
                --m_pendingEventCount;
            }
            if(real_events & WRITE) {
                fd_ctx->triggerEvent(WRITE);
                --m_pendingEventCount;
            }
        }

        Fiber::ptr cur = Fiber::GetThis();
        auto raw_ptr = cur.get();
        cur.reset();

        raw_ptr->swapOut();
    }
}

void IOManager::onTimerInsertedAtFront() {
    tickle();
}


// 重置 socket 大小
void IOManager::contextResize(size_t size) {
    m_fdContexts.resize(size);

    for(size_t i = 0; i < m_fdContexts.size(); ++i) {
        if(!m_fdContexts[i]) {
            m_fdContexts[i] = new FdContext;
            m_fdContexts[i]->fd = i;
        }
    }
}

// 判断是否可以停止，timeout 最近要触发的定时器事件间隔
bool IOManager::stopping(uint64_t& timeout) {
	//std::cout << "### iomanager stopping" << std::endl;
    timeout = getNextTimer();
	//std::cout << "436 line " << timeout << std::endl;
	//std::cout << "### timeout=" << timeout << " m_id= " <<MNSER::GetFiberId()<< 
	//	"m_pendingEventCount = " << m_pendingEventCount 
	//	 << " m_activeThreadCount= " << m_activeThreadCount 
	//	 << std::endl;
    return timeout == ~0ull
        && m_pendingEventCount == 0
        && Scheduler::stopping();
}

}
