//
// Created by Wesly Zhong on 2025/11/14.
//

#ifndef ATHENA_ROLEDAO_H
#define ATHENA_ROLEDAO_H

#include "dal/mongodb/MongodbDAO.hpp"
#include "dos/RoleDO.hpp"


class RoleDAO : public dal::MongodbDAO<RoleDO> {
public:
    RoleDAO() : MongodbDAO("game", "role") {
    }
};


#endif //ATHENA_ROLEDAO_H
