#ifndef __MNSER_THREAD_H__
#define __MNSER_THREAD_H__

#include "mutex.h"
#include <functional>

namespace MNSER {

class Thread: private Noncopyable {
public:
	typedef std::shared_ptr<Thread> ptr; // 都设置成 智能指针方便管理
	Thread(std::function<void()> cb, const std::string& name);

	~Thread();

	pid_t getId() const {return m_pid; }
	const std::string& getName() const { return m_name; } // 线程名称
	void join(); // 等待线程完成
	static Thread* GetThis();
	static const std::string& GetName();
	static void SetName(const std::string& name);
private:
	static void* run(void *arg);

	pid_t m_pid = -1;
	pthread_t m_thread = 0;
	std::function<void()> m_cb;
	std::string m_name;
	Semaphore m_semaphore;
};

}

#endif
