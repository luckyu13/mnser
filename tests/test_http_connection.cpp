#include "log.h"
#include "iomanager.h"
#include "http_parser.h"
#include "address.h"
#include "socket.h"
#include "http_connection.h"

#include <fstream>


static MNSER::Logger::ptr g_logger = MS_LOG_ROOT();

void test_pool() {
    MNSER::http::HttpConnectionPool::ptr pool(new MNSER::http::HttpConnectionPool(
                "www.sylar.top", "", 80, false, 10, 1000 * 30, 5));

    MNSER::IOManager::GetThis()->addTimer(1000, [pool](){
            auto r = pool->doGet("/", 300);
            MS_LOG_INFO(g_logger) << r->toString();
    }, true);
}

void run() {
#if 0
    //MNSER::Address::ptr addr = MNSER::Address::LookupAnyIPAddress("www.baidu.com:80");
    MNSER::Address::ptr addr = MNSER::Address::LookupAnyIPAddress("www.sylar.top:80");
    if(!addr) {
        MS_LOG_INFO(g_logger) << "get addr error";
        return;
    }

    MNSER::Socket::ptr sock = MNSER::Socket::CreateTCP(addr);
    bool rt = sock->connect(addr);
    if(!rt) {
        MS_LOG_INFO(g_logger) << "connect " << *addr << " failed";
        return;
    }

    MNSER::http::HttpConnection::ptr conn(new MNSER::http::HttpConnection(sock));
    MNSER::http::HttpRequest::ptr req(new MNSER::http::HttpRequest);
    req->setPath("/blog/");
    req->setHeader("host", "www.sylar.top");
   	// req->setHeader("host", "www.baidu.com");
    MS_LOG_INFO(g_logger) << "req:" << std::endl
        << *req;

    conn->sendRequest(req);
    auto rsp = conn->recvResponse();

    if(!rsp) {
        MS_LOG_INFO(g_logger) << "recv response error";
        return;
    }
    MS_LOG_INFO(g_logger) << "rsp:" << std::endl
        << *rsp;

    //std::ofstream ofs("rsp.dat");
    //ofs << *rsp;

    MS_LOG_INFO(g_logger) << "=========================";

    auto r = MNSER::http::HttpConnection::DoGet("http://www.sylar.top/blog/", 300);
    MS_LOG_INFO(g_logger) << "result=" << r->result
        << " error=" << r->error
        << " rsp=" << (r->response ? r->response->toString() : "");

    //MS_LOG_INFO(g_logger) << "=========================";
#endif
    test_pool();
}

int main(int argc, char **argv) {
	MNSER::IOManager iom(2);
	iom.schedule(run);
	return 0;
}

