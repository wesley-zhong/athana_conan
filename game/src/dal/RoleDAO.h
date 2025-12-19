//
// Created by Wesly Zhong on 2025/11/14.
//

#ifndef ATHENA_ROLEDAO_H
#define ATHENA_ROLEDAO_H

#include "mongodb/DAO.hpp"
#include "dos/RoleDO.hpp"


class RoleDAO : public DAO<RoleDO> {
public:
    RoleDAO() : DAO("game", "role") {
    }
};


#endif //ATHENA_ROLEDAO_H
