//
// Created by zhongweiqi on 2026/1/16.
//

#include "AthenaEtcdClient.h"

#include <iostream>
#include "core/log/XLog.h"
// 构造函数：将 string_view 组合成连接字符串
AthenaEtcdClient::AthenaEtcdClient(std::string_view ip, int port) {
    std::string url = "http://" + std::string(ip) + ":" + std::to_string(port);
    // 实例化 etcd 客户端
    client = std::make_unique<etcd::Client>(url);
}

int AthenaEtcdClient::connect() {
    // etcd-cpp-apiv3 通常是惰性连接，但可以通过头跳检查来验证连接
    auto response = client->head().get();
    if (response.is_ok()) {
        return 0;
    }
    return response.error_code();
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
        });
        // 注意：Watcher 对象必须在类中管理其生命周期，否则函数结束就会停止监听
        watchers.push_back(std::move(watcher));
    }
}

// 获取租约 (Lease)
int64_t AthenaEtcdClient::getLease(const std::string_view key) {
    // 默认租约时间，例如 10 秒
    auto response = client->leasegrant(10).get();
    if (response.is_ok()) {
        return response.value().lease();
    }
    return -1;
}

void AthenaEtcdClient::keepAlive(std::string key, std::string value, int ttl) {
    // 1. 申请一个租约
    auto lease_resp = client->leasegrant(ttl).get();
    if (!lease_resp.is_ok()) {
        ERR_LOG("Failed to grant lease: {} ", lease_resp.error_message());
        return;
    }
    int64_t lease_id = lease_resp.value().lease();

    // 2. 将 Key 绑定到这个租约上
    // 注意：put 操作需要带上 lease_id
    auto put_resp = client->put(key, value, lease_id).get();
    if (!put_resp.is_ok()) {
        ERR_LOG("Failed to bind key to lease: {} ", put_resp.error_message());
        return;
    }

    // 3. 创建 KeepAlive 对象进行自动续约
    // etcd::KeepAlive 内部会维护一个后台线程发送心跳
    auto keeper = client->leasekeepalive(lease_id);

    // 4. 管理 KeepAlive 对象的生命周期
    // 必须保存这个对象，否则续约会停止
    keep_alives[key] = keeper.get();

    std::cout << "KeepAlive started for key: " << key << " with lease: " << lease_id << std::endl;
}
