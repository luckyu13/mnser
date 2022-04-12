#include "log.h"
#include "config.h"

namespace MNSER {

const char * LogLevel::ToString(LogLevel::Level level) {
	switch (level) {
#define TMPFUNC(name) \
	case LogLevel::name: \
		return #name; \
		break;
	
	TMPFUNC(DEBUG);
	TMPFUNC(INFO);
	TMPFUNC(WARN);
	TMPFUNC(ERROR);
	TMPFUNC(FATAL);
#undef TMPFUNC
	default:
		return "UNKNOWN";
	}
	return "UNKNOWN";
}

LogLevel::Level LogLevel::FromString(const std::string& str) {
#define TMP_FUNC(level, t) \
	if (str == #t) \
		return LogLevel::level; \
	
	TMP_FUNC(DEBUG, debug);
	TMP_FUNC(INFO, info);
	TMP_FUNC(WARN, warn);
	TMP_FUNC(ERROR, error);
	TMP_FUNC(FATAL, fatal);

	TMP_FUNC(DEBUG, DEBUG);
	TMP_FUNC(INFO, INFO);
	TMP_FUNC(WARN, WARN);
	TMP_FUNC(ERROR, ERROR);
	TMP_FUNC(FATAL, FATAL);

	return LogLevel::UNKNOWN;
#undef TMP_FUNC
}

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, 
		const char *file, int32_t line, uint32_t elapse, 
		uint32_t threadId, uint32_t fiberId, uint64_t time
		, const std::string threadName): 
		m_logger(logger), 
		m_level(level),
		m_file(file), 
		m_line(line), 
		m_elapse(elapse), 
		m_threadId(threadId),
		m_fiberId(fiberId), 
		m_time(time), 
		m_threadName(threadName) {
		//std::cout << "m_file= " << m_file << "__FILE__ = " << __FILE__ << std::endl;
	
}

void LogEvent::format(const char * fmt, ...) {
	// 这个函数没有往日志里面写入啊
	va_list va;
	va_start(va, fmt);
	format(fmt, va);
	va_end(va);
}

void LogEvent::format(const char * fmt, va_list al) {
	char *buf = nullptr;
	int len = vasprintf(&buf, fmt, al);
	if (len != -1) {
		m_ss << std::string(buf, len);
		free(buf);
	}
}

LogEventWarp::LogEventWarp(std::shared_ptr<LogEvent> e)
	: m_event(e) {
}

LogEventWarp::~LogEventWarp() {
	m_event->getLogger()->log(m_event->getLevel(), m_event);
}

std::stringstream& LogEventWarp::getSS() {
	return m_event->getSS();
}


class MessageFormatItem: public LogFormatter::FormatItem {
public:
	MessageFormatItem(const std::string & str = "")	 {}
	void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)  {
		os << event->getContent();	
	}
};

class LevelFormatItem: public LogFormatter::FormatItem {
public:
	LevelFormatItem(const std::string & str = "")	 {}
	void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)  {
		os << LogLevel::ToString(level);	
	}
};

class ElapseFormatItem: public LogFormatter::FormatItem {
public:
	ElapseFormatItem(const std::string & str = "")	 {}
	void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)  {
		os << event->getElapse();	
	}
};

class NameFormatItem: public LogFormatter::FormatItem {
public:
	NameFormatItem(const std::string & str = "")	 {}
	void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)  {
		os << event->getLogger()->getName();
	}
};

class ThreadIdFormatItem: public LogFormatter::FormatItem {
public:
	ThreadIdFormatItem(const std::string & str = "")	 {}
	void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)  {
		os << event->getThreadId();	
	}
};

class NewLineFormatItem: public LogFormatter::FormatItem {
public:
	NewLineFormatItem(const std::string & str = "")	 {}
	void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)  {
		os << std::endl;	
	}
};

class DateTimeFormatItem: public LogFormatter::FormatItem {
public:
	DateTimeFormatItem(const std::string & fmt = "%Y-%m-%d %H:%M:%S"): m_fmt(fmt) {
		if (m_fmt.empty()) {
			m_fmt = "%Y-%m-%d %H:%M:%S";
		}
	}

