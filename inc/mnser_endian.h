#ifndef __MNSER_ENDIAN_H__
#define __MNSER_ENDIAN_H__
// 和字节序相关
// 字节序转化作用

#define MS_LITTLE_ENDIAN 	1  	// 小端
#define MS_BIG_ENDIAN 		2	// 大端

#include <byteswap.h>
#include <stdint.h>

namespace MNSER {

// 8 字节类型的字节序转化
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type 
byteswap(T value) {
	return (T)bswap_64((uint64_t)value);
}

// 4 字节类型的字节序转化
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
byteswap(T value) {
    return (T)bswap_32((uint32_t)value);
}

// 2 字节类型的字节序转化
template<class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
byteswap(T value) {
    return (T)bswap_16((uint16_t)value);
}

#if BYTE_ORDER == BIG_ENDIAN
#define MS_BYTE_ORDER MS_BIG_ENDIAN
#else
#define MS_BYTE_ORDER MS_LITTLE_ENDIAN
#endif

#if MS_BYTE_ORDER == MS_BIG_ENDIAN

// 在小端机上执行 byteswap，在大端机上什么都不做
template<class T>
T byteswapOnLittleEndian(T t) {
    return t;
}

// 在大端机上执行 byteswap，在小端机上什么都不做
template<class T>
T byteswapOnBigEndian(T t) {
    return byteswap(t);
}
#else

// 在小端机上执行 byteswap，在大端机上什么都不做
template<class T>
T byteswapOnLittleEndian(T t) {
    return byteswap(t);
}

// 在大端机上执行 byteswap，在小端机上什么都不做
template<class T>
T byteswapOnBigEndian(T t) {
    return t;
}

#endif

}


#endif
