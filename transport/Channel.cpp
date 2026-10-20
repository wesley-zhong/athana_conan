//
// Created by zhongweiqi on 2025/10/23.
//

#include "Channel.h"
#include <sstream>
#include "log/XLog.h"
#include "EventLoop.h"
#include "common/ObjectPool.hpp"

namespace transport
{
    namespace
    {
        // 配置兜底：0 视为默认容量；超过协议帧长上限(MAX_PACKET_SIZE+8)的容量纯属
        // 浪费，一并钳住。负数配置经 int→size_t 转换会变成超大值，也会被钳到这里——
        // RingByteBuf 对非法容量是抛异常，而 Channel 构造发生在 libuv 回调里，
        // 异常抛穿 C 栈是未定义行为，必须在构造前拦下
        size_t clampBuffSize(size_t size)
        {
            if (size == 0)
            {
                return Channel::DEFAULT_BUFF_SIZE;
            }
            size_t maxFrame = (size_t)Channel::MAX_PACKET_SIZE + 8;
            return size > maxFrame ? maxFrame : size;
        }
    }

    Channel::Channel(EventLoop* event_loop, uv_tcp_t* tcp,
                     size_t recvBuffSize, size_t sendBuffSize) : _eventLoop(event_loop), client(tcp),
                                                                  last_recv_time(0), last_send_time(0),
                                                                  userData(nullptr),
                                                                  id(generateId())
    {
        // 收发 ring 分开设容，容量可配（默认 64K，内部向上取 2 的幂，64K → 实际 128K）。
        // 容量必须 ≥ 该连接最大帧长（协议上限 MAX_PACKET_SIZE+8）：recv 小于最大入站帧时
        // 帧没收全就写满 ring，uv_alloc_cb 按“ring 满且无完整帧”关连接；send 小于最大
        // 出站帧时帧被 eventLoopWrite 静默丢弃——按业务实际最大帧长留足，不要盲目调小
        recv_buffer = new core::ByteBuffer(clampBuffSize(recvBuffSize));
        send_buff = new core::ByteBuffer(clampBuffSize(sendBuffSize));
        heartbeat_timer.data = this;
        addr_ = computeAddr();
    }

    Channel::~Channel()
    {
        delete recv_buffer;
        delete send_buff;
    }

