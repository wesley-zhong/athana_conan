//
// Created by Wesly Zhong on 2025/11/13.
//

#ifndef ATHENA_MODULE_H
#define ATHENA_MODULE_H

#include "BaseModule.h"

class Player;

class BaseDAO;

template<typename DO>
class Module : public BaseModule {
public:
    Module(Player *player, BaseDAO *baseDAO) : BaseModule(player, baseDAO) {
    }

    void initFromDB() {
    };

    virtual void fromDO(DO *pDO) = 0;

};


#endif //ATHENA_MODULE_H
