#include <sys/uio.h>
#include <cmath>
#include "bytearray.h"
#include "log.h"
#include "mnser_endian.h"

namespace MNSER {

static MNSER::Logger::ptr g_logger = MS_LOG_NAME("system");

ByteArray::Node::Node(size_t s)
	: ptr(new char[s])
	, next(nullptr)
	, size(s) {
}

ByteArray::Node::Node()
	: ptr(nullptr)
	, next(nullptr)
	, size(0) {
}

ByteArray::Node::~Node() {
	if (ptr) {
		delete [] ptr;
	}
}

// 构造指定大小内存块的ByteArray
ByteArray::ByteArray(size_t base_size)
	: m_baseSize(base_size)
	, m_position(0)
	, m_capacity(base_size)
	, m_endian(MS_BIG_ENDIAN)
	, m_size(0)
	, m_root(new Node(base_size))  // TODO 这个理解好像不对, 这里的 m_root 设置成哨兵节点，将这个节点作为链表的尾节点
	{
	m_cur = m_root;
}

// 析构函数
ByteArray::~ByteArray() {
    Node* tmp = m_root;
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
}

// 写入固定长度 int8_t 类型的数据
void ByteArray::writeFint8(int8_t value) {
    write(&value, sizeof(value));
}
// 写入固定长度 uint8_t 类型的数据
void ByteArray::writeFuint8(uint8_t value) {
    write(&value, sizeof(value));
}
// 写入固定长度 int16_t 类型的数据
void ByteArray::writeFint16(int16_t value) {
    if(m_endian != MS_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}
// 写入固定长度 uint16_t 类型的数据
void ByteArray::writeFuint16(uint16_t value) {
    if(m_endian != MS_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}
// 写入固定长度 int32_t 类型的数据
void ByteArray::writeFint32(int32_t value) {
    if(m_endian != MS_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}
// 写入固定长度 uint32_t 类型的数据
void ByteArray::writeFuint32(uint32_t value) {
    if(m_endian != MS_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}
// 写入固定长度 int64_t 类型的数据
void ByteArray::writeFint64(int64_t value) {
    if(m_endian != MS_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}
// 写入固定长度 uint64_t 类型的数据
void ByteArray::writeFuint64(uint64_t value) {
    if(m_endian != MS_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

static uint32_t EncodeZigzag32(const int32_t& v) {
#if 0
    if(v < 0) {
        return ((uint32_t)(-v)) * 2 - 1;
    } else {
        return v * 2;
    }
#endif
	return (uint32_t)((v<<1)^(v>>31));
}

static uint64_t EncodeZigzag64(const int64_t& v) {
#if 0
    if(v < 0) {
        return ((uint64_t)(-v)) * 2 - 1;
    } else {
        return v * 2;
    }
#endif
	return (uint64_t)((v<<1)^(v>>63));
}

static int32_t DecodeZigzag32(const uint32_t& v) {
    return (v >> 1) ^ -(v & 1);
}

static int64_t DecodeZigzag64(const uint64_t& v) {
    return (v >> 1) ^ -(v & 1);
}

// 写入有符号的 Varint32 类型的数据
void ByteArray::writeInt32(int32_t value) {
    writeUint32(EncodeZigzag32(value));
}
// 写入无符号的 Varint32 类型的数据
void ByteArray::writeUint32(uint32_t value) {
    uint8_t tmp[5];  // 极端情况 uint32_t 是 5 个字节
    uint8_t i = 0;
    while(value >= 0x80) { // 如果大于1000 0000 就还有数据
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

// 写入有符号的 Varint64 类型的数据
void ByteArray::writeInt64(int64_t value) {
    writeUint64(EncodeZigzag64(value));
}
// 写入无符号的 Varint64 类型的数据
void ByteArray::writeUint64(uint64_t value) {
    uint8_t tmp[10];  // 最大 10 个字节
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

// 写入 float 类型的数据
void ByteArray::writeFloat(float value) {
    uint32_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint32(v);
}

// 写入 double 类型的数据
void ByteArray::writeDouble(double value) {
    uint64_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint64(v);
}

// 写入 std::string 类型的数据，用 uint16_t 作为长度类型，写入string的长度
void ByteArray::writeStringF16(const std::string& value) {
    writeFuint16(value.size());
    write(value.c_str(), value.size());
}

// 写入 std::string 类型的数据，用 uint32_t 作为长度类型，写入string的长度
void ByteArray::writeStringF32(const std::string& value) {
    writeFuint32(value.size());
    write(value.c_str(), value.size());
}

// 写入 std::string 类型的数据，用 uint64_t 作为长度类型，写入string的长度
void ByteArray::writeStringF64(const std::string& value) {
    writeFuint64(value.size());
    write(value.c_str(), value.size());
}

// 写入 std::string 类型的数据，用 无符号Varint64 作为长度类型，写入string的长度
void ByteArray::writeStringVint(const std::string& value) {
    writeUint64(value.size());
    write(value.c_str(), value.size());
}

// 写入 std::string 类型的数据，不写入string的长度
void ByteArray::writeStringWithoutLength(const std::string& value) {
    write(value.c_str(), value.size());
}

// 读取固定长度 int8_t 类型的数据
int8_t ByteArray::readFint8() {
    int8_t v;
    read(&v, sizeof(v));
    return v;
}
// 读取固定长度 uint8_t 类型的数据
uint8_t ByteArray::readFuint8() {
    uint8_t v;
    read(&v, sizeof(v));
    return v;
}

#define XX(type) \
    type v; \
    read(&v, sizeof(v)); \
    if(m_endian == MS_BYTE_ORDER) { \
        return v; \
    } else { \
        return byteswap(v); \
    }

// 读取固定长度 int16_t 类型的数据
int16_t ByteArray::readFint16() {
	XX(int16_t);
}
// 读取固定长度 uint16_t 类型的数据
uint16_t ByteArray::readFuint16() {
	XX(uint16_t);
}

// 读取固定长度 int32_t 类型的数据
int32_t ByteArray::readFint32() {
	XX(int32_t);
}
// 读取固定长度 uint32_t 类型的数据
uint32_t ByteArray::readFuint32() {
	XX(uint32_t);
}

// 读取固定长度 int64_t 类型的数据
int64_t ByteArray::readFint64() {
	XX(int64_t);
}
// 读取固定长度 uint64_t 类型的数据
uint64_t ByteArray::readFuint64() {
	XX(uint64_t);
}
#undef XX

// 读取有符号的 Varint32 类型的数据
int32_t ByteArray::readInt32() {
    return DecodeZigzag32(readUint32());
}
// 读取无符号的 Varint32 类型的数据
uint32_t ByteArray::readUint32() {
    uint32_t result = 0;
    for(int i = 0; i < 32; i += 7) {
        uint8_t b = readFuint8();  // 固定读取32位
        if(b < 0x80) {
            result |= ((uint32_t)b) << i;
            break;
        } else {
            result |= (((uint32_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

// 读取有符号的 Varint64 类型的数据
int64_t ByteArray::readInt64() {
	return DecodeZigzag64(readUint64());
}
// 读取无符号的 Varint64 类型的数据
uint64_t ByteArray::readUint64() {
    uint64_t result = 0;
    for(int i = 0; i < 64; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            result |= ((uint64_t)b) << i;
            break;
        } else {
            result |= (((uint64_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

// 读取 float 类型的数据
float ByteArray::readFloat() {
    uint32_t v = readFuint32();
    float value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

// 读取 double 类型的数据
double ByteArray::readDouble() {
    uint64_t v = readFuint64();
    double value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

// 读取 std::string 类型的数据，用 uint16_t 作为长度类型，读取string的长度
std::string ByteArray::readStringF16() {
    uint16_t len = readFuint16();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

// 读取 std::string 类型的数据，用 uint32_t 作为长度类型，读取string的长度
std::string ByteArray::readStringF32() {
    uint32_t len = readFuint32();
    std::string buff;
    buff.resize(len);
	//MS_LOG_INFO(g_logger) << "Read string 32 len= " << len;
    read(&buff[0], len);
    return buff;
}

// 读取 std::string 类型的数据，用 uint64_t 作为长度类型，读取string的长度
std::string ByteArray::readStringF64() {
    uint64_t len = readFuint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

// 读取 std::string 类型的数据，用 无符号Varint64 作为长度类型，读取string的长度
std::string ByteArray::readStringVint() {
    uint64_t len = readUint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

// 清空 bytearray
void ByteArray::clear() {  // 留一个节点，清除其他节点
    m_position = m_size = 0;
    m_capacity = m_baseSize;
    Node* tmp = m_root->next;
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
    m_cur = m_root;
    m_root->next = NULL;
}

// 写入 size 长度的数据
void ByteArray::write(const void* buf, size_t size) {
    if(size == 0) {
        return;
    }

    addCapacity(size);  // 如果容量够了的话 自己这里是不会改变的

    size_t npos = m_position % m_baseSize;  // 当前节点的位置
    size_t ncap = m_cur->size - npos;		// 当前节点的容量
    size_t bpos = 0;						// 当前 buf 对应位置

    while(size > 0) {
        if(ncap >= size) {  // 当前节点可以装完
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, size);
            if(m_cur->size == (npos + size)) {  // 刚好将数据装完，将当前节点指向下一个节点
                m_cur = m_cur->next;
            }
            m_position += size; 	// 当前位置更新
            bpos += size;			// buf 位置更新
            size = 0;				// 已经写入完成了
        } else {
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, ncap);  // 当前节点写完
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
    }

    if(m_position > m_size) {
        m_size = m_position;
    }
}

// 读取 size 长度的数据
void ByteArray::read(void* buf, size_t size) {
    if(size > getReadSize()) { // 超出容量了
		MS_ASSERT2(size <= getReadSize(), "size = " + std::to_string(size));
        throw std::out_of_range("not enough len");
    }

    size_t npos = m_position % m_baseSize;  // 当前节点位置
    size_t ncap = m_cur->size - npos;		// 当前节点容量
    size_t bpos = 0;						// 当前 buf 位置
    while(size > 0) {
        if(ncap >= size) {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, size);
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;
            }
            m_position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
    }
}

// 从 position 读取 size 长度的数据
void ByteArray::read(void* buf, size_t size, size_t position) const {
    if(size > (m_size - position)) {
        throw std::out_of_range("not enough len");
    }

    size_t npos = position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;
    Node* cur = m_cur;
    while(size > 0) {
        if(ncap >= size) {
            memcpy((char*)buf + bpos, cur->ptr + npos, size);
            if(cur->size == (npos + size)) {
                cur = cur->next;
            }
            position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos, cur->ptr + npos, ncap);
            position += ncap;
            bpos += ncap;
            size -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
    }
}

// 设置当前 ByteArray 当前位置
void ByteArray::setPosition(size_t v) {
    if(v > m_capacity) {
        throw std::out_of_range("set_position out of range");
    }
    m_position = v;
    if(m_position > m_size) {
        m_size = m_position;
    }
    m_cur = m_root;
	// TODO 为什么这里使用的是 下一个节点
    while(v > m_cur->size) {
        v -= m_cur->size;
        m_cur = m_cur->next;
    }
    if(v == m_cur->size) {
        m_cur = m_cur->next;
    }
}

// 把 ByteArray 的数据写入到文件中
bool ByteArray::writeToFile(const std::string& name) const {
    std::ofstream ofs;
    ofs.open(name, std::ios::trunc | std::ios::binary);
    if(!ofs) {
        MS_LOG_ERROR(g_logger) << "writeToFile name=" << name
            << " error , errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    int64_t read_size = getReadSize();
    int64_t pos = m_position;
    Node* cur = m_cur;

    while(read_size > 0) {
        int diff = pos % m_baseSize;
        int64_t len = (read_size > (int64_t)m_baseSize ? m_baseSize : read_size) - diff;
        ofs.write(cur->ptr + diff, len);
        cur = cur->next;
        pos += len;
        read_size -= len;
    }

    return true;
}

// 从文件中读取数据
bool ByteArray::readFromFile(const std::string& name) {
    std::ifstream ifs;
    ifs.open(name, std::ios::binary);
    if(!ifs) {
        MS_LOG_ERROR(g_logger) << "readFromFile name=" << name
            << " error, errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    std::shared_ptr<char> buff(new char[m_baseSize], [](char* ptr) { delete[] ptr;});
    while(!ifs.eof()) {
        ifs.read(buff.get(), m_baseSize);
        write(buff.get(), ifs.gcount());
    }
    return true;
}

// 是否时小端
bool ByteArray::isLittleEndian() const {
    return m_endian == MS_LITTLE_ENDIAN;
}

// 设置是否为小端
void ByteArray::setIsLittleEndian(bool val) {
    if(val) {
        m_endian = MS_LITTLE_ENDIAN;
    } else {
        m_endian = MS_BIG_ENDIAN;
    }
}


// 将 ByteArray 中的数据 [m_postion, m_size) 转换成 std::string
std::string ByteArray::toString() const {
    std::string str;
    str.resize(getReadSize());
    if(str.empty()) {
        return str;
    }
    read(&str[0], str.size(), m_position);
    return str;
}

// 将 ByteArray 里面的数据 [m_postion, m_size) 转换成16进制的std::string(格式: FF FF FF)
std::string ByteArray::toHexString() const {
    std::string str = toString();
    std::stringstream ss;

    for(size_t i = 0; i < str.size(); ++i) {
        if(i > 0 && i % 32 == 0) {
            ss << std::endl;
        }
        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int)(uint8_t)str[i] << " ";
    }

    return ss.str();
}

// 获取可读取的缓存，保存成 struct iovec 数组，返回读取数据长度
uint64_t ByteArray::getReadBuffers(std::vector<struct iovec>& buffers, uint64_t len) const {
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0) {
        return 0;
    }

    uint64_t size = len;

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;

    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

// 从 position 开始获取数据，将数据放到 struct iovec 中间，返回读取数据长度
uint64_t ByteArray::getReadBuffers(std::vector<struct iovec>& buffers, uint64_t len, uint64_t position) const {
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0) {
        return 0;
    }

    uint64_t size = len;

    size_t npos = position % m_baseSize;
    size_t count = position / m_baseSize;
    Node* cur = m_root;
    while(count > 0) {
        cur = cur->next;
        --count;
    }

    size_t ncap = cur->size - npos;
    struct iovec iov;
    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

// 获取可写入的缓存，保存成 struct iovec 数组，返回读取数据长度
uint64_t ByteArray::getWriteBuffers(std::vector<struct iovec>& buffers, uint64_t len) {
    if(len == 0) {
        return 0;
    }
    addCapacity(len);
    uint64_t size = len;

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;
    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;

            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

// 扩容 ByteArray，使其可以容纳 size 个数据
void ByteArray::addCapacity(size_t size) {
    if(size == 0) {
        return;
    }
    size_t old_cap = getCapacity();
    if(old_cap >= size) {
        return;
    }

    size = size - old_cap;  // 需要扩充的容量大小
    size_t count = ceil(1.0 * size / m_baseSize);  // 需要扩充的节点数量，向上取整
    Node* tmp = m_root;
    while(tmp->next) {
        tmp = tmp->next;
    }

    Node* first = NULL;
    for(size_t i = 0; i < count; ++i) {
        tmp->next = new Node(m_baseSize);
        if(first == NULL) {
            first = tmp->next;
        }
        tmp = tmp->next;
        m_capacity += m_baseSize;
    }

    if(old_cap == 0) {
        m_cur = first;
    }
}

}
