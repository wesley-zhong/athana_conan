//
// Created by zhongweiqi on 2026/1/13.
//

#include "Discovery.h"

void Discovery::registerMyself(int myPort, int myType, std::string_view myName) {
    std::string key ="hello";
    std::string body = "body";
    client->keepAlive(key, body);
}

void Discovery::watchKeys(const std::vector<std::string> &keys,std::function<void(const std::string_view &, const std::string_view &)> watchKeysCB) {
    client->watchKeys(keys,watchKeysCB);

}