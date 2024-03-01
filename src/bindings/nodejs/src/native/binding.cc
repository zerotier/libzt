#include "ZeroTierSockets.h"
#include "macros.h"
#include "tcp.cc"
#include "udp.cc"

#include <napi.h>
#include <sstream>

#define ERROR(ERR, FUN)                                                                                                \
    do {                                                                                                               \
        if (ERR < 0) {                                                                                                 \
            auto error = Napi::Error::New(info.Env(), "Error during " FUN " call");                                    \
            error.Set(STRING("code"), NUMBER(ERR));                                                                    \
            throw error;                                                                                               \
        }                                                                                                              \
    } while (0)

#define CHECK_ERRNO(ERR, FUN)                                                                                          \
    do {                                                                                                               \
        if (ERR < 0) {                                                                                                 \
            auto error = Napi::Error::New(info.Env(), "Error during " FUN " call");                                    \
            error.Set(STRING("code"), NUMBER(ERR));                                                                    \
            error.Set(STRING("errno"), NUMBER(zts_errno));                                                             \
            throw error;                                                                                               \
        }                                                                                                              \
    } while (0)

// ### init ###

METHOD(init_from_storage)
{
    NB_ARGS(1);
    auto configPath = ARG_STRING(0);

    int err = zts_init_from_storage(std::string(configPath).c_str());
    ERROR(err, "init_from_storage");

    return VOID;
}

Napi::ThreadSafeFunction event_callback;

void event_handler(void* msgPtr)
{
    event_callback.Acquire();

    zts_event_msg_t* msg = reinterpret_cast<zts_event_msg_t*>(msgPtr);
    int event = msg->event_code;
    auto cb = [=](TSFN_ARGS) { jsCallback.Call({ NUMBER(event) }); };

    event_callback.NonBlockingCall(cb);

    event_callback.Release();
}

/**
 * @param cb { (event: number) => void } Callback that is called for every event.
 */
METHOD(init_set_event_handler)
{
    NB_ARGS(1);
    auto cb = ARG_FUNC(0);

    event_callback = Napi::ThreadSafeFunction::New(env, cb, "zts_event_listener", 0, 1);

    int err = zts_init_set_event_handler(&event_handler);
    ERROR(err, "init_set_event_handler");

    return VOID;
}

// ### node ###

METHOD(node_start)
{
    NO_ARGS();

    int err = zts_node_start();
    ERROR(err, "node_start");

    return VOID;
}

METHOD(node_is_online)
{
    NO_ARGS();
    return BOOL(zts_node_is_online());
}

METHOD(node_get_id)
{
    NO_ARGS();

    auto id = zts_node_get_id();

    std::ostringstream ss;
    ss << std::hex << id;

    return STRING(ss.str());
}

METHOD(node_stop)
{
    NO_ARGS();

    int err = zts_node_stop();
    ERROR(err, "node_stop");

    if (event_callback)
        event_callback.Abort();

    return VOID;
}

METHOD(node_free)
{
    NO_ARGS();

    int err = zts_node_free();
    ERROR(err, "node_free");

    if (event_callback)
        event_callback.Abort();

    return VOID;
}

// ### net ###

uint64_t convert_net_id(std::string net_id)
{
    return std::stoull(net_id, nullptr, 16);
}

METHOD(net_join)
{
    NB_ARGS(1);
    auto net_id = ARG_STRING(0);

    int err = zts_net_join(convert_net_id(net_id));
    ERROR(err, "net_join");

    return VOID;
}

METHOD(net_leave)
{
    NB_ARGS(1);
    auto net_id = ARG_STRING(0);

    int err = zts_net_leave(convert_net_id(net_id));
    ERROR(err, "net_leave");

    return VOID;
}

METHOD(net_transport_is_ready)
{
    NB_ARGS(1);
    auto net_id = ARG_STRING(0);

    return BOOL(zts_net_transport_is_ready(convert_net_id(net_id)));
}

// ### addr ###

METHOD(addr_get_str)
{
    NB_ARGS(2);
    auto net_id = ARG_STRING(0);
    auto ipv6 = ARG_BOOLEAN(1);

    auto family = ipv6 ? ZTS_AF_INET6 : ZTS_AF_INET;

    char addr[ZTS_IP_MAX_STR_LEN];

    int err = zts_addr_get_str(convert_net_id(net_id), family, addr, ZTS_IP_MAX_STR_LEN);
    ERROR(err, "addr_get_str");

    return STRING(addr);
}

// NAPI initialiser

INIT_ADDON(zts)
{
    // init
    EXPORT(init_from_storage);
    EXPORT(init_set_event_handler);

    // node
    EXPORT(node_start);
    EXPORT(node_is_online);
    EXPORT(node_get_id);
    EXPORT(node_stop);
    EXPORT(node_free);

    // net
    EXPORT(net_join);
    EXPORT(net_leave);
    EXPORT(net_transport_is_ready);

    // addr
    EXPORT(addr_get_str);

    INIT_CLASS(TCP::Socket);
    INIT_CLASS(TCP::Server);

    INIT_CLASS(UDP::Socket);
}
