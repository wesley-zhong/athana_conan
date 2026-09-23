//
// Created by zhongweiqi on 2026/9/23.
//

#ifndef ATHENA_GAMEACTOR_H
#define ATHENA_GAMEACTOR_H
#include <functional>
#include <vector>

#include "core/actor/ActorSystem.h"

enum ActorType
{
    LOGIC = 0,
    IO = 1,
    DB = 2,
    ACTOR_TYPE_COUNT
};

class GameActor
{
public:
    static void initActors(ActorType actorType, int count);
    static void execute(ActorType actorType, int hashCode, std::function<void()> func);

private:
    static std::vector<std::vector<core::actor::Actor*>> actors;
};


#endif //ATHENA_GAMEACTOR_H
