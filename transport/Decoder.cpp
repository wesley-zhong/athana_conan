//
// Created by zhongweiqi on 2025/10/23.
//

#include "Decoder.h"
#include "log/XLog.h"

namespace transport {

void Decoder::parseHeader(void* buff, int len, DecodedMsg* out)
{
    if (buff == nullptr || len < 8)
    {
        out->msgId = 0;
        out->body = nullptr;
        out->bodyLen = 0;
        return;
    }
    char* frame = static_cast<char*>(buff);
    out->msgId = ByteUtils::readInt32(frame + 4);
    out->body = frame + 8;
    out->bodyLen = len - 8;
}

DecodeResult Decoder::decodeMsg(Channel* channel, DecodedMsg* out)
{
    if (out->body == nullptr)
    {
        ERR_LOG("illegal frame, len < 8 from {}", channel->getAddr());
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
