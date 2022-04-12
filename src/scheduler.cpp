#include "scheduler.h"
#include "macro.h"
#include "log.h"
#include "config.h"
#include "hook.h"


namespace MNSER {

static MNSER::Logger::ptr g_logger = MS_LOG_NAME("system");

static thread_local Scheduler* t_scheduler = nullptr;  // 指向当前调度器
static thread_local Fiber* t_scheduler_fiber = nullptr;  // 指向调度器的协程

Scheduler::Scheduler(size_t n_threads, bool use_caller, const std::string name)
	:m_name(name) {
	MS_ASSERT(n_threads > 0);
    if(use_caller) { // 当前协程也使用调度器调度
        MNSER::Fiber::GetThis(); // 这里是为了创建一个主协程，并且这个协程用来作为调度协程
        --n_threads;

        MS_ASSERT(GetThis() == nullptr);
        t_scheduler = this;
		
		// std::bind(&Scheduler::run, this) 表示调用this的run
		// 主协程不需要栈大小
        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));  
        MNSER::Thread::SetName(m_name);
		//std::cout << "###" << std::endl;

        t_scheduler_fiber = m_rootFiber.get();
        m_rootThread = MNSER::GetThreadId();
        m_threadIds.push_back(m_rootThread);
    } else {
        m_rootThread = -1; // 不需要调度器来调度
    }
    m_threadCount = n_threads;
}

Scheduler::~Scheduler() {
    MS_ASSERT(m_stopping);
    if(GetThis() == this) {
        t_scheduler = nullptr;
    }
	//MS_LOG_INFO(g_logger) << "m_threadIds.size()= " << m_threadIds.size()
	//	<< " m_threads.size()= " << m_threads.size();
	//for (auto it:
}

// 启动协程调度器
void Scheduler::start() {
    MutexType::Lock lock(m_mutex);
    if(!m_stopping) {
        return;
    }
    m_stopping = false;
    MS_ASSERT(m_threads.empty());  // 还没有分配线程池, 

	// 分配线程池
    m_threads.resize(m_threadCount);
    for(size_t i = 0; i < m_threadCount; ++i) {
        m_threads[i].reset(new MNSER::Thread(std::bind(&Scheduler::run, this)
                            , m_name + "_" + std::to_string(i)));
        m_threadIds.push_back(m_threads[i]->getId());
    }
    lock.unlock();

    //if(m_rootFiber) {
    //    //m_rootFiber->swapIn();
    //    m_rootFiber->call();
    //    SYLAR_LOG_INFO(g_logger) << "call out " << m_rootFiber->getState();
    //}
}

// 停止协程调度器
void Scheduler::stop() {
    m_autoStop = true;
    if(m_rootFiber
            && m_threadCount == 0
            && (m_rootFiber->getState() == Fiber::TERM
                || m_rootFiber->getState() == Fiber::INIT)) {
        m_stopping = true;

        if(stopping()) {
            return;
        }
    }

    if(m_rootThread != -1) {  // use_caller的必须在非自己线程结束
        MS_ASSERT(GetThis() == this);
    } else {
        MS_ASSERT(GetThis() != this); // 如果不需要调度器来调度，那就不应该是 t_scheduler
    }

    m_stopping = true;
    for(size_t i = 0; i < m_threadCount; ++i) {
        tickle();
    }

    if(m_rootFiber) {
        tickle();
    }

    if(m_rootFiber) {
        //while(!stopping()) {
        //    if(m_rootFiber->getState() == Fiber::TERM
        //            || m_rootFiber->getState() == Fiber::EXCEPT) {
        //        m_rootFiber.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
        //        SYLAR_LOG_INFO(g_logger) << " root fiber is term, reset";
        //        t_fiber = m_rootFiber.get();
        //    }
        //    m_rootFiber->call();
        //}
        if(!stopping()) {
            m_rootFiber->call();
        }
    }
	
	//MS_LOG_DEBUG(g_logger) << "m_threads.size() = " 
	//	<< m_threads.size() << " ###";
    std::vector<Thread::ptr> thrs;
    {
        MutexType::Lock lock(m_mutex);
        thrs.swap(m_threads);
    }

    for(auto& i : thrs) {
        i->join();
    }
}

// 切换协程执行的线程
void Scheduler::switchTo(int thread_id) {
    MS_ASSERT(Scheduler::GetThis() != nullptr);
    if(Scheduler::GetThis() == this) {
        if(thread_id == -1 || thread_id == MNSER::GetThreadId()) {
            return;
        }
    }
    schedule(Fiber::GetThis(), thread_id);
    Fiber::YieldToHold();
}

// 输出调度器信息
std::ostream& Scheduler::dump(std::ostream& os) {
    os << "[Scheduler name=" << m_name
       << " size=" << m_threadCount
       << " active_count=" << m_activeThreadCount
       << " idle_count=" << m_idleThreadCount
       << " stopping=" << (m_stopping ? "true": "false")
       << " ]" << std::endl << "    ";
    for(size_t i = 0; i < m_threadIds.size(); ++i) {
        if(i) {
            os << ", ";
        }
        os << m_threadIds[i];
    }
    return os;
}

// 返回当前协程调度器
Scheduler* Scheduler::GetThis() {
    return t_scheduler;
}

