//
// Created by zhongweiqi on 2026/9/14.
//

#pragma once

#include <unordered_map>
#include <mutex>
#include <memory>
#include <type_traits>
#include <utility>

#include "Actor.h"

namespace core {

namespace actor {

    // actor 注册与消息路由：
    //   - spawn<T>() 创建并启动一个 actor（自动分配全局唯一 id、独立线程、独立无锁信箱）
    //   - send(to, closure) 按 id 投递闭包，投递路径无锁（注册表读多写少，仅查表时短暂加锁）
    // ActorSystem 拥有它 spawn 出的 actor 的生命周期，stopAll() 之后外部持有的指针全部失效。
    class ActorSystem {
    public:
        static ActorSystem &instance();

        ~ActorSystem();

        ActorSystem(const ActorSystem &) = delete;

        ActorSystem &operator=(const ActorSystem &) = delete;

        // 创建并启动一个 actor，返回其指针；失败返回 nullptr
        template<typename T, typename... Args>
        T *spawn(Args &&... args) {
            static_assert(std::is_base_of<Actor, T>::value, "T must inherit actor::Actor");
            T *a = new T(std::forward<Args>(args)...);
            {
                std::lock_guard<std::mutex> lock(_mutex);
                if (!a->start()) {
                    delete a;
                    return nullptr;
                }
                _actors[a->id()] = a;
            }
            return a;
        }

        // 按 id 查找 actor（返回原始指针，仅在系统运行期间有效）
        Actor *find(uint64 id);

        // 按 id 投递闭包（任意 lambda/可调用体）；目标不存在或 actor 未运行返回 false
        template<typename F>
        bool execute(uint64 to, F &&func) {
            static_assert(std::is_constructible<VOID_FUN, F &&>::value,
                          "ActorSystem::execute() requires a callable compatible with std::function<void()>");
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = _actors.find(to);
            if (it == _actors.end()) {
                ERR_LOG("actor {} not found, drop task", to);
                return false;
            }
            // 信箱入队本身无锁，这里持锁只为保证查表到投递之间 actor 不被销毁
            return it->second->execute(std::forward<F>(func));
        }

        // 带参重载：func 的入参直接跟在后面，由目标 actor 打包成 void() 闭包后串行执行
        template<typename F, typename... Args>
        bool execute(uint64 to, F &&func, Args &&... args) {
            std::lock_guard<std::mutex> lock(_mutex);
            auto it = _actors.find(to);
            if (it == _actors.end()) {
                ERR_LOG("actor {} not found, drop task", to);
                return false;
            }
            // 信箱入队本身无锁，这里持锁只为保证查表到投递之间 actor 不被销毁
            return it->second->execute(std::forward<F>(func), std::forward<Args>(args)...);
        }

        // 停止并销毁全部 actor（等各自信箱清空）
        void stopAll();

        size_t count();

    private:
        ActorSystem() = default;

        std::unordered_map<uint64, Actor *> _actors;
        std::mutex _mutex;
    };

}

// ============================ 使用示例 ============================
//
//  #include "actor/ActorSystem.h"
//
//  class PlayerActor : public actor::Actor {
//  public:
//      PlayerActor() : Actor("player") {}          // 信箱预分配 8192
//  protected:
//      void onStart() override { INFO_LOG("player actor online"); }
//  };
//
//  void demo() {
//      auto &sys = actor::ActorSystem::instance();
//      auto *pa = sys.spawn<PlayerActor>();                        // 一个 actor 一个线程
//      auto data = std::make_shared<PlayerData>();
//      sys.execute(pa->id(), [data]() { /* 业务处理，天然单线程串行 */ });
//      pa->execute([data](int hp) { data->setHp(hp); }, 100);      // 带参投递：参数跟在闭包后面
//      actor::ActorSystem::instance().stopAll();                   // 进程退出前调用
//  }

} // namespace core
