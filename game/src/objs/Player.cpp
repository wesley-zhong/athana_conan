//
// Created by zhongweiqi on 2025/10/28.
//

#include "Player.h"
#include "player_modules/role_module/RoleModule.h"

#define  REGISTER_MODULE(CLASS ) \
   CLASS* module = new  CLASS(this); \
   moduleContainer->registerModule(module)\

void Player::sendMsg(int msgId, std::shared_ptr<google::protobuf::Message> msg) {
    channel->sendMsg(msgId, msg);
}

void Player::initModules() {
    REGISTER_MODULE(RoleModule);

}