	void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)  {
		struct tm tm;
		time_t time = event->getTime();
		localtime_r(&time, &tm);
		char buf[64];
			strftime(buf, sizeof(buf), m_fmt.c_str(), &tm);
		os << buf;
	}

private:
	std::string m_fmt;
};

class FileFormatItem: public LogFormatter::FormatItem {
public:
	FileFormatItem(const std::string & str = "")	 {}
	void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)  {
		os << event->getFile();	
	}
};

class LineFormatItem: public LogFormatter::FormatItem {
public:
	LineFormatItem(const std::string & str = "")	 {}
	void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)  {
		os << event->getLine();	
	}
};

class TabFormatItem: public LogFormatter::FormatItem {
public:
	TabFormatItem(const std::string & str = "")	 {}
	void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)  {
		os << '\t';
	}
};

class FiberIdFormatItem: public LogFormatter::FormatItem {
public:
	FiberIdFormatItem(const std::string & str = "")	 {}
	void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)  {
		os << event->getFiberId();
	}
};

class ThreadNameFormatItem: public LogFormatter::FormatItem {
public:
	ThreadNameFormatItem(const std::string & str = "")	 {}
	void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)  {
		os << event->getThreadName();
	}
};

class StringFormatItem: public LogFormatter::FormatItem {
public:
	StringFormatItem(const std::string & str = ""):m_str(str) {}
	void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)  {
		os << m_str;
	}

private:
	std::string m_str;
};

LogFormatter::LogFormatter(const std::string &pattern): m_pattern(pattern) {
	init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
	std::stringstream ss;
	//std::cout << "LogFormatter::format " << __LINE__ << std::endl;
	for (auto &it: m_items) {
		it->format(ss, logger, level, event);
	}
	return ss.str();
}

std::ostream& LogFormatter::format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
	//std::cout << "LogFormatter::format " << __LINE__ << std::endl;
	for (auto &it: m_items) {
		it->format(ofs, logger, level, event);
	}
	return ofs;
}

void LogFormatter::init() {
	// 根据传入的模式初始化
	std::vector<std::tuple<std::string, std::string, int> > vec;
	std::string str;
	size_t n = m_pattern.size();
	//std::cout << m_pattern << std::endl;
	for (size_t i = 0; i<n; ++i) {
		//std::cout << i << " :" << str << std::endl;
		if (m_pattern[i] != '%') {
			str.push_back(m_pattern[i]);
			continue;
		}

		if (i+1 < n && m_pattern[i+1] == '%') {
			str.push_back('%');
			continue;
		}

		size_t j = i+1;
		//std::cout << "j = " << j << std::endl;
		int fmt_status = 0;
		size_t fmt_begin = 0;
		
		std::string tstr;
		std::string fmt;

		while (j < n) {
			if (!fmt_status && (!isalpha(m_pattern[j]) 
				&& m_pattern[j] != '{' && m_pattern[j] != '}') ) {
				tstr = m_pattern.substr(i+1, j-i-1);
				//std::cout << tstr << std::endl;
				break;
			}

			if (fmt_status == 0) {
				if (m_pattern[j] == '{') {
					tstr = m_pattern.substr(i+1, j-i-1);
					fmt_status = 1;
					fmt_begin = j;
					++j;
					continue;
				} 
			} else if (fmt_status == 1) {
				if (m_pattern[j] == '}') {
					fmt = m_pattern.substr(fmt_begin+1, j-fmt_begin-1);
					fmt_status = 0;
					++j;
					break;
				}
			}

			++j;
			if (j == n) {
				if (tstr.empty()) {
					tstr = m_pattern.substr(i+1);
				}
			}
		}
		if (fmt_status == 0) {
			if (!str.empty()) {
				vec.push_back(std::make_tuple(str, std::string(), 0));
				str.clear();
			}
			vec.push_back(std::make_tuple(tstr, fmt, 1));
			i = j - 1;
		} else if (fmt_status == 1) {
			//std::cout << j << std::endl;
			std::cout << __FILE__ << ":" << __LINE__ << " Pattern parse error: " << m_pattern << " - " << m_pattern.substr(i) << std::endl;
			m_error = true;
			vec.push_back(std::make_tuple("<<pattern-error>>", fmt, 0));
		}
	}
	
	if (!str.empty()) {
		vec.push_back(std::make_tuple(str, std::string(), 0));
	}

	static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)> > map_s_items = {
#define TMP_FUNC(str, C) \
	{#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt)); }}

	TMP_FUNC(m, MessageFormatItem),
	TMP_FUNC(p, LevelFormatItem),
	TMP_FUNC(r, ElapseFormatItem),
	TMP_FUNC(c, NameFormatItem),
	TMP_FUNC(t, ThreadIdFormatItem),
	TMP_FUNC(n, NewLineFormatItem),
	TMP_FUNC(d, DateTimeFormatItem),
	TMP_FUNC(f, FileFormatItem),
	TMP_FUNC(l, LineFormatItem),
	TMP_FUNC(T, TabFormatItem),
	TMP_FUNC(F, FiberIdFormatItem),
	TMP_FUNC(N, ThreadNameFormatItem),
#undef TMP_FUNC
	};

	//std::cout << vec.size() << std::endl;
	
	for(auto &i: vec) {
		//std::cout << std::get<0>(i) << " :<==>: " << std::get<1>(i) << " :<==>: " << std::get<2>(i) << std::endl;
		if (std::get<2>(i) == 0) {
			m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
		} else {
			auto it = map_s_items.find(std::get<0>(i));
			if (it == map_s_items.end()) {
				m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error-format" + std::get<0>(i) + ">>")));
				m_error = true;
			} else {
				m_items.push_back(it->second(std::get<1>(i)));
			}
		}
	}
	//std::cout << __LINE__ << " : " << m_items.size() << std::endl;
}

