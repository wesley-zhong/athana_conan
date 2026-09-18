#include "RoleDO.hpp"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <string_view>
#include "core/log/XLog.h"

std::string RoleDO::toString() const {
    rapidjson::StringBuffer __sb;
    rapidjson::Writer<rapidjson::StringBuffer> __w(__sb);
    __w.StartObject();
    __w.Key("_id");
    __w.Int64(_id);
    __w.Key("name");
    __w.String(name.c_str(), name.size());
    __w.EndObject();
    return std::string(__sb.GetString(), __sb.GetSize());
}

void RoleDO::fromString(std::string_view sv) {
    rapidjson::Document __doc;
    __doc.Parse(sv.data(), sv.size());
    if (__doc.HasParseError()) {
        ERR_LOG("{}::fromString parse failed, code={} offset={}", "RoleDO",
                static_cast<int>(__doc.GetParseError()), __doc.GetErrorOffset());
        return;
    }
    const rapidjson::Value& __obj = __doc;
    rapidjson::Value::ConstMemberIterator __it;
    __it = __obj.FindMember("_id");
    if (__it != __obj.MemberEnd() && __it->value.IsInt64())
        _id = __it->value.GetInt64();
    __it = __obj.FindMember("name");
    if (__it != __obj.MemberEnd() && __it->value.IsString())
        name = std::string(__it->value.GetString(), __it->value.GetStringLength());
}
