#include "hook.h"
#include "log.h"
#include "iomanager.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>

MNSER::Logger::ptr g_logger = MS_LOG_ROOT();

void test_sleep() {
	MNSER::IOManager iom(1);
	iom.schedule([](){ 
		MS_LOG_INFO(g_logger) << "sleep 10 begin";
		sleep(10);
		MS_LOG_INFO(g_logger) << "sleep 10 end";
	});
#if 1
	iom.schedule([](){ 
		MS_LOG_INFO(g_logger) << "sleep 2 begin";
		sleep(2);
		MS_LOG_INFO(g_logger) << "sleep 2 end";
	});
#endif
	MS_LOG_INFO(g_logger) << "Test in sleep()";
}

void test_sock() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, "220.181.38.251", &addr.sin_addr.s_addr);

    MS_LOG_INFO(g_logger) << "begin connect";
    int rt = connect(sock, (const sockaddr*)&addr, sizeof(addr));
    MS_LOG_INFO(g_logger) << "connect rt=" << rt << " errno=" << errno;

    if(rt) {
        return;
    }

    const char data[] = "GET / HTTP/1.0\r\n\r\n";
    rt = send(sock, data, sizeof(data), 0);
    MS_LOG_INFO(g_logger) << "send rt=" << rt << " errno=" << errno;

    if(rt <= 0) {
        return;
    }

    std::string buff;
    buff.resize(4096);

    rt = recv(sock, &buff[0], buff.size(), 0);
    MS_LOG_INFO(g_logger) << "recv rt=" << rt << " errno=" << errno;

    if(rt <= 0) {
        return;
    }

    buff.resize(rt);
    MS_LOG_INFO(g_logger) << buff;
}

int main(int argc, char* argv[]) {
	//test_sleep();
    MNSER::IOManager iom;
    iom.schedule(test_sock);
	return 0;
}
