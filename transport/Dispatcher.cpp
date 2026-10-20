//
// Created by zhongweiqi on 2025/10/29.
//

#include "Dispatcher.h"

namespace transport {

// 消息分发只通过 findMsgFunction() 由各 handler 自行驱动；
// 原先这里有一个 processMsg()，内部用 msgMap[msgId] 查找，对未注册的
// msgId 会向 map 插入空指针（污染表 + 无锁单例下的数据竞争），且从未有调用方，已删除。

} // namespace transport
