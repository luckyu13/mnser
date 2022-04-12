#include "mnser.h"


MNSER::Logger::ptr g_logger = MS_LOG_ROOT();

void test_assert() {
	MS_LOG_INFO(g_logger) << std::endl << MNSER::BackTraceToString(10, 0, "  ");
	MS_ASSERT2(1 == 0, "1 == 0");
}

int main(int argc, char* argv[]) {
	test_assert();
	return 0;
}