void LogAppender::setFormatter(LogFormatter::ptr val) {
	MutexType::Lock lock(m_mutex);
	m_formatter = val;
	if (m_formatter) {
		m_hasFormatter = true;
	} else {
		m_hasFormatter = false;
	}
}

LogFormatter::ptr LogAppender::getFormatter() { 
	MutexType::Lock lock(m_mutex);
	return m_formatter; 
}

Logger::Logger(const std::string& name) 
	:m_name(name), m_level(LogLevel::DEBUG) {
	m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
}

std::string Logger::toYamlString() {
	MutexType::Lock lock(m_mutex);
	YAML::Node node;
	node["name"] = m_name;
	if (m_level != LogLevel::UNKNOWN && m_level != LogLevel::INVALID) {
		node["level"] = LogLevel::ToString(m_level);
	}
	if (m_formatter) {
		node["formatter"] = m_formatter->getPattern();
	}
	for (auto& it: m_appenders) {
		node["appenders"].push_back(YAML::Load(it->toYamlString()));
	}
	std::stringstream ss;
	ss << node;
	//std::cout << ss.str() << "=============" << std::endl;
	return ss.str();
}

void Logger::clearAppender() {
	MutexType::Lock lock(m_mutex);
	m_appenders.clear();
}

void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
	//std::cout << "m_levle: " <<  LogLevel::ToString(m_level) << std::endl;
	//std::cout << "level: " << LogLevel::ToString(level) << std::endl;
	
	if (level >= m_level) {
		MutexType::Lock lock(m_mutex);
		auto self = shared_from_this();
		//std::cout << __FILE__ << "  " << __LINE__ << "My appenders  size is " << m_appenders.size() << std::endl;
		if (!m_appenders.empty()) {
			for (auto& it: m_appenders) {
				//std::cout << __LINE__ << " : " << event->getContent() << std::endl;
				it->log(self, level, event);
			}
		} else if (m_root) {
			m_root->log(level, event);
		}
	}
}

void Logger::debug(LogEvent::ptr event) {
	log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::ptr event) {
	log(LogLevel::INFO, event);
}

void Logger::warn(LogEvent::ptr event) {
	log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::ptr event) {
	log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::ptr event) {
	log(LogLevel::FATAL, event);
}


LogFormatter::ptr Logger::getFormatter() {
	MutexType::Lock lock(m_mutex);
	return m_formatter;
}

