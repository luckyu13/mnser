#include "mutex.h"

namespace MNSER {
	Semaphore::Semaphore(uint32_t count) { // count 表示 信号量大小
		if (sem_init(&m_semaphore, 0, count) != 0) {
			throw std::logic_error("In Semaphore, sem_init error");
		}
	}

	Semaphore::~Semaphore() {
		sem_destroy(&m_semaphore);
	}

	void Semaphore::wait() {
		if (sem_wait(&m_semaphore) != 0) {
			throw std::logic_error("In Semaphore, sem_wait error");
		}
	}

	void Semaphore::notify() {
		if (sem_post(&m_semaphore) != 0) {
			throw std::logic_error("In Semaphore, sem_post error");
		}
	}
}
