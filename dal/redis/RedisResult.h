#pragma once

#include "log/XLog.h"
#include "common/BaseType.h"
#include "../db/DBResult.h"
#include "hiredis/hiredis.h"
#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <type_traits>
#include <cstring>
#include <utility>
#include "RedisSerializable.h"

namespace dal
{
    // class BasePacket;
    // struct redisReply;
    class RedisResult : public DBResult
    {
    public:
        RedisResult();
        ~RedisResult();
        void setResult(redisReply* result);

        virtual bool isEmpty();
        virtual bool fetch();
        virtual uint32 getRowCount();
        virtual uint32 getFieldsCount();

        virtual const char* getData(int& len);
        virtual const char* getData();

        // base type
        template <typename T>
        RedisResult& operator >>(T& t)
        {
            static_assert(std::is_trivially_copyable_v<T>, "use getObject<T>() for serializable DOs");
            if (pos >= getFieldsCount())
            {
                ERR_LOG("redis row count upper limit");
                return *this;
            }

            // blob
            const char* p = m_reply->str ? m_reply->str : m_reply->element[pos]->str;
            memcpy(&t, p, sizeof(t));

            pos++;
            return *this;
        }

        // string type
        RedisResult& operator>>(std::string& value);

        std::string_view getStream();

        // serializable DO (single string reply, e.g. GET)
        template <typename T>
        std::optional<T> getObject()
        {
            static_assert(std::is_base_of_v<RedisSerializable, T>, "T must derive from dal::RedisSerializable");

            if (!m_reply)
            {
                ERR_LOG("redis getObject: reply is null");
                return std::nullopt;
            }
            if (m_reply->type == REDIS_REPLY_NIL)
            {
                // key not exist
                return std::nullopt;
            }
            if (m_reply->type != REDIS_REPLY_STRING)
            {
                ERR_LOG("redis getObject: expect string reply, type={}", m_reply->type);
                return std::nullopt;
            }

            T obj;
            obj.fromString(std::string_view(m_reply->str, m_reply->len));
            return obj;
        }

        // serializable DOs (array reply, e.g. MGET / LRANGE)
        template <typename T>
        std::vector<T> getObjects()
        {
            static_assert(std::is_base_of_v<RedisSerializable, T>, "T must derive from dal::RedisSerializable");

            std::vector<T> out;
            if (!m_reply)
            {
                ERR_LOG("redis getObjects: reply is null");
                return out;
            }
            if (m_reply->type != REDIS_REPLY_ARRAY)
            {
                ERR_LOG("redis getObjects: expect array reply, type={}", m_reply->type);
                return out;
            }

            out.reserve(m_reply->elements);
            for (size_t i = 0; i < m_reply->elements; ++i)
            {
                redisReply* e = m_reply->element[i];
                if (e->type != REDIS_REPLY_STRING)
                {
                    WARN_LOG("redis getObjects: skip non-string element, index={}, type={}", i, e->type);
                    continue;
                }
                T obj;
                obj.fromString(std::string_view(e->str, e->len));
                out.push_back(std::move(obj));
            }
            return out;
        }

        // reply type of the last command, -1 when no reply
        int replyType() const;

    private:
        redisReply* m_reply;
        uint32 pos;
    };
} // namespace dal
