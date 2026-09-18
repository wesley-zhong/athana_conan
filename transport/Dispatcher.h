#ifndef DISPATCHER_H_
#define DISPATCHER_H_

#include <functional>
#include <map>
#include "google/protobuf/message.h"
#include "common/Singleton.h"
#include "common/ObjectPool.hpp"
#include "Channel.h"

namespace transport {

#define REGISTER_MSG_ID_FUN(MSGID, MSG_TYPE, FUNCTION) \
transport::Dispatcher::Instance()->registerMsgHandler<MSG_TYPE>(MSGID, std::function(FUNCTION))

struct MsgFunction {
    std::function<void *(void *, int)> parseParam; //this may be use obj pool
    std::function<void(int64_t, Channel *, void *)> invoke;
};

class Dispatcher : public core::Singleton<Dispatcher> {
public:
    Dispatcher() = default;

    ~Dispatcher() = default;

    template<typename T>
    void registerMsgHandler(int msgId, std::function<void(int64_t, T *)> msgFuc);

    template<typename T>
    void registerMsgHandler(int msgId, std::function<void(Channel *, T *)> msgFuc);

    void processMsg(int msgId, int64_t playerId, Channel *channel, const void *body, int len);

    MsgFunction *findMsgFuncion(int msgId) {
        auto it = msgMap.find(msgId);
        if (it != msgMap.end()) {
            return it->second;
        }
        return nullptr;
    }

private:
    std::unordered_map<int, MsgFunction *> msgMap;
};

template<typename T>
void Dispatcher::registerMsgHandler(int msgId, std::function<void(int64_t, T *)> msgFuc) {
    auto *msgFunction = new MsgFunction();
    msgFunction->parseParam = [](void *body, int len) {
        T *msg = core::ObjPool::AcquirePtr<T>();
        msg->ParseFromArray(body, len);
        return msg;
    };
    msgFunction->invoke = [msgFuc](int64_t playerId, Channel *channel, void *msg) {
        msgFuc(playerId, (T *) msg);
        core::ObjPool::Release<T>((T *) msg);
    };
    msgMap[msgId] = msgFunction;
}

template<typename T>
void Dispatcher::registerMsgHandler(int msgId, std::function<void(Channel *, T *)> msgFuc) {
    auto *msgFunction = new MsgFunction();
    msgFunction->parseParam = [](void *body, int len) {
        T *msg = core::ObjPool::AcquirePtr<T>();
        msg->ParseFromArray(body, len);
        return msg;
    };
    msgFunction->invoke = [msgFuc](int64_t playerId, Channel *channel, void *msg) {
        msgFuc(channel, static_cast<T *>(msg));
        core::ObjPool::Release<T>(static_cast<T *>(msg));
    };
    msgMap[msgId] = msgFunction;
}


} // namespace transport

#endif
