
#ifndef XTIME_H
#define XTIME_H

#include <time.h>
#include <string>
#include "Platform.h"
#include "BaseType.h"


#ifdef SYSTEM_WIN
#include <Windows.h>
#else
#include <sys/time.h>
#endif // SYSTEM_WIN

namespace core {


class XTime
{
public:
	// leap year
	static bool isLeapYear(int year);
	static int yearMonthDays(int year, int month);

	static const struct tm * getTMStruct();
	static time_t getTime(struct tm * tm_);
	static std::string format(const char * fmt = "%Y-%m-%d %H:%M:%S");

	static void getTimeval(struct timeval * tp);
	// msec
	static int64 milliStamp();
	// usec
	static int64 microStamp();
	// sec
	static time_t stamp();

	// 纪元毫秒, 语义同 Java System.currentTimeMillis
	static int64 currentTimeMillis();

	static uint32 iclock();
};

} // namespace core

#endif
