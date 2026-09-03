//
// Created by zhongweiqi on 2025/10/23.
//

#include "Channel.h"
#include <sstream>
#include "core/log/XLog.h"
#include "EventLoop.h"
#include "core/common/ObjectPool.hpp"

void Channel::onRead(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf) {
    if (nread > 0) {
        recv_buffer->advanceWriteIndex(nread);
        last_recv_time = nowTime();
        return;
    }
    if (nread < 0) {
        if (nread == UV_EOF) {
            INFO_LOG(" client ={} closed ", this->getAddr());
        } else {
            ERR_LOG(" read error ", nread);
        }
        this->close();
    }
}

void Channel::sendMsg(int msgId, std::shared_ptr<google::protobuf::Message> msg) {
    _eventLoop->push([this, msgId, msg]() {
        this->eventLoopWrite(msgId, msg.get());
    });
    _eventLoop->async_write_task();
}

void Channel::initPackTime() {
    last_recv_time = last_send_time = nowTime();
}

void Channel::close() {
    if (closed) {
        return;
    }
    closed = true;
    _eventLoop->onClosed(this);
    uv_close((uv_handle_t *) client, [](uv_handle_t *handle) {
    });
    uv_timer_stop(getTimer());
}


void Channel::sendMsg(int msgId, google::protobuf::Message *msg) {
    _eventLoop->push([this, msgId, msg]() {
        this->eventLoopWrite(msgId, msg);
    });
    _eventLoop->async_write_task();
}

void Channel::eventLoopWrite(int msgId, google::protobuf::Message *msg) {
    if (closed) {
        return;
    }
    bool needCallSend = send_buff->storage().readableBytes() == 0;

    int64 bodyLen = (int64) msg->ByteSizeLong();
    int64 frameLen = bodyLen + 2 * sizeof(int32); // 4B packLen + 4B msgId + body
    if (frameLen > MAX_PACKET_SIZE) {
        ERR_LOG(" msgId = {} frame too large frameLen ={}, drop it", msgId, frameLen);
        return;
    }
    // the whole frame must fit into send buff, a partial frame would desync the stream
    if (send_buff->storage().writableBytes() < (size_t) frameLen) {
        ERR_LOG("send buff full, msgId ={} frameLen ={}, drop it", msgId, frameLen);
        return;
    }
    char *packBuf = getEventPackBuff((int32) bodyLen);
    if (packBuf == nullptr) {
        ERR_LOG(" msgId = {} alloc pack buff failed, drop it", msgId);
        return;
    }
    if (!msg->SerializeToArray(packBuf, (int32) bodyLen)) {
        ERR_LOG(" msgId = {} serialize failed", msgId);
        return;
    }
    int32 len = (int32) bodyLen;
    send_buff->writeInt32(len + 4);
    send_buff->writeInt32(msgId);
    send_buff->writeBytes(packBuf, len);
    last_send_time = nowTime();

    INFO_LOG("--------{} send msgId={}  len={} ",(void*)this, msgId, len + 4 + 4);
    // do send
    if (needCallSend) {
        doUvSend();
    }
}

void Channel::doUvSend() {
    if (closed) {
        return;
    }
    size_t needSendLen = 0;
    const uint8_t *sendPtr = send_buff->storage().linearReadablePtr(&needSendLen);
    if (needSendLen == 0) {
        return;
    }
    auto *req = new uv_write_t;
    uv_buf_t buf = uv_buf_init((char *) sendPtr, needSendLen);

    WritePack *write_pack = ObjPool::GetPool<WritePack>().acquirePtr();//new WritePack();
    write_pack->_channel = this;
    write_pack->sendSize = needSendLen;
    req->data = write_pack;
    uv_write(req, (uv_stream_t *) client, &buf, 1,
             [](uv_write_t *req1, int status) {
                 WritePack *write_pack = (WritePack *) req1->data;
                 Channel *channel = write_pack->_channel;
                 INFO_LOG(" -----------chanel: {} -write complete call back ={}  send len ={} ",(void*)channel, status, write_pack->sendSize);

                 if (status < 0) {
                     // 1. 记录错误日志
                     // uv_strerror 可以将错误码转为可读字符串
                     ERR_LOG("write failed: {}, addr: {}", uv_strerror(status), channel->getAddr());
                     // 2. 发生错误时，通常需要关闭连接
                     // 注意：不要在这里再次调用 doUvSend()
                     channel->close();
                 } else {
                     // 3. 只有成功时才推进索引并继续发送
                     channel->send_buff->storage().advanceReadIndex(write_pack->sendSize);
                     channel->doUvSend();
                 }

                 // 4. 无论成功与否，必须释放本次请求相关的内存
                 ObjPool::GetPool<WritePack>().release(write_pack);
                 free(req1);
             });
}


std::string Channel::getAddr() {
    struct sockaddr_storage addr;
    int addr_len = sizeof(addr);
    uv_tcp_getpeername(client, (struct sockaddr *) &addr, &addr_len);
    std::string right = getAddrString(addr);
    uv_tcp_getsockname(client, (struct sockaddr *) &addr, &addr_len);
    std::string left = getAddrString(addr);
    return "[L:/" + left + " - R:/" + right + "]";
}

std::string Channel::getAddrString(const struct sockaddr_storage &addr) {
    std::ostringstream ss;

    char ip[INET6_ADDRSTRLEN] = {0};
    int port = 0;

    // 提取 IP 和端口
    if (addr.ss_family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *) &addr;
        uv_ip4_name(addr4, ip, sizeof(ip));
        port = Endian::fromNetwork16(addr4->sin_port);
        ss << ip << ":" << port;
    } else if (addr.ss_family == AF_INET6) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) &addr;
        uv_ip6_name(addr6, ip, sizeof(ip));
        port = Endian::fromNetwork16(addr6->sin6_port);
        ss << ip << ":" << port;
    } else {
        fprintf(stderr, "Unknown address family\n");
    }
    return ss.str();
}

EventLoop *Channel::event_loop() {
    return _eventLoop;
}

uint64_t Channel::nowTime() {
    return uv_now(event_loop()->uv_loop());
}


char *Channel::getEventPackBuff(int needLen) {
    return _eventLoop->getPacketBuff(needLen);
}
