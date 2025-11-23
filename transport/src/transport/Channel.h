//
// Created by zhongweiqi on 2025/10/23.
//

#ifndef ATHENA_CHANNEL_H
#define ATHENA_CHANNEL_H
#include "google/protobuf/message.h"
#include "uv.h"
#include "core/common/ByteBuffer.h"
#include "ByteUtils.h"

class EventLoop;
class Channel;

struct SPackage {
    int32 _msgId;
    char *_body;
    int32 _bodyLen;

    void parseBody(char *body, int32 bodyLen) {
        _msgId = ByteUtils::readInt32(body);
        _body = body + sizeof(int32);
    }
};

struct WritePack {
    Channel *_channel;
    int32 sendSize;
    WritePack(){

    }

    ~WritePack(){
        _channel = nullptr;
        sendSize =0;
    }
    void consume();
};

class Channel {
public:
    Channel(EventLoop *event_loop, uv_tcp_t *client, uv_os_sock_t fd) : _eventLoop(event_loop),
                                                                        client(client), fd((uint64) fd) {
        recv_buffer = new ByteBuffer();
        send_buff = new ByteBuffer();
        heartbeat_timer.data = this;
    }

    void sendMsg(int msgId, std::shared_ptr<google::protobuf::Message> msg);

    void initPackTime();

    void sendMsg(int msgId, google::protobuf::Message *msg);

    void onRead(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);

    uint64 getFd() const {
        return fd;
    }

    EventLoop *event_loop();

    void setUserData(void *userData) {
        this->userData = userData;
    }

    void *getUserData() {
        return this->userData;
    }

    uv_timer_t *getTimer() {
        return &heartbeat_timer;
    }

    char *getEventPackBuff();

    uint64_t nowTime();

    void close();

    std::string getAddr();

    int getPack(char *outPacket) const {
        int packetLen = recv_buffer->getNextPackLen();
        if (packetLen > 0) {
            recv_buffer->readBytes(outPacket, packetLen);
        }
        return packetLen;
    }

    void doUvSend();


    ByteBuffer *recv_buffer;
    ByteBuffer *send_buff;

    EventLoop *_eventLoop;
    uint64 last_recv_time;
    uint64 last_send_time;
    uv_timer_t heartbeat_timer;
    uv_tcp_t *client;

private:
    void eventLoopWrite(int msgId, std::shared_ptr<google::protobuf::Message> body);

    void eventLoopWrite(int msgId, google::protobuf::Message *body);

    std::string getAddrString(const struct sockaddr_storage &addr);

    uint64 fd;
    void *userData;
    bool writing; // whether a uv_write is in-flight
    bool closed; // connection closed
};


#endif //ATHENA_CHANNEL_H
