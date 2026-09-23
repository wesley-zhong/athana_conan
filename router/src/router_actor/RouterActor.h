//
// Created by zhongweiqi on 2026/9/23.
//

#ifndef ATHENA_ROUTERACTOR_H
#define ATHENA_ROUTERACTOR_H
#include <functional>
#include <vector>

#include "core/actor/ActorSystem.h"

enum RouterActorType
{
    LOGIC = 0,
    IO = 1,
    DB = 2,
    ACTOR_TYPE_COUNT
};

class RouterActor
{
public:
    static void initActors(RouterActorType actorType, int count);
    static void execute(RouterActorType actorType, int hashCode, std::function<void()> func);

private:
    static std::vector<std::vector<core::actor::Actor*>> actors;
};


#endif //ATHENA_ROUTERACTOR_H
