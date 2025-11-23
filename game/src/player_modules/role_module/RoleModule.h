//
// Created by Wesly Zhong on 2025/11/13.
//

#ifndef ATHENA_ROLEMODULE_H
#define ATHENA_ROLEMODULE_H

#include "module/Module.h"
#include "RoleDO.h"
#include "dal/BaseDAO.h"
class Player;


class RoleModule : public Module<RoleDO> {
public:
    explicit RoleModule(Player *player) : Module<RoleDO>(player, new BaseDAO()) {

    }

    void fromDO(RoleDO *Do) override;

    void onLogin() override;

    void onAfterLogin() override;

};


#endif //ATHENA_ROLEMODULE_H
