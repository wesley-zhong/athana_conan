//
// Created by zhongweiqi on 2025/12/19.
//

#ifndef ATHENA_DAL_H
#define ATHENA_DAL_H
#include "mongodb/DAO.hpp"
#include <optional>

namespace Dal {
    template<typename T>
    T &DAO() {
        static T dao;
        return dao;
    }
}

#endif //ATHENA_DAL_H
