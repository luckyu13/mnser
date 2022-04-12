#ifndef __MNSER_CONFIG_H__
#define __MNSER_CONFIG_H__


#include <memory>
#include <string>
#include <sstream>
#include <ctype.h>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <map>
#include <list>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <functional>

#include "log.h"
#include "util.h"
#include "mutex.h"

namespace  MNSER {

template <class F, class T>
class LexicalCast {
public:
	T operator() (const F& from) { // 基础类型使用 boost 库
		return boost::lexical_cast<T>(from);
	}
};

template <class T>
class LexicalCast<std::string, std::vector<T> > {
public:
	std::vector<T> operator() (const std::string& from) { 
		YAML::Node node = YAML::Load(from);
		typename std::vector<T> vec;
		std::stringstream ss;
		for (size_t i=0; i<node.size(); ++i) {
			ss.str("");	
			ss << node[i];
			vec.push_back(LexicalCast<std::string, T>()(ss.str()));
		}
		return vec;
	}
};

template <class T>
class LexicalCast<std::vector<T>, std::string> {
public:
	std::string operator() (const std::vector<T>& from) { 
		YAML::Node node(YAML::NodeType::Sequence);
		for (auto& it: from) {
			node.push_back(YAML::Load(LexicalCast<T, std::string>()(it)));
		}

		std::stringstream ss;
		ss << node;
		return ss.str();
	}
};

template <class T>
class LexicalCast<std::string, std::list<T> > {
public:
	std::list<T> operator() (const std::string& from) { 
		YAML::Node node = YAML::Load(from);
		typename std::list<T> li;
		std::stringstream ss;
		for (size_t i=0; i<node.size(); ++i) {
			ss.str("");	
			ss << node[i];
			li.push_back(LexicalCast<std::string, T>()(ss.str()));
		}
		return li;
	}
};

template <class T>
class LexicalCast<std::list<T>, std::string> {
public:
	std::string operator() (const std::list<T>& from) { 
		YAML::Node node(YAML::NodeType::Sequence);
		for (auto& it: from) {
			node.push_back(YAML::Load(LexicalCast<T, std::string>()(it)));
		}

		std::stringstream ss;
		ss << node;
		return ss.str();
	}
};

template <class T>
class LexicalCast<std::string, std::set<T> > {
public:
	std::set<T> operator() (const std::string& from) { 
		YAML::Node node = YAML::Load(from);
		typename std::set<T> se;
		std::stringstream ss;
		for (size_t i=0; i<node.size(); ++i) {
			ss.str("");	
			ss << node[i];
			se.insert(LexicalCast<std::string, T>()(ss.str()));
		}
		return se;
	}
};

template <class T>
class LexicalCast<std::set<T>, std::string> {
public:
	std::string operator() (const std::set<T>& from) { 
		YAML::Node node(YAML::NodeType::Sequence);
		for (auto& it: from) {
			node.push_back(YAML::Load(LexicalCast<T, std::string>()(it)));
		}

		std::stringstream ss;
		ss << node;
		return ss.str();
	}
};

template <class T>
class LexicalCast<std::string, std::unordered_set<T> > {
public:
	std::unordered_set<T> operator() (const std::string& from) { 
		YAML::Node node = YAML::Load(from);
		typename std::unordered_set<T> se;
		std::stringstream ss;
		for (size_t i=0; i<node.size(); ++i) {
			ss.str("");	
			ss << node[i];
			se.insert(LexicalCast<std::string, T>()(ss.str()));
		}
		return se;
	}
};

template <class T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
	std::string operator() (const std::unordered_set<T>& from) { 
		YAML::Node node(YAML::NodeType::Sequence);
		for (auto& it: from) {
			node.push_back(YAML::Load(LexicalCast<T, std::string>()(it)));
		}

		std::stringstream ss;
		ss << node;
		return ss.str();
	}
};

template <class T>
class LexicalCast<std::string, std::map<std::string, T> > {
public:
	std::map<std::string, T> operator() (const std::string& from) { 
		YAML::Node node = YAML::Load(from);
		typename std::map<std::string, T> ma;
		std::stringstream ss;
		for (auto it = node.begin(); it != node.end(); ++it) {
			ss.str("");	
			ss << it->second;
			ma.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
		}
		return ma;
	}
};

template <class T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
	std::string operator() (const std::map<std::string, T>& from) { 
		YAML::Node node(YAML::NodeType::Map);
		for (auto& it: from) {
			node[it.first] = YAML::Load(LexicalCast<T, std::string>()(it.second));
		}
		std::stringstream ss;
		ss << node;
		return ss.str();
	}
};

template <class T>
class LexicalCast<std::string, std::unordered_map<std::string, T> > {
public:
	std::unordered_map<std::string, T> operator() (const std::string& from) { 
		YAML::Node node = YAML::Load(from);
		typename std::unordered_map<std::string, T> ma;
		std::stringstream ss;
		for (auto it = node.begin(); it != node.end(); ++it) {
			ss.str("");	
			ss << it->second;
			ma.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
		}
		return ma;
	}
};

template <class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
	std::string operator() (const std::unordered_map<std::string, T>& from) { 
		YAML::Node node(YAML::NodeType::Map);
		for (auto& it: from) {
			node[it.first] = YAML::Load(LexicalCast<T, std::string>()(it.second));
		}
		std::stringstream ss;
		ss << node;
		return ss.str();
	}
};

class ConfigVarBase {
public:
	typedef std::shared_ptr<ConfigVarBase> ptr;

	ConfigVarBase(const std::string &name, const std::string& description):
		m_name(name),
		m_description(description) {
		std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);  // 为方便管理，将配置器名称改成小写字母
	}
	virtual ~ConfigVarBase() {}

