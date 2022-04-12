#include "http_server.h"
#include "log.h"

static MNSER::Logger::ptr g_logger = MS_LOG_ROOT();

#define XX(...) #__VA_ARGS__


MNSER::IOManager::ptr worker;
void run() {
    g_logger->setLevel(MNSER::LogLevel::INFO);
    //MNSER::http::HttpServer::ptr server(new MNSER::http::HttpServer(true, worker.get(), MNSER::IOManager::GetThis()));
    MNSER::http::HttpServer::ptr server(new MNSER::http::HttpServer(true));
    MNSER::Address::ptr addr = MNSER::Address::LookupAnyIPAddress("0.0.0.0:8020");
    while(!server->bind(addr)) {
        sleep(2);
    }
    auto sd = server->getServletDispatch();
    sd->addServlet("/MNSER/xx", [](MNSER::http::HttpRequest::ptr req
                ,MNSER::http::HttpResponse::ptr rsp
                ,MNSER::http::HttpSession::ptr session) {
            rsp->setBody(req->toString());
            return 0;
    });

    sd->addGlobServlet("/MNSER/*", [](MNSER::http::HttpRequest::ptr req
                ,MNSER::http::HttpResponse::ptr rsp
                ,MNSER::http::HttpSession::ptr session) {
            rsp->setBody("Glob:\r\n" + req->toString());
            return 0;
    });

    sd->addGlobServlet("/MNSERx/*", [](MNSER::http::HttpRequest::ptr req
                ,MNSER::http::HttpResponse::ptr rsp
                ,MNSER::http::HttpSession::ptr session) {
            rsp->setBody(XX(<html>
<head><title>404 Not Found</title></head>
<body>
<center><h1>404 Not Found</h1></center>
<hr><center>nginx/1.16.0</center>
</body>
</html>
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
<!-- a padding to disable MSIE and Chrome friendly error page -->
));
            return 0;
    });

    server->start();
}

int main(int argc, char** argv) {
    MNSER::IOManager iom(1, true, "main");
    worker.reset(new MNSER::IOManager(3, false, "worker"));
    iom.schedule(run);
    return 0;
}
