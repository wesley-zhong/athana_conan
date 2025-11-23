#ifndef XLOG_H_
#define XLOG_H_
#include "spdlog/spdlog.h"

#define LOG_MAX_SIZE 1073741824
enum LogLevel
{
	LL_DEBUG,
	LL_INFO,
	LL_WARNING,
	LL_ERROR,
};

void xLogInitLog(LogLevel logLevel, const std::string &fileName);

#define DEBUG_LOG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define INFO_LOG(...)  SPDLOG_INFO (__VA_ARGS__)
#define WARN_LOG(...) SPDLOG_WARN(__VA_ARGS__)
#define ERR_LOG(...) SPDLOG_ERROR(__VA_ARGS__)

#endif
