#ifndef __MNSER_SOCKET_STREAM_H__
#define __MNSER_SOCKET_STREAM_H__

#include "stream.h"
#include "socket.h"

namespace MNSER {
class SocketStream: public Stream {
public:
	typedef std::shared_ptr<SocketStream> ptr;

    SocketStream(Socket::ptr sock, bool owner = true);

    ~SocketStream();

    // 读数据, buffer 接收数据的内存, length 接收数据的内存大小
    virtual int read(void* buffer, size_t length) override;

    // 读数据, ba 接收数据的ByteArray, length 接收数据的内存大小
    virtual int read(ByteArray::ptr ba, size_t length) override;

    // 写数据, buffer 写数据的内存, length 接收数据的内存大小
    virtual int write(const void* buffer, size_t length) override;

    // 写数据, ba 写数据的ByteArray, length 接收数据的内存大小
    virtual int write(ByteArray::ptr ba, size_t length) override;

    // 关闭流
    virtual void close() override;

	// 获取流关联的 socket
    Socket::ptr getSocket() const { return m_socket;}

	// 当前是否连接
    bool isConnected() const;

	// 获取对端地址
    Address::ptr getRemoteAddress();

	// 获取本端地址
    Address::ptr getLocalAddress();

	// 远端地址字符串
    std::string getRemoteAddressString();

	// 本端地址字符串
    std::string getLocalAddressString();

protected:
	Socket::ptr m_socket;
	bool m_owner;			// 是否主控
};

}


#endif
