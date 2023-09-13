#include "ZeroTierSockets.h"
#include "async.cc"
#include "asynclambda.h"
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
    NB_ARGS(1)
    auto configPath = ARG_STRING(0);

    int err = zts_init_from_storage(std::string(configPath).c_str());
    ERROR(err, "init_from_storage");

    return VOID;
}

Napi::ThreadSafeFunction event_callback;

void event_handler(void* msgPtr)
{
    event_callback.Acquire();

    zts_event_msg_t* msg = (zts_event_msg_t*)msgPtr;
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
    NB_ARGS(1)
    auto cb = ARG_FUNC(0);

    event_callback = Napi::ThreadSafeFunction::New(env, cb, "zts_event_listener", 0, 1);

    int err = zts_init_set_event_handler(&event_handler);
    ERROR(err, "init_set_event_handler");

    return VOID;
}

// ### node ###

METHOD(node_start)
{
    NO_ARGS()

    int err = zts_node_start();
    ERROR(err, "node_start");

    return VOID;
}

METHOD(node_is_online)
{
    NO_ARGS()
    return BOOL(zts_node_is_online());
}

METHOD(node_get_id)
{
    NO_ARGS()

    auto id = zts_node_get_id();

    std::ostringstream ss;
    ss << std::hex << id;

    return STRING(ss.str());
}

METHOD(node_stop)
{
    NO_ARGS()

    int err = zts_node_stop();
    ERROR(err, "node_stop");

    if (event_callback)
        event_callback.Abort();

    return VOID;
}

METHOD(node_free)
{
    NO_ARGS()

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
    NB_ARGS(1)
    auto net_id = ARG_STRING(0);

    int err = zts_net_join(convert_net_id(net_id));
    ERROR(err, "net_join");

    return VOID;
}

METHOD(net_leave)
{
    NB_ARGS(1)
    auto net_id = ARG_STRING(0);

    int err = zts_net_leave(convert_net_id(net_id));
    ERROR(err, "net_leave");

    return VOID;
}

METHOD(net_transport_is_ready)
{
    NB_ARGS(1)
    auto net_id = ARG_STRING(0);

    return BOOL(zts_net_transport_is_ready(convert_net_id(net_id)));
}

// ### addr ###

METHOD(addr_get_str)
{
    NB_ARGS(2)
    auto net_id = ARG_STRING(0);
    auto ipv6 = ARG_BOOLEAN(1);

    auto family = ipv6 ? ZTS_AF_INET6 : ZTS_AF_INET;

    char addr[ZTS_IP_MAX_STR_LEN];

    int err = zts_addr_get_str(convert_net_id(net_id), family, addr, ZTS_IP_MAX_STR_LEN);
    ERROR(err, "addr_get_str");

    return STRING(addr);
}

// ### bsd ###

METHOD(bsd_socket)
{
    NB_ARGS(3)
    auto family = ARG_NUMBER(0);
    auto type = ARG_NUMBER(1);
    auto protocol = ARG_NUMBER(2);

    int fd = zts_bsd_socket(family, type, protocol);
    CHECK_ERRNO(fd, "bsd_socket");

    return NUMBER(fd);
}

METHOD(bsd_close)
{
    NB_ARGS(1)
    auto fd = ARG_NUMBER(0);

    int err = zts_bsd_close(fd);
    CHECK_ERRNO(err, "bsd_close");

    return VOID;
}

METHOD(bsd_send)
{
    NB_ARGS(4)
    int fd = ARG_NUMBER(0);
    auto data = ARG_UINT8ARRAY(1);
    int flags = ARG_NUMBER(2);
    auto cb = ARG_FUNC(3);

    int size = data.ByteLength();
    auto data_vec = new std::vector<uint8_t>();
    data_vec->insert(data_vec->begin(), data.Data(), data.Data() + size);

    auto execute = [fd, data_vec, size, flags]() { return zts_bsd_send(fd, data_vec->data(), size, flags); };
    auto on_destroy = [data_vec]() { delete data_vec; };
    auto worker = new AsyncLambda(cb, "bsd_send", execute, on_destroy);
    worker->Queue();

    return VOID;
}

METHOD(bsd_recv)
{
    NB_ARGS(4)
    auto fd = ARG_NUMBER(0);
    int n = ARG_NUMBER(1);
    auto flags = ARG_NUMBER(2);
    auto cb = ARG_FUNC(3);

    auto worker = new BsdRecvWorker(cb, fd, n, flags);
    worker->Queue();

    return VOID;
}

