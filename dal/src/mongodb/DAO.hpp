//
// Created by Wesly Zhong on 2025/11/27.
//

#ifndef ATHENA_DAO_H
#define ATHENA_DAO_H
#include <bsoncxx/builder/basic/document.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/exception/exception.hpp>
#include "MongClientManager.h"
#include <optional>
#include "core/log/XLog.h"
#include "core/common/BaseType.h"

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


    std::optional<DO_T> find_one(int64 id) {
        auto client = MongClientManager::getClient();
        return this->find_oneImp(client, id);
    }

    bool update(const DO_T &obj) {
        auto client = MongClientManager::getClient();
        return update(client, obj);
    }

    void bulk_update(DO_T &dos ...) {
    }

    void bulk_update(const std::vector<DO_T &> dos) {
        // collection.update_many(make_document(kvp("i", make_document(kvp("$gt", 0)))),
        //                   make_document(kvp("$set", make_document(kvp("foo", "buzz")))));
        // tbl_coll.update_many()
    }

private:
    std::optional<DO_T> find_oneImp(mongocxx::pool::entry &client, int64 id) {
        auto find_one_result = (*client)[dbName][tableName].find_one(make_document(kvp("_id", id)));
        if (!find_one_result) {
            return std::nullopt;
        }
        DO_T doObj;
        doObj.fromBson(find_one_result->view());
        return doObj;
    }


    bool update(mongocxx::pool::entry &client, const DO_T &obj) {
        try {
            mongocxx::options::replace opts;
            opts.upsert(true);

            (*client)[dbName][tableName].replace_one(
                make_document(kvp("_id", obj._id)),
                obj.toBson(),
                opts
            );
            return true;
        } catch (const mongocxx::exception &e) {
            ERR_LOG("Mongo update failed, id={}, err={}", obj._id, e.what());
            return false;
        }
    }

    std::string dbName;
    std::string tableName;
};

#endif //ATHENA_DAO_H
