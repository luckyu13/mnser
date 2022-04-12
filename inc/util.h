#ifndef __MINISERVER_UTIL_H__
#define __MINISERVER_UTIL_H__

#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <stdint.h>
#include <cxxabi.h>
#include <execinfo.h>
#include <vector>
#include <string>
#include <ios>
#include <algorithm>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "fiber.h"

namespace MNSER {
pid_t GetThreadId();
uint32_t GetFiberId();

template <class T>
const char * TypeToName() {
	static const char * s_name = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, nullptr);
	return s_name;
}

// 获取当前调用栈，bt 保存栈信息，size 返回层数， skip 跳过栈顶层数
void BackTrace(std::vector<std::string>& bt, int size=64, int skip=1);

// 获取栈信息的字符串，size 返回层数，skip 跳过栈顶层数，prefix 栈前缀
std::string BackTraceToString(int size=64, int skip=2, const std::string& prefix="");

uint64_t GetCurrentMS();

uint64_t GetCurrentUS();

std::string ToUpper(const std::string& name);

std::string ToLower(const std::string& name);

std::string Time2Str(time_t ts = time(0), const std::string& format = "%Y-%m-%d %H:%M:%S");

time_t Str2Time(const char* str, const char* format = "%Y-%m-%d %H:%M:%S");

class FSUtil {
public:
    static void ListAllFile(std::vector<std::string>& files
                            ,const std::string& path
                            ,const std::string& subfix);
    static bool Mkdir(const std::string& dirname);
    static bool IsRunningPidfile(const std::string& pidfile);
    static bool Rm(const std::string& path);
    static bool Mv(const std::string& from, const std::string& to);
    static bool Realpath(const std::string& path, std::string& rpath);
    static bool Symlink(const std::string& frm, const std::string& to);
    static bool Unlink(const std::string& filename, bool exist = false);
    static std::string Dirname(const std::string& filename);
    static std::string Basename(const std::string& filename);
    static bool OpenForRead(std::ifstream& ifs, const std::string& filename
                    ,std::ios_base::openmode mode);
    static bool OpenForWrite(std::ofstream& ofs, const std::string& filename
                    ,std::ios_base::openmode mode);
};
}

#endif
