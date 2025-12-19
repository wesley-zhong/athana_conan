//
// Created by Wesly Zhong on 2025/11/27.
//

#ifndef ATHENA_MONGCLIENTINSTANCE_H
#define ATHENA_MONGCLIENTINSTANCE_H

#include <bsoncxx/builder/basic/document.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/pool.hpp>

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;

class MongClientManager {
public:
    static int init(const std::string &ip, const std::string &userName, const std::string &password);

    static mongocxx::pool::entry getClient();

    static mongocxx::pool *pool;
};


#endif //ATHENA_MONGCLIENTINSTANCE_H
