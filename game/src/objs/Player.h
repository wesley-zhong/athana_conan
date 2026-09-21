#ifndef GAME_ROLE_HPP_
#define GAME_ROLE_HPP_
#include "common/ObjectPool.hpp"
#include "log/XLog.h"
#include "transport/Channel.h"
#include "transport/EventLoop.h"
#include "module/Module.h"
#include "module/ModuleContainer.h"
#include "actor/Actor.h"

class Player : public core::ObjPool::PoolObjClass<Player>
{
    /* data */
public:
    Player(/* args */)
    {
    };

    Player(uint32_t pid, transport::Channel* channel)
    {
        this->pid = pid;
        setChannel(channel);
    }

    void initModules();

    Player(const Player& player)
    {
        this->pid = player.pid;
        this->channel = player.channel;
    }

    Player(Player&& player)
    {
        this->pid = player.pid;
        this->channel = player.channel;
        player.channel = nullptr;
        player.pid = 0;
    }

    ~Player()
    {
        INFO_LOG("------ CALL  ~Player");
    };

    void sendMsg(int msgId, std::shared_ptr<google::protobuf::Message> msg);

    uint32_t getPid()
    {
        return pid;
    }

    // call on io thread
    void loadDataFormDB();

    // call on logic thread
    void onLogin();

    void onLogout();

    void saveDataToDB();

    void setPid(uint32_t pid)
    {
        this->pid = pid;
    }

    void setChannel(transport::Channel* channel)
    {
        // 转为 shared_ptr 持有：连接关闭后 channel 会被回收，
        // Player 作为长生命周期对象不能持有裸指针
        if (channel != nullptr && channel->event_loop() != nullptr)
        {
            this->channel = channel->event_loop()->channelPtr(channel);
        }
        else
        {
            this->channel = nullptr;
        }
    }

    transport::Channel* getChannel()
    {
        return this->channel.get();
    }

    void setHashCode(uint32_t hashCode)
    {
        this->hashCode = hashCode;
    }

    uint32_t getHashCode() const
    {
        return this->hashCode;
    }

    void execute(std::function<void()> func) const
    {
        actor_->execute(func);
    }

private:
    uint32_t hashCode = 0;
    uint32_t pid = 0;
    std::shared_ptr<transport::Channel> channel;
    ModuleContainer* moduleContainer = new ModuleContainer();
    core::actor::Actor*  actor_ = nullptr;
};

#endif
