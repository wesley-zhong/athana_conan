#include "sol/sol.hpp"
#include "log/XLog.h"
#include "common/Tools.h"
#include "common/XFile.h"
#include "common/XTime.h"

class Lua_XLog
{
public:
	static void info_log(const char * log) { INFO_LOG(log); }
	static void warning_log(const char * log) { WARN_LOG(log); }
	static void error_log(const char * log) { ERR_LOG(log); }
};

void luabind_common(sol::state & lua)
{
	// file
	lua["writeFile"] = &core::XFile::writeFile;
	lua["isExist"] = &core::XFile::isExist;
	lua["mkdir"] = &core::XFile::mkdir;
	lua["rmdir"] = &core::XFile::rmdir;
	lua["createDir"] = &core::XFile::createDirectory;

	// log
	lua["infoLog"] = &Lua_XLog::info_log;
	lua["warningLog"] = &Lua_XLog::warning_log;
	lua["errorLog"] = &Lua_XLog::error_log;

//	// timer
//	lua.new_usertype<UTimer>("UTimer",
//		"start", &UTimer::start,
//		"stop", &UTimer::stop);

	// tool
	lua["sleep"] = &core::Tools::sleep;
	lua["gbk_utf8"] = &core::Tools::gbkToUtf8;
	lua["utf8_gbk"] = &core::Tools::utf8ToGbk;
	lua["srand"] = &core::Tools::srand;
	lua["random"] = &core::Tools::random;

	// time
	lua["isLeapYear"] = &core::XTime::isLeapYear;
	lua["yearMonthDays"] = &core::XTime::yearMonthDays;
	lua["milliStamp"] = &core::XTime::milliStamp;
	lua["microStamp"] = &core::XTime::microStamp;
	lua["stamp"] = &core::XTime::stamp;
	lua["iclock"] = &core::XTime::iclock;
}