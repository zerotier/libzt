#include "ZeroTierSockets.h"
#include "async.cc"
#include "asynclambda.h"
#include "lwip/tcp.h"
#include "macros.h"

#include <napi.h>

#define ERROR(ERR, FUN)                                                                                                \
    if (ERR < 0) {                                                                                                     \
        auto error = Napi::Error::New(info.Env(), "Error during " FUN " call");                                        \
        error.Set(STRING("code"), NUMBER(ERR));                                                                        \
        throw error;                                                                                                   \
    }

#define CHECK_ERRNO(ERR, FUN)                                                                                          \
    if (ERR < 0) {                                                                                                     \
        auto error = Napi::Error::New(info.Env(), "Error during " FUN " call");                                        \
        error.Set(STRING("code"), NUMBER(ERR));                                                                        \
        error.Set(STRING("errno"), NUMBER(zts_errno));                                                                 \
        throw error;                                                                                                   \
    }

// ### init ###

METHOD(init_from_storage)
{
    NB_ARGS(1)
    ARG_STRING(0, configPath)

    int err = zts_init_from_storage(std::string(configPath).c_str());
    ERROR(err, "init_from_storage")

    return VOID;
}

Napi::ThreadSafeFunction event_callback;

void event_handler(void* msgPtr)
{
    event_callback.Acquire();

    zts_event_msg_t* msg = (zts_event_msg_t*)msgPtr;
    int event = msg->event_code;
    auto cb = [=](Napi::Env env, Napi::Function jsCallback) { jsCallback.Call({ NUMBER(event) }); };

    event_callback.NonBlockingCall(cb);

    event_callback.Release();
}

METHOD(init_set_event_handler)
{
    NB_ARGS(1)
    ARG_FUNC(0, cb)

    event_callback = Napi::ThreadSafeFunction::New(env, cb, "zts_event_listener", 0, 1);

    int err = zts_init_set_event_handler(&event_handler);
    ERROR(err, "init_set_event_handler")

    return VOID;
}

// ### node ###

METHOD(node_start)
{
    NO_ARGS()

    int err = zts_node_start();
    ERROR(err, "node_start")

    return VOID;
}

METHOD(node_is_online)
{
    return BOOL(zts_node_is_online());
}

METHOD(node_get_id)
{
    NO_ARGS()

    auto id = zts_node_get_id();

    return BIGINT(id);
}

METHOD(node_stop)
{
    NO_ARGS()

    int err = zts_node_stop();
    ERROR(err, "node_stop")

    return VOID;
}

METHOD(node_free)
{
    NO_ARGS()

    int err = zts_node_free();
    ERROR(err, "node_free")

    return VOID;
}

// ### net ###

METHOD(net_join)
{
    NB_ARGS(1)
    ARG_UINT64(0, net_id)

    int err = zts_net_join(net_id);
    ERROR(err, "net_join")

    return VOID;
}

METHOD(net_leave)
{
    NB_ARGS(1)
    ARG_UINT64(0, net_id)

    int err = zts_net_leave(net_id);
    ERROR(err, "net_leave")

    return VOID;
}

METHOD(net_transport_is_ready)
{
    NB_ARGS(1)
    ARG_UINT64(0, net_id)

    return BOOL(zts_net_transport_is_ready(net_id));
}

// ### addr ###

METHOD(addr_get_str)
{
    NB_ARGS(2)
    ARG_UINT64(0, net_id)
    ARG_BOOLEAN(1, ipv6)

    auto family = ipv6 ? ZTS_AF_INET6 : ZTS_AF_INET;

    char addr[ZTS_IP_MAX_STR_LEN];

    int err = zts_addr_get_str(net_id, family, addr, ZTS_IP_MAX_STR_LEN);
    ERROR(err, "addr_get_str")

    return STRING(addr);
}

// ### bsd ###

METHOD(bsd_socket)
{
    NB_ARGS(3)
    ARG_INT32(0, family)
    ARG_INT32(1, type)
    ARG_INT32(2, protocol)

    int fd = zts_bsd_socket(family, type, protocol);
    CHECK_ERRNO(fd, "bsd_socket")

    return NUMBER(fd);
}

METHOD(bsd_close)
{
    NB_ARGS(1)
    ARG_INT32(0, fd)

    int err = zts_bsd_close(fd);
    CHECK_ERRNO(err, "bsd_close")

    return VOID;
}

METHOD(bsd_send)
{
    NB_ARGS(4)
    ARG_INT32(0, fd)
    ARG_UINT8ARRAY(1, data)
    ARG_INT32(2, flags)
    ARG_FUNC(3, cb)

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
    ARG_INT32(0, fd)
    ARG_INT32(1, n)
    ARG_INT32(2, flags)
    ARG_FUNC(3, cb)

    auto worker = new BsdRecvWorker(cb, fd, n, flags);
    worker->Queue();

    return VOID;
}

METHOD(bsd_sendto)
{
    NB_ARGS(4)
    ARG_INT32(0, fd)
    ARG_UINT8ARRAY(1, data)
    ARG_INT32(2, flags)
    ARG_STRING(3, ipaddr)
    ARG_INT32(4, port)
    ARG_FUNC(5, cb)

    int size = data.ByteLength();
    auto data_vec = new std::vector<uint8_t>();
    data_vec->insert(data_vec->begin(), data.Data(), data.Data() + size);

    struct zts_sockaddr_storage addr;
    zts_socklen_t addrlen = sizeof(struct zts_sockaddr_storage);
    zts_util_ipstr_to_saddr(ipaddr.c_str(), port, (struct zts_sockaddr*)&addr, &addrlen);

    auto execute = [fd, data_vec, size, flags, addr, addrlen]() {
        return zts_bsd_sendto(fd, data_vec->data(), size, flags, (struct zts_sockaddr*)&addr, addrlen);
    };
    auto on_destroy = [data_vec]() { delete data_vec; };
    auto worker = new AsyncLambda(cb, "bsd_send", execute, on_destroy);
    worker->Queue();

    return VOID;
}

