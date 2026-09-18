//
// Created by zhongweiqi on 2025/11/27.
//

#ifndef ATHENA_MONGODB_H
#define ATHENA_MONGODB_H
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>

namespace dal
{
    namespace MongDB
    {
        template <typename T>
        mongocxx::collection& getCollection()
        {
            static core::ObjectPool<T> s_pool(0, 1024);
            return s_pool;
        }
    }
} // namespace dal


#endif //ATHENA_MONGODB_H
