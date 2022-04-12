#ifndef __MNSER_FIBER_H__
#define __MNSER_FIBER_H__

#include <memory>
#include <functional>
#include <ucontext.h>

#include "util.h"
#include "macro.h"
#include "log.h"


namespace MNSER {
class Scheduler;
class Fiber: public std::enable_shared_from_this<Fiber> {
friend class Scheduler;
public:
	typedef std::shared_ptr<Fiber> ptr;
	
	/*
	 * 协程状态
	 */
	enum FiberState {
		INIT,		// 初始状态
		HOLD, 		// 暂停状态
		EXEC, 		// 执行状态
		TERM,		// 终止状态
		READY,		// 可执行状态
		EXCEPT,		// 异常状态
	};
	
	// cb: 协程执行函数 stackSize: 协程栈大小 use_caller: 是否在 MainFiber 上调度
	Fiber(std::function<void()> cb, size_t stackSize=0, bool use_caller=false);
	~Fiber();
	
	// 重置协程执行函数
	void reset(std::function<void()> cb);

	// 将当前协程切换到运行状态
	void swapIn();

	// 将当前协程切换到后台
	void swapOut();

	// 将当前线程切换到执行状态, 该函数由主协程执行
	void call();

	// 将当前线程切换到后台, 返回线程的主协程
	void back();

	uint64_t getId() const { return m_id; }
	FiberState getState() const { return m_state; }

public: // 静态成员函数
	
	// 设置当前线程运行的协程
	static void SetThis(Fiber* f);

	// 获取当前所在的协程
	static Fiber::ptr GetThis();

	// 将当前协程切换到后台，并设置READY状态
	static void YieldToReady();

	// 将当前协程切换到后台，并设置HOLD状态
	static void YieldToHold();

	// 当前协程的总数量
	static uint64_t TotalFibers();

	// 协程执行函数, 执行完成返回主协程
	static void MainFunc();

	// 协程执行函数，执行完毕之后返回协程调度函数
	static void CallerMainFunc();

	// 获取当前协程ID
	static uint64_t GetFiberId();

private:
	Fiber(); // 清除默认构造函数
	
private:
	uint64_t m_id = 0;					// 协程 ID
	uint32_t m_stacksize = 0;			// 协程栈大小
	FiberState m_state = INIT;			// 协程当前状态
	ucontext_t m_ctx;					// 协程运行时的上下文
	void* m_stack = nullptr; 			// 协程栈
	std::function<void()> m_cb;			// 协程执行函数
};

}

#endif
