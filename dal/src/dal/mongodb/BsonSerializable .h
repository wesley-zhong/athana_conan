//
// Created by zhongweiqi on 2025/12/18.
//

#ifndef ATHENA_BSONSERIALIZABLE_H
#define ATHENA_BSONSERIALIZABLE_H
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>

namespace dal
{
    class BsonSerializable
    {
    public:
        virtual bsoncxx::document::value toBson() const = 0;

        virtual void fromBson(bsoncxx::document::view v) = 0;
    };
} // namespace dal

#endif //ATHENA_BSONSERIALIZABLE_H
