//
// Created by zhongweiqi on 2025/10/28.
//

#include "GatewayServerNetWorkHandler.h"
#include "transport/Dispatcher.h"
#include "../controller/PlayerLoginHandler.h"
#include "transport/Channel.h"
#include "transport/Decoder.h"
#include "transport/EventLoop.h"
#include "log/XLog.h"
#include "transport/ByteUtils.h"
#include "gate_actor/GateActor.h"
#include "discovery/PeerConn.h"
#include "common/NodeInfo.h"


void GatewayServerNetWorkHandler::initAllMsgRegister()
{
}

void GatewayServerNetWorkHandler::startLogicThread(int ioThreadNum, int logicThreadNum)
{
    GateActor::initActors(IO, ioThreadNum);
    GateActor::initActors(LOGIC, logicThreadNum);
}

void GatewayServerNetWorkHandler::onConnect(transport::Channel* channel)
{
    INFO_LOG("++++++++  on new connection ={}", channel->getAddr());
}

//|---4 msgLen|----4 msgId|----4 magicNum|------body|   （客户端链路：12B 头，无 playerId，归属在 body 的 proto 里）
void GatewayServerNetWorkHandler::onMsg(transport::Channel* channel, void* buff, int len)
{
    // 拆帧头 + 查注册表 + 反序列化 + 取 channel shared_ptr，集中在 Decoder
    transport::DecodedMsg decoded;
    transport::Decoder::parseHeader(channel, buff, len, &decoded);
    // if (decoded.msgId == HEART_BEAT_REQUEST)
    // {
    //     auto res = std::make_shared<HeartBeatResponse>();
    //     res->set_servertime(8888);
    //     res->set_clienttime(7777);
    //     channel->sendMsg(HEART_BEAT_RESPONSE, res);
    //     return;
    // }
    INFO_LOG("=== on read   channel ={} len ={} msgId={}", channel->getAddr(), decoded.bodyLen, decoded.msgId);

    switch (transport::Decoder::decodeMsg(channel, &decoded))
    {
    case transport::DecodeResult::NOT_REGISTERED:
        // 网关未注册的 msgId 转发给 game 处理（proxyMsgToGame 内部拷贝后再跨线程）
        proxyMsgToGame(channel, (char*)buff, len);
        ERR_LOG(" msgId ={} not found process function", decoded.msgId);
        return;
    case transport::DecodeResult::PARSE_FAILED:
    case transport::DecodeResult::BAD_HEADER:
    case transport::DecodeResult::CHANNEL_GONE:
        return;
    case transport::DecodeResult::OK:
        break;
    }

    // TODO(route): 未注册转发的 msgId 在这里原样转发给 game；
    // buff 是 loop 线程共享的 scratch（下个包就会复用），转发必须同步拷贝
    int msgThreadHashCode = (int)((uintptr_t)channel);
    GateActor::execute(LOGIC, msgThreadHashCode, [playerId = decoded.playerId,
                           msg_function = decoded.msg_function,
                           channel_ptr = decoded.channel_ptr,
                           msg = decoded.msg]()
                       {
                           msg_function->invoke(playerId, channel_ptr.get(), msg);
                       });
}


void GatewayServerNetWorkHandler::onClosed(transport::Channel* channel)
{
    void* userData = channel->getUserData();
    if (userData == nullptr)
    {
        return;
    }
    INFO_LOG("connection ={}  closed ", channel->getAddr());
}

void GatewayServerNetWorkHandler::onEventTrigger(transport::Channel* channel, transport::TriggerEventEnum reason)
{
    if (reason == transport::READ_IDLE)
    {
        INFO_LOG("========== onEventTrigger ={}    READ_IDLE  should close it  ", channel->getAddr());
        channel->close();
    }
}

void GatewayServerNetWorkHandler::proxyMsgToGame(transport::Channel* channel, char* buff, int len)
{
    // buff 指向完整客户端帧（12B 头 + body）的起点，len 即整帧长度。
    // EventLoop 的 scratch 只在本次 onMsg 内有效，先拷贝再跨线程投递
    int frameLen = len;
    std::string frame(buff, (size_t)frameLen);

    // 选取目标 game 节点：目前按服务发现广播扩散，由目标节点按 playerId 判定归属
    auto channels = discovery::PeerConn::getChannelsByType(core::SRV_TYPE_GAME);
    if (channels.empty())
    {
        ERR_LOG("proxy msg to game failed: no game node live, frameLen ={}", frameLen);
        return;
    }
    for (auto& destChannel : channels)
    {
        // 客户端帧头无 playerId（12B：msgLen+msgId+magicNum），转发时拆出 body 重新封成
        // 内部 16B 帧，归属玩家目前没有会话表可查，先填 0（登录后 game 侧以 proto roleid 为准）
        const char* body = frame.data() + transport::Channel::CLIENT_HEADER_LEN;
        int bodyLen = frameLen - transport::Channel::CLIENT_HEADER_LEN;
        destChannel->sendRawMsg(transport::ByteUtils::readInt32(frame.data() + 4), body, bodyLen, 0);
    }
}
