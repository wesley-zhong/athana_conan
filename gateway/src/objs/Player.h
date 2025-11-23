#ifndef GAME_ROLE_HPP_
#define GAME_ROLE_HPP_
#include "core/common/ObjectPool.hpp"
#include "core/log/XLog.h"
class Player :public  ObjPool::PoolObjClass<Player>
{
private:
   uint32_t  pid;

    /* data */
public:
    Player(/* args */) {

    };
    Player(uint32_t pid) {
        this->pid = pid;
    }
    ~Player() {
      INFO_LOG("------ CALL  ~GameRole");
    };
    uint32_t  getPid(){
        return pid;
    }

    void setPid(uint32_t pid);

};

#endif