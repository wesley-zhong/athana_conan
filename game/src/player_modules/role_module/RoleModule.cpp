//
// Created by Wesly Zhong on 2025/11/13.
//

#include "RoleModule.h"
#include "core/log/XLog.h"


void RoleModule::fromDO(RoleDO* dataObj)
{
    // there is no data from db
    INFO_LOG("pid = {} on fromDO", owner->getPid());
    if (dataObj == nullptr)
    {
    }
}

void RoleModule::onLogin()
{
    INFO_LOG("pid = {} on login", owner->getPid());
    markDirty();
}

void RoleModule::onLogout()
{
    INFO_LOG("pid = {} on logout", owner->getPid());
}
