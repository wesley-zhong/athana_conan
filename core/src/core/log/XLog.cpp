#include "XLog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/async.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
//spdlog::logger *pSpdLog = nullptr;
void xLogInitLog(LogLevel logLevel, const std::string &logFileName)
{
	auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	console_sink->set_level(spdlog::level::debug);
	//console_sink->set_pattern("[multi_sink_example] [%^%l%$] %v");
    console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%s:%#] %v");

	spdlog::init_thread_pool(8192, 1);
    auto stdout_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt >();
    auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logFileName, 1024*1024*128, 128);

    std::vector<spdlog::sink_ptr> sinks {stdout_sink, rotating_sink};
    auto logger = std::make_shared<spdlog::async_logger>("loggername", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    spdlog::register_logger(logger);
	spdlog::set_default_logger(logger);
	//pSpdLog= logger.get();
	switch (logLevel)
	{
	case LogLevel::LL_DEBUG:
		spdlog::set_level(spdlog::level::debug);
		break;
	case LogLevel::LL_INFO:
		spdlog::set_level(spdlog::level::info);
		break;
	case LogLevel::LL_WARNING:
		spdlog::set_level(spdlog::level::warn);
		break;
	case LogLevel::LL_ERROR:
		spdlog::set_level(spdlog::level::err);
		break;

	default:
		break;
	}
	spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [thread %t] [%s:%#] %v");
	spdlog::flush_every(std::chrono::seconds(3));
	// spdlog::set_pattern("[%H:%M:%S %z] [%n] [%^---%L---%$] [thread %t] %v");
}