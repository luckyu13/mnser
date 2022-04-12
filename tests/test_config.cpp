#include "config.h"
#include "log.h"

#include <yaml-cpp/yaml.h>

void printYaml(const YAML::Node& node, int level) {
	if (node.IsScalar()) {
		MS_LOG_INFO(MS_LOG_ROOT()) << std::string(level * 4, ' ')
		<< node.Scalar() << " - " << node.Type() << " - " << level;
	} else if (node.IsNull()) {
		MS_LOG_INFO(MS_LOG_ROOT()) << std::string(level * 4, ' ')
		<< "Null" << " - " << node.Type() << " - " << level;
	} else if (node.IsMap()) {
		for (auto it = node.begin(); it != node.end(); ++it)  {
			MS_LOG_INFO(MS_LOG_ROOT()) << std::string(level * 4, ' ')
			<< it->first << " - " << it->second.Type() << " - " << level;
			printYaml(it->second, level + 1);
		}
	} else if (node.IsSequence()) {
		for (size_t i=0; i<node.size(); ++i) {
			MS_LOG_INFO(MS_LOG_ROOT()) << std::string(level * 4, ' ')
			<< i << " - " << node[i].Type() << " - " << level;
			printYaml(node[i], level + 1);
		}
	}
}

void test_yaml() {
	YAML::Node root = YAML::LoadFile("/home/lucky/WebServer/tests/log.yml");
	MS_LOG_INFO(MS_LOG_ROOT()) << root["test"].IsDefined();
	MS_LOG_INFO(MS_LOG_ROOT()) << root["logs"].IsDefined();
	MS_LOG_INFO(MS_LOG_ROOT()) << root;
	//MS_LOG_INFO(MS_LOG_ROOT()) << "===========***==============";
	//printYaml(root, 0);

}
#if 0

MNSER::ConfigVar<int>::ptr g_int_value_config = 
	MNSER::Config::Lookup("system.port", (int)8080, "system port");

MNSER::ConfigVar<float>::ptr g_float_value_config = 
	MNSER::Config::Lookup("system.value", (float)1.3f, "system value");

MNSER::ConfigVar<std::vector<int> >::ptr g_int_vec_value_config = 
	MNSER::Config::Lookup("system.int_vec", std::vector<int>{1,2}, "system int_vec");

MNSER::ConfigVar<std::list<int> >::ptr g_int_list_value_config = 
	MNSER::Config::Lookup("system.int_list", std::list<int>{1,2}, "system int_list");

MNSER::ConfigVar<std::set<int> >::ptr g_int_set_value_config = 
	MNSER::Config::Lookup("system.int_set", std::set<int>{1,2}, "system int_set");

MNSER::ConfigVar<std::unordered_set<int> >::ptr g_int_uset_value_config = 
	MNSER::Config::Lookup("system.int_uset", std::unordered_set<int>{1,2}, "system int_uset");

MNSER::ConfigVar<std::map<std::string, int> >::ptr g_int_map_value_config = 
	MNSER::Config::Lookup("system.str_int_map", std::map<std::string, int>{{"k",1000}}, "system str_int_map");

MNSER::ConfigVar<std::unordered_map<std::string, int> >::ptr g_int_umap_value_config = 
	MNSER::Config::Lookup("system.str_int_umap", std::unordered_map<std::string, int>{{"k",1000}}, "system str_int_umap");

void test_config() {
	MS_LOG_INFO(MS_LOG_ROOT()) << "int before: " << g_int_value_config->getValue();
	MS_LOG_INFO(MS_LOG_ROOT()) << "float before: " << g_float_value_config->getValue();
#define PP(var, name, prefix) \
	{ \
		auto& v = var->getValue(); \
		for (auto& it: v) { \
			MS_LOG_INFO(MS_LOG_ROOT()) << #prefix " " #name " : " << it; \
		} \
		MS_LOG_INFO(MS_LOG_ROOT()) << #prefix " " #name "yaml: " << var->toString(); \
	}
	
	PP(g_int_vec_value_config, int_vec, before);
	PP(g_int_list_value_config, int_list, before);
	PP(g_int_set_value_config, int_set, before);
	PP(g_int_uset_value_config, int_uset, before);

#define PP_M(var, name, prefix) \
	{ \
		auto& v = var->getValue(); \
		for (auto& it: v) { \
			MS_LOG_INFO(MS_LOG_ROOT()) << #prefix " " #name " : " << \
			"{ " << it.first << " : " << it.second << " }"; \
		} \
		MS_LOG_INFO(MS_LOG_ROOT()) << #prefix " " #name "yaml: " << var->toString(); \
	}

	PP_M(g_int_map_value_config, str_int_map, before);
	PP_M(g_int_umap_value_config, str_int_umap, before);

	YAML::Node root = YAML::LoadFile("/home/lucky/WebServer/tests/log.yml");
	MNSER::Config::LoadFromYaml(root);
	MS_LOG_INFO(MS_LOG_ROOT()) << "int after: " << g_int_value_config->getValue();
	MS_LOG_INFO(MS_LOG_ROOT()) << "float after: " << g_float_value_config->getValue();

	PP(g_int_vec_value_config, int_vec, after);
	PP(g_int_list_value_config, int_list, after);
	PP(g_int_set_value_config, int_set, after);
	PP(g_int_uset_value_config, int_uset, after);

	PP_M(g_int_map_value_config, str_int_map, after);
	PP_M(g_int_umap_value_config, str_int_umap, after);
#undef PP
#undef PP_M
}

