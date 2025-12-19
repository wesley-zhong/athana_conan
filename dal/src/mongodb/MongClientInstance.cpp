//
// Created by Wesly Zhong on 2025/11/27.
//

#include <fmt/format.h>
#include "MongClientInstance.h"

#include <mongocxx/instance.hpp>

MongClientInstance *instance;
mongocxx::instance mongoXXinstance{};
MongClientInstance *MongClientInstance::getInstance() {
    return instance;
}


int MongClientInstance::init(const std::string &ip, const std::string &userName, const std::string &password) {

    instance = new MongClientInstance();
    std::string s = fmt::format("mongodb://{}:{}@{}/?authSource=admin&connectTimeoutMS=2000&serverSelectionTimeoutMS=2000", userName, password, ip);
    mongocxx::uri uri(s);
    instance->client = mongocxx::client(uri);
    return 0;
}

mongocxx::collection MongClientInstance::getCollection(const std::string &dbName, const std::string &collectionName) {
    // if(client->list_database_names())

    instance->db = instance->client[dbName];
    return instance->db[collectionName];
}
