//
// Created by zhongweiqi on 2025/10/27.
//

#ifndef ATHENA_NETINTERFACE_H
#define ATHENA_NETINTERFACE_H
#include "NetInterface.h"
#include "transport/EventDefs.h"
class Channel;

class NetInterface {
public:
    virtual void on_connected(Channel *channel, int status) =0;

    virtual void on_new_connection(Channel *channel) =0;

    virtual void on_read(Channel *channel, char *body, int len) =0;

    virtual void on_closed(Channel *channel) =0;

    virtual void triggerEvent(Channel *channel, TriggerEventEnum reason) =0;
};

#endif //ATHENA_NETINTERFACE_H
