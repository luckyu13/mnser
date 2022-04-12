#ifndef __MNSER_LOG_H_
#define __MNSER_LOG_H_

#include <iostream>
#include <time.h>
#include <string>
#include <stdint.h>
#include <memory>
#include <list>
#include <string>
#include <fstream>
#include <vector>
#include <functional>
#include <map>
#include <sstream>
#include <stdarg.h>

#include "util.h"
#include "singleton.h"
#include "thread.h"
#include "mutex.h"

// 宏通过 LogEventWarp  LogEvent的包装器的析构函数将内容写到文件中
#define MS_LOG_LEVEL(logger, level) \
	if(logger->getLevel() <= level) \
		MNSER::LogEventWarp(MNSER::LogEvent::ptr(new MNSER::LogEvent(logger, level, __FILE__, __LINE__, \
			0, MNSER::GetThreadId(), MNSER::GetFiberId(), time(0), MNSER::Thread::GetName()))).getSS()

#define MS_LOG_DEBUG(logger) MS_LOG_LEVEL(logger, MNSER::LogLevel::DEBUG)
#define MS_LOG_INFO(logger) MS_LOG_LEVEL(logger, MNSER::LogLevel::INFO)
#define MS_LOG_WARN(logger) MS_LOG_LEVEL(logger, MNSER::LogLevel::WARN)
#define MS_LOG_ERROR(logger) MS_LOG_LEVEL(logger, MNSER::LogLevel::ERROR)
#define MS_LOG_FATAL(logger) MS_LOG_LEVEL(logger, MNSER::LogLevel::FATAL)

#define MS_LOG_FMT_LEVEL(logger, level, fmt, ...) \
	if(logger->getLevel() <= level) \
		MNSER::LogEventWarp(MNSER::LogEvent::ptr(new MNSER::LogEvent(logger, level, __FILE__, __LINE__, \
			0, MNSER::GetThreadId(), MNSER::GetFiberId(), time(0), Thread::GetName()))).getEvent()->format(fmt, __VA_ARGS__)

#define MS_LOG_FMT_DEBUG(logger, fmt, ...) MS_LOG_FMT_LEVEL(logger, MNSER::LogLevel::DEBUG, fmt, __VA_ARGS__)
#define MS_LOG_FMT_INFO(logger, fmt, ...) MS_LOG_FMT_LEVEL(logger, MNSER::LogLevel::INFO, fmt, __VA_ARGS__)
#define MS_LOG_FMT_WARN(logger, fmt, ...) MS_LOG_FMT_LEVEL(logger, MNSER::LogLevel::WARN, fmt, __VA_ARGS__)
#define MS_LOG_FMT_ERROR(logger, fmt, ...) MS_LOG_FMT_LEVEL(logger, MNSER::LogLevel::ERROR, fmt, __VA_ARGS__)
#define MS_LOG_FMT_FATAL(logger, fmt, ...) MS_LOG_FMT_LEVEL(logger, MNSER::LogLevel::FATAL, fmt, __VA_ARGS__)

#define MS_LOG_ROOT() MNSER::LoggerMgr::GetInstance()->getRoot()
#define MS_LOG_NAME(name) MNSER::LoggerMgr::GetInstance()->getLogger(name)

namespace  MNSER {

#define LogAppenderItemType_FILE 		0
#define LogAppenderItemType_STDOUT 	1

class Logger;
class LoggerManager;
// 日志级别
class LogLevel {
public:
	enum Level {
		UNKNOWN = 0,
		DEBUG = 1,
		INFO = 2,
		WARN = 3,
		ERROR = 4,
		FATAL = 5,
		INVALID = 100,
	};

	static const char * ToString(LogLevel::Level level);
	static LogLevel::Level FromString(const std::string& str);
};

class LogEvent {
public:
	typedef std::shared_ptr<LogEvent> ptr;
	LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, 
			const char *file, int32_t line, uint32_t elapse, 
			uint32_t threadId, uint32_t fiberId, uint64_t time
			, const std::string threadName);
	
	const char * getFile() const { return m_file; }
	int32_t getLine() const { return m_line; }
	uint32_t getElapse() const { return m_elapse; }
	int32_t getThreadId() const { return m_threadId; }
	int32_t getFiberId() const { return m_fiberId; }
	uint64_t getTime() const { return m_time; }
	std::string getThreadName() const { return m_threadName; }
	std::string getContent() const { return m_ss.str(); }
	LogLevel::Level getLevel() { return m_level; }
	std::shared_ptr<Logger> getLogger() { return m_logger; }
	std::stringstream& getSS() { return m_ss; }
	void format(const char * fmt, ...); // 以格式化写入内容
	void format(const char * fmt, va_list al);  // 以格式化写入内容

private:
	const char * m_file 	= nullptr; 	// 文件名
	int32_t m_line 			= 0;		// 行号
	uint32_t m_elapse 		= 0;		// 程序启动开始到现在的毫秒数
	int32_t m_threadId 		= 0;		// 线程ID
	int32_t m_fiberId 		= 0;		// 协程ID
	uint64_t m_time			= 0;		// 时间戳
	LogLevel::Level 		m_level;	// 日志级别
	std::stringstream 		m_ss;		// 日志流内容
	std::string 			m_threadName; 
	std::shared_ptr<Logger>	m_logger; 
};

