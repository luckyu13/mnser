#include "mnser.h"


MNSER::Logger::ptr g_logger = MS_LOG_ROOT();

void run_in_fiber() {
    MS_LOG_INFO(g_logger) << "run_in_fiber begin";
    MNSER::Fiber::YieldToHold();
    MS_LOG_INFO(g_logger) << "run_in_fiber end";
    MNSER::Fiber::YieldToHold();
}

void test_fiber() {
    MS_LOG_INFO(g_logger) << "main begin -1";
    {
        MNSER::Fiber::GetThis();
        MS_LOG_INFO(g_logger) << "main begin";
        MNSER::Fiber::ptr fiber(new MNSER::Fiber(run_in_fiber));
        fiber->call();
        MS_LOG_INFO(g_logger) << "main after swapIn";
        fiber->call();
        MS_LOG_INFO(g_logger) << "main after end";
        fiber->call();
    }
    MS_LOG_INFO(g_logger) << "main after end2";
}

int main(int argc, char** argv) {
    MNSER::Thread::SetName("main");

    std::vector<MNSER::Thread::ptr> thrs;
    for(int i = 0; i < 3; ++i) {
        thrs.push_back(MNSER::Thread::ptr(
                    new MNSER::Thread(&test_fiber, "name_" + std::to_string(i))));
    }
    for(auto i : thrs) {
        i->join();
    }
    return 0;
}