    // next_id 从 1 起发（先自增再取），0 永不发出去，保留为“无效 id”哨兵。
    // fetch_add 放任回绕：进程生命周期内建立 2^32 条连接才会重号，届时老连接
    // 早已回收；如需严格不重，这里换成 (n % max) + 1 之类的策略即可
    uint32 Channel::generateId()
    {
        return next_id.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    std::string Channel::computeAddr()
    {
        // client 的 Channel 在 connect 完成前构造：未连接的 socket 上 getpeername/getsockname
        // 均失败且不写出参，addr 必须零初始化并检查返回值，否则拿栈上残留值判 family
        struct sockaddr_storage addr{};
        int addr_len = sizeof(addr);
        std::string right = "unconnected";
        if (uv_tcp_getpeername(client, (struct sockaddr*)&addr, &addr_len) == 0)
        {
            right = getAddrString(addr);
        }
        addr_len = sizeof(addr);
        std::string left = "unconnected";
        if (uv_tcp_getsockname(client, (struct sockaddr*)&addr, &addr_len) == 0)
        {
            left = getAddrString(addr);
        }
        return "[L:/" + left + " - R:/" + right + "]";
    }

    void Channel::refreshAddr()
    {
        addr_ = computeAddr();
    }

    void Channel::onRead(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
    {
        if (nread > 0)
        {
            // ring 满时 uv_alloc_cb 走 dummy 1 字节路径并已 close()：
            // 这 1 字节不属于 ring，推进索引会被拒绝
            if (isClosed())
            {
                return;
            }
            if (!recv_buffer->advanceWriteIndex(nread))
            {
                // 理论不可达（nread ≤ alloc 给出的连续写窗口），防御性关闭
                ERR_LOG("advanceWriteIndex failed from {}, close it", this->getAddr());
                this->close();
                return;
            }
            last_recv_time = nowTime();
            return;
        }
        if (nread < 0)
        {
            if (nread == UV_EOF)
            {
                INFO_LOG(" client ={} closed ", this->getAddr());
            }
            else
            {
                ERR_LOG(" read error ", nread);
            }
            this->close();
        }
    }

    void Channel::sendMsg(int msgId, std::shared_ptr<google::protobuf::Message> msg)
    {
        // 通过注册表拿到 shared_ptr，任务持有期间 channel 保证存活
        std::shared_ptr<Channel> self = _eventLoop->channelPtr(this);
        if (self == nullptr || isClosed())
        {
            return;
        }
        _eventLoop->executeOnEventLoop([self, msgId, msg]()
        {
            self->eventLoopWrite(msgId, msg);
        });
    }

    void Channel::sendRawMsg(int msgId, const char* data, int len)
    {
        if (data == nullptr || len <= 0)
        {
            return;
        }
        // body 按值捕获进任务，跨线程投递期间数据保持存活
        std::string body(data, (size_t)len);
        sendRawMsg(msgId, std::move(body));
    }

    void Channel::sendRawMsg(int msgId, std::string body)
    {
        std::shared_ptr<Channel> self = _eventLoop->channelPtr(this);
        if (self == nullptr || isClosed())
        {
            return;
        }
        _eventLoop->executeOnEventLoop([self, msgId, body = std::move(body)]()
        {
            self->eventLoopRawWrite(msgId, body);
        });
    }

    void Channel::initPackTime()
    {
        last_recv_time = last_send_time = nowTime();
    }

    void Channel::close()
    {
        if (closed_.exchange(true, std::memory_order_acq_rel))
        {
            return;
        }
        std::shared_ptr<Channel> self = _eventLoop->channelPtr(this);
        if (self == nullptr)
        {
            return;
        }
        _eventLoop->executeOnEventLoop([self]()
        {
            self->closeInLoop();
        });
    }

    void Channel::closeInLoop()
    {
        uv_read_stop((uv_stream_t*)client);
        // close 回调完成后由 onHandleClosed 解除 EventLoop 的注册持有；
        // 在途 uv_write 会被 libuv 以 UV_ECANCELED 取消，回调先于 tcp close 回调执行
        uv_close((uv_handle_t*)client, [](uv_handle_t* h)
        {
            static_cast<Channel*>(h->data)->onHandleClosed(h);
        });
        uv_close((uv_handle_t*)getTimer(), [](uv_handle_t* h)
        {
            static_cast<Channel*>(h->data)->onHandleClosed(h);
        });
        _eventLoop->onClosed(this);
    }

    void Channel::onHandleClosed(uv_handle_t* handle)
    {
        if (handle == (uv_handle_t*)client)
        {
            tcp_closed_ = true;
        }
        else if (handle == (uv_handle_t*)getTimer())
        {
            timer_closed_ = true;
        }
        if (tcp_closed_ && timer_closed_)
        {
            _eventLoop->releaseChannel(this);
        }
    }

    void Channel::eventLoopWrite(int msgId, const std::shared_ptr<google::protobuf::Message>& msg)
    {
        if (closed_)
        {
            return;
        }
        bool needCallSend = send_buff->storage().readableBytes() == 0;

        int64 bodyLen = (int64)msg->ByteSizeLong();
        int64 frameLen = bodyLen + 2 * sizeof(int32); // 4B packLen + 4B msgId + body
        if (frameLen > MAX_PACKET_SIZE)
        {
            ERR_LOG(" msgId = {} frame too large frameLen ={}, drop it", msgId, frameLen);
            return;
        }
        // the whole frame must fit into send buff, a partial frame would desync the stream
        if (send_buff->storage().writableBytes() < (size_t)frameLen)
        {
            ERR_LOG("send buff full, msgId ={} frameLen ={}, drop it", msgId, frameLen);
            return;
        }
        char* packBuf = getEventPackBuff((int32)bodyLen);
        if (packBuf == nullptr)
        {
            ERR_LOG(" msgId = {} alloc pack buff failed, drop it", msgId);
            return;
        }
        if (!msg->SerializeToArray(packBuf, (int32)bodyLen))
        {
            ERR_LOG(" msgId = {} serialize failed", msgId);
            return;
        }
        int32 len = (int32)bodyLen;
        send_buff->writeInt32(len + 4);
        send_buff->writeInt32(msgId);
        send_buff->writeBytes(packBuf, len);
        last_send_time = nowTime();

        // do send
        if (needCallSend)
        {
            doUvSend();
        }
    }

    void Channel::eventLoopRawWrite(int msgId, const std::string& body)
    {
        if (closed_)
        {
            return;
        }
        bool needCallSend = send_buff->storage().readableBytes() == 0;

        int64 bodyLen = (int64)body.size();
        int64 frameLen = bodyLen + 2 * sizeof(int32); // 4B packLen + 4B msgId + body
        if (frameLen > MAX_PACKET_SIZE)
        {
            ERR_LOG(" msgId = {} frame too large frameLen ={}, drop it", msgId, frameLen);
            return;
        }
        if (send_buff->storage().writableBytes() < (size_t)frameLen)
        {
            ERR_LOG("send buff full, msgId ={} frameLen ={}, drop it", msgId, frameLen);
            return;
        }
        int32 len = (int32)bodyLen;
        send_buff->writeInt32(len + 4);
        send_buff->writeInt32(msgId);
        // writeBytes 参数已是 const char*，body.data() 直接传入
        send_buff->writeBytes(body.data(), len);
        last_send_time = nowTime();

        if (needCallSend)
        {
            doUvSend();
        }
    }

    void Channel::doUvSend()
    {
        if (closed_)
        {
            return;
        }
        size_t needSendLen = 0;
        const uint8_t* sendPtr = send_buff->storage().linearReadablePtr(&needSendLen);
        if (needSendLen == 0)
        {
            return;
        }
        auto* req = new uv_write_t;
        uv_buf_t buf = uv_buf_init((char*)sendPtr, (unsigned)needSendLen);

        WritePack* write_pack = core::ObjPool::GetPool<WritePack>().acquirePtr();
        write_pack->_channel = this;
        write_pack->sendSize = (int32)needSendLen;
        req->data = write_pack;
        int ret = uv_write(req, (uv_stream_t*)client, &buf, 1,
                           [](uv_write_t* req1, int status)
                           {
                               auto* write_pack = (WritePack*)req1->data;
                               Channel* channel = write_pack->_channel;

                               if (status < 0)
                               {
                                   // 在途写被关闭取消时不再重复 close
                                   ERR_LOG("write failed: {}, addr: {}", uv_strerror(status), channel->getAddr());
                                   if (status != UV_ECANCELED)
                                   {
                                       channel->close();
                                   }
                               }
                               else
                               {
                                   // 只有成功时才推进索引并继续发送；推进失败（理论不可达，
                                   // sendSize 即刚发出的连续可读长度）说明流已错位，关连接
                                   if (!channel->send_buff->storage().advanceReadIndex(write_pack->sendSize))
                                   {
                                       ERR_LOG("advanceReadIndex failed, addr: {}", channel->getAddr());
                                       channel->close();
                                   }
                                   else
                                   {
                                       channel->doUvSend();
                                   }
                               }

                               core::ObjPool::GetPool<WritePack>().release(write_pack);
                               delete req1;
                           });
        if (ret != 0)
        {
            // uv_write 同步失败时不会触发回调，这里手动回收
            ERR_LOG("uv_write failed: {}, addr: {}", uv_strerror(ret), getAddr());
            core::ObjPool::GetPool<WritePack>().release(write_pack);
            delete req;
            close();
        }
    }


    std::string Channel::getAddrString(const struct sockaddr_storage& addr)
    {
        std::ostringstream ss;

        char ip[INET6_ADDRSTRLEN] = {0};
        int port = 0;

        // 提取 IP 和端口
        if (addr.ss_family == AF_INET)
        {
            struct sockaddr_in* addr4 = (struct sockaddr_in*)&addr;
            uv_ip4_name(addr4, ip, sizeof(ip));
            port = core::Endian::fromNetwork16(addr4->sin_port);
            ss << ip << ":" << port;
        }
        else if (addr.ss_family == AF_INET6)
        {
            struct sockaddr_in6* addr6 = (struct sockaddr_in6*)&addr;
            uv_ip6_name(addr6, ip, sizeof(ip));
            port = core::Endian::fromNetwork16(addr6->sin6_port);
            ss << ip << ":" << port;
        }
        else
        {
            // getAddrString 只处理 v4/v6；这里带出 family 值方便定位（如 AF_UNSPEC=0，
            // 说明调用方在未连接/取地址失败时把未初始化的 storage 传了进来）
            ERR_LOG("getAddrString unknown address family ={}", (int) addr.ss_family);
        }
        return ss.str();
    }

    uint64_t Channel::nowTime()
    {
        return uv_now(event_loop()->uv_loop());
    }


    char* Channel::getEventPackBuff(int needLen)
    {
        return _eventLoop->getPacketBuff(needLen);
    }
} // namespace transport
