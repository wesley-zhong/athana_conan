//
// Created by Wesly Zhong on 2025/11/27.
//

#ifndef ATHENA_DAO_H
#define ATHENA_DAO_H
#include <bsoncxx/builder/basic/document.hpp>
#include <mongocxx/client.hpp>
#include "MongClientInstance.h"
#include <optional>


using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;

template<typename DO_T>
class DAO {
public:
    DAO(std::string dbName, std::string table) {
        this->dbName = dbName;
        this->tableName = table;
    }

    int init() {
        tbl_coll = MongClientInstance::getCollection(dbName, tableName);
        return 0;
    }

    std::optional<DO_T> find_one(int64 id) {
        auto find_one_result = tbl_coll.find_one(make_document(kvp("_id", id)));
        if (!find_one_result) {
            return std::nullopt;
        }
        DO_T doObj;
        doObj.fromBson(find_one_result->view());
        return doObj;
    }

    std::optional<DO_T> *find_one(int32 id) {
        return find_one(int64(id));
    }

    int update(DO_T &obj) {
        mongocxx::options::find_one_and_replace opts;
        opts.upsert(true);
        auto ret = tbl_coll.find_one_and_replace(make_document(kvp("_id", obj._id)), obj.toBson(), opts);
        if (ret) {
            ret.value();
        }
        return 0;
    }

private:
    mongocxx::collection tbl_coll;
    std::string dbName;
    std::string tableName;
};

#endif //ATHENA_DAO_H
