//
// Created by zhongweiqi on 2026/1/22.
//

#include "JsonUtils.h"

bool JsonUtils::DeserializeNodeInfo(const std::string &jsonStr, NodeInfo &node) {
    rapidjson::Document doc;
    if (doc.Parse(jsonStr.c_str()).HasParseError()) {
        return false;
    }

    if (!doc.IsObject()) return false;

    // 字符串解析
    if (doc.HasMember("service_id") && doc["service_id"].IsString())
        node.service_id = doc["service_id"].GetString();

    if (doc.HasMember("service_name") && doc["service_name"].IsString())
        node.service_name = doc["service_name"].GetString();

    if (doc.HasMember("ip") && doc["ip"].IsString())
        node.ip = doc["ip"].GetString();

    // 整数解析
    if (doc.HasMember("port") && doc["port"].IsInt()) node.port = doc["port"].GetInt();
    if (doc.HasMember("type") && doc["type"].IsInt()) node.type = doc["type"].GetInt();
    if (doc.HasMember("net_port") && doc["net_port"].IsInt()) node.net_port = doc["net_port"].GetInt();
    if (doc.HasMember("id") && doc["id"].IsInt()) node.id = doc["id"].GetInt();
    if (doc.HasMember("group_id") && doc["group_id"].IsInt()) node.group_id = doc["group_id"].GetInt();

    // int64 解析
    if (doc.HasMember("keep_alive_lease_id") && doc["keep_alive_lease_id"].IsInt64())
        node.keep_alive_lease_id = doc["keep_alive_lease_id"].GetInt64();

    // map 解析
    if (doc.HasMember("meta_data") && doc["meta_data"].IsObject()) {
        node.meta_data.clear();
        for (auto &m: doc["meta_data"].GetObject()) {
            node.meta_data[m.name.GetString()] = m.value.GetString();
        }
    }

    return true;

}

std::string JsonUtils::SerializeNodeInfo( NodeInfo*node) {
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();

    // 辅助宏或直接赋值
    doc.AddMember("service_id", rapidjson::Value(node->service_id.c_str(), allocator).Move(), allocator);
    doc.AddMember("service_name", rapidjson::Value(node->service_name.c_str(), allocator).Move(), allocator);
    doc.AddMember("ip", rapidjson::Value(node->ip.c_str(), allocator).Move(), allocator);
    doc.AddMember("port", node->port, allocator);
    doc.AddMember("type", node->type, allocator);
    doc.AddMember("net_port", node->net_port, allocator);
    doc.AddMember("id", node->id, allocator);
    doc.AddMember("group_id", node->group_id, allocator);
    doc.AddMember("keep_alive_lease_id", node->keep_alive_lease_id, allocator);

    // 处理 unordered_map
    rapidjson::Value metaObj(rapidjson::kObjectType);
    for (auto const &[key, val]: node->meta_data) {
        rapidjson::Value k(key.c_str(), allocator);
        rapidjson::Value v(val.c_str(), allocator);
        metaObj.AddMember(k, v, allocator);
    }
    doc.AddMember("meta_data", metaObj, allocator);

    // 转为字符串
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    return buffer.GetString();

}