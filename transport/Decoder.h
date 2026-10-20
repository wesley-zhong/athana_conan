//
// Created by zhongweiqi on 2025/10/23.
//

#ifndef ATHENA_DECODER_H
#define ATHENA_DECODER_H

#include <memory>
#include "Dispatcher.h"
#include "EventLoop.h"

namespace transport {

// 帧格式：| 4B packLen | 4B msgId | body |。onMsg 的 buff 指向完整帧起点，
// len 为整帧长度（EventLoop 已按 packLen 切好帧）。
// 这里集中原先各 handler 重复的“拆帧头 → 查注册表 → 反序列化 → 取 channel
// shared_ptr”流程，handler 只保留各自的业务分支（心跳应答/网关转发/actor 派发）
enum class DecodeResult {
    OK,              // 成功：msg/channel_ptr 可用，必须最终交给 invoke（invoke 负责归还对象池）
    BAD_HEADER,      // 帧长不足 8B 或入参为空
    NOT_REGISTERED,  // msgId 未注册（网关据此把消息转发给 game）
    PARSE_FAILED,    // 反序列化失败，已记日志
    CHANNEL_GONE,    // 连接已关闭回收，消息无处投递
};

struct DecodedMsg {
    int msgId = 0;
    char* body = nullptr;  // 指向帧内 body 起点，仅本次 onMsg 内有效（loop 线程共享 scratch）
    int bodyLen = 0;
    // TODO(session): 会话层建立 player->channel 映射后从帧头/会话取真实 playerId，
    // 登录前的消息暂无归属，保持 0
    int64_t playerId = 0;
    MsgFunction* msg_function = nullptr;
    void* msg = nullptr;  // 池化对象，OK 后必须 invoke，由 invoke 归还对象池
    std::shared_ptr<Channel> channel_ptr;
};

class Decoder {
public:
    // 只拆帧头（跳过 packLen、取 msgId、算出 body 指针/长度）。
    // len < 8 时 body 置空、msgId 置 0，由 decodeMsg 报 BAD_HEADER
    static void parseHeader(void* buff, int len, DecodedMsg* out);

    // parseHeader 之后的完整解析：查注册表 → 取 channel_ptr → 反序列化，
    // 成功时补齐 out 其余字段（先取 channel_ptr 再 parse，连接刚断开时不必白解析）
    static DecodeResult decodeMsg(Channel* channel, DecodedMsg* out);
};


} // namespace transport

#endif //ATHENA_DECODER_H
