#ifndef __MNSER_HTTP_SERVER_H__
#define __MNSER_HTTP_SERVER_H__

#include "tcp_server.h"
#include "http_session.h"
#include "http_servlet.h"

namespace MNSER {
namespace http {

class HttpServer : public TcpServer {
public:
    typedef std::shared_ptr<HttpServer> ptr;

    HttpServer(bool keepalive = false
               ,MNSER::IOManager* worker = MNSER::IOManager::GetThis()
               ,MNSER::IOManager* io_worker = MNSER::IOManager::GetThis()
               ,MNSER::IOManager* accept_worker = MNSER::IOManager::GetThis());

    // 获取ServletDispatch
    ServletDispatch::ptr getServletDispatch() const { return m_dispatch;}

    // 设置ServletDispatch
    void setServletDispatch(ServletDispatch::ptr v) { m_dispatch = v;}

	// 设置名称
    virtual void setName(const std::string& v) override;

protected:
    virtual void handleClient(Socket::ptr client) override;

private:
    bool m_isKeepalive;  					// 是否支持长连接
    ServletDispatch::ptr m_dispatch;   		// Servlet分发器
};

}
}

#endif
