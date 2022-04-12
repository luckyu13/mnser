#ifndef __MNSER_SCHEDULER_H__
#define __MNSER_SCHEDULER_H__

#include <memory>
#include <list>
#include <vector>
#include <iostream>
#include <atomic>

#include "mutex.h"
#include "fiber.h"
#include "thread.h"

namespace MNSER {

class Scheduler {
public:
	typedef std::shared_ptr<Scheduler> ptr;
	typedef Mutex MutexType;

	// n_thrads 线程个数 use_caller 创建调度器时如果为 true，那么这个线程也会被这个调度器管理 name 调度器名称
	Scheduler(size_t n_threads=1, bool use_caller=true, const std::string name="");
	~Scheduler();

	std::string getName() const { return m_name; }

	// 启动协程调度器
	void start();

	// 停止协程调度器
	void stop();

	// 切换协程执行的线程
	void switchTo(int thread_id=-1);

	// 输出调度器信息
	std::ostream& dump(std::ostream& os);

	// 调度协程：可以调度有函数对象和协程 thread_id标识fc需要在这个id下的线程执行，-1表示任何线程都可以执行
	template <class FiberOrCb>
    void schedule(FiberOrCb fc, int thread_id = -1) {
        bool need_tickle = false;
        {
            MutexType::Lock lock(m_mutex);
            need_tickle = scheduleNoLock(fc, thread_id);
        }

        if(need_tickle) { // 如果一开始 协程队列为空
            tickle();
        }
    }

	// 批量调度协程，这里会使原来的的指针指向 nullptr
	template<class InputIterator>
	void schedule(InputIterator begin, InputIterator end) {
		bool need_tickle = false;
		{
			MutexType::Lock lock(m_mutex);
			while (begin != end) {
				need_tickle = scheduleNoLock(&*begin, -1) || need_tickle; // 匹配 包含 swap 的构造函数，让原来的指针指向 nullptr
				++begin;
			}
		}
        if(need_tickle) { // 如果一开始 协程队列为空
            tickle();
        }
	}

public:
	// 返回当前协程调度器
	static Scheduler* GetThis();

	// 返回当前协程调度器的 调度协程
	static Fiber* GetMainFiber();

protected:
	// 通知调度器，有任务了
	virtual void tickle();

	// 协程调度函数
	virtual void run();

	// 协程是否可以停止
	virtual bool stopping();

	// 协程无任务执行时，就闲置协程，不能让协程终止
	virtual void idle();

	// 设置当前的协程调度器
	void setThis();
	
	// 返回是否有空闲协程
	bool hasIdleThreads() { return m_idleThreadCount > 0; }

private:
	struct FiberAndFunc {
		Fiber::ptr fiber; 			 	// 执行体是协程
		std::function<void()> func; 	// 执行体是函数
		int thread_id;					// 线程ID

		// FiberAndFunc 构造函数
		// 初始化只能指定 fiber 或者 func 其中一个
		FiberAndFunc(Fiber::ptr f, int t_id)
			:fiber(f), thread_id(t_id) {
		}

		FiberAndFunc(Fiber::ptr* f, int t_id)
			:thread_id(t_id) {
			fiber.swap(*f); // 这里传入地址，使用 swap 方法，可以释放原来的指向，让其引用计数减1
		}

		FiberAndFunc(std::function<void()> f, int t_id)
			:func(f), thread_id(t_id) {
		}

		FiberAndFunc(std::function<void()>* f, int t_id)
			:thread_id(t_id) {
			func.swap(*f);
		}

		FiberAndFunc() 
			:thread_id(-1) {
		}	 

		void reset() {
			fiber = nullptr;
			func = nullptr;
			thread_id = -1;
		}
	};

	template<class FiberOrCb>
	bool scheduleNoLock(FiberOrCb ff, int thread_id) {
		bool need_tickle = m_fibers.empty();  // 开始如果任务队列为空，就需要通知有任务
		FiberAndFunc ft(ff, thread_id);
		if (ft.fiber || ft.func) {
			m_fibers.push_back(ft);
		}
		return need_tickle;
	}

protected:
	std::vector<int> m_threadIds;					// 协程下的线程Id
	size_t m_threadCount = 0;						// 线程数量
	std::atomic<size_t> m_activeThreadCount = {0};	// 工作线程数量
	std::atomic<size_t> m_idleThreadCount = {0};	// 空闲线程数量
	bool m_stopping = true;							// 是否正在停止
	bool m_autoStop = false;						// 是否自动停止
	int m_rootThread = 0;							// 主线程id 

private:
	MutexType m_mutex;
	std::string m_name; 					// 调度器名称
	std::list<FiberAndFunc> m_fibers;		// 等待执行的协程队列
	std::vector<Thread::ptr> m_threads;		// 线程池
	Fiber::ptr m_rootFiber;					// use_caller==true时 调度协程

};

class SchedulerSwitcher : public Noncopyable {
public:
    SchedulerSwitcher(Scheduler* target = nullptr);
    ~SchedulerSwitcher();
private:
    Scheduler* m_caller;
};

}
#endif
