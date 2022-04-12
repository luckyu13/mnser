#ifndef __MNSER_SOCKET_H__
#define __MNSER_SOCKET_H__

#include <netinet/tcp.h>
#include <memory>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "noncopyable.h"
#include "address.h"

namespace MNSER {
class Socket: public std::enable_shared_from_this<Socket>, Noncopyable {
public:
	typedef std::shared_ptr<Socket> ptr;
	typedef std::weak_ptr<Socket> weak_ptr;

	enum Type {
		TCP = SOCK_STREAM,  		// TCP 类型
		UDP = SOCK_DGRAM			// UDP 类型
	};

	enum Family {
		IPv4 = AF_INET,				// Ipv4 socket
		IPv6 = AF_INET6,			// Ipv6 socket
		UNIX = AF_UNIX,				// unix 域 socket
	};

public:
	// 创建 TCP 套接字
	static Socket::ptr CreateTCP(MNSER::Address::ptr address);

	// 创建 UDP 套接字
	static Socket::ptr CreateUDP(MNSER::Address::ptr address);
	
	// 创建 IPv4 的 Tcp Socket
	static Socket::ptr CreateTCPSocket();

	// 创建 IPv4 的 Udp Socket
	static Socket::ptr CreateUDPSocket();

	// 创建 IPv6 的 Tcp Socket
	static Socket::ptr CreateTCPSocket6();

	// 创建 IPv6 的 Udp Socket
	static Socket::ptr CreateUDPSocket6();

	// 创建 UNIX 域 的 TCP Socket
	static Socket::ptr CreateUnixTCPSocket();

	// 创建 UNIX 域 的 UDP Socket
	static Socket::ptr CreateUnixUDPSocket();

public:
	// Socket 构造函数
	Socket(int family, int type, int protocol=0);

	virtual ~Socket();

	// 获取发送超时时间
	int64_t getSendTimeout();

	// 设置发送超时时间
	void setSendTimeout(int64_t v);

	// 获取接收超时时间
	int64_t getRecvTimeout();

	// 设置接收超时时间
	void setRecvTimeout(int64_t v);

	// 获取sockopt, => getsockopt
	bool getOption(int level, int option, void* result, socklen_t* len);

	// 获取sockopt模板, 使用模板类，可以减少最后一个参数的书写
	template<class T>
	bool getOption(int level, int option, T& result) {
		socklen_t length = sizeof(T);
		return getOption(level, option, &result, &length);
	}

	// 设置sockopt，=> setsockopt
	bool setOption(int level, int option, const void* result, socklen_t len);

	// 设置sockopt模板
	template<class T>
	bool setOption(int level, int option, const T& value) {
		return setOption(level, option, &value, sizeof(T));
	}

	// socket 常用函数
	// 接收
	virtual Socket::ptr accept();

	// 绑定
	virtual bool bind(const Address::ptr addr);

	// 连接
	virtual bool connect(const Address::ptr addr, uint64_t timeout_ms=-1);

	// 重连接
	virtual bool reconnect(uint64_t timeout_ms=-1);

	// 监听
	virtual bool listen(int backlog=SOMAXCONN);

	// 关闭
	virtual bool close();

	// 发送数据, buffer 接收数据的内存, length 接收数据的大小, flags 标志字, 返回 >0 接收的数据大小 =0 socket被关闭 <0 出错
	virtual int send(const void* buffer, size_t length, int flags=0);

	// 发送数据, buffers 接收数据的内存, length 接收数据的大小, flags 标志字, 返回 >0 接收的数据大小 =0 socket被关闭 <0 出错
	virtual int send(const struct iovec* buffers, size_t length, int flags=0);

	// 发送数据, buffer 接收数据的内存, length 接收数据的大小, to 段地址, flags 标志字, 返回 >0 接收的数据大小 =0 socket被关闭 <0 出错
	virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags=0);
  
	// 发送数据, buffers 接收数据的内存, length 接收数据的大小, to 段地址, flags 标志字, 返回 >0 接收的数据大小 =0 socket被关闭 <0 出错
	virtual int sendTo(const struct iovec* buffers, size_t length, const Address::ptr to, int flags=0);

