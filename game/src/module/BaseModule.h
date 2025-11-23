//
// Created by Wesly Zhong on 2025/11/15.
//

#ifndef ATHENA_BASEMODULE_H
#define ATHENA_BASEMODULE_H

#include "dal/BaseDAO.h"

class Player;

class DO;

class BaseModule {
public:
    BaseModule(Player *player, BaseDAO *dao) {
        this->player = player;
        this->baseDao = dao;
    }

    virtual void initFromDB() = 0;

    virtual void onLogin() = 0;

    virtual void onAfterLogin() = 0;

protected:
    DO *loadDataFromDB() {
        // return baseDao.findOne
        return nullptr;
    }

protected:
    Player *player;
    BaseDAO *baseDao;
};


#endif //ATHENA_BASEMODULE_H
