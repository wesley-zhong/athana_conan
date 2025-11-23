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
    bool needCallSend = send_buff->storage().readableBytes() == 0;

    int ret = msg->SerializeToArray(getEventPackBuff(), 8192);
    if (!ret) {
        ERR_LOG(" msgId = {} serialize failed", msgId);
        return;
    }
    size_t len = msg->ByteSizeLong();
    send_buff->writeInt32(len + 4);
    send_buff->writeInt32(msgId);
    send_buff->writeBytes(getEventPackBuff(), len);
    last_send_time = nowTime();

    // do send
    if (needCallSend) {
        doUvSend();
    }
}

void Channel::doUvSend() {
    size_t needSendLen = 0;
    const uint8_t *sendPtr = send_buff->storage().linearReadablePtr(&needSendLen);
    if (needSendLen == 0) {
        return;
    }
    auto *req = new uv_write_t;
    uv_buf_t buf = uv_buf_init((char *) sendPtr, needSendLen);

    WritePack *write_pack =  ObjPool::getPool<WritePack>().acquirePtr();//new WritePack();
    write_pack->_channel = this;
    write_pack->sendSize = needSendLen;
    req->data = write_pack;
    uv_write(req, (uv_stream_t *) client, &buf, 1,
             [](uv_write_t *req1, int status) {
                 WritePack *write_pack = (WritePack *) req1->data;
                 //  INFO_LOG(" ------------write complete call back ={}  send len ={} ", status, write_pack->sendSize);
                 write_pack->_channel->send_buff->storage().advanceReadIndex(write_pack->sendSize);
                 write_pack->_channel->doUvSend();
                 ObjPool::getPool<WritePack>().release(write_pack, true) ;
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


char *Channel::getEventPackBuff() {
    return _eventLoop->getPacketBuff();
}
