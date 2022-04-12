#include <dlfcn.h>

#include "hook.h"
#include "log.h"
#include "fiber.h"
#include "iomanager.h"
#include "macro.h"
#include "config.h"
#include "fd_manager.h"  // FdCtx 主要作用就是判断这个文件是不是 socket，是不是用户 设置了 NonLock

MNSER::Logger::ptr g_logger = MS_LOG_NAME("system");

namespace  MNSER {
static MNSER::ConfigVar<int>::ptr g_tcp_connect_timeout =
    MNSER::Config::Lookup("tcp.connect.timeout", 5000, "tcp connect timeout");

static thread_local bool t_hook_enable = false;

bool is_hook_enable() {
	return t_hook_enable;
}

#define HOOK_FUN(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(ioctl) \
    XX(getsockopt) \
    XX(setsockopt)

void hook_init() {

	static bool is_inited = false;
	if (is_inited) {
		return ;
	}
// dlsym动态库取函数
// dlsym 取出对应函数的地址
#define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);  
    HOOK_FUN(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter {
    _HookIniter() {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();  // 为了设置统一的超时时间

        g_tcp_connect_timeout->addListener([](const int& old_value,
			const int& new_value){
                MS_LOG_INFO(g_logger) << "tcp connect timeout changed from "
                                         << old_value << " to " << new_value;
                s_connect_timeout = new_value;
        });
    }
};

static _HookIniter s_hook_initer;  // 为了在 main 之前初始化

void set_hook_enable(bool flag) {
    t_hook_enable = flag;
}

}

struct timer_info {  // 存放条件
    int cancelled = 0;
};

// nowan
template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name,
        uint32_t event, int timeout_so, Args&&... args) {
    if(!MNSER::t_hook_enable) {  							// 不使用 hook
        return fun(fd, std::forward<Args>(args)...);
    }

    MNSER::FdCtx::ptr ctx = MNSER::FdMgr::GetInstance()->get(fd);
    if(!ctx) {  											// 不存在，就不是 socket
        return fun(fd, std::forward<Args>(args)...);
    }

    if(ctx->isClose()) { 									// 存在，但是这个是 关闭的  是文件句柄，不执行
        errno = EBADF;
        return -1;
    }

    if(!ctx->isSocket() || ctx->getUserNonblock()) { 		// 用户设置非阻塞了,那就执行自己就行了
        return fun(fd, std::forward<Args>(args)...);
    }

    uint64_t to = ctx->getTimeout(timeout_so);  			// 获取超时时间
    std::shared_ptr<timer_info> tinfo(new timer_info);  	// 创建一个条件

retry:
    ssize_t n = fun(fd, std::forward<Args>(args)...);  		// 尝试读数据
    while(n == -1 && errno == EINTR) {  					// 如果没有读到数据，但是是被系统中断
        n = fun(fd, std::forward<Args>(args)...);
    }
	// EAGAIN 出现在连续读几次，但是没有读出错误的情况下
    if(n == -1 && errno == EAGAIN) {  						// 重试几次还是没有读到数据
        MNSER::IOManager* iom = MNSER::IOManager::GetThis(); 		// 首先取出 iomanager
        MNSER::Timer::ptr timer;	
        std::weak_ptr<timer_info> winfo(tinfo);  					// 声明一个条件
		// 超时时间 不等于 -1,就表示设置的有超时的情况，这种情况自己就设置一个条件定时器
        if(to != (uint64_t)-1) {  									
            timer = iom->addConditionTimer(to, 						// 等待 to 这么长的超时时间
				[winfo, fd, iom, event]() {  						// 条件定时器就是表示，等待 to 毫秒，如果
                auto t = winfo.lock();								// 如果条件还成立
                if(!t || t->cancelled) {							// 如果条件过时，或者取消了，就直接返回 
                    return;
                }
                t->cancelled = ETIMEDOUT;							// 否则设置超时错误,取消这个事件
                iom->cancelEvent(fd, (MNSER::IOManager::Event)(event)); 
            }, winfo);
        }
		// 没有设置超时时间或者设置了条件定时器成功
        int rt = iom->addEvent(fd, (MNSER::IOManager::Event)(event));  
        if(MS_UNLIKELY(rt)) {
            MS_LOG_ERROR(g_logger) << hook_fun_name << " addEvent("
                << fd << ", " << event << ")";
            if(timer) {												// 如果设置了条件定时器，但是这里添加时间错误了，就取消这个定时器
                timer->cancel();
            }
            return -1;
        } else { 													// 事件添加成功
            MNSER::Fiber::YieldToHold();  							// 如果执行成功，让出协程
			// 等到有事件回来的时候，就会返回到这里
            if(timer) {												// 如果有定时器，就取消掉定时器
                timer->cancel();									// 取消点这个之后就不会执行定时器中取消事件的
            }
            if(tinfo->cancelled) {									// 这就是表示是定时事件回来的，就设置错误，返回
                errno = tinfo->cancelled;
                return -1;
            }
            goto retry;												// 这就表示有数据回来了，就去读取
        }
    }
	// 这里表示读出了数据
	// 直接返回数据就行
    
    return n;
}

extern "C" {
#define XX(name) name ## _fun name ## _f = nullptr;
    HOOK_FUN(XX);
#undef XX

// sleep 
// 使用一个协程来完成定时功能
unsigned int sleep(unsigned int seconds) {
    if(!MNSER::t_hook_enable) {
        return sleep_f(seconds);
    }
	
    MNSER::Fiber::ptr fiber = MNSER::Fiber::GetThis();
    MNSER::IOManager* iom = MNSER::IOManager::GetThis();
    iom->addTimer(seconds * 1000, std::bind((void(MNSER::Scheduler::*)
            (MNSER::Fiber::ptr, int thread))&MNSER::IOManager::schedule
            ,iom, fiber, -1));
	//std::cout << "### sleep " << 1000*seconds<< std::endl;
    MNSER::Fiber::YieldToHold();
    return 0;
}

int usleep(useconds_t usec) {
    if(!MNSER::t_hook_enable) {
        return usleep_f(usec);
    }
    MNSER::Fiber::ptr fiber = MNSER::Fiber::GetThis();
    MNSER::IOManager* iom = MNSER::IOManager::GetThis();
    iom->addTimer(usec / 1000, std::bind((void(MNSER::Scheduler::*)
            (MNSER::Fiber::ptr, int thread))&MNSER::IOManager::schedule
            ,iom, fiber, -1));
    MNSER::Fiber::YieldToHold();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if(!MNSER::t_hook_enable) {
        return nanosleep_f(req, rem);
    }

    int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 /1000;
    MNSER::Fiber::ptr fiber = MNSER::Fiber::GetThis();
    MNSER::IOManager* iom = MNSER::IOManager::GetThis();
    iom->addTimer(timeout_ms, std::bind((void(MNSER::Scheduler::*)
            (MNSER::Fiber::ptr, int thread))&MNSER::IOManager::schedule
            ,iom, fiber, -1));
    MNSER::Fiber::YieldToHold();
    return 0;
}

// socket
int socket(int domain, int type, int protocol) {
    if(!MNSER::t_hook_enable) {
        return socket_f(domain, type, protocol);
    }
    int fd = socket_f(domain, type, protocol);
    if(fd == -1) {
        return fd;
    }
    MNSER::FdMgr::GetInstance()->get(fd, true);
    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms) {
    if(!MNSER::t_hook_enable) {
        return connect_f(fd, addr, addrlen);
    }

    MNSER::FdCtx::ptr ctx = MNSER::FdMgr::GetInstance()->get(fd);
    if(!ctx || ctx->isClose()) {  // 如果文件描述符关闭，或者没有这个文件描述符，返回错误
        errno = EBADF;
        return -1;
    }

    if(!ctx->isSocket()) { // 如果不是 socket，直接返回
        return connect_f(fd, addr, addrlen);
    }

    if(ctx->getUserNonblock()) {  // 用户已经自己设置过
        return connect_f(fd, addr, addrlen);
    }

    int n = connect_f(fd, addr, addrlen);  // 开始连接
    if(n == 0) {
        return 0;
    } else if(n != -1 || errno != EINPROGRESS) {
        return n;
    }

    MNSER::IOManager* iom = MNSER::IOManager::GetThis();
    MNSER::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    if(timeout_ms != (uint64_t)-1) {  // 如果设置的有超时
        timer = iom->addConditionTimer(timeout_ms, [winfo, fd, iom]() {
                auto t = winfo.lock();
                if(!t || t->cancelled) {
                    return;
                }
                t->cancelled = ETIMEDOUT;
                iom->cancelEvent(fd, MNSER::IOManager::WRITE);
        }, winfo);
    }

    int rt = iom->addEvent(fd, MNSER::IOManager::WRITE);
    if(rt == 0) {
        MNSER::Fiber::YieldToHold();
        if(timer) {
            timer->cancel();
        }
        if(tinfo->cancelled) {
            errno = tinfo->cancelled;  // 都取消掉这个 weak_ptr
            return -1;
        }
    } else {
        if(timer) {
            timer->cancel();
        }
        MS_LOG_ERROR(g_logger) << "connect addEvent(" << fd << ", WRITE) error";
    }

    int error = 0;
    socklen_t len = sizeof(int);
    if(-1 == getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len)) { // 如果连接出错
        return -1;
    }
    if(!error) {
        return 0;
    } else {
        errno = error;
        return -1;
    }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return connect_with_timeout(sockfd, addr, addrlen, MNSER::s_connect_timeout);
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen) {
    int fd = do_io(sockfd, accept_f, "accept", MNSER::IOManager::READ, SO_RCVTIMEO, addr, addrlen);
    if(fd >= 0) {
        MNSER::FdMgr::GetInstance()->get(fd, true);  // 将连接的客户端的描述符放到 描述符管理器 去管理
    }
    return fd;
}

// read and write
ssize_t read(int fd, void *buf, size_t count) {
    return do_io(fd, read_f, "read", MNSER::IOManager::READ, SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, readv_f, "readv", MNSER::IOManager::READ, SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
    return do_io(sockfd, recv_f, "recv", MNSER::IOManager::READ, SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen) {
    return do_io(sockfd, recvfrom_f, "recvfrom", MNSER::IOManager::READ, SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
    return do_io(sockfd, recvmsg_f, "recvmsg", MNSER::IOManager::READ, SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
    return do_io(fd, write_f, "write", MNSER::IOManager::WRITE, SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
    return do_io(fd, writev_f, "writev", MNSER::IOManager::WRITE, SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int s, const void *msg, size_t len, int flags) {
    return do_io(s, send_f, "send", MNSER::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags, const struct sockaddr *to, socklen_t tolen) {
    return do_io(s, sendto_f, "sendto", MNSER::IOManager::WRITE, SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
    return do_io(s, sendmsg_f, "sendmsg", MNSER::IOManager::WRITE, SO_SNDTIMEO, msg, flags);
}

int close(int fd) {
    if(!MNSER::t_hook_enable) {
        return close_f(fd);
    }

    MNSER::FdCtx::ptr ctx = MNSER::FdMgr::GetInstance()->get(fd);
    if(ctx) {
        auto iom = MNSER::IOManager::GetThis();  // 首先获取当前的 调度器
        if(iom) {
            iom->cancelAll(fd);					 // 然后取消所以和fd相关的事件
        }
        MNSER::FdMgr::GetInstance()->del(fd);	 // 在管理器中清楚掉
    }
    return close_f(fd);
}

// op
int fcntl(int fd, int cmd, ... /* arg */ ) {  // 目的就是为了 NonLock
    va_list va;
    va_start(va, cmd);
    switch(cmd) {
        case F_SETFL:
            {
                int arg = va_arg(va, int);
                va_end(va);
                MNSER::FdCtx::ptr ctx = MNSER::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return fcntl_f(fd, cmd, arg);
                }
                ctx->setUserNonblock(arg & O_NONBLOCK);  // 用户设置了 NonBlock
                if(ctx->getSysNonblock()) {
                    arg |= O_NONBLOCK;
                } else {
                    arg &= ~O_NONBLOCK;
                }
                return fcntl_f(fd, cmd, arg);
            }
            break;
        case F_GETFL:
            {
                va_end(va);
                int arg = fcntl_f(fd, cmd);
                MNSER::FdCtx::ptr ctx = MNSER::FdMgr::GetInstance()->get(fd);
                if(!ctx || ctx->isClose() || !ctx->isSocket()) {
                    return arg;
                }
                if(ctx->getUserNonblock()) {  // 如果用户态的 NobLock
                    return arg | O_NONBLOCK;
                } else {
                    return arg & ~O_NONBLOCK;
                }
            }
            break;
#if 0
#define FUNC(type) \
		case type: 								\
			{ 									\
				int arg = v_arg(va, (int)); 	\
				va_end(va); 					\
				return fcntl_f(fd, cmd, arg); 	\
			} \
			break;
		
		FUNC(F_DUPFD);
		FUNC(F_DUPFD_CLOEXEC);
		FUNC(F_SETFD);
		FUNC(F_SETOWN);
		FUNC(F_SETSIG);
		FUNC(F_SETLEASE);
		FUNC(F_NOTIFY);
		FUNC(F_SETPIPE_SZ);
#undef FUNC
#else 
        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
            {
                int arg = va_arg(va, int);
                va_end(va);
                return fcntl_f(fd, cmd, arg); 
            }
            break;
#endif
#if 0
#define FUNC(type) \
        case type: \
            { \
                va_end(va); \
                return fcntl_f(fd, cmd); \
            } \
            break;

		FUNC(F_GETFD);
		FUNC(F_GETOWN);
		FUNC(F_GETSIG);
		FUNC(F_GETLEASE)
		FUNC(F_GETPIPE_SZ);
#undef FUNC
#else
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
            {
                va_end(va);
                return fcntl_f(fd, cmd);
            }
            break;
#endif 
#if 0
#define FUNC(type) \
        case type: \
            { \
                (struct flock*) arg = va_arg(va, (struct flock*)); \
                va_end(va); \
                return fcntl_f(fd, cmd, arg); \
            } \
            break;

		FUNC(F_SETLK);
		FUNC(F_SETLKW);
		FUNC(F_GETLK);

#undef FUNC
#else
        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
            {
                struct flock* arg = va_arg(va, struct flock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
#endif
#if 0
#define FUNC(type) \
        case type: \
            { \
                (struct f_owner_exlock*) arg = va_arg(va, (struct f_owner_exlock*)); \
                va_end(va); \
                return fcntl_f(fd, cmd, arg); \
            } \
            break;

		FUNC(F_GETOWN_EX);
		FUNC(F_SETOWN_EX);
#undef FUNC
#else
        case F_GETOWN_EX:
        case F_SETOWN_EX:
            {
                struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
                va_end(va);
                return fcntl_f(fd, cmd, arg);
            }
            break;
#endif
        default:
            va_end(va);
            return fcntl_f(fd, cmd);
    }
}

int ioctl(int d, unsigned long int request, ...) {
    va_list va;
    va_start(va, request);
    void* arg = va_arg(va, void*);
    va_end(va);

    if(FIONBIO == request) {
        bool user_nonblock = !!*(int*)arg;
        MNSER::FdCtx::ptr ctx = MNSER::FdMgr::GetInstance()->get(d);
        if(!ctx || ctx->isClose() || !ctx->isSocket()) {
            return ioctl_f(d, request, arg);
        }
        ctx->setUserNonblock(user_nonblock);  // 这里也需要设置用户级别的非阻塞
    }
    return ioctl_f(d, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen) {
    return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen) {
    if(!MNSER::t_hook_enable) {
        return setsockopt_f(sockfd, level, optname, optval, optlen);
    }
    if(level == SOL_SOCKET) {
        if(optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
            MNSER::FdCtx::ptr ctx = MNSER::FdMgr::GetInstance()->get(sockfd);
            if(ctx) {
                const timeval* v = (const timeval*)optval;
                ctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }
    return setsockopt_f(sockfd, level, optname, optval, optlen);
}

}
