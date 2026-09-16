#pragma once

#include "core/log/XLog.h"
#include "core/common/BaseType.h"
#include "DBResult.h"
#include "hiredis/hiredis.h"
#include <string>

// class BasePacket;
// struct redisReply;
class RedisResult : public DBResult
{
public:
	RedisResult();
	~RedisResult();
	void setResult(redisReply * result);

	virtual bool isEmpty();
	virtual bool fetch();
	virtual uint32 getRowCount();
	virtual uint32 getFieldsCount();

	virtual const char * getData(int & len);
	virtual const char * getData();

	// base type
	template<typename T>
	RedisResult & operator >> (T & t)
	{
		if (pos >= getFieldsCount())
		{
			ERR_LOG("redis row count upper limit");
			return *this;
		}

		// blob
		const char * p = m_reply->str ? m_reply->str : m_reply->element[pos]->str;
		memcpy(&t, p, sizeof(t));

		pos++;
		return *this;
	}

	// string type
	RedisResult &operator>>(std::string& value);

	std::string_view getStream();
private:
	redisReply * m_reply;
	uint32 pos;
};

