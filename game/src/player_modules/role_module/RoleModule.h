//
// Created by Wesly Zhong on 2025/11/13.
//

#ifndef ATHENA_ROLEMODULE_H
#define ATHENA_ROLEMODULE_H

#include "module/Module.h"
#include "dos/RoleDO.hpp"
#include "dal/Dal.hpp"
#include "dao/RoleDAO.h"

class Player;


class RoleModule : public Module<RoleDO>
{
public:
    explicit RoleModule(Player* player) : Module(player, dao_), dao_()
    {
    }

    void fromDO(RoleDO* Do) override;

    void onLogin() override;

    void onLogout() override;

private:
    RoleDAO dao_;
};


#endif //ATHENA_ROLEMODULE_H