METHOD(bsd_recvfrom)
{
    NB_ARGS(4)
    ARG_INT32(0, fd)
    ARG_INT32(1, n)
    ARG_INT32(2, flags)
    ARG_FUNC(3, cb)

    auto worker = new BsdRecvFromWorker(cb, fd, n, flags);
    worker->Queue();

    return VOID;
}

// ### no namespace socket stuff

METHOD(bind)
{
    NB_ARGS(3)
    ARG_INT32(0, fd)
    ARG_STRING(1, ipstr)
    ARG_INT32(2, port)

    int err = zts_bind(fd, std::string(ipstr).c_str(), port);
    CHECK_ERRNO(err, "bind")

    return VOID;
}

METHOD(listen)
{
    NB_ARGS(2)
    ARG_INT32(0, fd)
    ARG_INT32(1, backlog)

    int err = zts_listen(fd, backlog);
    CHECK_ERRNO(err, "listen")

    return VOID;
}

METHOD(accept)
{
    NB_ARGS(2)
    ARG_INT32(0, fd)
    ARG_FUNC(1, cb)

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
    ARG_INT32(0, fd)
    ARG_STRING(1, ipstr)
    ARG_INT32(2, port)
    ARG_INT32(3, timeout)
    ARG_FUNC(4, cb)

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
    ARG_INT32(0, fd)

    int err = zts_shutdown_rd(fd);
    CHECK_ERRNO(err, "shutdown_rd")

    return VOID;
}

METHOD(shutdown_wr)
{
    NB_ARGS(1)
    ARG_INT32(0, fd)

    int err = zts_shutdown_wr(fd);
    CHECK_ERRNO(err, "shutdown_wr")

    return VOID;
}

METHOD(getpeername)
{
    NB_ARGS(1)
    ARG_INT32(0, fd)

    char addr[ZTS_IP_MAX_STR_LEN];
    unsigned short port;

    int err = zts_getpeername(fd, addr, ZTS_IP_MAX_STR_LEN, &port);
    CHECK_ERRNO(err, "getpeername")

    return OBJECT(ADD_FIELD(address, STRING(addr)) ADD_FIELD(port, NUMBER(port)));
}

METHOD(getsockname)
{
    NB_ARGS(1)
    ARG_INT32(0, fd)

    char addr[ZTS_IP_MAX_STR_LEN];
    unsigned short port;

    int err = zts_getsockname(fd, addr, ZTS_IP_MAX_STR_LEN, &port);
    CHECK_ERRNO(err, "getsockname")

    return OBJECT(
        ADD_FIELD(address, STRING(addr));
        ADD_FIELD(port, NUMBER(port))
    );
}

METHOD(set_recv_timeout)
{
    NB_ARGS(3)
    ARG_INT32(0, fd)
    ARG_INT32(1, seconds)
    ARG_INT32(2, microseconds)

    int err = zts_set_recv_timeout(fd, seconds, microseconds);
    CHECK_ERRNO(err, "set_recv_timeout")

    return VOID;
}

METHOD(set_send_timeout)
{
    NB_ARGS(3)
    ARG_INT32(0, fd)
    ARG_INT32(1, seconds)
    ARG_INT32(2, microseconds)

    int err = zts_set_send_timeout(fd, seconds, microseconds);
    CHECK_ERRNO(err, "set_recv_timeout")

    return VOID;
}

// TODO destructor frees pcb block?
class TCP_PCB : public Napi::ObjectWrap<TCP_PCB> {
  public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports);
    TCP_PCB(CALLBACKINFO);
    struct tcp_pcb* pcb;
};

Napi::Object TCP_PCB::Init(Napi::Env env, Napi::Object exports)
{
    Napi::Function func = DefineClass(env, "TCP_Socket", {});

    return exports;
}

// NAPI initialiser

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    // init
    EXPORT(init_from_storage)
    EXPORT(init_set_event_handler)

    // node
    EXPORT(node_start)
    EXPORT(node_is_online)
    EXPORT(node_get_id)
    EXPORT(node_stop)
    EXPORT(node_free)

    // net
    EXPORT(net_join)
    EXPORT(net_leave)
    EXPORT(net_transport_is_ready)

    // addr
    EXPORT(addr_get_str)

    // bsd
    EXPORT(bsd_socket)
    EXPORT(bsd_close)
    EXPORT(bsd_send)
    EXPORT(bsd_recv)
    EXPORT(bsd_sendto)
    EXPORT(bsd_recvfrom)

    // no ns socket
    EXPORT(bind)
    EXPORT(listen)
    EXPORT(accept)
    EXPORT(connect)
    EXPORT(shutdown_rd)
    EXPORT(shutdown_wr)
    EXPORT(getpeername)
    EXPORT(getsockname)
    EXPORT(set_recv_timeout)
    EXPORT(set_send_timeout)

    return exports;
}

NODE_API_MODULE(zts, Init)
