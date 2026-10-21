//
// Created by zhongweiqi on 2025/10/28.
//

#include "Player.h"
#include "player_modules/role_module/RoleModule.h"

#define  REGISTER_MODULE(CLASS ) \
   CLASS* module = new  CLASS(this); \
   moduleContainer->registerModule(module)

void Player::sendMsg(int msgId, std::shared_ptr<google::protobuf::Message> msg)
{
    if (channel == nullptr)
    {
        return;
    }
    // 帧头 playerId：下游（router/网关）按它路由回这个玩家
    channel->sendMsg(msgId, msg, (int64_t)pid);
}

void Player::initModules()
{
    REGISTER_MODULE(RoleModule);
}

void Player::loadDataFormDB()
{
    moduleContainer->forEach([](BaseModule* module)
    {
        module->loadDataFromDB();
    });
}

void Player::saveDataToDB()
{
    moduleContainer->forEach([](BaseModule* module)
    {
        module->saveDataToDB();
    });
}


void Player::onLogin()
{
    hashCode = pid;
    moduleContainer->forEach([](BaseModule* module)
    {
        module->onLogin();
    });
}

void Player::onLogout()
{
    moduleContainer->forEach([](BaseModule* module)
    {
        module->onLogout();
    });
}
