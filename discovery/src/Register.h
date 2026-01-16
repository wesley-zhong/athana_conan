//
// Created by zhongweiqi on 2026/1/16.
//

#ifndef ATHENA_REGISTER_H
#define ATHENA_REGISTER_H
#include<string>

class Register {
    public:
    void registerMySelf( std::string_view service_id,  std::string_view serverInfo);
};


#endif //ATHENA_REGISTER_H