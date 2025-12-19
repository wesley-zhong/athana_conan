//
// Created by Wesly Zhong on 2025/11/27.
//

#ifndef ATHENA_MONGCLIENTINSTANCE_H
#define ATHENA_MONGCLIENTINSTANCE_H

#include <bsoncxx/builder/basic/document.hpp>
#include <mongocxx/client.hpp>

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;

class MongClientInstance {
public:
    static MongClientInstance *getInstance();

    static int init(const std::string &ip, const std::string &userName, const std::string &password);

    static mongocxx::collection getCollection(const std::string &dbName, const std::string &collectionName);


    mongocxx::client client;
    mongocxx::database db;
};


#endif //ATHENA_MONGCLIENTINSTANCE_H
