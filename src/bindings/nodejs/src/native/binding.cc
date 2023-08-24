#include "ZeroTierSockets.h"
#include "async.cc"
#include "asynclambda.h"

#include <napi.h>

using namespace Napi;

#define CALLBACKINFO const CallbackInfo& info

#define VOID return env.Undefined();

#define NO_ARGS() Env env = info.Env();

#define NB_ARGS(N)                                                                                                     \
    Env env = info.Env();                                                                                              \
    if (info.Length() < N) {                                                                                           \
        TypeError::New(env, "Wrong number of arguments. Expected: " #N).ThrowAsJavaScriptException();                  \
        VOID;                                                                                                          \
    }

#define ARG_FUNC(POS, NAME)                                                                                            \
    if (! info[POS].IsFunction()) {                                                                                    \
        TypeError::New(env, "Argument at position " #POS "should be a function.").ThrowAsJavaScriptException();        \
        VOID;                                                                                                          \
    }                                                                                                                  \
    auto NAME = info[POS].As<Function>();

#define ARG_INT32(POS, NAME)                                                                                           \
    if (! info[POS].IsNumber()) {                                                                                      \
        TypeError::New(env, "Argument at position " #POS "should be a number.").ThrowAsJavaScriptException();          \
        VOID;                                                                                                          \
    }                                                                                                                  \
    auto NAME = info[POS].As<Number>().Int32Value();

#define ARG_STRING(POS, NAME)  auto NAME = std::string(info[POS].ToString());
#define ARG_BOOLEAN(POS, NAME) auto NAME = info[POS].ToBoolean();

#define ARG_UINT64(POS, NAME)                                                                                          \
    if (! info[POS].IsBigInt()) {                                                                                      \
        TypeError::New(env, "Argument at position " #POS "should be a BigInt.").ThrowAsJavaScriptException();          \
        VOID;                                                                                                          \
    }                                                                                                                  \
    bool lossless;                                                                                                     \
    auto NAME = info[POS].As<BigInt>().Uint64Value(&lossless);

#define ARG_UINT8ARRAY(POS, NAME)                                                                                      \
    if (! info[POS].IsTypedArray()) {                                                                                  \
        TypeError::New(env, "Argument at position " #POS "should be a Uint8Array.").ThrowAsJavaScriptException();      \
        VOID;                                                                                                          \
    }                                                                                                                  \
    auto NAME = info[POS].As<Uint8Array>();

#define EXPORT(F) exports[#F] = Function::New(env, F);

#define CHECK(ERR, FUN)                                                                                                \
    if (ERR < 0) {                                                                                                     \
        auto error = Error::New(env, "Error during " FUN " call");                                                     \
        error.Set(String::New(env, "code"), Number::New(env, ERR));                                                    \
        error.ThrowAsJavaScriptException();                                                                            \
        VOID;                                                                                                          \
    }

#define CHECK_ERRNO(ERR, FUN)                                                                                          \
    if (ERR < 0) {                                                                                                     \
        auto error = Error::New(env, "Error during " FUN " call");                                                     \
        error.Set(String::New(env, "code"), Number::New(env, ERR));                                                    \
        error.Set(String::New(env, "errno"), Number::New(env, zts_errno));                                             \
        error.ThrowAsJavaScriptException();                                                                            \
        VOID;                                                                                                          \
    }

// ### init ###

Value init_from_storage(CALLBACKINFO)
{
    NB_ARGS(1)
    ARG_STRING(0, configPath)

    int err = zts_init_from_storage(std::string(configPath).c_str());
    CHECK(err, "init_from_storage")

    VOID;
}

ThreadSafeFunction event_callback;

void event_handler(void* msgPtr)
{
    event_callback.Acquire();

    zts_event_msg_t* msg = (zts_event_msg_t*)msgPtr;
    double event = msg->event_code;
    auto cb = [=](Env env, Function jsCallback) { jsCallback.Call({ Number::New(env, event) }); };

    event_callback.NonBlockingCall(cb);

    event_callback.Release();
}

Value init_set_event_handler(CALLBACKINFO)
{
    NB_ARGS(1)
    ARG_FUNC(0, cb)

    event_callback = ThreadSafeFunction::New(env, cb, "zts_event_listener", 0, 1);

    int err = zts_init_set_event_handler(&event_handler);
    CHECK(err, "init_set_event_handler")

    VOID;
}

// ### node ###

Value node_start(CALLBACKINFO)
{
    NO_ARGS()

    int err = zts_node_start();
    CHECK(err, "node_start")

    VOID;
}

Value node_is_online(CALLBACKINFO)
{
    return Boolean::New(info.Env(), zts_node_is_online());
}

Value node_get_id(CALLBACKINFO)
{
    NO_ARGS()

    auto id = zts_node_get_id();

    return BigInt::New(env, id);
}

Value node_stop(CALLBACKINFO)
{
    NO_ARGS()

    int err = zts_node_stop();
    CHECK(err, "node_stop")

    VOID;
}

Value node_free(CALLBACKINFO)
{
    NO_ARGS()

    int err = zts_node_free();
    CHECK(err, "node°free")

    VOID;
}

// ### net ###

Value net_join(CALLBACKINFO)
{
    NB_ARGS(1)
    ARG_UINT64(0, net_id)

    int err = zts_net_join(net_id);
    CHECK(err, "net_join")

    VOID;
}

Value net_leave(CALLBACKINFO)
{
    NB_ARGS(1)
    ARG_UINT64(0, net_id)

    int err = zts_net_leave(net_id);
    CHECK(err, "net_leave")

    VOID;
}

Value net_transport_is_ready(CALLBACKINFO)
{
    NB_ARGS(1)
    ARG_UINT64(0, net_id)

    return Boolean::New(env, zts_net_transport_is_ready(net_id));
}

// ### addr ###

Value addr_get_str(CALLBACKINFO)
{
    NB_ARGS(2)
    ARG_UINT64(0, net_id)
    ARG_BOOLEAN(1, ipv6)

    auto family = ipv6 ? ZTS_AF_INET6 : ZTS_AF_INET;

    char addr[ZTS_IP_MAX_STR_LEN];

    int err = zts_addr_get_str(net_id, family, addr, ZTS_IP_MAX_STR_LEN);
    CHECK(err, "addr_get_str")

    return String::New(env, addr);
}

// ### bsd ###

Value bsd_socket(CALLBACKINFO)
{
    NB_ARGS(3)
    ARG_INT32(0, family)
    ARG_INT32(1, type)
    ARG_INT32(2, protocol)

    int fd = zts_bsd_socket(family, type, protocol);
    CHECK_ERRNO(fd, "bsd_socket")

    return Number::New(env, fd);
}

Value bsd_close(CALLBACKINFO)
{
    NB_ARGS(1)
    ARG_INT32(0, fd)

    int err = zts_bsd_close(fd);
    CHECK_ERRNO(err, "bsd_close")

    VOID;
}

Value bsd_send(CALLBACKINFO)
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

    VOID;
}

Value bsd_recv(CALLBACKINFO)
{
    NB_ARGS(4)
    ARG_INT32(0, fd)
    ARG_INT32(1, n)
    ARG_INT32(2, flags)
    ARG_FUNC(3, cb)

    auto worker = new BsdRecvWorker(cb, fd, n, flags);
    worker->Queue();

    VOID;
}

Value bsd_sendto(CALLBACKINFO)
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

    VOID;
}

Value bsd_recvfrom(CALLBACKINFO)
{
    NB_ARGS(4)
    ARG_INT32(0, fd)
    ARG_INT32(1, n)
    ARG_INT32(2, flags)
    ARG_FUNC(3, cb)

    auto worker = new BsdRecvFromWorker(cb, fd, n, flags);
    worker->Queue();

    VOID;
}

// ### no namespace socket stuff

Value bind(CALLBACKINFO)
{
    NB_ARGS(3)
    ARG_INT32(0, fd)
    ARG_STRING(1, ipstr)
    ARG_INT32(2, port)

    int err = zts_bind(fd, std::string(ipstr).c_str(), port);
    CHECK_ERRNO(err, "bind")

    VOID;
}

Value listen(CALLBACKINFO)
{
    NB_ARGS(2)
    ARG_INT32(0, fd)
    ARG_INT32(1, backlog)

    int err = zts_listen(fd, backlog);
    CHECK_ERRNO(err, "listen")

    VOID;
}

Value accept(CALLBACKINFO)
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

    VOID;
}

Value connect(CALLBACKINFO)
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

    VOID;
}

Value shutdown_rd(CALLBACKINFO)
{
    NB_ARGS(1)
    ARG_INT32(0, fd)

    int err = zts_shutdown_rd(fd);
    CHECK_ERRNO(err, "shutdown_rd")

    VOID;
}

Value shutdown_wr(CALLBACKINFO)
{
    NB_ARGS(1)
    ARG_INT32(0, fd)

    int err = zts_shutdown_wr(fd);
    CHECK_ERRNO(err, "shutdown_wr")

    VOID;
}

Value getpeername(CALLBACKINFO)
{
    NB_ARGS(1)
    ARG_INT32(0, fd)

    char addr[ZTS_IP_MAX_STR_LEN];
    unsigned short port;

    int err = zts_getpeername(fd, addr, ZTS_IP_MAX_STR_LEN, &port);
    CHECK_ERRNO(err, "getsockname")

    auto obj = Object::New(env);
    obj["address"] = String::New(env, addr);
    obj["port"] = Number::New(env, port);

    return obj;
}

Value getsockname(CALLBACKINFO)
{
    NB_ARGS(1)
    ARG_INT32(0, fd)

    char addr[ZTS_IP_MAX_STR_LEN];
    unsigned short port;

    int err = zts_getsockname(fd, addr, ZTS_IP_MAX_STR_LEN, &port);
    CHECK_ERRNO(err, "getsockname")

    auto obj = Object::New(env);
    obj["address"] = String::New(env, addr);
    obj["port"] = Number::New(env, port);

    return obj;
}

Value set_recv_timeout(CALLBACKINFO)
{
    NB_ARGS(3)
    ARG_INT32(0, fd)
    ARG_INT32(1, seconds)
    ARG_INT32(2, microseconds)

    int err = zts_set_recv_timeout(fd, seconds, microseconds);
    CHECK_ERRNO(err, "set_recv_timeout")

    VOID;
}

Value set_send_timeout(CALLBACKINFO)
{
    NB_ARGS(3)
    ARG_INT32(0, fd)
    ARG_INT32(1, seconds)
    ARG_INT32(2, microseconds)

    int err = zts_set_send_timeout(fd, seconds, microseconds);
    CHECK_ERRNO(err, "set_recv_timeout")

    VOID;
}

// NAPI initialiser

Object Init(Env env, Object exports)
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

NODE_API_MODULE(hello, Init)