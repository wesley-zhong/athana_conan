//
// Created by zhongweiqi on 2025/10/23.
//

#ifndef ATHENA_CHANNEL_H
#define ATHENA_CHANNEL_H
#include <atomic>
#include <memory>
#include <string>
#include "google/protobuf/message.h"
#include "uv.h"
#include "common/ByteBuffer.h"
#include "ByteUtils.h"

namespace transport {

class EventLoop;
class Channel;

struct WritePack {
    Channel *_channel;
    int32 sendSize;
};

// 一条连接的封装。生命周期约定：
// - 只能在 EventLoop(loop 线程) 内创建，EventLoop 通过 shared_ptr 注册表持有；
// - 跨线程(业务线程/actor 任务/PeerConn)持有必须用 EventLoop::channelPtr() 返回的
//   shared_ptr，裸指针只允许在 loop 线程回调内使用；
// - close() 线程安全：仅标记关闭并入队，真正的 uv_close/通知/释放都在 loop 线程完成；
// - 析构只释放堆内存(ByteBuffer)，uv 句柄的收尾全部在 closeInLoop 的 close 回调里完成，
//   因此最后一个 shared_ptr 在任意线程释放都是安全的。
class Channel {
public:
    // single packet max size: frame = 4B packLen + 4B msgId + body
    static constexpr int MAX_PACKET_SIZE = 4 * 1024 * 1024;

    // 只能在 loop 线程调用（uv handle 已绑定该 loop）
    Channel(EventLoop *event_loop, uv_tcp_t *client);
    ~Channel();

    Channel(const Channel &) = delete;
    Channel &operator=(const Channel &) = delete;

    // 线程安全：序列化与落盘都在 loop 线程完成，msg 由 shared_ptr 延长生命周期
    void sendMsg(int msgId, std::shared_ptr<google::protobuf::Message> msg);

    void initPackTime();

    void onRead(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);

    EventLoop *event_loop() {
        return _eventLoop;
    }

    void setUserData(void *userData) {
        this->userData = userData;
    }

    void *getUserData() {
        return this->userData;
    }

    uv_timer_t *getTimer() {
        return &heartbeat_timer;
    }

    char *getEventPackBuff(int needLen);

    uint64_t nowTime();

    // 线程安全：标记关闭并入队清理；幂等
    void close();

    bool isClosed() const {
        return closed_.load(std::memory_order_acquire);
    }

    // 构造时缓存的本端/对端地址，关闭后仍可安全打印
    std::string getAddr() const {
        return addr_;
    }

    // loop 线程：连接建立后刷新地址（出站连接构造时尚未 connect，无法取 peer）
    void refreshAddr();

    // peek next frame length without consuming:
    // -1 incomplete, -2 illegal length(caller should close), else frame bytes = packLen + 4
    int peekNextPackLen() const {
        int readableBytes = (int) recv_buffer->storage().readableBytes();
        if (readableBytes < 8) {
            return -1;
        }
        uint32 packLen = recv_buffer->getInt32();
        if (packLen > (uint32) (MAX_PACKET_SIZE - 4)) {
            return -2;
        }
        if (packLen > readableBytes - 4) {
            return -1;
        }
        return packLen + 4;
    }

    int getPack(char *outPacket, int packLen) const {
        return (int) recv_buffer->readBytes(outPacket, packLen);
    }

    void doUvSend();


    core::ByteBuffer *recv_buffer;
    core::ByteBuffer *send_buff;

    EventLoop *_eventLoop;
    uint64 last_recv_time;
    uint64 last_send_time;
    uv_timer_t heartbeat_timer;
    uv_tcp_t *client;

private:
    friend class EventLoop;

    void eventLoopWrite(int msgId, const std::shared_ptr<google::protobuf::Message> &body);

    void closeInLoop();

    // loop 线程：一个 uv 句柄(tcp/timer)的 close 回调完成后回收，两个都完成才解除注册表持有
    void onHandleClosed(uv_handle_t *handle);

    static std::string getAddrString(const struct sockaddr_storage &addr);

    std::string computeAddr();

    void *userData;
    std::atomic<bool> closed_{false};
    bool tcp_closed_ = false;
    bool timer_closed_ = false;
    std::string addr_;
};


} // namespace transport

#endif //ATHENA_CHANNEL_H
