#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "fd_manager.h"
#include "hook.h"

namespace MNSER {
// 通过文件句柄构造
FdCtx::FdCtx(int fd)
	:m_isInit(false),
	 m_isSocket(false),
	 m_sysNonblock(false),
	 m_userNonblock(false),
	 m_isClosed(false),
	 m_fd(fd),
	 m_recvTimeout(-1),
	 m_sendTimeout(-1) {
	 init();
}

FdCtx::~FdCtx() {
}

bool FdCtx::init() {
	if (m_isInit) {
		return true;
	}

	struct stat fd_stat;
    if(-1 == fstat(m_fd, &fd_stat)) {
        m_isInit = false;
        m_isSocket = false;
    } else {
        m_isInit = true;
        m_isSocket = S_ISSOCK(fd_stat.st_mode);
    }

    if(m_isSocket) { // 如果是 socket 描述符，就把这个fd设置成非阻塞情况
        int flags = fcntl_f(m_fd, F_GETFL, 0);
        if(!(flags & O_NONBLOCK)) {
            fcntl_f(m_fd, F_SETFL, flags | O_NONBLOCK);
        }
        m_sysNonblock = true;
    } else {
        m_sysNonblock = false;
    }

    m_userNonblock = false;  // 用户级的阻塞设置成 false
    m_isClosed = false;
    return m_isInit;
}

void FdCtx::setTimeout(int type, uint64_t v) {
	if (type == SO_RCVTIMEO) {
		m_recvTimeout = v;
	} else {
		m_sendTimeout = v;
	}	
}

uint64_t FdCtx::getTimeout(int type) {
    if(type == SO_RCVTIMEO) {
        return m_recvTimeout;
    }
	return m_sendTimeout;
}

FdManager::FdManager() {
	m_datas.resize(64);
}

// 获取/创建文件句柄，fd 文件句柄，auto_create 是否自动创建
FdCtx::ptr FdManager::get(int fd, bool auto_create) {
    if(fd == -1) {
        return nullptr;
    }
    RWLockType::ReadLock lock(m_mutex);
    if((int)m_datas.size() <= fd) {
        if(auto_create == false) {
            return nullptr;
        }
    } else {
        if(m_datas[fd] || !auto_create) {
            return m_datas[fd];
        }
    }
    lock.unlock();

	// 没有这个 FdCtx 同时设置了自动创造
    RWLockType::WriteLock lock2(m_mutex);
    FdCtx::ptr ctx(new FdCtx(fd));
    if(fd >= (int)m_datas.size()) {
        m_datas.resize(fd * 1.5);
    }
    m_datas[fd] = ctx;
    return ctx;
}

void FdManager::del(int fd) {
    RWLockType::WriteLock lock(m_mutex);
    if((int)m_datas.size() <= fd) {
        return;
    }
    m_datas[fd].reset();
}

}
