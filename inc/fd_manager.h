#ifndef __MNSER_FD_MANAGER_H__
#define __MNSER_FD_MANAGER_H__

#include <memory>
#include <vector>

#include "thread.h"
#include "singleton.h"

namespace MNSER {

// 文件句柄上下文类，用来存储 socket 的各种信息
class FdCtx: public std::enable_shared_from_this<FdCtx> {
public:
	typedef std::shared_ptr<FdCtx> ptr;
	
	// 通过文件句柄构造
	FdCtx(int fd);

	~FdCtx();

	bool isInit() const { return m_isInit; }
	bool isSocket() const { return m_isSocket; }
	bool isClose() const { return m_isClosed; }
	void setUserNonblock(bool v) { m_userNonblock = v; }
	bool getUserNonblock() const { return m_userNonblock; }
	void setSysNonblock(bool v) { m_sysNonblock = v; }
	bool getSysNonblock() const { return m_sysNonblock; }
	void setTimeout(int type, uint64_t v);
	uint64_t getTimeout(int type);

private:
	// 初始化
	bool init();

private:
    bool m_isInit: 1; 			// 是否初始化
    bool m_isSocket: 1; 		// 是否socket
    bool m_sysNonblock: 1; 		// 是否hook非阻塞
    bool m_userNonblock: 1; 	// 是否用户主动设置非阻塞
    bool m_isClosed: 1; 		// 是否关闭
    int m_fd; 					// 文件句柄
    uint64_t m_recvTimeout; 	// 读超时时间毫秒
    uint64_t m_sendTimeout; 	// 写超时时间毫秒

};

class FdManager {
public:
	typedef RWLock RWLockType;
	
	FdManager();

	// 获取/创建文件句柄，fd 文件句柄，auto_create 是否自动创建
	FdCtx::ptr get(int fd, bool auto_create=false);

	void del(int fd);

private:
	RWLockType m_mutex;
	std::vector<FdCtx::ptr> m_datas;  // 文件句柄集合
};

// 文件句柄
typedef Singleton<FdManager> FdMgr;

}

#endif
