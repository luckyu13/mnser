#include "bytearray.h"
#include "log.h"

static MNSER::Logger::ptr g_logger = MS_LOG_ROOT();
void test() {
#define XX(type, len, write_fun, read_fun, base_len) {\
    std::vector<type> vec; \
    for(int i = 0; i < len; ++i) { \
        vec.push_back(rand()); \
    } \
    MNSER::ByteArray::ptr ba(new MNSER::ByteArray(base_len)); \
    for(auto& i : vec) { \
        ba->write_fun(i); \
    } \
    ba->setPosition(0); \
    for(size_t i = 0; i < vec.size(); ++i) { \
        type v = ba->read_fun(); \
        MS_ASSERT(v == vec[i]); \
    } \
    MS_ASSERT(ba->getReadSize() == 0); \
    MS_LOG_INFO(g_logger) << #write_fun "/" #read_fun \
                    " (" #type " ) len=" << len \
                    << " base_len=" << base_len \
                    << " size=" << ba->getSize(); \
}

    XX(int8_t,  100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t,  100, writeFint16,  readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t,  100, writeFint32,  readFint32, 1);
    XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(int64_t,  100, writeFint64,  readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t,  100, writeInt32,  readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t,  100, writeInt64,  readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);
#undef XX

MS_LOG_INFO(g_logger) << " ================== ";
#define XX(type, len, write_fun, read_fun, base_len) {\
    std::vector<type> vec; \
    for(int i = 0; i < len; ++i) { \
        vec.push_back(rand()); \
    } \
    MNSER::ByteArray::ptr ba(new MNSER::ByteArray(base_len)); \
    for(auto& i : vec) { \
        ba->write_fun(i); \
    } \
    ba->setPosition(0); \
    for(size_t i = 0; i < vec.size(); ++i) { \
        type v = ba->read_fun(); \
        MS_ASSERT(v == vec[i]); \
    } \
    MS_ASSERT(ba->getReadSize() == 0); \
    MS_LOG_INFO(g_logger) << #write_fun "/" #read_fun \
                    " (" #type " ) len=" << len \
                    << " base_len=" << base_len \
                    << " size=" << ba->getSize(); \
    ba->setPosition(0); \
    MS_ASSERT(ba->writeToFile("/tmp/" #type "_" #len "-" #read_fun ".dat")); \
    MNSER::ByteArray::ptr ba2(new MNSER::ByteArray(base_len * 2)); \
    MS_ASSERT(ba2->readFromFile("/tmp/" #type "_" #len "-" #read_fun ".dat")); \
    ba2->setPosition(0); \
    MS_ASSERT(ba->toString() == ba2->toString()); \
    MS_ASSERT(ba->getPosition() == 0); \
    MS_ASSERT(ba2->getPosition() == 0); \
}
    XX(int8_t,  100, writeFint8, readFint8, 1);
    XX(uint8_t, 100, writeFuint8, readFuint8, 1);
    XX(int16_t,  100, writeFint16,  readFint16, 1);
    XX(uint16_t, 100, writeFuint16, readFuint16, 1);
    XX(int32_t,  100, writeFint32,  readFint32, 1);
    XX(uint32_t, 100, writeFuint32, readFuint32, 1);
    XX(int64_t,  100, writeFint64,  readFint64, 1);
    XX(uint64_t, 100, writeFuint64, readFuint64, 1);

    XX(int32_t,  100, writeInt32,  readInt32, 1);
    XX(uint32_t, 100, writeUint32, readUint32, 1);
    XX(int64_t,  100, writeInt64,  readInt64, 1);
    XX(uint64_t, 100, writeUint64, readUint64, 1);

#undef XX
}

void test_order() {
	MNSER::ByteArray::ptr ba(new MNSER::ByteArray(2));
    std::vector<uint32_t> vec; 
    for(int i = 0; i < 10; ++i) { 
        vec.push_back(rand()); 
    }
	for (auto& i: vec) {
		ba->writeFuint32(i);
	}
    std::vector<uint64_t> vec1; 
    for(int i = 0; i < 10; ++i) { 
        vec1.push_back(rand()); 
    }
	for (auto& i: vec1) {
		ba->writeFuint64(i);
	}
    std::vector<uint8_t> vec2; 
    for(int i = 0; i < 10; ++i) { 
        vec2.push_back(rand()%255); 
    }
	ba->setPosition(0);
	for (auto&i: vec) {
		std::cout << "i= " << i << " readn num= " << ba->readFuint32() << std::endl;
	}
	for (auto&i: vec1) {
		std::cout << "i= " << i << " readn num= " << ba->readFuint64() << std::endl;
	}
	auto pos = ba->getPosition();
	ba->setPosition(ba->getSize());
	for (auto& i: vec2) {
		ba->writeFuint8(i);
	}
	ba->setPosition(pos);
	for (auto&i: vec2) {
		std::cout << "i= " << (int)i << " readn num= " << (int)ba->readFuint8() << std::endl;
	}
	pos = ba->getPosition();
	ba->writeStringVint("hello ");
	ba->setPosition(pos);
	std::cout << ba->readStringVint() << std::endl;
	pos = ba->getPosition();
	ba->writeStringF32("world!");
	ba->setPosition(pos);
	std::cout << ba->readStringF32() << std::endl;

	ba->setPosition(0);
	for (auto&i: vec) {
		std::cout << "i= " << i << " readn num= " << ba->readFuint32() << std::endl;
	}
	for (auto&i: vec1) {
		std::cout << "i= " << i << " readn num= " << ba->readFuint64() << std::endl;
	}
	for (auto&i: vec2) {
		std::cout << "i= " << (int)i << " readn num= " << (int)ba->readFuint8() << std::endl;
	}
	std::cout << ba->readStringVint() ;
	std::cout << ba->readStringF32() << std::endl;
}

void test_rand() {
	MNSER::ByteArray::ptr ba(new MNSER::ByteArray(2));
	int8_t i8 = -8;
	uint8_t ui8 = 9;
	int64_t i64 = 64;
	uint64_t ui64 = 65;
	
	int64_t vi64 = -657483;
	uint64_t vui64 = 657483;

	ba->writeFint8(i8);
	ba->writeFuint8(ui8);
	ba->writeFint64(i64);
	ba->writeFuint64(ui64);
	ba->writeInt64(vi64);
	ba->writeUint64(vui64);
	size_t pos = ba->getPosition();
	ba->setPosition(0);
	//std::cout << "ba->getReadSize()= " << ba->getReadSize() << std::endl;
	//std::cout << "size = " << ba->getSize() << std::endl;
	//std::cout << "position= " << ba->getPosition() << std::endl;
	std::cout << (int)ba->readFint8() << " - " << (int)i8 << std::endl;
	std::cout << (int)ba->readFuint8() << " - " << (int)ui8 << std::endl;
	std::cout << ba->readFint64() << " - " << i64 << std::endl;
	std::cout << ba->readFuint64() << " - " << ui64 << std::endl;
	std::cout << ba->readInt64() << " - " << vi64 << std::endl;
	std::cout << ba->readUint64() << " - " << vui64 << std::endl;
	std::cout << "pos= " << pos << " cur pos= " << ba->getPosition() << std::endl;
	ba->writeStringVint("hello ");
	ba->writeStringF32("world!");
	ba->setPosition(pos);
	std::cout << ba->readStringVint() ;
	std::cout << ba->readStringF32() << std::endl;
	std::cout << std::endl << " ============== " << std::endl;
	ba->setPosition(0);
	std::cout << (int)ba->readFint8() << " - " << (int)i8 << std::endl;
	std::cout << (int)ba->readFuint8() << " - " << (int)ui8 << std::endl;
	std::cout << ba->readFint64() << " - " << i64 << std::endl;
	std::cout << ba->readFuint64() << " - " << ui64 << std::endl;
	std::cout << ba->readInt64() << " - " << vi64 << std::endl;
	std::cout << ba->readUint64() << " - " << vui64 << std::endl;
	std::cout << "pos= " << pos << " cur pos= " << ba->getPosition() << std::endl;
	std::cout << ba->readStringVint() ;
	std::cout << ba->readStringF32() << std::endl;
}

int main(int argc, char** argv) {
    //test();
	test_rand();
	//test_order();
    return 0;
}
