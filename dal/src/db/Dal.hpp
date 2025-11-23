//
// Created by zhongweiqi on 2025/10/18.
//
#pragma once
#include <string>
#include "MysqlResult.h"
#include  "RedisResult.h"
#include "DB_Interface_mysql.h"
#include "hiredis/hiredis.h"
#include "DB_Interface_redis.h"


namespace Dal {
    template<typename T>
    T *initDB(const char *host, unsigned int port, const char *dbname, const char *user, const char *pswd
    ) {
        T *obj = new T(host, port, dbname, user, pswd);
        obj->connect();
        return obj;
    }

    namespace DB {
        extern DBInterfaceMysql *mysql;

        void init(const std::string &ip, unsigned int port, const std::string &dbname, const std::string &username,
                  const std::string &password);

        //  template<typename T_KEY, typename T_VALUE>  Todo  this should support string and long ,int  as key
        int execute(DBResult *result, const std::string &cmd);
    }

    namespace Cache {
        extern DBInterfaceRedis *redis;

        void init(const std::string &ip, unsigned int port, const std::string &dbname, const std::string &username,
                  const std::string &password);

        //  template<typename T_KEY, typename T_VALUE>  Todo  this should support string and long ,int  as key
        int execute(DBResult *result, const std::string &cmd);
    }
    namespace MongoDB{

        }
}
