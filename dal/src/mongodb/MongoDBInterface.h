//
// Created by Wesly Zhong on 2025/11/23.
//

#ifndef ATHENA_MONGODBINTERFACE_H
#define ATHENA_MONGODBINTERFACE_H
#include <mongocxx/instance.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/uri.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/exception/exception.hpp>
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
class MongoDBInterface {
public:
    int testInit();

};


#endif //ATHENA_MONGODBINTERFACE_H
