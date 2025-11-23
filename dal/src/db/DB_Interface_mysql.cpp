#include <string>
#include <cstring>
#include "mysql.h"
#include "DB_Interface_mysql.h"
#include "core/log/XLog.h"
#include "MysqlResult.h"
#include "SqlPrepare.h"

DBInterfaceMysql::DBInterfaceMysql(const char *host, unsigned int port, const char *dbname, const char *user,
                                   const char *pswd
) : DB_Interface(host, port) {
    m_dbname = dbname;
    m_user = user;
    m_pswd = pswd;

    if (!mysql_init(&mMysql_)) {
        ERR_LOG("mysql init error!!");
    }
}

DBInterfaceMysql::~DBInterfaceMysql() {
    detach();
}

bool DBInterfaceMysql::connect() {
    if (mysql_options(&mMysql_, MYSQL_SET_CHARSET_NAME, "utf8")) {
        ERR_LOG("mysql_options(MYSQL_SET_CHARSET_NAME) Errno:{} failed: {}", getErrno(), getError());
        return false;
    }
    // 关闭 SSL（兼容老版本客户端）
    mysql_ssl_set(&mMysql_, NULL, NULL, NULL, NULL, NULL);
    my_bool reconnect = 1;
    if (mysql_options(&mMysql_, MYSQL_OPT_RECONNECT, &reconnect)) {
        ERR_LOG("mysql_options(MYSQL_OPT_RECONNECT) Errno:{} failed: {}", getErrno(), getError());
        return false;
    }

    if (!mysql_real_connect(&mMysql_, m_ip.c_str(), m_user.c_str(), m_pswd.c_str(), m_dbname.c_str(), m_port, NULL,
                            0)) {
        ERR_LOG("mysql_real_connect Errno:{} error: {}  client info ={}  version ={}", getErrno(), getError(), mysql_get_client_info(), mysql_get_client_version());
        return false;
    }

    return true;
}

bool DBInterfaceMysql::detach() {
    ::mysql_close(&mMysql_);
    return true;
}

int DBInterfaceMysql::execute(DBResult *result, const char *cmd, int len) {
    int nResult = mysql_real_query(&mMysql_, cmd, (len <= 0 ? strlen(cmd) : len));
    if (nResult != 0) {
        ERR_LOG("mysql_real_query Errno:{} error: {}", getErrno(), getError());
        return -1;
    }

    MYSQL_RES *pResult = mysql_store_result(&mMysql_);


    if (pResult) {
        uint32 nrows = (uint32) mysql_num_rows(pResult);
        uint32 nfields = (uint32) mysql_num_fields(pResult);

        MYSQL_ROW arow;
        while ((arow = mysql_fetch_row(pResult)) != NULL) {
            unsigned long *lengths = mysql_fetch_lengths(pResult);

            for (uint32 i = 0; i < nfields; ++i) {
                // arow[i], lengths[i]
            }
        }

        mysql_free_result(pResult);
    } else {
    }


    static_cast<MysqlResult *>(result)->setResult(pResult);

    return 0;
}

const char *DBInterfaceMysql::getError() {
    return mysql_error(&mMysql_);
}

int DBInterfaceMysql::getErrno() {
    return mysql_errno(&mMysql_);
}

bool DBInterfaceMysql::ping() {
    return mysql_ping(&mMysql_) == 0;
}

MYSQL *DBInterfaceMysql::mysql() {
    return &mMysql_;
}


namespace MySQL {
    int threadSafe() {
        return mysql_thread_safe();
    }

    void libraryInit() {
        mysql_library_init(-1, nullptr, nullptr);
    }

    void libraryEnd() {
        mysql_library_end();
    }

    char const *getLibraryVersion() {
        return MYSQL_SERVER_VERSION;
    }
};
