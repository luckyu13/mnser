#ifndef __MNSER_IOMANAGER_H__
#define __MNSER_IOMANAGER_H__

#include "scheduler.h"
#include "mutex.h"
#include "timer.h"

namespace MNSER {
class IOManager: public Scheduler, public TimerManager {
public:
	typedef std::shared_ptr<IOManager> ptr;
	typedef RWLock RWLockType;

	enum Event {
		NONE 	= 0x0, // 无时间
		READ 	= 0x1, // 读事件
		WRITE 	= 0x4, // 写事件
	};

private:
	// 事件上下文
	struct FdContext {
		typedef Mutex MutexType;

		// 事件上下文
		struct EventContext {					
			// 一个事件要么执行一个函数，要么执行一个线程
			Scheduler* scheduler = nullptr;  	// 事件执行调度器
			Fiber::ptr fiber;					// 事件协程
			std::function<void()> func;			// 事件回调函数
		};

		// 获取事件上下文
		EventContext& getContext(Event event);
		
		// 重置事件上下位
		void resetContext(EventContext& ctx);

		// 触发事件
		void triggerEvent(Event evnet);

		EventContext read;		// 读事件
		EventContext write;		// 写事件
		int fd; 				// 事件关联的句柄
		Event curEvents;		// 当前的事件
		MutexType mutex;		// 事件的 mutex
	};

public:
	IOManager(size_t n_threads=1, bool use_caller=true, const std::string& name="");
	~IOManager();

	// 添加事件 fd 描述符句柄，event 事件类型，cb 回调函数，成功返回0失败返回-1
	int addEvent(int fd, Event event, std::function<void()> cb=nullptr);

	// 删除事件，fd 描述符句柄，event 事件类型
	bool delEvent(int fd, Event evnet);

	// 取消事件，fd 描述符句柄，event 事件类型
	bool cancelEvent(int fd, Event eevent);

	// 取消所有事件，fd 描述符句柄
	bool cancelAll(int fd);

	// 返回当前指向的 IOManager
	static IOManager* GetThis();

protected:
	void tickle() override;
	bool stopping() override;
	void idle() override;
	void onTimerInsertedAtFront() override;

	// 重置 socket 大小
	void contextResize(size_t size);

	// 判断是否可以停止，timeout 最近要触发的定时器事件间隔
	bool stopping(uint64_t& timeout);

private:
	int m_epfd; 										// epoll 句柄
	int m_tickleFds[2];									// pipe 句柄
	std::atomic<size_t> m_pendingEventCount = {0};		// 代办事件数量
	RWLockType m_mutex;									
	std::vector<FdContext*> m_fdContexts;				// 事件上下文容器
};

}

#endif
