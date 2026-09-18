//
// Created by Wesly Zhong on 2025/11/27.
//

#ifndef ATHENA_ROLEDO_H
#define ATHENA_ROLEDO_H


#include <string>
#include <string_view>
#include <bsoncxx/builder/basic/document.hpp>

#include "dal/mongodb/BsonSerializable .h"
#include "dal/redis/RedisSerializable.h"
#include "common/BaseType.h"
class RoleDO : public dal::BsonSerializable, public dal::RedisSerializable {
public:
    int64_t _id;
    std::string name;

    bsoncxx::document::value toBson() const;

    void fromBson(bsoncxx::document::view v);

    std::string toString() const override;

    void fromString(std::string_view sv) override;
};


#endif //ATHENA_ROLEDO_H
