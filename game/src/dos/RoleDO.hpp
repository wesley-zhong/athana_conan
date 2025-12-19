//
// Created by Wesly Zhong on 2025/11/27.
//

#ifndef ATHENA_ROLEDO_H
#define ATHENA_ROLEDO_H


#include <string>
#include <bsoncxx/builder/basic/document.hpp>

#include "mongodb/BsonSerializable .h"
#include "core/common/BaseType.h"
class RoleDO : public BsonSerializable {
public:
    int64 _id;
    std::string name;

    bsoncxx::document::value toBson() const;

    void fromBson(bsoncxx::document::view v);
};


#endif //ATHENA_ROLEDO_H
