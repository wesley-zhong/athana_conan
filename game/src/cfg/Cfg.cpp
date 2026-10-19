//
// Created by zhongweiqi on 2026/10/15.
//

#include "Cfg.h"

void Cfg::init(const std::string& filePath)
{
     Tables = new cfg::Tables();
    if (Tables->load([filePath](::luban::ByteBuf& buf, const std::string& s) { return buf.loadFromFile(filePath + s + ".json"); }))
    {
        std::cout << "== load succ == " << std::endl;
    }
    else
    {
        std::cout << "== load fail == " << std::endl;
    }
}

  cfg::Tables*  Cfg::Tables = nullptr;