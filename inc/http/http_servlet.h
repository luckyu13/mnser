#ifndef __MNSER_HTTP_SERVLET_H__
#define __MNSER_HTTP_SERVLET_H__

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <unordered_map>

#include "http.h"
#include "http_session.h"
#include "thread.h"
#include "mutex.h"
#include "util.h"

namespace MNSER {
namespace http {
class Servlet {
public:
	typedef std::shared_ptr<Servlet> ptr;

    Servlet(const std::string& name)
        :m_name(name) {}

	virtual ~Servlet() {};
	
	// 处理请求 reques HTTP请求, response HTTP响应, session HTTP连接响应
    virtual int32_t handle(MNSER::http::HttpRequest::ptr request
                   , MNSER::http::HttpResponse::ptr response
                   , MNSER::http::HttpSession::ptr session) = 0;
	
	// 返回 Servlet 名称
	const std::string& getName() { return m_name; }
protected:
	std::string m_name;
};

// 函数式 Servlet
class FunctionServlet : public Servlet {
public:
    typedef std::shared_ptr<FunctionServlet> ptr;

    // 函数回调类型定义
    typedef std::function<int32_t (MNSER::http::HttpRequest::ptr request
                   , MNSER::http::HttpResponse::ptr response
                   , MNSER::http::HttpSession::ptr session)> callback;


    FunctionServlet(callback cb);

	// 处理请求 reques HTTP请求, response HTTP响应, session HTTP连接响应
    virtual int32_t handle(MNSER::http::HttpRequest::ptr request
                   , MNSER::http::HttpResponse::ptr response
                   , MNSER::http::HttpSession::ptr session) override;
private:
    callback m_cb;   // 回调函数
};

class IServletCreator {
public:
    typedef std::shared_ptr<IServletCreator> ptr;
    virtual ~IServletCreator() {}
    virtual Servlet::ptr get() const = 0;
    virtual std::string getName() const = 0;
};

class HoldServletCreator : public IServletCreator {
public:
    typedef std::shared_ptr<HoldServletCreator> ptr;
    HoldServletCreator(Servlet::ptr slt)
        :m_servlet(slt) {
    }

    Servlet::ptr get() const override {
        return m_servlet;
    }

    std::string getName() const override {
        return m_servlet->getName();
    }
private:
    Servlet::ptr m_servlet;
};

template<class T>
class ServletCreator : public IServletCreator {
public:
    typedef std::shared_ptr<ServletCreator> ptr;

    ServletCreator() {
    }

    Servlet::ptr get() const override {
        return Servlet::ptr(new T);
    }

    std::string getName() const override {
        return TypeToName<T>();
    }
};

// Servlet 分发器
class ServletDispatch : public Servlet {
public:
    typedef std::shared_ptr<ServletDispatch> ptr;
    typedef RWLock RWMutexType;

    ServletDispatch();

	// 处理请求 reques HTTP请求, response HTTP响应, session HTTP连接响应
    virtual int32_t handle(MNSER::http::HttpRequest::ptr request
                   , MNSER::http::HttpResponse::ptr response
                   , MNSER::http::HttpSession::ptr session) override;

    // 添加servlet
    void addServlet(const std::string& uri, Servlet::ptr slt);

    // 添加servlet
    void addServlet(const std::string& uri, FunctionServlet::callback cb);

    // 添加模糊匹配servlet
    void addGlobServlet(const std::string& uri, Servlet::ptr slt);

    // 添加模糊匹配servlet
    void addGlobServlet(const std::string& uri, FunctionServlet::callback cb);

	// 添加 IServletCreator
    void addServletCreator(const std::string& uri, IServletCreator::ptr creator);

	// 添加模糊 IServletCreator
    void addGlobServletCreator(const std::string& uri, IServletCreator::ptr creator);

	// 添加 ServletCreator
    template<class T>
    void addServletCreator(const std::string& uri) {
        addServletCreator(uri, std::make_shared<ServletCreator<T> >());
    }

	// 添加模糊 ServletCreator
    template<class T>
    void addGlobServletCreator(const std::string& uri) {
        addGlobServletCreator(uri, std::make_shared<ServletCreator<T> >());
    }

    // 删除servlet
    void delServlet(const std::string& uri);

    // 删除模糊匹配servlet
    void delGlobServlet(const std::string& uri);

    // 返回默认servlet
    Servlet::ptr getDefault() const { return m_default;}

    // 设置默认servlet
    void setDefault(Servlet::ptr v) { m_default = v;}

    // 通过uri获取servlet
    Servlet::ptr getServlet(const std::string& uri);

    // 通过uri获取模糊匹配servlet
    Servlet::ptr getGlobServlet(const std::string& uri);

    // 通过uri获取servlet
    Servlet::ptr getMatchedServlet(const std::string& uri);

	// 
    void listAllServletCreator(std::map<std::string, IServletCreator::ptr>& infos);
    void listAllGlobServletCreator(std::map<std::string, IServletCreator::ptr>& infos);
private:
    
    RWMutexType m_mutex;  													// 读写互斥量
    std::unordered_map<std::string, IServletCreator::ptr> m_datas;  		// 精准匹配servlet MAP
    std::vector<std::pair<std::string, IServletCreator::ptr> > m_globs;  	// 模糊匹配servlet 数组
    Servlet::ptr m_default;  												// 默认servlet，所有路径都没匹配到时使用
};

// NotFoundServlet 默认返回 404
class NotFoundServlet: public Servlet {
public:
    typedef std::shared_ptr<NotFoundServlet> ptr;

    NotFoundServlet();

	// 处理请求 reques HTTP请求, response HTTP响应, session HTTP连接响应
    virtual int32_t handle(MNSER::http::HttpRequest::ptr request
                   , MNSER::http::HttpResponse::ptr response
                   , MNSER::http::HttpSession::ptr session) override;

private:
    std::string m_content;
};

}
}

#endif

