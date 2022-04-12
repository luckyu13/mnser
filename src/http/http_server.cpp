#include "http_server.h"


namespace MNSER {
namespace http {

HttpServer::HttpServer(bool keepalive, MNSER::IOManager* worker, MNSER::IOManager* io_worker 
		   ,MNSER::IOManager* accept_worker )
	: TcpServer(worker, io_worker, accept_worker)
	, m_isKeepalive(keepalive) {

	m_dispatch.reset(new ServletDispatch);

    m_type = "http";
#if 0
    m_dispatch->addServlet("/_/status", Servlet::ptr(new StatusServlet));
    m_dispatch->addServlet("/_/config", Servlet::ptr(new ConfigServlet));
#endif
}

static MNSER::Logger::ptr g_logger = MS_LOG_NAME("system");

void HttpServer::setName(const std::string& v) {
    TcpServer::setName(v);
    m_dispatch->setDefault(std::make_shared<NotFoundServlet>());
}

void HttpServer::handleClient(Socket::ptr client)  {
    MS_LOG_DEBUG(g_logger) << "handleClient " << *client;
    HttpSession::ptr session(new HttpSession(client));
    do {
        auto req = session->recvRequest();
        if(!req) {
            MS_LOG_DEBUG(g_logger) << "recv http request fail, errno="
                << errno << " errstr=" << strerror(errno)
                << " cliet:" << *client << " keep_alive=" << m_isKeepalive;
            break;
        }

        HttpResponse::ptr rsp(new HttpResponse(req->getVersion()
                            ,req->isClose() || !m_isKeepalive));
        rsp->setHeader("Server", getName());
        m_dispatch->handle(req, rsp, session);
        session->sendResponse(rsp);

        if(!m_isKeepalive || req->isClose()) {
            break;
        }
    } while(true);
    session->close();
}

}
}
