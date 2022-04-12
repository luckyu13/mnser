#ifndef __MNSER_MACRO_H__
#define __MNSER_MACRO_H__

#include <string.h>
#include <assert.h>

#include "log.h"
#include "util.h"

/*
 * Likely(x) 等价于 if(x)
 * UnLikely(x) 等价于 if(x)
 * __builtin_expect(!!(x), 1) 函数就是将信息给编译器，在判断的时候进行优化，以减少指令跳转带来的性能损失
 *
 * 使用likely ，执行if后面语句的可能性大些，编译器将if{}是的内容编译到前面
 * 使用unlikely ，执行else后面语句的可能性大些,编译器将else{}里的内容编译到前面。
 * 使用 likely 可以优化编译器
 */

#if defined __GUNC__ || defined __llvm__ 
#define MS_LIKELY(x) __builtin_expect(!!(x), 1)
#define MS_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define MS_LIKELY(x) (x)
#define MS_UNLIKELY(x) (x)
#endif

/*
 * 断言封装
 */
#define MS_ASSERT(x) \
	if(MS_LIKELY(!(x))) { \
		MS_LOG_ERROR(MS_LOG_ROOT()) << "ASSERTION: " #x \
			<< "\nbacktrace: \n" \
			<< MNSER::BackTraceToString(100, 2, "   "); \
		assert(x); \
	}

#define MS_ASSERT2(x, w) \
	if(MS_LIKELY(!(x))) { \
		MS_LOG_ERROR(MS_LOG_ROOT()) << "ASSERTION: " #x \
			<< "\n" << w \
			<< "\nbacktrace: \n" \
			<< MNSER::BackTraceToString(100, 2, "   "); \
		assert(x); \
	}

#endif