// 日志事件包装器
class LogEventWarp {
public:
	LogEventWarp(LogEvent::ptr e);
	~LogEventWarp();

	LogEvent::ptr getEvent() const { return m_event; }
	std::stringstream& getSS();
private:
	LogEvent::ptr m_event;
};

// 不同日志器格式不一样
/*
 * %m 消息
 * %p 日志级别
 * %r 累计毫秒数
 * %c 日志名称
 * %t 线程id
 * %n 换行
 * %d 时间
 * %f 文件名
 * %l 行号
 * %T 制表符
 * %F 协程id
 * %N 线程名称
 * 默认格式 : "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
 */
class LogFormatter {
public:
	typedef std::shared_ptr<LogFormatter> ptr;
	LogFormatter(const std::string &pattern);
	// 解析成固定格式的输出
	std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);
	std::ostream& format(std::ostream& ofs, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);

public:
	class FormatItem {
		public:
			typedef std::shared_ptr<FormatItem> ptr;
			virtual ~FormatItem() {}
			virtual void format(std::ostream &os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
	};

public:
	void init();
	bool isError() const { return m_error; }
	std::string getPattern() const { return m_pattern; }

private:
	std::string m_pattern;
	std::vector<FormatItem::ptr> m_items;
	bool  m_error = false;  // 用于 setFormatter(const std::string&) 函数
};

// 日志输出地
class LogAppender {
friend class Logger;
public:
	typedef std::shared_ptr<LogAppender> ptr;
	typedef SpinLock MutexType;
	//typedef Mutex MutexType;

	virtual ~LogAppender() {}

	virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;

	void setFormatter(LogFormatter::ptr val) ;
	LogFormatter::ptr getFormatter();
	void setLevel(LogLevel::Level level) { m_level = level; }
	LogLevel::Level getLevel() const { return m_level; }

	// 配置文件操作
	virtual std::string toYamlString() = 0;

protected:
	LogLevel::Level m_level = LogLevel::DEBUG;
	bool m_hasFormatter = false; // 记录是不是有 formatter
	LogFormatter::ptr m_formatter;
	MutexType m_mutex;
};

// 日志器
class Logger: public std::enable_shared_from_this<Logger> {
friend class LoggerManager;
public:
	typedef std::shared_ptr<Logger> ptr; // 使用智能指针方便内存管理
	typedef SpinLock MutexType;
	//typedef Mutex MutexType;

	Logger(const std::string& name="root");

	void log(LogLevel::Level level, LogEvent::ptr event);

	// 不同的日志级别
	void debug(LogEvent::ptr event);
	void info(LogEvent::ptr event);
	void warn(LogEvent::ptr event);
	void error(LogEvent::ptr event);
	void fatal(LogEvent::ptr event);

	// 输出器的操作
	void addAppender(LogAppender::ptr appender);
	void delAppender(LogAppender::ptr appender);
	void clearAppender();
	// 自己信息的返回
	LogLevel::Level getLevel() const { return m_level; }
	void setLevel(LogLevel::Level level) { m_level = level; }
	const std::string& getName() const { return m_name; }
	
	// 格式器的操作
	LogFormatter::ptr getFormatter () ;
	void setFormatter(LogFormatter::ptr formatter);
	void setFormatter(const std::string& str);

	// 配置文件操作
	std::string toYamlString();
private:
	std::string m_name;					// 日志名称
	LogLevel::Level m_level;			// 日志级别
	std::list<LogAppender::ptr> m_appenders;			// Appender集合
	Logger::ptr m_root;
	LogFormatter::ptr m_formatter;
	MutexType m_mutex;
};


// 输出到控制太的 Appender
class StdoutLogAppender: public LogAppender {
public:
	typedef std::shared_ptr<StdoutLogAppender> ptr;
	void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
	// 配置文件操作
	std::string toYamlString() override;
};

// 输出到文件的 Apperder
class FileLogAppender: public LogAppender {
public:
	typedef std::shared_ptr<FileLogAppender> ptr;
	FileLogAppender(const std::string &filename);
	void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;

	// 重新打开文件， 文件打开成功返回 true
	bool reopen();
	// 配置文件操作
	std::string toYamlString() override;

private:
	std::string m_filename;
	std::ofstream m_filestream;
	uint64_t m_lastTime = 0;
};

class LoggerManager {
public:
	typedef SpinLock MutexType;
	//typedef Mutex MutexType;

	LoggerManager();

	std::string toYamlString();
	Logger::ptr getRoot() const { return m_root; }
	Logger::ptr getLogger(const std::string& str);
	void init();

	void addLogger(MNSER::Logger::ptr logger);
	void addLogger(const std::string& name);

private:
	Logger::ptr _addLogger(const std::string& name);
	Logger::ptr m_root;
	std::map<std::string, typename Logger::ptr> m_loggers;
	MutexType m_mutex;
};

typedef MNSER::Singleton<LoggerManager> LoggerMgr;

}


#endif
