//
// Created by Wesly Zhong on 2025/11/13.
//

#ifndef ATHENA_ROLEMODULE_H
#define ATHENA_ROLEMODULE_H

#include "module/Module.h"
#include "dos/RoleDO.hpp"
#include "dao/Dal.hpp"
#include "dao/RoleDAO.h"

class Player;


class RoleModule : public Module<RoleDO>
{
public:
    explicit RoleModule(Player* player) : Module(player, Dal::DAO<RoleDAO>())
    {
    }

    void fromDO(RoleDO* Do) override;

    void onLogin() override;

    void onLogout() override;
};


#endif //ATHENA_ROLEMODULE_H