	// 接收数据,buffer 接收数据的内存, length 接收数据的大小, flags 标志字, 返回 >0 接收的数据大小 =0 socket被关闭 <0 出错
	virtual int recv(void* buffer, size_t length, int flags=0);

	// 接收数据,buffers 接收数据的内存, length 接收数据的大小, flags 标志字, 返回 >0 接收的数据大小 =0 socket被关闭 <0 出错
	virtual int recv(struct iovec* buffers, size_t length, int flags=0);

	// 接收数据,buffer 接收数据的内存, length 接收数据的大小, from 发送段地址, flags 标志字, 返回 >0 接收的数据大小 =0 socket被关闭 <0 出错
	virtual int recvFrom(void* buffer, size_t length, Address::ptr from, int flags=0);

	// 接收数据,buffers 接收数据的内存, length 接收数据的大小, from 发送段地址, flags 标志字, 返回 >0 接收的数据大小 =0 socket被关闭 <0 出错
	virtual int recvFrom(struct iovec* buffers, size_t length, Address::ptr from, int flags=0);
	
	// 输出信息到流中
	virtual std::ostream& dump(std::ostream& os) const;

	// 输出自己的信息
	virtual std::string toString() const;

	// 获取远端地址
	Address::ptr getRemoteAddress();

	// 获取本地地址
	Address::ptr getLocalAddress();

public:
	// 返回协议族
	int getFamily() const { return m_family; }

	// 返回socket类型
	int getType() const { return m_type; }

	// 返回 socket 的协议
	int getProtocol() const { return m_protocol; }

	// 放回socket句柄
	int getSocket() const { return m_sock; }

	// socket 是否已经连接
	bool isConnected() const { return m_isConnected; }

	// 返回 Socket 错误
	int getError();

	// 是否有效
	bool isValid() const;

	// 取消读
	bool cancelRead();

	// 取消写
	bool cancelWrite();

	// 取消accept
	bool cancelAccept();

	// 取消所有事件
	bool cancelAll();

protected:
	// 初始化 socket
	void initSock();

	// 创建新的 socket
	void newSock();

	// 初始化sock
	virtual bool init(int sock);

protected:
	int m_sock;						// socket 句柄
	int m_family;					// 协议族
	int m_type;						// 类型
	int m_protocol;					// 协议
	bool m_isConnected;				// 是否连接
	Address::ptr m_localAddress;	// 本地地址
	Address::ptr m_remoteAddress;	// 远端地址
};

#if 0
class SSLSocket: public Socket {
public:
	typedef std::shared_ptr<SSLSocket> ptr;

	static SSLSocket::ptr CreateTCP(MNSER::Address::ptr address);
	static SSLSocket::ptr CreateTCPSocket();
	static SSLSocket::ptr CreateTCPSocket6();

	SSLSocket(int family, int type, int protocol=0);
    virtual Socket::ptr accept() override;
    virtual bool bind(const Address::ptr addr) override;
    virtual bool connect(const Address::ptr addr, uint64_t timeout_ms = -1) override;
    virtual bool listen(int backlog = SOMAXCONN) override;
    virtual bool close() override;
    virtual int send(const void* buffer, size_t length, int flags = 0) override;
    virtual int send(const iovec* buffers, size_t length, int flags = 0) override;
    virtual int sendTo(const void* buffer, size_t length, const Address::ptr to, int flags = 0) override;
    virtual int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags = 0) override;
    virtual int recv(void* buffer, size_t length, int flags = 0) override;
    virtual int recv(iovec* buffers, size_t length, int flags = 0) override;
    virtual int recvFrom(void* buffer, size_t length, Address::ptr from, int flags = 0) override;
    virtual int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags = 0) override;

	bool loadCertificates(const std::string& cert_file, const std::string& key_file);
	virtual std::ostream& dump(std::ostream& os) const override;

protected:
	virtual bool init(int sock) override;

private:
	std::shared_ptr<SSL_CTX> m_ctx;
	std::shared_ptr<SSL> m_ssl;
};
#endif

// 流式输出
std::ostream& operator<<(std::ostream& os, const Socket& sock);

}

#endif
