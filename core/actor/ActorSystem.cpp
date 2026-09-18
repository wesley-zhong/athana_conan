//
// Created by zhongweiqi on 2026/9/14.
//

#include "ActorSystem.h"
#include "../log/XLog.h"

#include <vector>

namespace core {

namespace actor {

    ActorSystem &ActorSystem::instance() {
        static ActorSystem s_instance;
        return s_instance;
    }

    ActorSystem::~ActorSystem() {
        stopAll();
    }

    Actor *ActorSystem::find(uint64 id) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _actors.find(id);
        return it == _actors.end() ? nullptr : it->second;
    }

    void ActorSystem::stopAll() {
        // 先摘下全部 actor 再逐个停止：stop() 会等各自信箱清空、线程退出
        std::vector<Actor *> dead;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            dead.reserve(_actors.size());
            for (auto &kv : _actors) {
                dead.push_back(kv.second);
            }
            _actors.clear();
        }
        for (Actor *a : dead) {
            a->stop();
            delete a;
        }
    }

    size_t ActorSystem::count() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _actors.size();
    }

}

} // namespace core
