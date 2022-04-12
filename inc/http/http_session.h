#ifndef __MNSER_HTTP_SESSION_H__
#define __MNSER_HTTP_SESSION_H__

#include "socket_stream.h"
#include "http.h"

namespace MNSER {
namespace http {
class HttpSession: public SocketStream {
public:
    typedef std::shared_ptr<HttpSession> ptr;

    HttpSession(Socket::ptr sock, bool owner = true);

	// 接收 http 请求
    HttpRequest::ptr recvRequest();

	// 发送 http 响应, 返回值：>0成功 =0对方关闭 <0socket异常
    int sendResponse(HttpResponse::ptr rsp);
};

}
}

#endif
