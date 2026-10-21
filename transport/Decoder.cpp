//
// Created by zhongweiqi on 2025/10/23.
//

#include "Decoder.h"
#include "log/XLog.h"

namespace transport {

void Decoder::parseHeader(Channel* channel, void* buff, int len, DecodedMsg* out)
{
    bool inner = channel->protoMode() == Channel::ProtoMode::INNER;
    int headerLen = inner ? Channel::INNER_HEADER_LEN : Channel::CLIENT_HEADER_LEN;
    if (buff == nullptr || len < headerLen)
    {
        out->msgId = 0;
        out->body = nullptr;
        out->bodyLen = 0;
        return;
    }
    char* frame = static_cast<char*>(buff);
    // 魔数不匹配的帧正常进不到这里（peekNextPackLen 已拦），此处兜底防错解；
    // 两种链路帧头都以魔数收尾：INNER@+12、CLIENT@+8
    int magicOff = inner ? 12 : 8;
    if (ByteUtils::readInt32(frame + magicOff) != Channel::MAGIC_NUM)
    {
        ERR_LOG("bad magic num from frame, msgId ={}", ByteUtils::readInt32(frame + 4));
        out->msgId = 0;
        out->body = nullptr;
        out->bodyLen = 0;
        return;
    }
    if (inner)
    {
        out->playerId = (int64_t)(int32_t)ByteUtils::readInt32(frame + 8);
    }
    // CLIENT 帧头无 playerId 字段，保持默认 0：未登录消息本就无归属，
    // 已登录消息的归属由 gateway 查会话补齐（body 内 proto 也带 roleid）
    out->msgId = ByteUtils::readInt32(frame + 4);
    out->body = frame + headerLen;
    out->bodyLen = len - headerLen;
}

DecodeResult Decoder::decodeMsg(Channel* channel, DecodedMsg* out)
{
	if (out->body == nullptr)
	{
		ERR_LOG("illegal frame, len < header or bad magic from {}", channel->getAddr());
		return DecodeResult::BAD_HEADER;
	}

    out->msg_function = Dispatcher::Instance()->findMsgFunction(out->msgId);
    if (out->msg_function == nullptr)
    {
        return DecodeResult::NOT_REGISTERED;
    }

    // actor 任务异步执行，channel 必须用 shared_ptr 持有，防止连接关闭后被释放；
    // 先取 channel_ptr 再 parse，连接刚断开时不必白白解析一次
    out->channel_ptr = channel->event_loop()->channelPtr(channel);
    if (out->channel_ptr == nullptr)
    {
        return DecodeResult::CHANNEL_GONE;
    }

    out->msg = out->msg_function->parseParam(out->body, out->bodyLen);
    if (out->msg == nullptr)
    {
        ERR_LOG("parse msg failed, msgId ={}", out->msgId);
        return DecodeResult::PARSE_FAILED;
    }
    return DecodeResult::OK;
}

} // namespace transport
