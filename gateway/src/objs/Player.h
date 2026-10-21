#ifndef GAME_ROLE_HPP_
#define GAME_ROLE_HPP_
#include "Channel.h"
#include "common/ObjectPool.hpp"
#include "log/XLog.h"

class Player : public core::ObjPool::PoolObjClass<Player>
{
private:
    uint32 pid;
    int64 sid;
    transport::Channel* channel;
    bool valid;

    /* data */
public:
    Player(/* args */)
    {
    };

    Player(uint32 pid, int64 sid, transport::Channel* channel)
    {
        this->pid = pid;
        this->channel = channel;
        valid = false;
        this->sid = sid;
    }

    ~Player()
    {
        INFO_LOG("------ CALL  ~GameRole");
    };

    uint32_t getPid()
    {
        return pid;
    }

    void setValid()
    {
        valid = true;
        channel->setUserData(this);
    }

    void setPid(uint32_t pid);
};

#endif
