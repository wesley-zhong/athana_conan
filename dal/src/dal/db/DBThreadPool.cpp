#include <string>
#include "core/common/BaseType.h"
#include "mysql.h"
#include "DB_Interface_mysql.h"
#include "hiredis/hiredis.h"
#include "../redis/DB_Interface_redis.h"
#include "SqlPrepare.h"
#include <functional>
#include "DBThreadPool.h"

namespace dal
{
    DBTask::DBTask()
    {
    }

    DBTask::~DBTask()
    {
    }

    void DBTask::dbi(DB_Interface* dbi)
    {
        _dbi = dbi;
    }

    //------------------------------------------------

    DBSqlTask::DBSqlTask(std::shared_ptr<SqlPrepare> pre, std::shared_ptr<SqlResultSet> result) :
        _pre(pre), _result(result)
    {
    }

    DBSqlTask::~DBSqlTask()
    {
    }

    void DBSqlTask::run()
    {
        _ret = _pre->prepare(static_cast<DBInterfaceMysql*>(_dbi)->mysql());
        DBResult* result = (DBResult*)(_result.get());
        if (_ret >= 0) _ret = _pre->execute(result);
        if (_ret < 0)
        {
            _errno = _dbi->getErrno();
            _error = _dbi->getError();
        }
        //recycle(this);
    }

    void DBSqlTask::complete()
    {
        const char* str = NULL;
        if (_ret < 0) str = _error.c_str();
        if (backfunc)
        {
            backfunc(_errno, str, _result);
            backfunc = nullptr;
        }
    }

    //------------------------------------------------

    DBRedisTask::DBRedisTask(std::shared_ptr<RedisCommand> command, std::shared_ptr<RedisResult> result)
    {
        _command = command;
        _result = result;
    }

    DBRedisTask::~DBRedisTask()
    {
    }

    void DBRedisTask::run()
    {
        DBResult* result = (DBResult*)(_result.get());
        _ret = static_cast<DBInterfaceRedis*>(_dbi)->execute(_command.get(), result);
        if (_ret < 0)
        {
            _errno = _dbi->getErrno();
            _error = _dbi->getError();
        }
    }

    void DBRedisTask::complete()
    {
        const char* str = NULL;
        if (_ret < 0) str = _error.c_str();
        if (backfunc)
        {
            backfunc(_errno, str, _result);
            backfunc = nullptr;
        }
    }

    //------------------------------------------------

    DBThread::DBThread(const DBConfig& config) : Actor("db-thread"), _config(config)
    {
    }

    void DBThread::onStart()
    {
        if (_config.device == "redis")
        {
            m_db = new DBInterfaceRedis(_config.ip.c_str(), _config.port, _config.dbname.c_str(), _config.user.c_str(),
                                        _config.pswd.c_str());
        }
        else
        {
            m_db = new DBInterfaceMysql(_config.ip.c_str(), _config.port, _config.dbname.c_str(), _config.user.c_str(),
                                        _config.pswd.c_str());
        }
        if (!m_db->connect())
        {
            ERR_LOG("DBThread connect {}:{} failed", _config.ip, _config.port);
        }
    }

    void DBThread::onStop()
    {
        if (m_db)
        {
            delete m_db;
            m_db = nullptr;
        }
    }

    //------------------------------------------------

    DBThreadPool::DBThreadPool(DBConfig config)
    {
        m_config = config;
    }

    DBThreadPool::~DBThreadPool()
    {
        exit();
    }

    void DBThreadPool::create(int count)
    {
        for (int i = 0; i < count; ++i)
        {
            auto* t = new DBThread(m_config);
            if (t->start())
            {
                _threads.push_back(t);
            }
            else
            {
                delete t;
            }
        }
    }

    void DBThreadPool::exit()
    {
        for (DBThread* t : _threads)
        {
            t->stop();
            delete t;
        }
        _threads.clear();
    }

    void DBThreadPool::executeTask(DBTask* task, int threadHashCode)
    {
        if (task == nullptr || _threads.empty())
        {
            return;
        }
        _threads[threadHashCode % (int)_threads.size()]->executeTask(task);
    }

    const DBConfig* DBThreadPool::getConfig()
    {
        return &m_config;
    }
} // namespace dal