#endif

class Person {
public:
    Person() {};
    std::string m_name;
    int m_age = 0;
    bool m_sex = 0;

    std::string toString() const {
        std::stringstream ss;
        ss << "[Person name=" << m_name
           << " age=" << m_age
           << " sex=" << m_sex
           << "]";
        return ss.str();
    }

    bool operator==(const Person& oth) const {
        return m_name == oth.m_name
            && m_age == oth.m_age
            && m_sex == oth.m_sex;
    }
};

namespace MNSER {

template<>
class LexicalCast<std::string, Person> {
public:
    Person operator()(const std::string& v) {
        YAML::Node node = YAML::Load(v);
        Person p;
        p.m_name = node["name"].as<std::string>();
        p.m_age = node["age"].as<int>();
        p.m_sex = node["sex"].as<bool>();
        return p;
    }
};

template<>
class LexicalCast<Person, std::string> {
public:
    std::string operator()(const Person& p) {
        YAML::Node node;
        node["name"] = p.m_name;
        node["age"] = p.m_age;
        node["sex"] = p.m_sex;
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

}

MNSER::ConfigVar<Person>::ptr g_person =
    MNSER::Config::Lookup("class.person", Person(), "system person");

MNSER::ConfigVar<std::map<std::string, Person> >::ptr g_person_map =
    MNSER::Config::Lookup("class.map", std::map<std::string, Person>(), "system person");

MNSER::ConfigVar<std::map<std::string, std::vector<Person> > >::ptr g_person_vec_map =
    MNSER::Config::Lookup("class.vec_map", std::map<std::string, std::vector<Person> >(), "system person");

void test_class() {
    MS_LOG_INFO(MS_LOG_ROOT()) << "before: " << g_person->getValue().toString() << " - " << g_person->toString();
#define XX_PM(g_var, prefix) \
    { \
        auto m = g_person_map->getValue(); \
        for(auto& i : m) { \
            MS_LOG_INFO(MS_LOG_ROOT()) <<  prefix << ": " << i.first << " - " << i.second.toString(); \
        } \
        MS_LOG_INFO(MS_LOG_ROOT()) <<  prefix << ": size=" << m.size(); \
    }

#if 0
    g_person->addListener([](const Person& old_value, const Person& new_value){
        MS_LOG_INFO(MS_LOG_ROOT()) << "old_value=" << old_value.toString()
                << " new_value=" << new_value.toString();
    });
#endif

    XX_PM(g_person_map, "class.map before");
    MS_LOG_INFO(MS_LOG_ROOT()) << "g_person_vec_map before: " << g_person_vec_map->toString();

    YAML::Node root = YAML::LoadFile("/home/lucky/WebServer/tests/log.yml");
    MNSER::Config::LoadFromYaml(root);

    MS_LOG_INFO(MS_LOG_ROOT()) << "after: " << g_person->getValue().toString() << " - " << g_person->toString();
    XX_PM(g_person_map, "class.map after");
    MS_LOG_INFO(MS_LOG_ROOT()) << "g_person_vec_map after: " << g_person_vec_map->toString();
}

void test_log() {
	static MNSER::Logger::ptr s_log = MS_LOG_NAME("system");
	MS_LOG_INFO(s_log) << "This is system log" << std::endl;
	std::cout << MNSER::LoggerMgr::GetInstance()->toYamlString() << std::endl;
    YAML::Node root = YAML::LoadFile("/home/lucky/WebServer/tests/log.yml");
    MNSER::Config::LoadFromYaml(root);
	std::cout << " ############## " << std::endl;	
	std::cout << MNSER::LoggerMgr::GetInstance()->toYamlString() << std::endl;
	
	MS_LOG_INFO(s_log) << "This is system log after load" << std::endl;

	s_log->setFormatter("%d%T ## %T%m%n");
	MS_LOG_INFO(s_log) << "This is system log after setFormatter" << std::endl;
}

int main(int argc, char* argv[]) {
	//test_yaml();
	//test_config();
	//test_class();
	test_log();
	return 0;
}

