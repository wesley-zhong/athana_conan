//
// Created by zhongweiqi on 2026/1/22.
//

#ifndef ATHENA_JSONUTILS_H
#define ATHENA_JSONUTILS_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

// RapidJSON 头文件
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "AthenaDiscovery.h"

class JsonUtils {
public:
    static std::string SerializeNodeInfo( NodeInfo *node);

    static bool DeserializeNodeInfo(const std::string &jsonStr, NodeInfo &node);

};


#endif //ATHENA_JSONUTILS_H
