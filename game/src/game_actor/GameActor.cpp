//
// Created by zhongweiqi on 2026/9/23.
//

#include "GameActor.h"


std::vector<std::vector<core::actor::Actor*>> GameActor::actors(ACTOR_TYPE_COUNT);

static const char* actorTypeName(ActorType actorType)
{
    switch (actorType)
    {
        case LOGIC: return "logic";
        case IO:    return "io";
        case DB:    return "db";
        default:    return "unknown";
    }
}


void GameActor::initActors(ActorType actorType, int count)
{
    auto& sys = core::actor::ActorSystem::instance();
    for (int i = 0; i < count; ++i)
    {
        if (core::actor::Actor* a = sys.spawn<core::actor::Actor>(std::string(actorTypeName(actorType)) + "-" + std::to_string(i)))
        {
            actors[(int)actorType].push_back(a);
        }
    }
}


void GameActor::execute(ActorType actorType, int hashCode, std::function<void()> func)
{
    auto& bucket = actors[(int)actorType];
    if (bucket.empty())
    {
        ERR_LOG("actor type [{}] not initialized, drop task", actorTypeName(actorType));
        return;
    }
    bucket[static_cast<size_t>(hashCode) % bucket.size()]->execute(std::move(func));
}
