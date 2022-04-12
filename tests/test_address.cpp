#include "address.h"
#include "log.h"

MNSER::Logger::ptr g_logger = MS_LOG_ROOT();

void test() {
	std::vector<MNSER::Address::ptr> addrs;

	MS_LOG_INFO(g_logger) << "begin";
	bool v = MNSER::Address::Lookup(addrs, "www.baidu.com:ftp");
	if (!v) {
		MS_LOG_ERROR(g_logger) << "Lookup fail";
		return;
	}

	for (size_t i=0; i<addrs.size(); ++i) {
		MS_LOG_INFO(g_logger) << addrs[i]->toString();
	}

	MS_LOG_INFO(g_logger) << "end";
}

void test_iface() {
	std::multimap<std::string, std::pair<MNSER::Address::ptr, uint32_t> > results;
	bool v = MNSER::Address::GetInterfaceAddresses(results, AF_INET6);
	if (!v) {
		MS_LOG_ERROR(g_logger) << "GetInterfaceAddresses fail";
		return ;
	}

	for (auto&i : results) {
		MS_LOG_INFO(g_logger) << i.first << " - " << i.second.first->toString() 
			<< " - " << i.second.second;
	}
}

void test_ipv4() {
	auto addr = MNSER::IPAddress::Create("127.0.0.8");
	//auto addr = MNSER::IPAddress::Create("www.baidu.com");
	if (addr) {
		MS_LOG_INFO(g_logger) << addr->toString();
	}
}

int main(int argc, char ** argv) {
	//test();
	//test_iface();
	test_ipv4();
	return 0;
}
