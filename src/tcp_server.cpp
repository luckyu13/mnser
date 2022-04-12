#include "tcp_server.h"
#include "log.h"
#include "config.h"

namespace MNSER {
static MNSER::ConfigVar<uint64_t>::ptr g_tcp_server_read_timeout = 
	MNSER::Config::Lookup("tcp_server.read_timeout", (uint64_t)(60*1000*2), "tcp server read timeout");

static MNSER::Logger::ptr g_logger = MS_LOG_NAME("system");

TcpServer::TcpServer(MNSER::IOManager* worker, MNSER::IOManager* ioWorker
		 ,MNSER::IOManager* acceptWorker)
	: m_worker(worker)
	, m_ioWorker(ioWorker)
	, m_acceptWorker(acceptWorker)
	, m_recvTimeout(g_tcp_server_read_timeout->getValue()) 
	, m_name("ms v1.0")
	, m_isStop(true) {
}

TcpServer::~TcpServer() {
	for (auto& sock: m_socks) {
		sock->close();
	}
	m_socks.clear();
}

bool TcpServer::bind(MNSER::Address::ptr addr, bool ssl) {
	std::vector<Address::ptr> addrs;
	std::vector<Address::ptr> fails;
	addrs.push_back(addr);
	return bind(addrs, fails, ssl);
}

bool TcpServer::bind(const std::vector<Address::ptr>& addrs
		, std::vector<Address::ptr>& fails, bool ssl) {
	m_ssl = ssl;
	for (auto& addr: addrs) {
        Socket::ptr sock = Socket::CreateTCP(addr);
        if(!sock->bind(addr)) {
            MS_LOG_ERROR(g_logger) << "bind fail errno="
                << errno << " errstr=" << strerror(errno)
                << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        if(!sock->listen()) {
            MS_LOG_ERROR(g_logger) << "listen fail errno="
                << errno << " errstr=" << strerror(errno)
                << " addr=[" << addr->toString() << "]";
            fails.push_back(addr);
            continue;
        }
        m_socks.push_back(sock);
    }

    for(auto& i : m_socks) {
        MS_LOG_INFO(g_logger) << "type=" << m_type
            << " name=" << m_name
            << " ssl=" << m_ssl
            << " server bind success: " << *i;
    }
    if(!fails.empty()) {
		for(auto& i : fails) {
			MS_LOG_INFO(g_logger) << "Address bind fial: "
				<< i->toString();
		}
        //m_socks.clear();
        return false;
    }

    return true;
}

bool TcpServer::loadCertificates(const std::string& cert_file, const std::string& key_file) {
	MS_LOG_INFO(g_logger) << "Load Certificates";
}

bool TcpServer::start() {
    if(!m_isStop) {
        return true;
    }
    m_isStop = false;
    for(auto& sock : m_socks) {
        m_acceptWorker->schedule(std::bind(&TcpServer::startAccept,
                    shared_from_this(), sock));
    }
    return true;
}

void TcpServer::stop() {
    m_isStop = true;
    auto self = shared_from_this();
    m_acceptWorker->schedule([this, self]() {
        for(auto& sock : m_socks) {
            sock->cancelAll();
            sock->close();
        }
        m_socks.clear();
    });
}

void TcpServer::setConf(const TcpServerConf& v) {
    m_conf.reset(new TcpServerConf(v));
}

std::string TcpServer::toString(const std::string& prefix)  {
    std::stringstream ss;
    ss << prefix << "[type=" << m_type
       << " name=" << m_name
       << " worker=" << (m_worker ? m_worker->getName() : "")
       << " accept=" << (m_acceptWorker ? m_acceptWorker->getName() : "")
       << " recv_timeout=" << m_recvTimeout << "]" << std::endl;
    std::string pfx = prefix.empty() ? "    " : prefix;
    for(auto& i : m_socks) {
        ss << pfx << pfx << *i << std::endl;
    }
    return ss.str();
}

void TcpServer::handleClient(Socket::ptr client) {
	MS_LOG_INFO(g_logger) << "Handle Client";
}

void TcpServer::startAccept(Socket::ptr sock) {
    while(!m_isStop) {
        Socket::ptr client = sock->accept();
        if(client) {
            client->setRecvTimeout(m_recvTimeout);
            m_ioWorker->schedule(std::bind(&TcpServer::handleClient,
                        shared_from_this(), client));  // shared_from_this 是为了说明使用现在自己的 handleCilent
        } else {
            MS_LOG_ERROR(g_logger) << "accept errno=" << errno
                << " errstr=" << strerror(errno);
        }
    }
}

}

