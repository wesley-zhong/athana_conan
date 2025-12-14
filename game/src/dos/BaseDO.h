//
// Created by Wesly Zhong on 2025/11/27.
//

#ifndef ATHENA_BASEDO_H
#define ATHENA_BASEDO_H

#include "core/common/BaseType.h"
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

template<typename ID_T>
class BaseDO {
public:
    ID_T _id;
    int64 _ver;

    virtual int parse(bsoncxx::document::value& value) =0;

    virtual std::string toByte() = 0;
};


#endif //ATHENA_BASEDO_H
