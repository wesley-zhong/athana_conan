//
// Created by zhongweiqi on 2026/1/16.
//

#include "AthenaEtcdClient.h"

#include <iostream>
#include "core/log/XLog.h"

using namespace etcd;

// 构造函数：将 string_view 组合成连接字符串
AthenaEtcdClient::AthenaEtcdClient(std::string addrs) {
    client = std::make_unique<etcd::Client>(addrs);
}

int AthenaEtcdClient::connect() {
    // etcd-cpp-apiv3 通常是惰性连接，但可以通过头跳检查来验证连接
    auto response = client->head().get();
    if (response.is_ok()) {
        return 0;
    }
    return response.error_code();
}

std::string AthenaEtcdClient::get(const std::string &key) {
    auto response = client->get(key).get();
    if (response.is_ok()) {
        return response.value().as_string();
    }
    return nullptr;
}

// 获取一组 Key 的值
std::vector<std::string> AthenaEtcdClient::get(const std::vector<std::string_view> &keys) {
    std::vector<std::string> results;
    for (auto const &key: keys) {
        // 注意：etcd API 需要 std::string，string_view 必须转换
        auto response = client->get(std::string(key)).get();
        if (response.is_ok()) {
            results.push_back(response.value().as_string());
        } else {
            results.push_back(""); // 或者根据需求处理错误
        }
    }
    return results;
}

std::map<std::string, std::string> AthenaEtcdClient::getPrefix(const std::string &key) {
    return getKeysWithValues(key);
}

// 监听 Key 的变化
void AthenaEtcdClient::watchKeys(const std::vector<std::string> &keys,
                                 std::function<void(const std::string_view &, const std::string_view &)> callback) {
    for (auto const &key: keys) {
        std::string k(key);
        // Watcher 会在后台线程运行回调
        auto watcher = std::make_unique<etcd::Watcher>(*client, k, [callback](etcd::Response const &resp) {
            if (resp.is_ok()) {
                // 将结果转回 string_view 传给回调
                for (auto const &event: resp.events()) {
                    callback(event.kv().key(), event.kv().as_string());
                }
            }
        }, true);
        // 注意：Watcher 对象必须在类中管理其生命周期，否则函数结束就会停止监听
        watchers.push_back(std::move(watcher));
    }
}

void AthenaEtcdClient::keepAlive(std::string key, std::string value, int ttl) {
    // 0. 防止重复注册导致的资源浪费
    if (keep_alives.find(key) != keep_alives.end()) {
        WARN_LOG("Key {} is already being kept alive, updating...", key);
        // 如果需要，可以在这里先停止旧的
    }

    // 1. 申请租约
    auto keeper = client->leasekeepalive(ttl).get();
    if (!keeper) {
        ERR_LOG("Failed create keep alive{} ");
        return;
    }
    int64_t lease_id = keeper->Lease();

    // 2. 将 Key 绑定到租约
    auto put_resp = client->put(key, value, lease_id).get();
    if (!put_resp.is_ok()) {
        ERR_LOG("Failed to bind key to lease: {} ", put_resp.error_message());
        // 绑定失败应撤销租约，防止孤儿租约
        client->leaserevoke(lease_id);
        keeper->Cancel();
        return;
    }
    keep_alives[key] = keeper;

    INFO_LOG("KeepAlive started for key: {} with lease: {}", key, lease_id);
}

std::map<std::string, std::string> AthenaEtcdClient::getKeysWithValues(std::string const &prefix) {
    Response keys_response = client->keys(prefix).get();

    if (!keys_response.is_ok()) {
        throw std::runtime_error("Failed to get keys");
    }

    std::map<std::string, std::string> result;

    // 对每个键获取值（同步方式）
    for (const auto &key: keys_response.keys()) {
        // 跳过目录（以/结尾的）
        if (!key.empty() && key.back() == '/') {
            continue;
        }

        // 同步获取值
        Response value_response = client->get(key).get();
        if (value_response.is_ok()) {
            result[key] = value_response.value().as_string();
        }
    }
    return result;
}