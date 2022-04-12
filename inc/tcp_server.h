#ifndef __MNSER_TCP_SERVER_H__
#define __MNSER_TCP_SERVER_H__

#include <memory>
#include <functional>
#include <vector>

#include "address.h"
#include "iomanager.h"
#include "socket.h"
#include "noncopyable.h"
#include "config.h"

namespace MNSER {

struct TcpServerConf {
	typedef std::shared_ptr<TcpServerConf> ptr;

	std::vector<std::string> address;

	int keepalive = 0;				// 是否是长连接
	int timeout = 1000*2*60;  		// 超时时间，默认 2min
	int ssl = 0;	
	std::string id;
	std::string type="http";
    std::string name;
    std::string cert_file;
    std::string key_file;
    std::string accept_worker;
    std::string io_worker;
    std::string process_worker;
    std::map<std::string, std::string> args;

    bool isValid() const {
        return !address.empty();
    }

    bool operator==(const TcpServerConf& oth) const {
        return address == oth.address
            && keepalive == oth.keepalive
            && timeout == oth.timeout
            && name == oth.name
            && ssl == oth.ssl
            && cert_file == oth.cert_file
            && key_file == oth.key_file
            && accept_worker == oth.accept_worker
            && io_worker == oth.io_worker
            && process_worker == oth.process_worker
            && args == oth.args
            && id == oth.id
            && type == oth.type;
    }
};

// 配置系统词义转换
template<>
class LexicalCast<std::string, TcpServerConf> {
public:
    TcpServerConf operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        TcpServerConf conf;
        conf.id = node["id"].as<std::string>(conf.id);
        conf.type = node["type"].as<std::string>(conf.type);
        conf.keepalive = node["keepalive"].as<int>(conf.keepalive);
        conf.timeout = node["timeout"].as<int>(conf.timeout);
        conf.name = node["name"].as<std::string>(conf.name);
        conf.ssl = node["ssl"].as<int>(conf.ssl);
        conf.cert_file = node["cert_file"].as<std::string>(conf.cert_file);
        conf.key_file = node["key_file"].as<std::string>(conf.key_file);
        conf.accept_worker = node["accept_worker"].as<std::string>();
        conf.io_worker = node["io_worker"].as<std::string>();
        conf.process_worker = node["process_worker"].as<std::string>();
        conf.args = LexicalCast<std::string
            ,std::map<std::string, std::string> >()(node["args"].as<std::string>(""));
        if(node["address"].IsDefined()) {
            for(size_t i = 0; i < node["address"].size(); ++i) {
                conf.address.push_back(node["address"][i].as<std::string>());
            }
        }
        return conf;
    }
};

// 配置系统词义转换
template<>
class LexicalCast<TcpServerConf, std::string> {
public:
    std::string operator()(const TcpServerConf& conf) {
        YAML::Node node;
        node["id"] = conf.id;
        node["type"] = conf.type;
        node["name"] = conf.name;
        node["keepalive"] = conf.keepalive;
        node["timeout"] = conf.timeout;
        node["ssl"] = conf.ssl;
        node["cert_file"] = conf.cert_file;
        node["key_file"] = conf.key_file;
        node["accept_worker"] = conf.accept_worker;
        node["io_worker"] = conf.io_worker;
        node["process_worker"] = conf.process_worker;
        node["args"] = YAML::Load(LexicalCast<std::map<std::string, std::string>
            , std::string>()(conf.args));
        for(auto& i : conf.address) {
            node["address"].push_back(i);
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// TCP 服务器封装
class TcpServer: public std::enable_shared_from_this<TcpServer>,
		Noncopyable {
public:
	typedef std::shared_ptr<TcpServer> ptr;

	TcpServer(MNSER::IOManager* worker=MNSER::IOManager::GetThis()
			 ,MNSER::IOManager* ioWorker=MNSER::IOManager::GetThis()
			 ,MNSER::IOManager* acceptWorker=MNSER::IOManager::GetThis());

	virtual ~TcpServer();

    // 绑定地址, 返回是否绑定成功
    virtual bool bind(MNSER::Address::ptr addr, bool ssl = false);

    // 绑定地址数组
    virtual bool bind(const std::vector<Address::ptr>& addrs
                        ,std::vector<Address::ptr>& fails
                        ,bool ssl = false);

	// 导入证书
    bool loadCertificates(const std::string& cert_file, const std::string& key_file);

    // 启动服务
    virtual bool start();

    // 停止服务
    virtual void stop();

    // 返回读取超时时间(毫秒)
    uint64_t getRecvTimeout() const { return m_recvTimeout;}

    // 返回服务器名称
    std::string getName() const { return m_name;}

    // 设置读取超时时间(毫秒)
    void setRecvTimeout(uint64_t v) { m_recvTimeout = v;}

    // 设置服务器名称
    virtual void setName(const std::string& v) { m_name = v;}

    // 是否停止
    bool isStop() const { return m_isStop;}

	// 获取服务器配置
    TcpServerConf::ptr getConf() const { return m_conf;}

	// 设置服务器的配置
    void setConf(TcpServerConf::ptr v) { m_conf = v;}

	// 设置服务器的配置
    void setConf(const TcpServerConf& v);

	// 服务器信息输出
    virtual std::string toString(const std::string& prefix = "");

	// 获取当前连接的sock
    std::vector<Socket::ptr> getSocks() const { return m_socks;}

protected:
    // 处理新连接的Socket类
    virtual void handleClient(Socket::ptr client);

    // 开始接受连接
    virtual void startAccept(Socket::ptr sock);

protected:
    std::vector<Socket::ptr> m_socks;    		// 监听Socket数组
    IOManager* m_worker;    					// 新连接的Socket工作的调度器
    IOManager* m_ioWorker;
    IOManager* m_acceptWorker;    				// 服务器Socket接收连接的调度器
    uint64_t m_recvTimeout;    					// 接收超时时间(毫秒)
    std::string m_name;    						// 服务器名称
    std::string m_type = "tcp";    				// 服务器类型
    bool m_isStop;    							// 服务器是否停止
    bool m_ssl = false;
    TcpServerConf::ptr m_conf;					// 服务器配置
};


}

#endif
