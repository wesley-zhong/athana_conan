//
// Created by Wesly Zhong on 2025/11/27.
//

#ifndef ATHENA_MONGODBDAO_H
#define ATHENA_MONGODBDAO_H

#include <bsoncxx/builder/basic/document.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/bulk_write.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/model/replace_one.hpp>
#include <mongocxx/result/bulk_write.hpp>
#include "MongClientManager.h"
#include <optional>
#include <vector>
#include "log/XLog.h"
#include "common/BaseType.h"

using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_array;
using bsoncxx::builder::basic::make_document;

namespace dal
{
    template <typename DO_T>
    class MongodbDAO
    {
    public:
        // find_one 结果状态：区分"未找到"与"访问出错"。DB_ERROR 时调用方不应走
        // "新建默认数据再 upsert"路径，否则会用新档覆盖已有数据
        enum class Status { OK, NOT_FOUND, DB_ERROR };

        MongodbDAO(std::string dbName, std::string table) : dbName(std::move(dbName)), tableName(std::move(table))
        {
        }


        // 便捷版：未找到或出错都返回 nullopt（出错有 ERR_LOG），不区分两者
        std::optional<DO_T> find_one(int64 id)
        {
            std::optional<DO_T> out;
            find_one(id, out);
            return out;
        }

        // 状态版：仅 OK 时 out 有值；NOT_FOUND 与 DB_ERROR 需要区分处理时使用
        Status find_one(int64 id, std::optional<DO_T>& out)
        {
            auto client = MongClientManager::getClient();
            return find_oneImp(client, id, out);
        }

        bool update(const DO_T& obj)
        {
            auto client = MongClientManager::getClient();
            return update(client, obj);
        }

        // 按 _id 逐条 replace + upsert，一次 bulk_write 提交（默认 ordered，
        // 任一条失败即停止，此前已写入的条目不回滚）
        bool bulk_update(const std::vector<DO_T>& dos)
        {
            if (dos.empty())
            {
                return true;
            }
            try
            {
                auto client = MongClientManager::getClient();
                auto coll = (*client)[dbName][tableName];
                mongocxx::bulk_write bulk = coll.create_bulk_write();

                // model 只持有 view，底层 document::value 必须活到 execute() 之后；
                // 先 reserve 防止 vector 扩容搬移使已 append 的 view 失效
                std::vector<bsoncxx::document::value> filters;
                std::vector<bsoncxx::document::value> replacements;
                filters.reserve(dos.size());
                replacements.reserve(dos.size());

                for (const auto& obj : dos)
                {
                    filters.emplace_back(make_document(kvp("_id", obj._id)));
                    replacements.emplace_back(obj.toBson());
                    mongocxx::model::replace_one op(filters.back().view(), replacements.back().view());
                    op.upsert(true);
                    bulk.append(op);
                }

                // 返回值仅在写入被 acknowledged 时才有值
                auto result = bulk.execute();
                if (!result)
                {
                    ERR_LOG("Mongo bulk_update not acknowledged, db={}, table={}, count={}",
                            dbName, tableName, dos.size());
                    return false;
                }
                INFO_LOG("Mongo bulk_update ok, db={}, table={}, count={}, upserted={}",
                         dbName, tableName, dos.size(), result->upserted_count());
                return true;
            }
            catch (const std::exception& e)
            {
                ERR_LOG("Mongo bulk_update failed, db={}, table={}, count={}, err={}",
                        dbName, tableName, dos.size(), e.what());
                return false;
            }
        }

    private:
        Status find_oneImp(mongocxx::pool::entry& client, int64 id, std::optional<DO_T>& out)
        {
            try
            {
                auto find_one_result = (*client)[dbName][tableName].find_one(make_document(kvp("_id", id)));
                if (!find_one_result)
                {
                    return Status::NOT_FOUND;
                }
                DO_T doObj;
                doObj.fromBson(find_one_result->view());
                out = std::move(doObj);
                return Status::OK;
            }
            catch (const std::exception& e)
            {
                // fromBson/toBson 抛的 bsoncxx::exception 不派生自 mongocxx::exception，
                // 必须按基类 std::exception 捕获
                ERR_LOG("Mongo find_one failed, db={}, table={}, id={}, err={}",
                        dbName, tableName, id, e.what());
                return Status::DB_ERROR;
            }
        }


        bool update(mongocxx::pool::entry& client, const DO_T& obj)
        {
            try
            {
                mongocxx::options::replace opts;
                opts.upsert(true);

                // 返回值仅在写入被 acknowledged 时才有值
                auto result = (*client)[dbName][tableName].replace_one(
                    make_document(kvp("_id", obj._id)),
                    obj.toBson(),
                    opts
                );
                if (!result)
                {
                    ERR_LOG("Mongo update not acknowledged, db={}, table={}, id={}",
                            dbName, tableName, obj._id);
                    return false;
                }
                INFO_LOG("Mongo update ok, db={}, table={}, id={}, matched={}, modified={}",
                         dbName, tableName, obj._id,
                         result->matched_count(), result->modified_count());
                return true;
            }
            catch (const std::exception& e)
            {
                ERR_LOG("Mongo update failed, id={}, err={}", obj._id, e.what());
                return false;
            }
        }

        std::string dbName;
        std::string tableName;
    };
} // namespace dal

#endif //ATHENA_MONGODBDAO_H
