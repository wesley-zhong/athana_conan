//
// Created by zhongweiqi on 2026/10/15.
//

#ifndef ATHENA_CFG_H
#define ATHENA_CFG_H
#include <string>
#include "schema.h"

class Cfg
{
public:
    static cfg::Tables* Tables;
    static int init(const std::string& filePath);
};


#endif //ATHENA_CFG_H
