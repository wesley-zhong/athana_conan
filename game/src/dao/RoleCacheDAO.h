//
// Created by Wesly Zhong on 2026/9/18.
//

#ifndef ATHENA_ROLECACHEDAO_H
#define ATHENA_ROLECACHEDAO_H

#include "dal/redis/RedisDAO.hpp"
#include "dos/RoleDO.hpp"


class RoleCacheDAO : public dal::RedisDAO<RoleDO> {
public:
    RoleCacheDAO() : RedisDAO("role") {
    }
};


#endif //ATHENA_ROLECACHEDAO_H
