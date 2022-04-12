#ifndef __MNSER_HTTP_PARSER_H__
#define __MNSER_HTTP_PARSER_H__

#include <memory>

#include "http.h"
#include "http11_parser.h"
#include "httpclient_parser.h"

namespace MNSER {
namespace http {

class HttpRequestParser {
public:
	typedef std::shared_ptr<HttpRequestParser> ptr;

    HttpRequestParser();
	// 是否解析完成
    int isFinished();
	// 是否有错误
    int hasError(); 
	// 解析协议
    size_t execute(char* data, size_t len);
	// 返回消息体长度
    uint64_t getContentLength();
    HttpRequest::ptr 	getData() 	const { return m_data;}  	// 返回 HttpRequest 对象
    const http_parser& 	getParser() const { return m_parser;}  	// 返回 http_parser 结构体
    void setError(int v) { m_error = v;} 						// 设置错误

public:
	// 返回 HttpRequest 协议解析换曾大小
    static uint64_t GetHttpRequestBufferSize();
	// 返回 HttpRequest 协议最大消息体大小
    static uint64_t GetHttpRequestMaxBodySize();
private:
    http_parser m_parser;  		// 解析结构体
    HttpRequest::ptr m_data;  	// HttpRequest对象
    int m_error;  				// 错误码, 1000: invalid method, 1001: invalid version, 1002: invalid field
};

class HttpResponseParser {
public:
	typedef std::shared_ptr<HttpResponseParser> ptr;

	HttpResponseParser();
	// 是否解析完成
    int isFinished();
	// 是否有错误
    int hasError(); 
	// 解析协议
    size_t execute(char* data, size_t len, bool chunck);
	// 返回消息体长度
    uint64_t getContentLength();
    HttpResponse::ptr getData() 	const { return m_data;}  	// 返回 HttpRequest 对象
    const httpclient_parser& getParser() const { return m_parser;}  	// 返回 http_parser 结构体
    void setError(int v) { m_error = v;}  						// 设置错误

public:
	// 返回 HttpResponse 协议解析换曾大小
    static uint64_t GetHttpResponseBufferSize();
	// 返回 HttpRequest 协议最大消息体大小
    static uint64_t GetHttpResponseMaxBodySize();
private:
    httpclient_parser m_parser;
    HttpResponse::ptr m_data;
    int m_error;   // 错误码, 1001: invalid version, 1002: invalid field
};

}
}
#endif