void Logger::setFormatter(LogFormatter::ptr formatter) {
	MutexType::Lock lock(m_mutex);
	m_formatter = formatter;
	std::cout << " ###" << m_appenders.size() << std::endl;
	for (auto &it: m_appenders) {
		MutexType::Lock lock(it->m_mutex);
		if (it->m_hasFormatter) {
			it->m_formatter = m_formatter;
		}
	}
}

void Logger::setFormatter(const std::string& str) {
	MNSER::LogFormatter::ptr new_formatter(new MNSER::LogFormatter(str));
	if (new_formatter->isError()) {
		std::cout << "Logger setFormatter name=" << m_name << " value="
		<< str << " invalid formatter." << std::endl;
		return ;
	}
	setFormatter(new_formatter);
}

void Logger::addAppender(LogAppender::ptr appender) {
	MutexType::Lock lock(m_mutex);
	if (!appender->getFormatter()) {
		MutexType::Lock lock(appender->m_mutex);
		appender->m_formatter = m_formatter;
	}
	m_appenders.push_back(appender);
};

void Logger::delAppender(LogAppender::ptr appender) {
	MutexType::Lock lock(m_mutex);
	for (auto it = m_appenders.begin(); 
		it != m_appenders.end(); ++it) {
		if (*it == appender) {
			m_appenders.erase(it);
			break;
		}
	}
};

bool FileLogAppender::reopen() {
	MutexType::Lock lock(m_mutex);
	if (m_filestream) {
		m_filestream.close();
	}
	// unwan
	m_filestream.open(m_filename, std::ios_base::app);
	return !!m_filestream;
}

std::string FileLogAppender::toYamlString() {
	MutexType::Lock lock(m_mutex);
	YAML::Node node;
	node["type"] = "FileLogAppender";
	node["file"] = m_filename;
	if (m_level != LogLevel::UNKNOWN && m_level != LogLevel::INVALID) {
		node["level"] = LogLevel::ToString(m_level);
	}
	if (m_hasFormatter && m_formatter) {
		node["formatter"] = m_formatter->getPattern();
	}
	std::stringstream ss;
	ss << node;
	return ss.str();
}

void StdoutLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event)  {
	if (m_level <= level) {
		//std::cout << __LINE__ << " : " << std::endl;
		MutexType::Lock lock(m_mutex);
		m_formatter->format(std::cout, logger, level, event);
	}
}

std::string StdoutLogAppender::toYamlString() {
	MutexType::Lock lock(m_mutex);
	YAML::Node node;
	node["type"] = "StdoutLogAppender";
	if (m_level != LogLevel::UNKNOWN && m_level != LogLevel::INVALID) {
		node["level"] = LogLevel::ToString(m_level);
	}
	if (m_hasFormatter && m_formatter) {
		node["formatter"] = m_formatter->getPattern();
	}
	std::stringstream ss;
	ss << node;
	return ss.str();
	
}

FileLogAppender::FileLogAppender(const std::string &filename): m_filename(filename) {
	reopen();
}

void FileLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event)  {
	if (m_level <= level) {
		uint64_t now = event->getTime();
		if (now >= m_lastTime) {
			// 防止把文件删除之后，没有感知到被删，防止占用文件句柄
			reopen();
			m_lastTime = now;
		}

		MutexType::Lock lock(m_mutex);
		if (!m_formatter->format(m_filestream, logger, level, event)) {
			std::cout << "error" << std::endl;
		}
	}
}

LoggerManager::LoggerManager() {
	m_root.reset(new Logger);
	m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));

	m_loggers[m_root->m_name] = m_root;
	//std::cout << " ### LoggerManager m_root->m_name = " << m_root->m_name << std::endl;
	init();
}

void LoggerManager::addLogger(MNSER::Logger::ptr logger) {
	auto it = m_loggers.find(logger->m_name);
	if (it != m_loggers.end()) {
		return ;
	}
	m_loggers[logger->m_name] = logger;
}

void LoggerManager::addLogger(const std::string& name) {
	_addLogger(name);
}

