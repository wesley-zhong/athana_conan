//
// Created by zhongweiqi on 2026/9/3.
//

#ifndef ATHENA_QUESTDO_H
#define ATHENA_QUESTDO_H
#include <string>
#include <string_view>
#include <vector>
#include <bsoncxx/builder/basic/document.hpp>

#include "dal/mongodb/BsonSerializable .h"
#include "dal/redis/RedisSerializable.h"
#include "common/BaseType.h"

class QuestDO : public dal::BsonSerializable, public dal::RedisSerializable
{
public:
    int64_t _id;
    std::vector<int64_t> questIdsFinished;
    std::vector<int64_t> curQuestIds;

    bsoncxx::document::value toBson() const;

    void fromBson(bsoncxx::document::view v);

    std::string toString() const override;

    void fromString(std::string_view sv) override;
};
#endif //ATHENA_QUESTDO_H
