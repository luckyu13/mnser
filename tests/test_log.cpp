#include <iostream>
#include "log.h"

int main(int argc, char* argv[])
{
	using namespace MNSER;
	MNSER::Logger::ptr logger(new MNSER::Logger);
	logger->addAppender(MNSER::LogAppender::ptr(new MNSER::StdoutLogAppender));

	MNSER::FileLogAppender::ptr fileAppender(new MNSER::FileLogAppender("./log.txt"));
	MNSER::LogFormatter::ptr fmt(new MNSER::LogFormatter("%d:=>%T%p%T%m%n"));
	fileAppender->setFormatter(fmt);
	fileAppender->setLevel(MNSER::LogLevel::DEBUG);

	logger->addAppender(fileAppender);
	MNSER::StdoutLogAppender::ptr sout(new MNSER::StdoutLogAppender);
	logger->addAppender(sout);
	sout->setFormatter(fmt);
	MNSER::LogEvent::ptr event(new LogEvent(logger, MNSER::LogLevel::DEBUG, __FILE__, __LINE__, 0, 0, 0, 0, "thread1"));
	//event->getSS() << "Error";
	//logger->log(MNSER::LogLevel::ERROR, event);
	//MS_LOG_DEBUG(logger) << "debug";
	//MS_LOG_FATAL(logger) << "fatal";
	std::cout << "==================" << std::endl;
	//MS_LOG_FMT_DEBUG(logger, "This is a %s, id is %d", "debug test", 20);
	auto l = MNSER::LoggerMgr::GetInstance()->getLogger("x1");
	MS_LOG_INFO(l) << "x1";
	MNSER::LoggerMgr::GetInstance()->addLogger(logger);
	auto root = MNSER::LoggerMgr::GetInstance()->getLogger("root");
	MS_LOG_INFO(root) << "root";
	return 0;
}
