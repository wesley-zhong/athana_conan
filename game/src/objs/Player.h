#ifndef GAME_ROLE_HPP_
#define GAME_ROLE_HPP_
#include "core/common/ObjectPool.hpp"
#include "core/log/XLog.h"
#include "transport/Channel.h"
#include "module/Module.h"
#include "module/ModuleContainer.h"

class Player : public ObjPool::PoolObjClass<Player> {

    /* data */
public:
    Player(/* args */) {
    };

    Player(uint32_t pid, Channel *channel) {
        this->pid = pid;
        this->channel = channel;
    }
    void  initModules();

    Player(const Player &player) {
        this->pid = player.pid;
        this->channel = player.channel;
    }

    Player(Player &&player) {
        this->pid = player.pid;
        this->channel = player.channel;
        player.channel = nullptr;
        player.pid = 0;
    }

    ~Player() {
        INFO_LOG("------ CALL  ~GameRole");
    };

    void sendMsg(int msgId, std::shared_ptr<google::protobuf::Message> msg);

    uint32_t getPid() {
        return pid;
    }

    void setPid(uint32_t pid) {
        this->pid = pid;
    }
private:
    uint32_t pid;
    Channel *channel;
    ModuleContainer * moduleContainer = new ModuleContainer();
};

#endif
