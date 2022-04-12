#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include "log.h"
#include "thread.h"

MNSER::Logger::ptr g_logger = MS_LOG_ROOT();

int count = 0;

MNSER::Mutex s_mutex;
MNSER::SpinLock sl_mutex;

#define LOG MS_LOG_INFO(g_logger)

void fun1() {
	MS_LOG_INFO(g_logger) << "name: " << MNSER::Thread::GetName()
						<< " this.name: " << MNSER::Thread::GetThis()->getName()
						<< " id: " << MNSER::GetThreadId()
						<< " this.id: " << MNSER::Thread::GetThis()->getId();
	for (int i=0; i<80000000; ++i) {
		MNSER::SpinLock::Lock lock(sl_mutex);
		++count;
	}
}

int main() {
	clock_t start, finish;
	start = clock();

	MS_LOG_INFO(g_logger) << "Test Thrad begin..." << std::endl;

	std::vector<MNSER::Thread::ptr> ts;
	for (int i=0; i<5; ++i) {
		MNSER::Thread::ptr ptr(new MNSER::Thread(&fun1, "thread_" + std::to_string(i)));
		ts.push_back(ptr);
	}
	for (auto& it: ts) {
		it->join();
	}
	LOG << "count = " << count << std::endl;
	finish = clock();
	LOG << "All time = " << (double)(finish - start) / CLOCKS_PER_SEC << "s";
	return 0;
}
