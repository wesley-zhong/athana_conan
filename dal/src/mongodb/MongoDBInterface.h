//
// Created by Wesly Zhong on 2025/11/23.
//

#ifndef ATHENA_MONGODBINTERFACE_H
#define ATHENA_MONGODBINTERFACE_H
#include <cstdint>
#include <iostream>
#include <vector>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;

class MongoDBInterface {
public:
    int testInit();

    template<typename R, typename KT>
    R *find_one(KT key);
};

template<typename R, typename KT>
R * MongoDBInterface::find_one(KT key) {
    R *result = new R();

    return result;
}

#endif //ATHENA_MONGODBINTERFACE_H
