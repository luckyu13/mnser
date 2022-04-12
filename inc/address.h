#ifndef __MNSER_ADDRESS_H__
#define __MNSER_ADDRESS_H__

#include <iostream>
#include <string.h>
#include <memory>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <map>
#include <sys/un.h>
#include <netdb.h>
#include <ifaddrs.h>


namespace MNSER {

class IPAddress;

class Address {
public:
	typedef std::shared_ptr<Address> ptr;

	// 通过 sockaddr 创建 Address， 返回余 sockaddr 相匹配的 Address，失败返回nullptr
	static Address::ptr Create(const sockaddr* addr, socklen_t addrlen);

	// 通过 host 地址返回对应条件的所有的 Address
	static bool Lookup(std::vector<Address::ptr>& result, const std::string& host, 
		int family=AF_INET, int type=0, int protocol=0);

	// 通过 host 地址返回任意一个 Address
	static Address::ptr LookupAny(const std::string& host, int family=AF_INET,
		int type=0, int protocol=0);

	// 通过 host 地址返回任意一个 IPAdress
	static std::shared_ptr<IPAddress> LookupAnyIPAddress(const std::string& host,
		int family=AF_INET, int type=0, int protocol=0);

	// 返回本机所有网卡名，地址，子网掩码位数
	static bool GetInterfaceAddresses(std::multimap<std::string, 
		std::pair<Address::ptr, uint32_t> >& result, int family=AF_INET);

	// 获取指定网卡的地址和子网掩码位数
	static bool GetInterfaceAddresses(std::vector<std::pair<Address::ptr, uint32_t> >& result,
		const std::string& iface, int family=AF_INET);

	virtual ~Address();

	// 获取协议族
	int getFamily() const;

	// 获取地址，不可读写
	virtual const sockaddr* getAddr() const = 0;

	// 获取地址，可读写
	virtual sockaddr* getAddr() = 0;

	// 获取 sockaddr 长度
	virtual socklen_t getAddrLen() const = 0;

	// 可读性持续输出地址
	virtual std::ostream& insert(std::ostream& os) const = 0;

	// 返回可读性字符串
	std::string toString() const;

	// 小于号
	bool operator<(const Address& rhs) const;

	// 等于号
	bool operator==(const Address& rhs) const;

	// 不等于
	bool operator!=(const Address& rhs) const;
};

class IPAddress: public Address {
public:
	typedef std::shared_ptr<IPAddress> ptr;

	// 通过域名创建 IPAddress
	static IPAddress::ptr Create(const char* address, uint16_t port=0);

	// 获取地址的广播地址，prefix_len子网掩码位数
	virtual IPAddress::ptr broadcastAddress(uint32_t prefix_len) = 0;

	// 获取地址的网段，prefix_len子网掩码位数
	virtual IPAddress::ptr networdAddress(uint32_t prefix_len) = 0;

	// 获取子网掩码地址
	virtual IPAddress::ptr subnetMask(uint32_t prefix_len) = 0;

	// 返回端口号
	virtual uint32_t getPort() const = 0;

	// 设置端口号
	virtual void setPort(uint16_t v) = 0;
};

class IPv4Address: public IPAddress {
public:
	typedef std::shared_ptr<IPv4Address> ptr;

	// 通过文本式的四段式IP创建ip地址
	static IPv4Address::ptr Create(const char* address, uint16_t port=0);

	IPv4Address(const sockaddr_in& address);

	IPv4Address(uint32_t address=INADDR_ANY, uint16_t port=0);

	const sockaddr* getAddr() const  override;
	sockaddr* getAddr() override;
	socklen_t getAddrLen() const override;
	std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;
    void setPort(uint16_t v) override;

private:
	sockaddr_in m_addr;
};

class IPv6Address: public IPAddress {
public:
	typedef std::shared_ptr<IPv6Address> ptr;

	// 通过四段式IP创建ip地址
	static IPv6Address::ptr Create(const char* address, uint16_t port=0);

	IPv6Address();

	IPv6Address(const sockaddr_in6& address);

	IPv6Address(const uint8_t address[16], uint16_t port=0);

	const sockaddr* getAddr() const  override;
	sockaddr* getAddr() override;
	socklen_t getAddrLen() const override;
	std::ostream& insert(std::ostream& os) const override;

    IPAddress::ptr broadcastAddress(uint32_t prefix_len) override;
    IPAddress::ptr networdAddress(uint32_t prefix_len) override;
    IPAddress::ptr subnetMask(uint32_t prefix_len) override;
    uint32_t getPort() const override;
    void setPort(uint16_t v) override;

private:
	sockaddr_in6 m_addr;
};

class UnixAddress: public Address {
public:
	typedef std::shared_ptr<UnixAddress> ptr;
	
	UnixAddress();

	UnixAddress(const std::string& path);

    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
	std::ostream& insert(std::ostream& os) const override;
    socklen_t getAddrLen() const override;

    void setAddrLen(uint32_t v);
    std::string getPath() const;

private:
	sockaddr_un m_addr;
	socklen_t m_length;
};

class UnknownAddress: public Address {
public:
	typedef std::shared_ptr<UnknownAddress> ptr;

	UnknownAddress(int family);
	UnknownAddress(const sockaddr& addr);
    const sockaddr* getAddr() const override;
    sockaddr* getAddr() override;
	std::ostream& insert(std::ostream& os) const override;
    socklen_t getAddrLen() const override;

private:
	sockaddr m_addr;
};

std::ostream& operator<<(std::ostream& os, const Address& addr);

}

#endif
