//
// Created by Wesly Zhong on 2025/11/14.
//

#ifndef ATHENA_ROLEDAO_H
#define ATHENA_ROLEDAO_H

#include "core/common/BaseType.h"
#include "mongodb/DAO.hpp"
#include "dos/RoleDO.hpp"
#include "mongodb/MongClientInstance.h"

class RoleDAO : public DAO<RoleDO> {
public:
    RoleDAO() : DAO("game", "role") {

    }
};


#endif //ATHENA_ROLEDAO_H
