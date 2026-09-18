#pragma once
/************************************************************************
* @file      RedisDAO.h
* @brief     redis template DAO, key format: prefix:id
* @version   0.1
************************************************************************/

#include <string>
#include <optional>
#include <type_traits>
#include "core/log/XLog.h"
#include "core/common/BaseType.h"
#include "RedisCommand.h"
#include "RedisResult.h"
#include "RedisSerializable.h"
#include "DB_Interface_redis.h"

namespace dal::Cache
{
    // local re-declaration of the connection defined in db/Dal.cpp
    extern DBInterfaceRedis* redis;
}

namespace dal
{
    template <typename DO_T>
    class RedisDAO
    {
        static_assert(std::is_base_of_v<RedisSerializable, DO_T>,
                      "DO_T must derive from dal::RedisSerializable");

    public:
        explicit RedisDAO(std::string keyPrefix, DBInterfaceRedis* db = nullptr)
        {
            m_keyPrefix = std::move(keyPrefix);
            m_db = db ? db : dal::Cache::redis;
        }

        std::optional<DO_T> find_one(int64 id)
        {
            return find_one(std::to_string(id));
        }

        std::optional<DO_T> find_one(const std::string& key)
        {
            RedisCommand cmd("GET");
            cmd.pushString(makeKey(key));
            RedisResult result;
            if (!exec(&cmd, &result))
            {
                return std::nullopt;
            }
            // key not exist -> nullopt
            return result.getObject<DO_T>();
        }

        bool update(int64 id, const DO_T& obj)
        {
            return update(std::to_string(id), obj);
        }

        bool update(const std::string& key, const DO_T& obj)
        {
            RedisCommand cmd("SET");
            cmd.pushString(makeKey(key));
            cmd.pushData(obj.toString());
            RedisResult result;
            if (!exec(&cmd, &result))
            {
                return false;
            }
            if (result.replyType() != REDIS_REPLY_STATUS)
            {
                ERR_LOG("Redis update failed, key={}, replyType={}", makeKey(key), result.replyType());
                return false;
            }
            INFO_LOG("Redis update ok, key={}", makeKey(key));
            return true;
        }

        bool remove(int64 id)
        {
            return remove(std::to_string(id));
        }

        bool remove(const std::string& key)
        {
            RedisCommand cmd("DEL");
            cmd.pushString(makeKey(key));
            RedisResult result;
            if (!exec(&cmd, &result))
            {
                return false;
            }
            if (result.replyType() != REDIS_REPLY_INTEGER)
            {
                ERR_LOG("Redis remove failed, key={}, replyType={}", makeKey(key), result.replyType());
                return false;
            }
            INFO_LOG("Redis remove ok, key={}", makeKey(key));
            return true;
        }

    private:
        std::string makeKey(const std::string& key) const
        {
            return m_keyPrefix + ":" + key;
        }

        bool exec(RedisCommand* cmd, RedisResult* result)
        {
            if (!m_db)
            {
                ERR_LOG("RedisDAO execute failed, redis not init, keyPrefix={}", m_keyPrefix);
                return false;
            }
            int ret = m_db->execute(cmd, result);
            if (ret < 0)
            {
                ERR_LOG("RedisDAO execute failed, keyPrefix={}, errno={}, error={}",
                        m_keyPrefix, m_db->getErrno(), m_db->getError());
                return false;
            }
            return true;
        }

    private:
        std::string m_keyPrefix;
        DBInterfaceRedis* m_db;
    };
} // namespace dal
