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
    channel->sendMsg(msgId, msg);
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
