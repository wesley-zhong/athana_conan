#ifndef GAME_ROLE_HPP_
#define GAME_ROLE_HPP_
#include "core/common/ObjectPool.hpp"
#include "core/log/XLog.h"
#include "transport/Channel.h"
#include "module/Module.h"
#include "module/ModuleContainer.h"
#include "core/actor/Actor.h"

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
        this->channel = channel;
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
        this->channel = channel;
    }

    transport::Channel* getChannel()
    {
        return this->channel;
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
    uint32_t hashCode;
    uint32_t pid;
    transport::Channel* channel;
    ModuleContainer* moduleContainer = new ModuleContainer();
    core::actor::Actor*  actor_;
};

#endif
