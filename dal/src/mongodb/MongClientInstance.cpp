//
// Created by Wesly Zhong on 2025/11/27.
//

#include <fmt/format.h>
#include "MongClientInstance.h"

int MongClientInstance::init(const std::string &ip, const std::string &userName, const std::string &password) {
    std::string s = fmt::format("mongodb://{}:{}@{}/?authSource=admin", userName, password, ip);
    mongocxx::uri uri(s);
    client = new mongocxx::client(uri);
    return 0;
}

mongocxx::collection MongClientInstance::getCollection(const std::string &dbName, const std::string &collectionName) {
   // if(client->list_database_names())
    db = ((*client)[dbName]);
    return db[collectionName];
}


mongocxx::client *MongClientInstance::client = nullptr;
mongocxx::database MongClientInstance::db =mongocxx::database();
