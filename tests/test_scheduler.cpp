#include "mnser.h"

static MNSER::Logger::ptr g_logger = MS_LOG_ROOT();

void test_fiber() {
	static int s_count = 5;
	MS_LOG_INFO(g_logger) << "In test_fiber, s_count= " << s_count;
	sleep(1);
	
	if (--s_count >= 0) {
		MNSER::Scheduler::GetThis()->schedule(&test_fiber, MNSER::GetThreadId());
	}
}

int main(int argc, char* argv[]) {
	MS_LOG_INFO(g_logger) << "Main start...";
	MNSER::Scheduler sc(3, false, "test");
	sc.start();
    //sleep(2);
    MS_LOG_INFO(g_logger) << "schedule";
    sc.schedule(&test_fiber);
    sc.stop();
    MS_LOG_INFO(g_logger) << "over";
	return 0;
}

