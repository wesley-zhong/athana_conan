#ifndef  GATE_PLAYER_HPP_
#define GATE_PLAYER_HPP_
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

    int64_t getSid()
    {
        return sid;
    }

    transport::Channel* getChannel()
    {
        return channel;
    }

    void setValid()
    {
        // userData 挂接挪到 createPlayer 建号时：登录未完成就掉线也能按连接反查
        valid = true;
    }

    void setPid(uint32_t pid);
};

#endif
