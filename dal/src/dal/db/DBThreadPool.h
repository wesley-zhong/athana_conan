#pragma once

#include <vector>
#include "core/actor/Actor.h"
#include "core/common/ObjectPool.hpp"

struct DBConfig {
    std::string device = "mysql"; // mysql or redis
    std::string ip = "local";
    unsigned int port = 3306;
    std::string dbname;
    std::string user = "root";
    std::string pswd;
};

class DB_Interface;
class RedisCommand;
class RedisResult;
class SqlPrepare;
class SqlResultSet;

// DB 任务基类：由 DB 线程执行 run()，完成后 complete() 触发回调
class DBTask {
public:
    DBTask();

    virtual ~DBTask();

    void dbi(DB_Interface *dbi);

    virtual void run() = 0;

protected:
    DB_Interface *_dbi;
    int _ret;
    int32 _errno = 0;
    std::string _error;
};

class DBSqlTask : public DBTask, public ObjPool::PoolObjClass<DBSqlTask> {
public:
    DBSqlTask(std::shared_ptr<SqlPrepare> pre, std::shared_ptr<SqlResultSet> result);

    ~DBSqlTask();

    virtual void run();

    virtual void complete();

public:
    std::function<void(int32, const char *, std::shared_ptr<SqlResultSet>)> backfunc;

private:
    std::shared_ptr<SqlPrepare> _pre;
    std::shared_ptr<SqlResultSet> _result;
};

class DBRedisTask : public DBTask {
public:
    DBRedisTask(std::shared_ptr<RedisCommand> command, std::shared_ptr<RedisResult> result);

    ~DBRedisTask();

    virtual void run();

    virtual void complete();

public:
    std::function<void(int32, const char *, std::shared_ptr<RedisResult>)> backfunc;

private:
    std::shared_ptr<RedisCommand> _command;
    std::shared_ptr<RedisResult> _result;
};

// DB 线程 actor：一个 actor 一个线程 + 一条 DB 连接，该线程上的任务串行执行
class DBThread : public actor::Actor {
public:
    explicit DBThread(const DBConfig &config);

    // 提交 DB 任务：闭包在本 DB 线程内注入自身 dbi 后执行
    void executeTask(DBTask *task) {
        execute([this, task] {
            task->dbi(m_db);
            task->run();
        });
    }

protected:
    void onStart() override;

    void onStop() override;

private:
    DBConfig _config;
    DB_Interface *m_db = nullptr;
};

// DB 线程池：N 个 DBThread，按 hash 路由任务
class DBThreadPool {
public:
    explicit DBThreadPool(DBConfig config);

    ~DBThreadPool();

    void create(int count);

    void exit();

    void executeTask(DBTask *task, int threadHashCode = 0);

    const DBConfig *getConfig();

private:
    DBConfig m_config;
    std::vector<DBThread *> _threads;
};
