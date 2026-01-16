//
// Created by zhongweiqi on 2026/1/16.
//

#ifndef ATHENA_ATHENAETCDCLIENT_H
#define ATHENA_ATHENAETCDCLIENT_H
#include <string>
#include "etcd/Client.hpp"
#include "etcd/KeepAlive.hpp"
#include "etcd/Response.hpp"
#include "etcd/SyncClient.hpp"
#include "etcd/Value.hpp"
#include "etcd/Watcher.hpp"
#include "etcd/v3/Transaction.hpp"
#include "etcd/v3/action_constants.hpp"

class AthenaEtcdClient {
public:
    AthenaEtcdClient(std::string_view ip, int port);

    int connect();

    // 建议返回 string 而不是 string_view，防止悬挂指针
    std::vector<std::string> get(const std::vector<std::string_view> &keys);

    void watchKeys(const std::vector<std::string> &keys,
                   std::function<void(const std::string_view &, const std::string_view &)> callback);

    int64_t getLease(const std::string_view key);

    void keepAlive(std::string key, std::string value, int ttl = 20);

private:
    std::unique_ptr<etcd::Client> client;
    // 必须持有 Watcher，否则监听会立即停止
    std::vector<std::unique_ptr<etcd::Watcher> > watchers;
    std::map<std::string, std::shared_ptr<etcd::KeepAlive> > keep_alives;
};


#endif //ATHENA_ATHENAETCDCLIENT_H
