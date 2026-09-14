//
// Created by zhongweiqi on 2026/9/3.
//

#ifndef ATHENA_QUESTDO_H
#define ATHENA_QUESTDO_H
#include <bsoncxx/builder/basic/document.hpp>

#include "mongodb/BsonSerializable .h"
#include "core/common/BaseType.h"

class QuestDO : public BsonSerializable
{
public:
    int64_t _id;
    std::vector<int64_t> questIdsFinished;
    std::vector<int64_t> curQuestIds;

    bsoncxx::document::value toBson() const;

    void fromBson(bsoncxx::document::view v);
};
#endif //ATHENA_QUESTDO_H
