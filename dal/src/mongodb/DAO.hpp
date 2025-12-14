//
// Created by Wesly Zhong on 2025/11/27.
//

#ifndef ATHENA_DAO_H
#define ATHENA_DAO_H

#include <cstdint>
#include <iostream>
#include <vector>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include "MongClientInstance.h"


using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;

template<typename DO_T, typename ID_T>
class DAO {
public:
    DAO(std::string dbName, std::string table) {
        this->dbName = dbName;
        this->tableName = table;
    }

    int init() {
        tbl_coll = MongClientInstance::getCollection(dbName, tableName);
    }

    DO_T *find_one(ID_T id) {
        auto find_one_result = tbl_coll.find_one(make_document(kvp("_id", id)));
        if (!find_one_result) {
            return nullptr;
        }
        DO_T *pDO = new DO_T();
        pDO->parse(find_one_result.view());
        return pDO;
    }

    int update(DO_T *obj) {
        tbl_coll.find_one_and_replace(make_document(kvp("_id", obj->_id)), obj);
        return 0;
    }

private:
    mongocxx::collection tbl_coll;
    std::string dbName;
    std::string tableName;
};

#endif //ATHENA_DAO_H
