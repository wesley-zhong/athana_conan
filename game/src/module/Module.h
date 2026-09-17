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
protected:
    DO* dataDO_;
    DAO<DO> dao_;

public:
    Module(Player* player, DAO<DO>& dao) : BaseModule(player), dataDO_(nullptr), dao_(dao)
    {
    }

    virtual ~Module()
    {
        delete dataDO_;
    }

    virtual void fromDO(DO* pDO) = 0;

    void loadDataFromDB() override
    {
        auto ret = dao_.find_one(owner->getPid());
        if (!ret)
        {
            // 没有库记录, 由子类创建默认 DO
            fromDO(nullptr);
            return;
        }
        dataDO_ = new DO(ret.value());
        fromDO(dataDO_);
    }
    void saveDataToDB() override
    {
        if (!is_dirty || dataDO_ == nullptr)
        {
            return;
        }
        dao_.update(*dataDO_);
    }
};


#endif //ATHENA_MODULE_H
