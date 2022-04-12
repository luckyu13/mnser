#include "http/http.h"
#include "log.h"

void test_request() {
    MNSER::http::HttpRequest::ptr req(new MNSER::http::HttpRequest);
    req->setHeader("host" , "www.baidu.com");
    req->setBody("hello MNSER");
    req->dump(std::cout) << std::endl;
}

void test_response() {
    MNSER::http::HttpResponse::ptr rsp(new MNSER::http::HttpResponse);
    rsp->setHeader("X-X", "MNSER");
    rsp->setBody("hello MNSER");
    rsp->setStatus((MNSER::http::HttpStatus)400);
    rsp->setClose(false);

    rsp->dump(std::cout) << std::endl;
}

int main(int argc, char** argv) {
    test_request();
    test_response();
    return 0;
}
