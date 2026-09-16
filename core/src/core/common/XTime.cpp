#include "XTime.h"
#include <ctime>
using namespace std;

#ifdef SYSTEM_WIN
#include <winsock.h> // struct timeval
#endif

namespace
{
	// Unix 纪元时间 (秒 + 微秒), 全部时间函数的统一来源
	// Windows 下基于 UTC 的 FILETIME, 不经过本地时间/时区换算
	void epochTime(int64 * sec, int64 * usec)
	{
#ifdef SYSTEM_WIN
		FILETIME ft;
		GetSystemTimeAsFileTime(&ft);
		ULARGE_INTEGER ticks;
		ticks.LowPart = ft.dwLowDateTime;
		ticks.HighPart = ft.dwHighDateTime;
		// FILETIME 是 1601-01-01 UTC 起的 100 纳秒数, 转为 Unix 纪元
		const int64 intervals = static_cast<int64>(ticks.QuadPart) - 116444736000000000LL;
		*sec = intervals / 10000000;
		*usec = intervals % 10000000 / 10;
#else
		struct timeval tv;
		gettimeofday(&tv, NULL);
		*sec = tv.tv_sec;
		*usec = tv.tv_usec;
#endif
	}
}

bool XTime::isLeapYear(int year)
{
	return (year % 4 == 0 && ((year % 400 == 0) || (year % 100 != 0)));
}

int XTime::yearMonthDays(int year, int month)
{
	switch (month)
	{
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
		return 31;
	case 4:
	case 6:
	case 9:
	case 11:
		return 30;
	case 2:
		//Need to consider a leap year in February
		return isLeapYear(year) ? 29 : 28;
	default:
		return 0;
	}
}


const struct tm * XTime::getTMStruct()
{
	time_t now = time(NULL);
	static struct tm tmBuf;
#ifdef SYSTEM_WIN
	localtime_s(&tmBuf, &now);
#else
	localtime_r(&now, &tmBuf);
#endif
	return &tmBuf;
}

std::string XTime::format(const char * fmt)
{
	char buf[256] = { 0 };
	if (0 == strftime(buf, sizeof(buf), fmt, getTMStruct())) {
		buf[0] = '\0';
	}
	return std::string(buf);
}

time_t XTime::getTime(struct tm * tm_)
{
	return std::mktime(tm_);
}

void XTime::getTimeval(struct timeval * tp)
{
	int64 sec, usec;
	epochTime(&sec, &usec);
	tp->tv_sec = static_cast<long>(sec);
	tp->tv_usec = static_cast<long>(usec);
}

// 纪元毫秒, 语义同 Java System.currentTimeMillis
int64 XTime::currentTimeMillis()
{
	int64 sec, usec;
	epochTime(&sec, &usec);
	return sec * 1000 + usec / 1000;
}

// msec
int64 XTime::milliStamp()
{
	return currentTimeMillis();
}

// usec
int64 XTime::microStamp()
{
	int64 sec, usec;
	epochTime(&sec, &usec);
	return sec * 1000000 + usec;
}

// sec
time_t XTime::stamp()
{
	int64 sec, usec;
	epochTime(&sec, &usec);
	return static_cast<time_t>(sec);
}

uint32 XTime::iclock()
{
	return (uint32)((double)std::clock() / CLOCKS_PER_SEC * 1000);
}
