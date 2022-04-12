#include <atomic>  // 这个库就是为了定义原子类型的数据
#include <stdint.h>
#include "fiber.h"
#include "config.h"
#include "scheduler.h"

namespace MNSER {
static Logger::ptr g_logger = MS_LOG_NAME("system");

std::atomic<uint64_t> s_fiber_id {0}; // 当前运行的协程 ID
std::atomic<uint64_t> s_fiber_count {0}; // 当前总共的协程数量

static thread_local Fiber* t_fiber = nullptr;
static thread_local Fiber::ptr t_threadFiber = nullptr; 

static ConfigVar<uint32_t>::ptr g_fiber_stack_size = 
	Config::Lookup<uint32_t>("fiber.stack_size", 128 * 1024, "fiber stack size");

class MallocStackAllocator {
public:
	static void* Alloc(size_t size) {
		return malloc(size);
	}

	static void Dealloc(void* p, size_t size) {
		return free(p);
	}
};

using MSAlloctor = MallocStackAllocator;

Fiber::Fiber() { // 这个构造函数只有主协程用得到,而且这个实现使用类单例模式的思想
	m_state = EXEC;
	SetThis(this);

	if (0 != getcontext(&m_ctx)) {
		MS_ASSERT2(false, "getcontext");
	}

	++s_fiber_count;
	MS_LOG_DEBUG(g_logger) << "Fiber::Fiber main";
}

// 这个构造函数指定了回调函数，分配了栈空间
Fiber::Fiber(std::function<void()> cb, size_t stackSize, bool use_caller) 
	: m_id(++s_fiber_id), m_cb(cb)  {
	++s_fiber_count;
	m_stacksize = stackSize ? stackSize: g_fiber_stack_size->getValue();

	m_stack = MSAlloctor::Alloc(m_stacksize);
	if (m_stack == nullptr) {
		MS_ASSERT2(false, "MSAlloctor::Alloc");
	}
	// 为协程分配栈空间之后，就保存当前的上下文
	if (getcontext(&m_ctx) !=  0) {
		MS_ASSERT2(false, "getcontext");
	}

	m_ctx.uc_link = nullptr;  			// 后继执行函数
	m_ctx.uc_stack.ss_sp = m_stack;
	m_ctx.uc_stack.ss_size = m_stacksize;

	if(!use_caller) {  // 如果不是协程调度的函数
		makecontext(&m_ctx, &Fiber::MainFunc, 0);
	} else {
		makecontext(&m_ctx, &Fiber::CallerMainFunc, 0);
	}

	MS_LOG_DEBUG(g_logger) << "Fiber::Fiber id = " << m_id;
}

Fiber::~Fiber() {
	//MS_LOG_DEBUG(g_logger) << "Start Fiber::~Fiber id=" << m_id 
	//	<< " total=" << s_fiber_count
	//	<< " m_state = " << m_state;
	--s_fiber_count;
	if (m_stack) { // 如果有栈，自己的状态就应该是在这几个状态
		MS_ASSERT(m_state == TERM
			|| m_state == EXCEPT
			|| m_state == INIT);

		MSAlloctor::Dealloc(m_stack, m_stacksize);
	} else { // 没有栈就只有是当前线程的主协程
		MS_ASSERT(!m_cb);
		MS_ASSERT(m_state == EXEC);

		Fiber* cur = t_fiber;
		if (cur == this) {  // 如果就是现在这个协程调用自己的析构函数，那就将当前运行的协程指针指向 nullptr
			SetThis(nullptr);
			MS_LOG_DEBUG(g_logger) << "Thread id: " << MNSER::GetThreadId() 
				<< " Main Fiber Set nullptr";
		}
	}
	MS_LOG_DEBUG(g_logger) << "Finish Fiber::~Fiber id=" << m_id 
		<< " total=" << s_fiber_count;
}

// 重置协程执行函数
void Fiber::reset(std::function<void()> cb) {
	MS_ASSERT(m_stack);  // 主协程不能 reset
	MS_ASSERT(m_state == TERM
		|| m_state == EXCEPT
		|| m_state == INIT);
	m_cb = cb;
	if (0 != getcontext(&m_ctx)) {
		MS_ASSERT2(false, "getcontext");
	}

	m_ctx.uc_link = nullptr;
	m_ctx.uc_stack.ss_sp = m_stack;
	m_ctx.uc_stack.ss_size = m_stacksize;
	
	makecontext(&m_ctx, &Fiber::MainFunc, 0);
	m_state = INIT;
}

// 将当前协程切换到运行状态，操作对象应该是子协程
void Fiber::swapIn() {
	SetThis(this);
	MS_ASSERT(m_state != EXEC);
	m_state = EXEC;
	if (0 != swapcontext(&Scheduler::GetMainFiber()->m_ctx, &m_ctx)) {
		MS_ASSERT2(false, "swapcontext");
	}
}

// 将当前协程切换到后台
void Fiber::swapOut() {
	SetThis(Scheduler::GetMainFiber());
	if (0 != swapcontext(&m_ctx, &Scheduler::GetMainFiber()->m_ctx)) {
		MS_ASSERT2(false, "swapcontext");
	}
}

// 将当前线程切换到执行状态, 该函数由主协程执行
void Fiber::call() {
	SetThis(this);
	m_state = EXEC;
	if (0 != swapcontext(&t_threadFiber->m_ctx, &m_ctx)) {
		MS_ASSERT2(false, "swapcontext");
	}
}


// 将当前线程切换到后台, 返回线程的主协程
void Fiber::back() {
	SetThis(t_threadFiber.get());
	if (0 != swapcontext(&m_ctx, &t_threadFiber->m_ctx)) {
		MS_ASSERT2(false, "swapcontext");
	}
}


// 设置当前线程运行的协程
void Fiber::SetThis(Fiber* f) {
	t_fiber = f;
}

// 获取当前所在的协程
Fiber::ptr Fiber::GetThis() {
	if (t_fiber) {
		return t_fiber->shared_from_this();
	}
	Fiber::ptr main_fiber(new Fiber);
	MS_ASSERT(t_fiber == main_fiber.get()); // 主协程应该只有一个
	t_threadFiber = main_fiber;
	//main_fiber.reset();
	return t_fiber->shared_from_this();
}

// 将当前协程切换到后台，并设置READY状态
void Fiber::YieldToReady() {
	Fiber::ptr cur = GetThis();
	MS_ASSERT(cur->m_state == EXEC);
	cur->m_state = READY;
	cur->swapOut();
	//cur->back();
}
 
// 将当前协程切换到后台，并设置HOLD状态
void Fiber::YieldToHold() {
	Fiber::ptr cur = GetThis();
	MS_ASSERT(cur->m_state == EXEC);
	//cur->m_state = HOLD;
	cur->swapOut();
	//cur->back();
}


// 当前协程的总数量
uint64_t Fiber::TotalFibers() {
	return s_fiber_count;
}

// 协程执行函数, 执行完成返回主协程
void Fiber::MainFunc() {
	//MS_LOG_INFO(g_logger) << "m_id = " << MNSER::GetFiberId() << " start run";
	Fiber::ptr cur = GetThis();
	MS_ASSERT(cur);
	try {
		cur->m_cb();
		cur->m_cb = nullptr;
		cur->m_state = TERM;
	} catch (std::exception &e) {
		cur->m_state = EXCEPT;
		MS_LOG_ERROR(g_logger) << "Fiber Except: " << e.what() 
			<< " fiber_id = " << cur->getId() << std::endl
			<< BackTraceToString();		
	} catch (...) {
		cur->m_state = EXCEPT;
		MS_LOG_ERROR(g_logger) << "Fiber Except: " 
			<< " fiber_id = " << cur->getId() << std::endl
			<< BackTraceToString();		
	}

	auto raw_ptr = cur.get();
	cur.reset(); // 将智能指针的引用计数减一
	raw_ptr->swapOut(); // 返回到主协程
	//raw_ptr->back(); // 返回到主协程

	MS_ASSERT2(false, "never reach fiber_id = " + std::to_string(raw_ptr->getId()));
}

// 协程执行函数，执行完毕之后返回协程调度函数
void Fiber::CallerMainFunc() {
	Fiber::ptr cur = GetThis();
	MS_ASSERT(cur);
	try {
		cur->m_cb();
		cur->m_cb = nullptr;
		cur->m_state = TERM;
	} catch (std::exception& e) {
		cur->m_state = EXCEPT;
		MS_LOG_ERROR(g_logger) << "Fiber Except: " << e.what() 
			<< " fiber_id = " << cur->getId() << std::endl
			<< BackTraceToString();		
	} catch (...) {
		cur->m_state = EXCEPT;
		MS_LOG_ERROR(g_logger) << "Fiber Except: " 
			<< " fiber_id = " << cur->getId() << std::endl
			<< BackTraceToString();		
	}

	auto raw_ptr = cur.get();
	cur.reset(); // 将智能指针的引用计数减一
	raw_ptr->back(); // 返回到调度器协程
	//raw_ptr->swapOut(); // 返回到调度器协程

	MS_ASSERT2(false, "never reach fiber_id = " + std::to_string(raw_ptr->getId()));
}

// 获取当前协程ID
uint64_t Fiber::GetFiberId() {
	if (t_fiber) {
		return t_fiber->getId();
	}
	return 0;
}

}