// 返回当前协程调度器的 调度协程
Fiber* Scheduler::GetMainFiber() {
    return t_scheduler_fiber;
}

// 通知调度器，有任务了
void Scheduler::tickle() {
    MS_LOG_INFO(g_logger) << "tickle";
}

// 协程调度函数
void Scheduler::run() {
    MS_LOG_DEBUG(g_logger) << m_name << " run";
    set_hook_enable(true);
    setThis();  // 让调度器切换过来
    if(MNSER::GetThreadId() != m_rootThread) { // 就是run线程
        t_scheduler_fiber = Fiber::GetThis().get();
    }

    Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
    Fiber::ptr cb_fiber;  // cb_fiber 可以从空闲的协程中找一个

    FiberAndFunc ft;
    while(true) {
        ft.reset();
        bool tickle_me = false;
        bool is_active = false;
        { // 取出一个执行的协程
            MutexType::Lock lock(m_mutex);
            auto it = m_fibers.begin();  // 取出一个协程
            while(it != m_fibers.end()) { // 在协程池中找一个自己可以操作的协程
                if(it->thread_id != -1 && it->thread_id != MNSER::GetThreadId()) {  // 指定了线程执行，而且不是当前线程执行
					// 如果这个协程不是这个线程执行，那么可以通过线程间通信的方式告诉其他线程，但是每个线程都在一直轮转执行，可以不同通知的
                    ++it;
                    tickle_me = true;  // 通知别人处理，让别人处理
                    continue;
                }

				// 找到一个自己应该执行的协程或者函数
                MS_ASSERT(it->fiber || it->func);
                if(it->fiber && it->fiber->getState() == Fiber::EXEC) { // 其他线程正在执行这个协程
                    ++it;
                    continue;
                }
				// 只能自己处理这个事件
                ft = *it; 
                m_fibers.erase(it++);
                ++m_activeThreadCount; // 先加1 表示有任务了,让stopping不会是false
                is_active = true;
                break;
            }
            tickle_me |= it != m_fibers.end();
        }

        if(tickle_me) { // 自己没有处理的协程，通知别人处理
            tickle();
        }

		// ft 就是从队列里面取出的任务
        if(ft.fiber && (ft.fiber->getState() != Fiber::TERM
                        && ft.fiber->getState() != Fiber::EXCEPT)) { // 如果取出的协程
		//std::cout << "Choose ft id = " << ft.fiber->getId() << std::endl;
            ft.fiber->swapIn();
            --m_activeThreadCount; // 协程执行完毕了，活动线程减一
			//std::cout << "###" << " --m_activeThreadCount = " << m_activeThreadCount 
			//<< " m_fibers.size() = " << m_fibers.size()
			//<< std::endl;

            if(ft.fiber->getState() == Fiber::READY) {  // 如果协程是 READY 就再次加入调度器中
                schedule(ft.fiber); // 在加入调度器
            } else if(ft.fiber->getState() != Fiber::TERM
                    && ft.fiber->getState() != Fiber::EXCEPT) { // 如果执行结束或者执行出现异常
                ft.fiber->m_state = Fiber::HOLD;  
            }
            ft.reset();
        } else if(ft.func) {
            if(cb_fiber) {
			//std::cout << "###" << cb_fiber->getId() << std::endl;
                cb_fiber->reset(ft.func);
            } else {
                cb_fiber.reset(new Fiber(ft.func));
            }
            ft.reset(); // 释放 ft
            cb_fiber->swapIn();  // 返回的状态就不会是 INIT
            --m_activeThreadCount;
            if(cb_fiber->getState() == Fiber::READY) {
                schedule(cb_fiber);
                cb_fiber.reset();  // 智能指针引用计数减一
            } else if(cb_fiber->getState() == Fiber::EXCEPT
                    || cb_fiber->getState() == Fiber::TERM) {
                cb_fiber->reset(nullptr);
            } else {// 其他状态 EXEC HOLD
                cb_fiber->m_state = Fiber::HOLD;
                cb_fiber.reset();
            }
        } else {  // ft 没有 fiber 也没有 func，表示没有需要执行的了
            if(is_active) {
                --m_activeThreadCount;
                continue;
            }
            if(idle_fiber->getState() == Fiber::TERM) { // 所有协程和函数都已经执行完毕
                MS_LOG_INFO(g_logger) << "idle fiber term" << ", m_id=" << idle_fiber->getId();
                break;
            }

            ++m_idleThreadCount;
            idle_fiber->swapIn();
            --m_idleThreadCount;
            if(idle_fiber->getState() != Fiber::TERM
                    && idle_fiber->getState() != Fiber::EXCEPT) {
                idle_fiber->m_state = Fiber::HOLD;
            }
        }
    }
}

// 协程是否可以停止
bool Scheduler::stopping() {
    MutexType::Lock lock(m_mutex);
    return m_autoStop && m_stopping
        && m_fibers.empty() && m_activeThreadCount == 0;
}


// 协程无任务执行时，就闲置协程
void Scheduler::idle() {
    MS_LOG_INFO(g_logger) << "idle";
    while(!stopping()) {
        MNSER::Fiber::YieldToHold();
    }
}

// 设置当前的协程调度器
void Scheduler::setThis() {
	t_scheduler = this;
}

}
