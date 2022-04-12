#include "tcp_server.h"
#include "log.h"
#include "iomanager.h"

MNSER::Logger::ptr g_logger = MS_LOG_ROOT();

void run() {
    auto addr = MNSER::Address::LookupAny("0.0.0.0:8033");
    //auto addr2 = MNSER::UnixAddress::ptr(new MNSER::UnixAddress("/tmp/unix_addr"));
    std::vector<MNSER::Address::ptr> addrs;
    addrs.push_back(addr);
    //addrs.push_back(addr2);

    MNSER::TcpServer::ptr tcp_server(new MNSER::TcpServer);
    std::vector<MNSER::Address::ptr> fails;
    while(!tcp_server->bind(addrs, fails)) {
        sleep(2);
    }
    tcp_server->start();
    
}
int main(int argc, char** argv) {
    MNSER::IOManager iom(2);
    iom.schedule(run);
    return 0;
}

