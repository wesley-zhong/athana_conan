#include "QuestDO.hpp"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <string_view>
#include "log/XLog.h"

std::string QuestDO::toString() const {
    rapidjson::StringBuffer __sb;
    rapidjson::Writer<rapidjson::StringBuffer> __w(__sb);
    __w.StartObject();
    __w.Key("_id");
    __w.Int64(_id);
    __w.Key("questIdsFinished");
    __w.StartArray();
    for (const auto& __e : questIdsFinished) {
        __w.Int64(__e);
    }
    __w.EndArray();
    __w.Key("curQuestIds");
    __w.StartArray();
    for (const auto& __e : curQuestIds) {
        __w.Int64(__e);
    }
    __w.EndArray();
    __w.EndObject();
    return std::string(__sb.GetString(), __sb.GetSize());
}

void QuestDO::fromString(std::string_view sv) {
    rapidjson::Document __doc;
    __doc.Parse(sv.data(), sv.size());
    if (__doc.HasParseError()) {
        ERR_LOG("{}::fromString parse failed, code={} offset={}", "QuestDO",
                static_cast<int>(__doc.GetParseError()), __doc.GetErrorOffset());
        return;
    }
    const rapidjson::Value& __obj = __doc;
    rapidjson::Value::ConstMemberIterator __it;
    __it = __obj.FindMember("_id");
    if (__it != __obj.MemberEnd() && __it->value.IsInt64())
        _id = __it->value.GetInt64();
    __it = __obj.FindMember("questIdsFinished");
    if (__it != __obj.MemberEnd() && __it->value.IsArray()) {
        for (const auto& __ae : __it->value.GetArray()) {
            if (__ae.IsInt64())
                questIdsFinished.push_back(__ae.GetInt64());
        }
    }
    __it = __obj.FindMember("curQuestIds");
    if (__it != __obj.MemberEnd() && __it->value.IsArray()) {
        for (const auto& __ae : __it->value.GetArray()) {
            if (__ae.IsInt64())
                curQuestIds.push_back(__ae.GetInt64());
        }
    }
}
