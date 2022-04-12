#ifndef __MNSER_BYTEARRAY_H__
#define __MNSER_BYTEARRAY_H__

#include <memory>
#include <vector>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <stdint.h>

namespace MNSER {

class ByteArray {
public:
	typedef std::shared_ptr<ByteArray> ptr;

	// ByteArray 存储节点
	struct Node {  				// 使用链表可以处理分配大内存的情况
		Node(size_t ); 			// 构建指定大小的内存块
		Node();					// 
		~Node();				// 析构函数，释放内存
		char *ptr;				// 内存块指针
		Node* next; 			// 下一个内存块地址
		size_t size;			// 内存块大小
	};

	// 构造指定大小内存块的ByteArray
	ByteArray(size_t base_size=4096);

	// 析构函数
	~ByteArray();

	// 写入固定长度 int8_t 类型的数据
	void writeFint8(int8_t value);
	// 写入固定长度 uint8_t 类型的数据
	void writeFuint8(uint8_t value);

	// 写入固定长度 int16_t 类型的数据
	void writeFint16(int16_t value);
	// 写入固定长度 uint16_t 类型的数据
	void writeFuint16(uint16_t value);

	// 写入固定长度 int32_t 类型的数据
	void writeFint32(int32_t value);
	// 写入固定长度 uint32_t 类型的数据
	void writeFuint32(uint32_t value);

	// 写入固定长度 int64_t 类型的数据
	void writeFint64(int64_t value);
	// 写入固定长度 uint64_t 类型的数据
	void writeFuint64(uint64_t value);

	// 写入有符号的 Varint32 类型的数据
	void writeInt32(int32_t value);
	// 写入无符号的 Varint32 类型的数据
	void writeUint32(uint32_t value);

	// 写入有符号的 Varint64 类型的数据
	void writeInt64(int64_t value);
	// 写入无符号的 Varint64 类型的数据
	void writeUint64(uint64_t value);

	// 写入 float 类型的数据
	void writeFloat(float value);

	// 写入 double 类型的数据
	void writeDouble(double value);

	// 写入 std::string 类型的数据，用 uint16_t 作为长度类型，写入string的长度
	void writeStringF16(const std::string& value);

	// 写入 std::string 类型的数据，用 uint32_t 作为长度类型，写入string的长度
	void writeStringF32(const std::string& value);

	// 写入 std::string 类型的数据，用 uint64_t 作为长度类型，写入string的长度
	void writeStringF64(const std::string& value);

	// 写入 std::string 类型的数据，用 无符号Varint64 作为长度类型，写入string的长度
	void writeStringVint(const std::string& value);

	// 写入 std::string 类型的数据，不写入string的长度
	void writeStringWithoutLength(const std::string& value);


	// 读取固定长度 int8_t 类型的数据
	int8_t readFint8();
	// 读取固定长度 uint8_t 类型的数据
	uint8_t readFuint8();

	// 读取固定长度 int16_t 类型的数据
	int16_t readFint16();
	// 读取固定长度 uint16_t 类型的数据
	uint16_t readFuint16();

	// 读取固定长度 int32_t 类型的数据
	int32_t readFint32();
	// 读取固定长度 uint32_t 类型的数据
	uint32_t readFuint32();

	// 读取固定长度 int64_t 类型的数据
	int64_t readFint64();
	// 读取固定长度 uint64_t 类型的数据
	uint64_t readFuint64();

	// 读取有符号的 Varint32 类型的数据
	int32_t readInt32();
	// 读取无符号的 Varint32 类型的数据
	uint32_t readUint32();

	// 读取有符号的 Varint64 类型的数据
	int64_t readInt64();
	// 读取无符号的 Varint64 类型的数据
	uint64_t readUint64();

	// 读取 float 类型的数据
	float readFloat();

	// 读取 double 类型的数据
	double readDouble();

	// 读取 std::string 类型的数据，用 uint16_t 作为长度类型，读取string的长度
	std::string readStringF16();

	// 读取 std::string 类型的数据，用 uint32_t 作为长度类型，读取string的长度
	std::string readStringF32();

	// 读取 std::string 类型的数据，用 uint64_t 作为长度类型，读取string的长度
	std::string readStringF64();

	// 读取 std::string 类型的数据，用 无符号Varint64 作为长度类型，读取string的长度
	std::string readStringVint();

	// 清空 bytearray
	void clear();

	// 写入 size 长度的数据
	void write(const void* buf, size_t size);

	// 读取 size 长度的数据
	void read(void* buf, size_t size);

	// 从 position 读取 size 长度的数据
	void read(void* buf, size_t size, size_t position) const;

	// 设置当前 ByteArray 当前位置
	void setPosition(size_t v);

	// 把 ByteArray 的数据写入到文件中
	bool writeToFile(const std::string& name) const;

	// 从文件中读取数据
	bool readFromFile(const std::string& name);

	// 是否时小端
	bool isLittleEndian() const;

	// 设置是否为小端
	void setIsLittleEndian(bool val);

	// 将 ByteArray 中的数据 [m_postion, m_size) 转换成 std::string
	std::string toString() const;

	// 将 ByteArray 里面的数据 [m_postion, m_size) 转换成16进制的std::string(格式: FF FF FF)
	std::string toHexString() const;

	// 获取可读取的缓存，保存成 struct iovec 数组，返回读取数据长度
	uint64_t getReadBuffers(std::vector<struct iovec>& buffers, uint64_t len=~0ull) const;

	// 从 position 开始获取数据，将数据放到 struct iovec 中间，返回读取数据长度
	uint64_t getReadBuffers(std::vector<struct iovec>& buffers, uint64_t len, uint64_t position) const;

	// 获取可写入的缓存，保存成 struct iovec 数组，返回读取数据长度
	uint64_t getWriteBuffers(std::vector<struct iovec>& buffers, uint64_t len);
	
	// 获取 ByteArray 当前位置
	size_t getPosition() const { return m_position; }

	// 返回内存块大小
	size_t getBaseSize() const { return m_baseSize; }

	// 返回可读取数据的大小
	size_t getReadSize() const { return m_size - m_position; }

	// 返回数据的长度
	size_t getSize() const { return m_size; }

private:
	// 扩容 ByteArray，使其可以容纳 size 个数据
	void addCapacity(size_t size);

	// 获取当前的可写入容量
	size_t getCapacity() const { return m_capacity - m_position; }
private:
	size_t m_baseSize;			// 内存块大小
	size_t m_position;			// 当前操作位置
	size_t m_capacity;			// 当前总容量
	size_t m_size;				// 当前数据的大小
	size_t m_endian;			// 字节序，默认大端
	Node* m_root;				// 第一个内存块指针
	Node* m_cur;				// 当前操作的内存块指针
};

}

#endif
