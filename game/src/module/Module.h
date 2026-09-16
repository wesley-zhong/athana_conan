//
// Created by Wesly Zhong on 2025/11/13.
//

#ifndef ATHENA_MODULE_H
#define ATHENA_MODULE_H

#include "BaseModule.h"
#include "dal/mongodb/DAO.hpp"
#include "gateway/src/objs/Player.h"

class BaseDAO;

template <typename DO>
class Module : public BaseModule
{
private:
    DO dataDO;
    DAO<DO> dao_;

public:
    Module(Player* player, DAO<DO>& dao) : BaseModule(player), dao_(dao)
    {
    }

    virtual void fromDO(DO* pDO) = 0;

    void loadDataFromDB() override
    {
        auto ret = dao_.find_one(owner->getPid());
        if (!ret)
        {
            fromDO(nullptr);
            return;
        }
        dataDO = ret.value();
        fromDO(&dataDO);
    }
    void saveDataToDB() override
    {
        if (!is_dirty)
        {
            return;
        }
        dao_.update(dataDO);
    }
};


#endif //ATHENA_MODULE_H
