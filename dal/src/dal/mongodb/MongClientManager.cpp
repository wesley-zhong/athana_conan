//
// Created by Wesly Zhong on 2025/11/27.
//

#include <fmt/format.h>
#include "MongClientManager.h"

#include <mongocxx/instance.hpp>


static mongocxx::instance mongoXXinstance{};


int MongClientManager::init(const std::string &ip, const std::string &userName, const std::string &password) {
    std::string s = fmt::format(
        "mongodb://{}:{}@{}/?authSource=admin&connectTimeoutMS=2000&serverSelectionTimeoutMS=5000", userName, password,
        ip);
    mongocxx::uri uri(s);
    pool = new mongocxx::pool(uri);
    return 0;
}

mongocxx::pool::entry MongClientManager::getClient() {
    return pool->acquire();
}

mongocxx::pool *MongClientManager::pool = nullptr;
