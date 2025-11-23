#include "sol/sol.hpp"

#include "hiredis/hiredis.h"
#include "db/DB_Interface_redis.h"
#include "db/RedisResult.h"
#include "db/DBThreadPool.h"
#include "db/RedisCommand.h"
class Lua_Redis
{
public:
    bool conncet(const std::string &ip, int port)
    {
        redisCtx = redisConnect(ip.c_str(), 6379);
        if (redisCtx == nullptr || redisCtx->err)
        {
            if (redisCtx)
            {
                ERR_LOG("Error: {} ", redisCtx->errstr);
                return false;
            }

            ERR_LOG("Can't allocate redis context");
            return false;
        }
        return true;
    }
    std::string getString(const std::string& key) const
    {
        redisReply *reply = (redisReply *)redisCommand(redisCtx, ("GET " + key).c_str());
        INFO_LOG("GET key ={}: {}", key, reply->str);
        std::string ret = std::string(reply->str, reply->len);
        freeReplyObject(reply);
        return ret;
    }
    void setString(const std::string &key, const std::string &value)
    {
        char **argv;
        size_t *argvlen;
        argv = (char **)malloc(sizeof(*argv) * (3));
        argvlen = (size_t *)malloc(sizeof(*argvlen) * (3));
        argv[0] = (char *)"SET";
        argvlen[0] = sizeof("SET") - 1;
        argv[1] = (char *)key.c_str();
        argvlen[1] = key.length();

        argv[2] = (char *)value.c_str();
        argvlen[2] = value.length();


        redisReply *reply = (redisReply *)redisCommandArgv(redisCtx, 3, (const char**)argv, (const size_t*)argvlen);
        if (reply == NULL)
        {
            ERR_LOG("Error:  Couldn't execute redisCommandArgv");
            return ;
        }

        if (reply->type == REDIS_REPLY_INTEGER)
        {
            INFO_LOG("%s reply: %lld\n", argv[0], reply->integer);
        }

        INFO_LOG("SET key = {}: {}", key, value);
        freeReplyObject(reply);
    }
private:
    redisContext *redisCtx;
};


class Lua_RedisResult
{
public:
	Lua_RedisResult(std::shared_ptr<RedisResult> result):
		m_result(result)
	{

	}

	//get
	int8 getInt8() { return getValue<int8>(); }
	uint8 getUint8() { return getValue<uint8>(); }
	int16 getInt16() { return getValue<int16>(); }
	uint16 getUint16() { return getValue<uint16>(); }
	int32 getInt32() { return getValue<int32>(); }
	uint32 getUint32() { return getValue<uint32>(); }
	int64 getInt64() { return getValue<int64>(); }
	uint64 getUint64() { return getValue<uint64>(); }
	float getFloat() { return getValue<float>(); }
	double getDouble() { return getValue<double>(); }
	std::string getString() { return getValue<std::string>(); }
	std::string_view getData() { return m_result->getStream(); }

	bool fetch() {
		return m_result->fetch();
	}

	template<class T>
	T getValue()
	{
		T t;
		*m_result >> t;
		return t;
	}

private:
	std::shared_ptr<RedisResult> m_result;
};

class Lua_RedisCommand
{
public:
	Lua_RedisCommand(const char * cmd):
		m_command(new RedisCommand(cmd))
	{
		
	}

	void pushInt8(int8 value) { m_command->pushInt8(value); }
	void pushUint8(uint8 value) { m_command->pushUint8(value); }
	void pushInt16(int16 value) { m_command->pushInt16(value); }
	void pushUint16(uint16 value) { m_command->pushUint16(value); }
	void pushInt32(int32 value) { m_command->pushInt32(value); }
	void pushUint32(uint32 value) { m_command->pushUint32(value); }
	void pushInt64(int64 value) { m_command->pushInt64(value); }
	void pushUint64(uint64 value) { m_command->pushUint64(value); }
	void pushFloat(float value) { m_command->pushFloat(value); }
	void pushDouble(double value) { m_command->pushDouble(value); }
	void pushString(std::string value) { m_command->pushString(value); }
	void pushData(std::string sv) { m_command->pushData(sv); }

	void addToPool(DBThreadPool * pool, std::function<void(int32, const char* , Lua_RedisResult *)> backfunc)
	{
		auto dbTask = new DBRedisTask(m_command, std::unique_ptr<RedisResult>());

		// back func
		dbTask->backfunc = [backfunc](int32 errno_, const char * err, std::shared_ptr<RedisResult> result) {

			if (backfunc != nullptr)
			{
				Lua_RedisResult _result(result);
				backfunc(errno_, err, &_result);
			}
		};
		pool->executeTask(dbTask);
	}

private:
	std::shared_ptr<RedisCommand> m_command;
};

void luabind_redis(sol::state & lua)
{
    lua.new_usertype<Lua_Redis>("Redis",
                                    "connect", &Lua_Redis::conncet,
                                    "setString", &Lua_Redis::setString,
                                    "getString", &Lua_Redis::getString);

	lua.new_usertype<Lua_RedisCommand>("RedisCommand",
		sol::constructors<Lua_RedisCommand(const char *)>(),
		"pushInt8", &Lua_RedisCommand::pushInt8,
		"pushUint8", &Lua_RedisCommand::pushUint8,
		"pushInt16", &Lua_RedisCommand::pushInt16,
		"pushUint16", &Lua_RedisCommand::pushUint16,
		"pushInt32", &Lua_RedisCommand::pushInt32,
		"pushUint32", &Lua_RedisCommand::pushUint32,
		"pushInt64", &Lua_RedisCommand::pushInt64,
		"pushUint64", &Lua_RedisCommand::pushUint64,
		"pushFloat", &Lua_RedisCommand::pushFloat,
		"pushDouble", &Lua_RedisCommand::pushDouble,
		"pushString", &Lua_RedisCommand::pushString,
		"pushData", &Lua_RedisCommand::pushData,
		"addToPool", &Lua_RedisCommand::addToPool);

	lua.new_usertype<Lua_RedisResult>("RedisResult",
		"getInt8", &Lua_RedisResult::getInt8,
		"getUint8", &Lua_RedisResult::getUint8,
		"getInt16", &Lua_RedisResult::getInt16,
		"getUint16", &Lua_RedisResult::getUint16,
		"getInt32", &Lua_RedisResult::getInt32,
		"getUint32", &Lua_RedisResult::getUint32,
		"getInt64", &Lua_RedisResult::getInt64,
		"getUint64", &Lua_RedisResult::getUint64,
		"getFloat", &Lua_RedisResult::getFloat,
		"getDouble", &Lua_RedisResult::getDouble,
		"getString", &Lua_RedisResult::getString,
		"getData", &Lua_RedisResult::getData,
		"fetch", &Lua_RedisResult::fetch);
}