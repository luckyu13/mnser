#ifndef __MNSER_STREAM_H__
#define __MNSER_STREAM_H__

#include <memory>

#include "bytearray.h"

namespace MNSER {

class Stream {
public:
	typedef std::shared_ptr<Stream> ptr;

    virtual ~Stream() {}

    // 读数据, buffer 接收数据的内存, length 接收数据的内存大小
    virtual int read(void* buffer, size_t length) = 0;

    // 读数据, ba 接收数据的ByteArray, length 接收数据的内存大小
    virtual int read(ByteArray::ptr ba, size_t length) = 0;

    // 读固定长度, buffer 接收数据的内存, length 接收数据的内存大小
    virtual int readFixSize(void* buffer, size_t length);

    // 读固定长度, ba 接收数据的ByteArray, length 接收数据的内存大小
    virtual int readFixSize(ByteArray::ptr ba, size_t length);

    // 写数据, buffer 写数据的内存, length 接收数据的内存大小
    virtual int write(const void* buffer, size_t length) = 0;

    // 写数据, ba 写数据的ByteArray, length 接收数据的内存大小
    virtual int write(ByteArray::ptr ba, size_t length) = 0;

    // 写固定长度, buffer 写数据的内存, length 接收数据的内存大小
    virtual int writeFixSize(const void* buffer, size_t length);

    // 写固定长度, ba 写数据的ByteArray, length 接收数据的内存大小
    virtual int writeFixSize(ByteArray::ptr ba, size_t length);

    // 关闭流
    virtual void close() = 0;
};

}


#endif
