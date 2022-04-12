#include "mnser.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>

MNSER::Logger::ptr g_logger = MS_LOG_ROOT();
int sock = 0;
void test_fiber() {
	MS_LOG_INFO(g_logger) << "Test in fiber";

	sock = socket(AF_INET, SOCK_STREAM, 0);
	fcntl(sock, F_SETFL, O_NONBLOCK);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);

    inet_pton(AF_INET, "220.181.38.148", &addr.sin_addr.s_addr);

	if(!connect(sock, (const sockaddr*)&addr, sizeof(addr))) {
	} else if(errno == EINPROGRESS) {
		MS_LOG_INFO(g_logger) << "add event errno=" << errno << " " << strerror(errno);
		MNSER::IOManager::GetThis()->addEvent(sock, MNSER::IOManager::READ, [](){
			MS_LOG_INFO(g_logger) << "read callback";
		});
		MNSER::IOManager::GetThis()->addEvent(sock, MNSER::IOManager::WRITE, [](){
			MS_LOG_INFO(g_logger) << "write callback";
			//close(sock);
			MNSER::IOManager::GetThis()->cancelEvent(sock, MNSER::IOManager::READ);
			close(sock);
		});
	} else {
		MS_LOG_INFO(g_logger) << "else " << errno << " " << strerror(errno);
	}
}

void test1() {
	MNSER::IOManager iom(2, false);
	iom.schedule(&test_fiber);
}

MNSER::Timer::ptr s_timer;
void test_timer() {
    MNSER::IOManager iom(2);
    s_timer = iom.addTimer(5000, [](){
        static int i = 0;
        MS_LOG_INFO(g_logger) << "hello timer i=" << i;
		#if 1
        if(++i == 3) {
            s_timer->reset(10000, true);  // 重新设置时间
            //s_timer->cancel();
        }
		#endif
    }, true);
}

int main(int argc, char* argv[]) {
	//test1();
	test_timer();
	return 0;
}
