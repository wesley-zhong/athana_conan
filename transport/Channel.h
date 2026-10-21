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
// - 跨线程(业务线程/actor 任务/PeerConn)持有必须用 EventLoop::channelPtr()/channelById()
//   返回的 shared_ptr，裸指针只允许在 loop 线程回调内使用；长期引用存 getId() 更安全；
// - close() 线程安全：仅标记关闭并入队，真正的 uv_close/通知/释放都在 loop 线程完成；
// - 析构只释放堆内存(ByteBuffer)，uv 句柄的收尾全部在 closeInLoop 的 close 回调里完成，
//   因此最后一个 shared_ptr 在任意线程释放都是安全的。
class Channel {
public:
    // 链路协议模式（构造时随连接确定，收发帧头随之不同）：
    // - INNER 服务器间链路：| 4B msgLen | 4B msgId | 4B playerId | 4B magicNum | body |
    // - CLIENT 对外客户端链路：| 4B msgLen | 4B msgId | 4B magicNum | body |（无 playerId）
    // msgLen 语义一致：除自身 4B 外的头 + body 长度，整帧 = 该值 + 4
    enum class ProtoMode { INNER, CLIENT };

    static constexpr int INNER_HEADER_LEN = 4 * sizeof(int32);
    static constexpr int INNER_LEN_OVERHEAD = INNER_HEADER_LEN - sizeof(int32);
    static constexpr int CLIENT_HEADER_LEN = 3 * sizeof(int32);
    static constexpr int CLIENT_LEN_OVERHEAD = CLIENT_HEADER_LEN - sizeof(int32);
    // 帧头尾部魔数（INNER@+12 / CLIENT@+8）：帧流错位/对端不认识本协议时快速判定并断连
    static constexpr uint32 MAGIC_NUM = 0x41544841; // 'ATHA'
    // single packet max size: frame = header(16B inner / 8B client) + body
    static constexpr int MAX_PACKET_SIZE = 4 * 1024 * 1024;

    // 收发缓冲默认容量（字节）。ring 向上取 2 的幂：64K → 实际分配 128K，可用 128K-1
    static constexpr size_t DEFAULT_BUFF_SIZE = 64 * 1024;

    // 只能在 loop 线程调用（uv handle 已绑定该 loop）。
    // recvBuffSize/sendBuffSize 必须 ≥ 该连接的最大帧长：recv 偏小时入站帧没收全就写满
    // ring，uv_alloc_cb 会按“ring 满”关连接；send 偏小时出站帧被 eventLoopWrite 丢弃。
    // protoMode 决定收发帧头格式（服务器间 INNER / 对外客户端 CLIENT），连接生命周期内不变
    Channel(EventLoop *event_loop, uv_tcp_t *client,
            size_t recvBuffSize, size_t sendBuffSize, ProtoMode protoMode);
    ~Channel();

    Channel(const Channel &) = delete;
    Channel &operator=(const Channel &) = delete;

    // 线程安全：序列化与落盘都在 loop 线程完成，msg 由 shared_ptr 延长生命周期。
    // playerId 写入内部帧头（登录前的握手/心跳等无归属消息传 0）；
    // 客户端链路帧头无此字段，该参数被忽略（归属信息走 proto body）
    void sendMsg(int msgId, std::shared_ptr<google::protobuf::Message> msg, int64_t playerId = 0);

    // 线程安全：按帧协议原样发送裸字节（router 中转场景：不反序列化直接转发 body）
    void sendRawMsg(int msgId, const char *data, int len, int64_t playerId = 0);

    // 线程安全：同上，body 直接按值转移，避免一次拷贝
    void sendRawMsg(int msgId, std::string body, int64_t playerId = 0);

    void initPackTime();

    void onRead(uv_stream_t *client, ssize_t nread, const uv_buf_t *buf);

    EventLoop *event_loop() {
        return _eventLoop;
    }

    void setUserData(void *data) {
        this->userData = data;
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

    // 连接唯一 id：generateId() 生成，进程内唯一，0 保留为无效值
    uint32 getId() const {
        return id;
    }

    // 本连接的链路协议（构造时确定）：INNER 16B 头 / CLIENT 12B 头
    ProtoMode protoMode() const {
        return proto_mode;
    }

    // loop 线程：连接建立后刷新地址（出站连接构造时尚未 connect，无法取 peer）
    void refreshAddr();

    // peek next frame length without consuming:
    // -1 incomplete, -2 illegal length/bad magic(caller should close), else frame bytes = len field + 4
    int peekNextPackLen() const {
        int headerLen = proto_mode == ProtoMode::INNER ? INNER_HEADER_LEN : CLIENT_HEADER_LEN;
        auto readableBytes = (int) recv_buffer->storage().readableBytes();
        if (readableBytes < headerLen) {
            return -1;
        }
        // 头必须整体偷看：INNER 布局 msgLen@+0、msgId@+4、playerId@+8、magicNum@+12；
        // CLIENT 布局 msgLen@+0、msgId@+4、magicNum@+8（无 playerId）
        char hdr[INNER_HEADER_LEN];
        recv_buffer->storage().peek(hdr, headerLen);
        uint32 msgLen = ByteUtils::readInt32(hdr);
        if (msgLen > (uint32) (MAX_PACKET_SIZE - 4)) {
            return -2;
        }
        // 魔数在头尾（INNER@+12 / CLIENT@+8）：不匹配说明流已错位或对端不讲本协议，
        // 立即断连比黑盒错解更安全
        int magicOff = proto_mode == ProtoMode::INNER ? 12 : 8;
        if (ByteUtils::readInt32(hdr + magicOff) != MAGIC_NUM) {
            return -2;
        }
        if (msgLen > (uint32) (readableBytes - 4)) {
            return -1;
        }
        return (int) msgLen + 4;
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

    void eventLoopWrite(int msgId, const std::shared_ptr<google::protobuf::Message> &body, int64_t playerId);

    void eventLoopRawWrite(int msgId, const std::string &body, int64_t playerId);

    void closeInLoop();

    // loop 线程：一个 uv 句柄(tcp/timer)的 close 回调完成后回收，两个都完成才解除注册表持有
    void onHandleClosed(uv_handle_t *handle);

    static std::string getAddrString(const struct sockaddr_storage &addr);
    static uint32 generateId();

    std::string computeAddr();

    void *userData;
    std::atomic<bool> closed_{false};
    bool tcp_closed_ = false;
    bool timer_closed_ = false;
    std::string addr_;
    uint32 id;
    // 本连接的链路协议（构造时确定），决定收发帧头布局
    ProtoMode proto_mode;
    // inline 变量（C++17）：类内初始化的静态成员须标 inline 才能过 MSVC C2864
    inline static std::atomic<uint32> next_id{0};
};


} // namespace transport

#endif //ATHENA_CHANNEL_H