Logger::ptr LoggerManager::_addLogger(const std::string& name) {
	Logger::ptr new_logger(new Logger(name));
	new_logger->m_root = m_root;
	m_loggers[name] = new_logger;
	return new_logger;
}

Logger::ptr LoggerManager::getLogger(const std::string& name) {
	MutexType::Lock lock(m_mutex);
	// std::cout << "### getLogger for name = " << name << std::endl;
	auto it = m_loggers.find(name);
	// std::cout << "### getLogger endl" << std::endl;
	if (it != m_loggers.end()) {
		return it->second;
	}
	return _addLogger(name);
}

std::string LoggerManager::toYamlString() {
	MutexType::Lock lock(m_mutex);
	YAML::Node node;
	for (auto& it: m_loggers) {
		//std::cout << "***" << std::endl << it.second->toYamlString() << "***" << std::endl;
		node.push_back(YAML::Load(it.second->toYamlString()));
	}
	std::stringstream ss;
	ss << node;
	return ss.str();
}

void LoggerManager::init() {

};

struct LogAppenderItem {
	int type = LogAppenderItemType_FILE;
	LogLevel::Level level = LogLevel::UNKNOWN;
	std::string formatter;
	std::string file;

	/*
	 * 重载 == 运算符
	 * 因为 set 在使用 find 方法的时候需要使用 ==
	 */
	bool operator==(const LogAppenderItem& other) const {
		return type == other.type
			&& level == other.level
			&& formatter == other.formatter
			&& file == other.file;
	}

	bool operator!=(const LogAppenderItem& other) const {
		return type != other.type
			|| level != other.level
			|| formatter != other.formatter
			|| file != other.file;
	}

};

struct LogItem {
	std::string name;
	LogLevel::Level level = LogLevel::UNKNOWN;
	std::vector<LogAppenderItem> appenders;
	std::string formatter;

	/*
	 * 重载 == 运算符
	 * 因为 set 在使用 find 方法的时候需要使用 ==
	 */
	bool operator==(const LogItem& other) const {
		return name == other.name
			&& level == other.level
			&& appenders == other.appenders
			&& formatter == other.formatter;
	}

	bool operator!=(const LogItem& other) {
		return name != other.name
			|| level != other.level
			|| appenders != other.appenders
			|| formatter != other.formatter;
	}

	bool operator<(const LogItem& other) const {
		return name < other.name;
	}

	bool isValid() const {
		return !name.empty();
	}
};

template <>
class LexicalCast<std::string, LogItem > {
public:
	LogItem operator() (const std::string& from) { 
		YAML::Node node = YAML::Load(from);
		LogItem lim;
		if (!node["name"].IsDefined()) {
			std::cout << "log config error: name is null, " << node 
				<< std::endl;
			throw std::logic_error("log config name is null");
		}
		lim.name = node["name"].as<std::string>();
		lim.level = MNSER::LogLevel::FromString(node["level"].IsDefined() ? node["level"].as<std::string>() : "");
		if (node["formatter"].IsDefined()) {
			lim.formatter = node["formatter"].as<std::string>();
		}

		if (node["appenders"].IsDefined()) {
			for (size_t i=0; i<node["appenders"].size(); ++i) {
				auto ap = node["appenders"][i];
				if (!ap["type"].IsDefined()) {
					std::cout << "log config error: appender type is null, " << ap << std::endl;
					continue;
				}
				std::string type = ap["type"].as<std::string>();
				LogAppenderItem lai;
				if (type == "FileLogAppender") {
					if (!ap["file"].IsDefined()) {
						std::cout << "log config error: fileappender file is null, " << ap << std::endl;
						continue;
					}
					lai.type = LogAppenderItemType_FILE;
					lai.file = ap["file"].as<std::string>();
				} else if (type == "StdoutLogAppender") {
					lai.type = LogAppenderItemType_STDOUT;
				} else {
					std::cout << "log config error: appender type is invalid, " << ap << std::endl;
					continue;
				}
				if (ap["formatter"].IsDefined()) {
					lai.formatter = ap["formatter"].as<std::string>();
				}
				lim.appenders.push_back(lai);
			}
		}
		return lim;
	}
};