// ### no namespace socket stuff

METHOD(bind)
{
    NB_ARGS(3)
    auto fd = ARG_NUMBER(0);
    std::string ipstr = ARG_STRING(1);
    int port = ARG_NUMBER(2);

    int err = zts_bind(fd, ipstr.c_str(), port);
    CHECK_ERRNO(err, "bind");

    return VOID;
}

METHOD(listen)
{
    NB_ARGS(2)
    auto fd = ARG_NUMBER(0);
    auto backlog = ARG_NUMBER(1);

    int err = zts_listen(fd, backlog);
    CHECK_ERRNO(err, "listen");

    return VOID;
}

METHOD(accept)
{
    NB_ARGS(2)
    int fd = ARG_NUMBER(0);
    auto cb = ARG_FUNC(1);

    auto worker = new AsyncLambda(
        cb,
        "accept",
        [fd]() {
            char remote_addr[ZTS_IP_MAX_STR_LEN];
            unsigned short port;
            return zts_accept(fd, remote_addr, ZTS_IP_MAX_STR_LEN, &port);
        },
        []() {});
    worker->Queue();

    return VOID;
}

METHOD(connect)
{
    NB_ARGS(5)
    int fd = ARG_NUMBER(0);
    std::string ipstr = ARG_STRING(1);
    int port = ARG_NUMBER(2);
    int timeout = ARG_NUMBER(3);
    auto cb = ARG_FUNC(4);

    auto worker = new AsyncLambda(
        cb,
        "connect",
        [=]() { return zts_connect(fd, ipstr.c_str(), port, timeout); },
        []() {});
    worker->Queue();

    return VOID;
}

METHOD(shutdown_rd)
{
    NB_ARGS(1)
    auto fd = ARG_NUMBER(0);

    int err = zts_shutdown_rd(fd);
    CHECK_ERRNO(err, "shutdown_rd");

    return VOID;
}

METHOD(shutdown_wr)
{
    NB_ARGS(1)
    auto fd = ARG_NUMBER(0);

    int err = zts_shutdown_wr(fd);
    CHECK_ERRNO(err, "shutdown_wr");

    return VOID;
}

METHOD(getpeername)
{
    NB_ARGS(1)
    auto fd = ARG_NUMBER(0);

    char addr[ZTS_IP_MAX_STR_LEN];
    unsigned short port;

    int err = zts_getpeername(fd, addr, ZTS_IP_MAX_STR_LEN, &port);
    CHECK_ERRNO(err, "getpeername");

    return OBJECT({
        ADD_FIELD("address", STRING(addr));
        ADD_FIELD("port", NUMBER(port));
    });
}

METHOD(getsockname)
{
    NB_ARGS(1)
    auto fd = ARG_NUMBER(0);

    char addr[ZTS_IP_MAX_STR_LEN];
    unsigned short port;

    int err = zts_getsockname(fd, addr, ZTS_IP_MAX_STR_LEN, &port);
    CHECK_ERRNO(err, "getsockname");

    return OBJECT({
        ADD_FIELD("address", STRING(addr));
        ADD_FIELD("port", NUMBER(port))
    });
}

METHOD(set_recv_timeout)
{
    NB_ARGS(3)
    auto fd = ARG_NUMBER(0);
    auto seconds = ARG_NUMBER(1);
    auto microseconds = ARG_NUMBER(2);

    int err = zts_set_recv_timeout(fd, seconds, microseconds);
    CHECK_ERRNO(err, "set_recv_timeout");

    return VOID;
}

METHOD(set_send_timeout)
{
    NB_ARGS(3)
    auto fd = ARG_NUMBER(0);
    auto seconds = ARG_NUMBER(1);
    auto microseconds = ARG_NUMBER(2);

    int err = zts_set_send_timeout(fd, seconds, microseconds);
    CHECK_ERRNO(err, "set_recv_timeout");

    return VOID;
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

    // bsd
    EXPORT(bsd_socket);
    EXPORT(bsd_close);
    EXPORT(bsd_send);
    EXPORT(bsd_recv);

    // no ns socket
    EXPORT(bind);
    EXPORT(listen);
    EXPORT(accept);
    EXPORT(connect);
    EXPORT(shutdown_rd);
    EXPORT(shutdown_wr);
    EXPORT(getpeername);
    EXPORT(getsockname);
    EXPORT(set_recv_timeout);
    EXPORT(set_send_timeout);


    INIT_CLASS(TCP::Socket);
    INIT_CLASS(UDP::Socket);
}
