//
// Created by zhongweiqi on 2026/1/16.
//

#ifndef ATHENA_ATHENAETCDCLIENT_H
#define ATHENA_ATHENAETCDCLIENT_H

#include <string>
#include<map>
#include <vector>
#include <atomic>
#include <mutex>
#include <thread>
#include <utility>
#include "etcd/Client.hpp"
#include "etcd/KeepAlive.hpp"
#include "etcd/Value.hpp"
#include "etcd/Watcher.hpp"

namespace discovery
{
    class AthenaEtcdClient
    {
    public:
        AthenaEtcdClient(std::string ip);

        ~AthenaEtcdClient();

        int connect();

        // 建议返回 string 而不是 string_view，防止悬挂指针
        std::vector<std::string> get(const std::vector<std::string_view>& keys);

        std::string get(const std::string& key);

        std::map<std::string, std::string> getPrefix(const std::string& key);

        void watchKeys(const std::vector<std::string>& keys,
                       std::function<void(etcd::Event::EventType, const std::string_view&, const std::string_view&)>
                       callback);

        void keepAlive(std::string key, std::string value, int ttl = 20);

    private:
        std::map<std::string, std::string> getKeysWithValues(std::string const& prefix);

        // 后台监测：lease 在服务端过期（进程暂停超过 ttl、etcd 重启、网络中断）后，
        // KeepAlive 刷新线程会收到 ttl=0 并静默退出，节点从 etcd 消失且无人知晓；
        // 监测线程发现 key 丢失后自动重新注册
        void monitorLoop();

        std::unique_ptr<etcd::Client> client;
        // 必须持有 Watcher，否则监听会立即停止
        std::vector<std::unique_ptr<etcd::Watcher>> watchers;
        std::map<std::string, std::shared_ptr<etcd::KeepAlive>> keep_alives;
        // 注册表：key -> (注册值, ttl)，供掉线自动重注册使用
        std::map<std::string, std::pair<std::string, int>> registrations;
        std::mutex regMutex;
        std::thread monitorThread;
        std::atomic<bool> monitorRunning{false};
    };
} // namespace discovery

#endif //ATHENA_ATHENAETCDCLIENT_H
