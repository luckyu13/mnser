#include "thread.h"
#include "log.h"
#include "util.h"

namespace MNSER {

static thread_local Thread* t_thread = nullptr;
static thread_local std::string t_thread_name = "UNKNOW";

static MNSER::Logger::ptr g_logger = MS_LOG_NAME("system"); // 全局的日志器

Thread::Thread(std::function<void()> cb, const std::string& name)
	:m_cb(cb), m_name(name) {
	if (name.empty()) {
		m_name = "UNKNOW";
	}

	int ret = pthread_create(&m_thread, nullptr, &Thread::run, this);
	if (ret != 0) {
		MS_LOG_ERROR(g_logger) << "pthread_create thread fail, errno = " << ret
			<< "name = " << name;
		throw std::logic_error("In Thread, pthread_create error");
	}
	m_semaphore.wait();
}

Thread::~Thread() {
	if (m_thread) {
		pthread_detach(m_thread);
	}
}
void Thread::join() {
	if (m_thread) {
		int ret = pthread_join(m_thread, nullptr);
		if (ret != 0) {
			MS_LOG_ERROR(g_logger) << "pthread_join fail, errno = " << ret
				<< "name = " << m_name;
			throw std::logic_error("In Thread, pthread_join error");
		}
		m_thread = 0;
	}
}

Thread* Thread::GetThis() {
	return t_thread;
}

const std::string& Thread::GetName() {
	return t_thread_name;
}

void Thread::SetName(const std::string& name) {
	if (name.empty()) {
		return;
	}
	if (t_thread) {
		t_thread->m_name = name;
	}
	t_thread_name = name;
}

void* Thread::run(void *arg) {
	Thread* thread = (Thread*) arg; 
	t_thread = thread;
	t_thread_name = thread->m_name;
	thread->m_pid = MNSER::GetThreadId();
	pthread_setname_np(pthread_self(), thread->m_name.substr(0, 15).c_str());

	std::function<void()> cb;
	cb.swap(thread->m_cb);

	thread->m_semaphore.notify();
	cb();
	return 0;
}

}