template <>
class LexicalCast<LogItem, std::string> {
public:
	std::string operator() (const LogItem& from) { 
		YAML::Node node;
		
		node["name"] = from.name;
		if (from.level != MNSER::LogLevel::UNKNOWN 
			&& from.level != MNSER::LogLevel::INVALID) {
			node["level"] = LogLevel::ToString(from.level);
		}

		if (!from.formatter.empty()) {
			node["formatter"] = from.formatter;
		}
		
		for (auto& it: from.appenders) {
			YAML::Node n;
			if (it.type == LogAppenderItemType_FILE) {
				n["type"] = "FileLogAppender";
				n["file"] = it.file;
			} else if (it.type == LogAppenderItemType_STDOUT) {
				n["type"] = "StdoutLogAppender";
			} else {
				std::cout << "log conig error: invalid appender type: " << it.type << std::endl;
				continue;
			}
			if (from.level != MNSER::LogLevel::UNKNOWN 
				&& from.level != MNSER::LogLevel::INVALID) {
				n["level"] = LogLevel::ToString(from.level);
			}
			if (!from.formatter.empty()) {
				n["formatter"] = from.formatter;
			}
			node["appenders"].push_back(n);
		}
		
		std::stringstream ss;
		ss << node;
		return ss.str();
	}
};

/*
 * 全局的日志配置文件对象
 */
MNSER::ConfigVar<std::set<LogItem> >::ptr g_log_config = 
	MNSER::Config::Lookup("logs", std::set<LogItem>(), "logs config");

// 在 main 函数之前为 loggers 集合添加监听
struct LogInit {
	LogInit() {
		g_log_config->addListener([](const std::set<LogItem>& old_logs, const std::set<LogItem>& new_logs) {
			MS_LOG_INFO(MS_LOG_ROOT()) << "on_logger_config_change";
			// new_logs = old_logs;
			for (auto& it: new_logs) {
				//std::cout << "***" << it.name << std::endl;
				auto f = old_logs.find(it); // set 的 find 的比较器默认使用的时 < 所以这就就是根据 name 查找的
				MNSER::Logger::ptr logger;
				if (f == old_logs.end()) {
					// 原来的 loggers 中没有这个容器
					// 新增
					//std::cout << " new logger name=" << it.name << __LINE__ << std::endl;
					logger = MS_LOG_NAME(it.name);
				} else {
					if (*f == it) {
						continue;
					}
					logger = MS_LOG_NAME(it.name); // 如果有所改变，就记录到 logger 变量中，后续更改
				}
				// 开始更改各个变量
				// 更新 Level
				logger->setLevel(it.level);

				// 更新 formatter
				if (!it.formatter.empty()) {
					logger->setFormatter(it.formatter);
				}

				// 更新 Appender 没有完成
				logger->clearAppender();
				for (auto& a: it.appenders) {
					MNSER::LogAppender::ptr aPtr;
					if (a.type == LogAppenderItemType_FILE) {
						aPtr.reset(new FileLogAppender(a.file));
					} else if (a.type == LogAppenderItemType_STDOUT) {
						// nowan
						aPtr.reset(new StdoutLogAppender);
					} else {
						continue;
					}

					aPtr->setLevel(a.level);

					if (!a.formatter.empty()) {
						MNSER::LogFormatter::ptr fmt(new MNSER::LogFormatter(a.formatter));
						if (!fmt->isError()) {
							aPtr->setFormatter(fmt);
						} else {
							std::cout << "log name = " << it.name << " appender type = " 
								<< a.type << " formatter = " << a.formatter
								<< " is invalid" << std::endl;
						}
					}
					logger->addAppender(aPtr);
				}
			}
			// 清除没有使用的节点
			for (auto& it: old_logs) {
				auto f = new_logs.find(it);
				if (f != new_logs.end()) {
					auto logger = MS_LOG_NAME(it.name);
					logger->setLevel(MNSER::LogLevel::INVALID);
					logger->clearAppender();
				}
			}
			
		});
	}
};

static LogInit __log_init;  // 这一句就是在主函数运行之前注册回调函数
}
