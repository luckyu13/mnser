#include "config.h"

namespace MNSER {

	static void ListAllMember(const std::string& prefix, const YAML::Node& node, 
		std::list<std::pair<std::string, const YAML::Node> >& output) {
		if (prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._1234567890") != std::string::npos) {
			MS_LOG_ERROR(MS_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
			return ;
		}

		output.push_back(std::make_pair(prefix, node));
		if (node.IsMap()) {
			for (auto it = node.begin(); it != node.end(); ++it) {
				ListAllMember(prefix.empty() ? it->first.Scalar()
					: prefix + "." + it->first.Scalar(), it->second, output);
			}
		}
	}

	void Config::LoadFromYaml(const YAML::Node& root) {
		std::list<std::pair<std::string, const YAML::Node> > all_nodes;
		ListAllMember("", root, all_nodes); // 将所有的节点导入

		for (auto& it: all_nodes) {
			std::string key = it.first;
			if (key.empty()) {
				continue;
			}
			
			// 如果这里的 配置有更新，就更新内存中的配置信息
			std::transform(key.begin(), key.end(), key.begin(), ::tolower);
			ConfigVarBase::ptr var = LookupBase(key);

			if (var != nullptr) {
				if (it.second.IsScalar()) {
					var->fromString(it.second.Scalar());
				} else {
					std::stringstream ss;
					ss << it.second;
					var->fromString(ss.str());
				}
			}
		}
	}

	void Config::LoadFromConfigDir(const std::string& path, bool force) {

	}

	ConfigVarBase::ptr Config::LookupBase(const std::string& name) {
		RWLockType::ReadLock lock(GetMutex());
		auto it = GetDatas().find(name);
		return it == GetDatas().end() ? nullptr: it->second;
	}

	void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
		RWLockType::ReadLock lock(GetMutex());
		ConfigVarMap& m = GetDatas();
		for (auto& it: m) {
			cb(it.second);
		}
	}
};
