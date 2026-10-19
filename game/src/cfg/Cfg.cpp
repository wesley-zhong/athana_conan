//
// Created by zhongweiqi on 2026/10/15.
//

#include "Cfg.h"
#include "log/XLog.h"

int Cfg::init(const std::string& filePath)
{
    Tables = new cfg::Tables();
    if (Tables->load([filePath](::luban::ByteBuf& buf, const std::string& s)
    {
        bool ret = buf.loadFromFile(filePath + s + ".bytes");
        if (ret)
        {
            INFO_LOG("----- load:  {}     success:", filePath + s);
        }
        else
        {
            INFO_LOG("----- load: {}      failed", filePath + s);
        }
        return ret;
    }))
    {
        INFO_LOG("#########  LOAD SUCCESS ");
        return true;
    }

    INFO_LOG("== load fail == ");
    return false;
}

cfg::Tables* Cfg::Tables = nullptr;
