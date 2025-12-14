//
// Created by Wesly Zhong on 2025/11/27.
//

#ifndef ATHENA_ROLEDO_H
#define ATHENA_ROLEDO_H

#include "BaseDO.h"
#include <string>

class RoleDO : public BaseDO<int64> {
public:
    std::string name;

    int parse(bsoncxx::document::value& value);

     std::string toByte();

};


#endif //ATHENA_ROLEDO_H
