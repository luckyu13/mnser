#ifndef __MNSER_NONCOPYABLE_H__
#define __MNSER_NONCOPYABLE_H__

namespace MNSER {

/*
 * 让对象无法拷贝复制
 */
class Noncopyable {
public:
	Noncopyable() = default; // c++ 11 新特性，通过 =delete 实现对应默认构造函数的去除
	~Noncopyable() = default;
	Noncopyable(const Noncopyable&) = delete;
	Noncopyable& operator==(const Noncopyable&) = delete;
private:

};
}

#endif