public:
	const std::string& getName() const { return m_name; }
	const std::string& getDescription() const { return m_description; }
	
	/*
	 * 将 YAML 节点转换成 字符串
	 */
	virtual std::string toString() = 0; 

	/*
	 * 从字符串中初始换内容
	 */
	virtual bool fromString(const std::string& val) = 0;

	/*
	 * 获取配置器参数名称
	 */
	virtual std::string getTypeName() const = 0;

protected:
	std::string m_name;  			// 配置器名称
	std::string m_description;		// 配置器参数描述
};

template <class T, 
		  class FromStr=LexicalCast<std::string, T>, 
		  class ToStr=LexicalCast<T, std::string> >
class ConfigVar: public ConfigVarBase {
public:
	typedef std::shared_ptr<ConfigVar> ptr;
	typedef std::function< void (const T& old_val, const T& new_val) > on_change_val;
	typedef RWLock RWLockType;

	ConfigVar(const std::string& name, 
			  const T& default_value,
			  const std::string& description = ""):
		ConfigVarBase(name, description), m_val(default_value) 
		{ }
	
	std::string toString() override {
		try {
			RWLockType::ReadLock lock(m_mutex);
			return ToStr()(m_val);
		} catch (std::exception& e) {
			MS_LOG_ERROR(MS_LOG_ROOT()) << "ConfigVar::toString exception: "
			<< e.what() << " convert: " << TypeToName<T>() << " to string" 
			<< " name = " << m_name;
		}
		return "";
	}

	bool fromString(const std::string& val) override {
		try {
			setValue(FromStr()(val));
		} catch (std::exception& e) {
			MS_LOG_ERROR(MS_LOG_ROOT()) << "ConfigVar::fromString exception: "
			<< e.what() << " convert string to " << TypeToName<T>() 
			<< " name =" << m_name << " - "  << val;
		}
		return false;
	}

	std::string getTypeName() const override {
		return TypeToName<T>();
	}

	void setValue(const T& v) {
		{
			RWLockType::ReadLock lock(m_mutex);
			if (v == m_val) {
				return;
			}
			for (auto &it: m_cbs) {
				it.second(m_val, v);
			}
		}
		RWLockType::WriteLock lock(m_mutex);
		m_val = v;
	}

	const T getValue() {
		RWLockType::ReadLock lock(m_mutex);
		return m_val;
	}

	int addListener(on_change_val cb) {
		static uint64_t s_cb_id = 0;
		RWLockType::WriteLock lock(m_mutex);
		s_cb_id++;
		m_cbs[s_cb_id] = cb;
		return s_cb_id;
	}

	void delListener(uint64_t cb_id) {
		RWLockType::WriteLock lock(m_mutex);
		m_cbs.erase(cb_id);
	}

	on_change_val getListenser(uint64_t cb_id) {
		RWLockType::ReadLock lock(m_mutex);
		auto it = m_cbs.find(cb_id);
		return it == m_cbs.end() ? nullptr: it->second;
	}

	void clearListener() {
		RWLockType::WriteLock lock(m_mutex);
		m_cbs.clear();
	}

private:
	T m_val;			// 自己配置的参数值
	std::map<int, on_change_val> m_cbs;  // function 对象不支持判断是否相同，所有使用一个 map 来实现回调函数的唯一性

	RWLockType m_mutex;
};

class Config {
public:
	typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;
	typedef RWLock RWLockType;


	template <class T> 
	static typename ConfigVar<T>::ptr Lookup(const std::string& name, const T& default_value, const std::string& description="") {
		RWLockType::WriteLock lock(GetMutex());
		auto it = GetDatas().find(name);
		if (it != GetDatas().end()) {
			auto tmp = std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
			if (tmp != nullptr) {
				MS_LOG_INFO(MS_LOG_ROOT()) << "Lookup name = " << name << " exists.";
				return tmp;
			} else {
				MS_LOG_ERROR(MS_LOG_ROOT()) << "Lookup name = " << name << " exists but type not "
				<< TypeToName<T>() << " real type = " << it->second->getTypeName() << " " 
				<< it->second->toString();
				return nullptr;
			}
		}

		if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._1234567890") != std::string::npos) {
			MS_LOG_ERROR(MS_LOG_ROOT()) << "Lookup name invlid " << name;
			throw std::invalid_argument(name);
		}

		typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
		GetDatas()[name] = v; 
		return v;
	}

	template <class T>
	static typename ConfigVar<T>::ptr Lookup(const std::string& name) {
		RWLockType::ReadLock lock(GetMutex());
		auto it = GetDatas().find(name);
		if (it == GetDatas().end()) {
			return nullptr;
		}
		return std::dynamic_pointer_cast<ConfigVar<T> >(it->second);
	}

	/*
	 * 从 Yaml 节点中导入信息
	 */
	static void LoadFromYaml(const YAML::Node& root); 

	/*
	 * 从文件中读取配置信息
	 */
	static void LoadFromConfigDir(const std::string& path, bool force=false);

	static ConfigVarBase::ptr LookupBase(const std::string& name);

	static void Visit(std::function<void(ConfigVarBase::ptr)> cb);

private:
	/*
	 * 使用静态函数，可以保证这个变量会在使用的时候一定被初始化了
	 * 使用静态成员函数是因为
	 *   静态成员函数的初始化顺序不确定，
	 *   通过静态成员函数的方法，可以确保这个静态成员在需要的时候一定会被创建
	 */
	static ConfigVarMap& GetDatas() {
		static ConfigVarMap s_datas;
		return s_datas;
	}
	
	static  RWLockType& GetMutex() {
		static RWLockType s_mutex;
		return s_mutex;
	}
};

}
#endif
