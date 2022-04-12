#ifndef __MNSER_MUTEX_H__
#define __MNSER_MUTEX_H__

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <memory>

#include "noncopyable.h"

namespace MNSER {

/*
 * 信号量
 */
class Semaphore: private Noncopyable {
public:
	Semaphore(uint32_t count = 0); // count 表示 信号量大小
	~Semaphore();

	/*
	 * 获取信号量
	 */
	void wait();

	/*
	 * 释放信号量
	 */
	void notify();

private:
	sem_t m_semaphore;
};

/*
 * 锁的模板
 * 通过模板，就可以使用不同的互斥量进行上锁了
 */
template <class T>
struct ScopeLockTmpl {
	ScopeLockTmpl(T& mutex): m_mutex(mutex), m_locked(false) {
		m_mutex.lock();
		m_locked = true;
	}

	~ScopeLockTmpl() {
		unlock();
	}

	void lock() {
		if (!m_locked) {
			m_mutex.lock();
			m_locked = true;
		}
	}

	void unlock() {
		if (m_locked) {
			m_mutex.unlock();
			m_locked = false;
		}
	}

	bool isLocked() const {
		return m_locked;
	}

private:
	T& m_mutex;
	bool m_locked = false;
};

/*
 * 读锁的模板
 * 通过模板，就可以使用不同的互斥量进行上锁了
 */
template <class T>
struct ReadScopeLockTmpl {
	ReadScopeLockTmpl(T& mutex): m_mutex(mutex), m_locked(false) {
		m_mutex.rdlock();
		m_locked = true;
	}

	~ReadScopeLockTmpl() {
		unlock();
	}

	void lock() {
		if (!m_locked) {
			m_mutex.rdlock();
			m_locked = true;
		}
	}

	void unlock() {
		if (m_locked) {
			m_mutex.unlock();
			m_locked = false;
		}
	}

	bool isLocked() const {
		return m_locked;
	}

private:
	T& m_mutex;
	bool m_locked = false;
};

/*
 * 写锁的模板
 * 通过模板，就可以使用不同的互斥量进行上锁了
 */
template <class T>
struct WriteScopeLockTmpl {
	WriteScopeLockTmpl(T& mutex): m_mutex(mutex), m_locked(false) {
		m_mutex.wrlock();
		m_locked = true;
	}

	~WriteScopeLockTmpl() {
		unlock();
	}

	void lock() {
		if (!m_locked) {
			m_mutex.wrlock();
			m_locked = true;
		}
	}

	void unlock() {
		if (m_locked) {
			m_mutex.unlock();
			m_locked = false;
		}
	}

	bool isLocked() const {
		return m_locked;
	}

private:
	T& m_mutex;
	bool m_locked = false;
};

/*
 * 互斥量
 * 在构造函数中初始化
 * 在析构函数中销毁
 */
class Mutex: private Noncopyable {
public:
	typedef ScopeLockTmpl<Mutex> Lock;  // 锁的定义

	Mutex() {
		if(pthread_mutex_init(&m_mutex, nullptr) != 0) {
			throw std::logic_error("In Mutex, pthread_mutex_init error");
		}
	}

	~Mutex() {
		pthread_mutex_destroy(&m_mutex);
	}

	void lock() {
		if (pthread_mutex_lock(&m_mutex) != 0) {
			throw std::logic_error("In Mutex, pthread_mutex_lock error");
		}
	}

	void unlock() {
		if (pthread_mutex_unlock(&m_mutex) != 0) {
			throw std::logic_error("In Mutex, pthread_mutex_unlock error");
		}
	}

private:
	pthread_mutex_t m_mutex;
};

/*
 * 读写互斥量
 * 在构造函数中初始化
 * 在析构函数中销毁
 */
class RWLock: private Noncopyable {
public:
	typedef ReadScopeLockTmpl<RWLock> ReadLock;  // 锁的定义
	typedef WriteScopeLockTmpl<RWLock> WriteLock;  // 锁的定义

	RWLock() {
		if(pthread_rwlock_init(&m_lock, nullptr) != 0) {
			throw std::logic_error("In RWLock, pthread_rwlock_init error");
		}
	}

	~RWLock() {
		pthread_rwlock_destroy(&m_lock);
	}

	void rdlock() {
		if (pthread_rwlock_rdlock(&m_lock) != 0) {
			throw std::logic_error("In RWLock, pthread_rwlock_rdlock error");
		}
	}

	void wrlock() {
		if (pthread_rwlock_wrlock(&m_lock) != 0) {
			throw std::logic_error("In RWLock, pthread_rwlock_wrlock error");
		}
	}

	void unlock() {
		if (pthread_rwlock_unlock(&m_lock) != 0) {
			throw std::logic_error("In RWLock, pthread_rwlock_unlock error");
		}
	}

private:
	pthread_rwlock_t m_lock;
};

/*
 * 自旋锁
 * 在构造函数中初始化
 * 在析构函数中销毁
 */
class SpinLock: private Noncopyable {
public:
	typedef ScopeLockTmpl<SpinLock> Lock;  // 锁的定义

	SpinLock() {
		if(pthread_spin_init(&m_lock, 0) != 0) {
			throw std::logic_error("In SpinLock, pthread_spin_init error");
		}
	}

	~SpinLock() {
		pthread_spin_destroy(&m_lock);
	}

	void lock() {
		if (pthread_spin_lock(&m_lock) != 0) {
			throw std::logic_error("In SpinLock, pthread_spin_lock error");
		}
	}

	void unlock() {
		if (pthread_spin_unlock(&m_lock) != 0) {
			throw std::logic_error("In SpinLock, pthread_spin_unlock error");
		}
	}

private:
	pthread_spinlock_t m_lock;
};

}
#endif


