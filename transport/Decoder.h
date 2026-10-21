//
// Created by zhongweiqi on 2025/10/23.
//

#ifndef ATHENA_DECODER_H
#define ATHENA_DECODER_H

#include <memory>
#include "Dispatcher.h"
#include "EventLoop.h"

namespace transport {

// 两种帧格式，按链路区分（Channel::proto_mode，EventLoop 切帧时已按各自格式校验）：
// - INNER  服务器间：| 4B msgLen | 4B msgId | 4B playerId | 4B magicNum | body |
// - CLIENT 对外客户端：| 4B msgLen | 4B msgId | 4B magicNum | body |（无 playerId）
// msgLen 语义一致：除自身 4B 外的头 + body 长度。onMsg 的 buff 指向完整帧起点，
// len 为整帧长度。这里集中原先各 handler 重复的“拆帧头 → 查注册表 → 反序列化 → 取 channel
// shared_ptr”流程，handler 只保留各自的业务分支（心跳应答/网关转发/actor 派发）
enum class DecodeResult {
    OK,              // 成功：msg/channel_ptr 可用，必须最终交给 invoke（invoke 负责归还对象池）
    BAD_HEADER,      // 帧长不足头长(16B/12B)、入参为空或魔数不匹配
    NOT_REGISTERED,  // msgId 未注册（网关据此把消息转发给 game）
    PARSE_FAILED,    // 反序列化失败，已记日志
    CHANNEL_GONE,    // 连接已关闭回收，消息无处投递
};

struct DecodedMsg {
    int msgId = 0;
    char* body = nullptr;  // 指向帧内 body 起点，仅本次 onMsg 内有效（loop 线程共享 scratch）
    int bodyLen = 0;
    // 帧头 playerId（有符号拓宽到 int64）。INNER 链路取自帧头，登录前的握手/心跳等
    // 无归属消息为 0；CLIENT 链路帧头无此字段，恒为 0（归属信息走 proto body）
    int64_t playerId = 0;
    MsgFunction* msg_function = nullptr;
    void* msg = nullptr;  // 池化对象，OK 后必须 invoke，由 invoke 归还对象池
    std::shared_ptr<Channel> channel_ptr;
};

class Decoder {
public:
    // 按连接的链路协议拆帧头（跳过 len、取 msgId/playerId、校验魔数、算出 body 指针/长度）。
    // 帧长不足或魔数不匹配时 body 置空、msgId 置 0，由 decodeMsg 报 BAD_HEADER
    static void parseHeader(Channel* channel, void* buff, int len, DecodedMsg* out);

    // parseHeader 之后的完整解析：查注册表 → 取 channel_ptr → 反序列化，
    // 成功时补齐 out 其余字段（先取 channel_ptr 再 parse，连接刚断开时不必白解析）
    static DecodeResult decodeMsg(Channel* channel, DecodedMsg* out);
};


} // namespace transport

#endif //ATHENA_DECODER_H
