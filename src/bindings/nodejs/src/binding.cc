#include "ZeroTierSockets.h"
#include "async.cc"
#include "asynclambda.h"

#include <napi.h>

using namespace Napi;

#define VOID() return env.Undefined();

#define NO_ARGS() Env env = info.Env();

#define NB_ARGS(N)                                                                                                     \
    Env env = info.Env();                                                                                              \
    if (info.Length() < N) {                                                                                           \
        TypeError::New(env, "Wrong number of arguments. Expected: " #N).ThrowAsJavaScriptException();                  \
        VOID();                                                                                        \
    }

#define ARG_FUNC(POS, NAME)                                                                                            \
    if (! info[POS].IsFunction()) {                                                                                    \
        TypeError::New(env, "Argument at position " #POS "should be a function.").ThrowAsJavaScriptException();        \
        VOID();                                                                                             \
    }                                                                                                                  \
    auto NAME = info[POS].As<Function>();

#define ARG_INT32(POS, NAME)                                                                                           \
    if (! info[POS].IsNumber()) {                                                                                      \
        TypeError::New(env, "Argument at position " #POS "should be a number.").ThrowAsJavaScriptException();          \
        VOID();                                                                                             \
    }                                                                                                                  \
    auto NAME = info[POS].As<Number>().Int32Value();

#define ARG_STRING(POS, NAME)  auto NAME = std::string(info[POS].ToString());
#define ARG_BOOLEAN(POS, NAME) auto NAME = info[POS].ToBoolean();

#define ARG_UINT64(POS, NAME)                                                                                          \
    if (! info[POS].IsBigInt()) {                                                                                      \
        TypeError::New(env, "Argument at position " #POS "should be a BigInt.").ThrowAsJavaScriptException();          \
        VOID();                                                                                            \
    }                                                                                                                  \
    bool lossless;                                                                                                     \
    auto NAME = info[POS].As<BigInt>().Uint64Value(&lossless);

#define ARG_UINT8ARRAY(POS, NAME)                                                                                      \
    if (! info[POS].IsTypedArray()) {                                                                                  \
        TypeError::New(env, "Argument at position " #POS "should be a Uint8Array.").ThrowAsJavaScriptException();      \
        VOID();                                                                                            \
    }                                                                                                                  \
    auto NAME = info[POS].As<Uint8Array>();

#define EXPORT(F) exports[#F] = Function::New(env, F);

// ### init ###

Value init_from_storage(const CallbackInfo& info)
{
    NB_ARGS(1)
    ARG_STRING(0, configPath)

    int err = ZTS_ERR_OK;
    if ((err = zts_init_from_storage(std::string(configPath).c_str())) != ZTS_ERR_OK) {
        Error::New(env, "Unable to set config path.").ThrowAsJavaScriptException();
        VOID();
    }

    VOID();
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

Value init_set_event_handler(const CallbackInfo& info)
{
    NB_ARGS(1)
    ARG_FUNC(0, cb)

    event_callback = ThreadSafeFunction::New(env, cb, "zts_event_listener", 0, 1);

    int err = ZTS_ERR_OK;
    if ((err = zts_init_set_event_handler(&event_handler)) != ZTS_ERR_OK) {
        Error::New(env, "Unable to start service.").ThrowAsJavaScriptException();
        VOID();
    }

    VOID();
}

// ### node ###

Value node_start(const CallbackInfo& info)
{
    NO_ARGS()
    int err = ZTS_ERR_OK;

    if ((err = zts_node_start()) != ZTS_ERR_OK) {
        Error::New(env, "Unable to start service.").ThrowAsJavaScriptException();
        VOID();
    }

    VOID();
}

Value node_is_online(const CallbackInfo& info)
{
    return Boolean::New(info.Env(), zts_node_is_online());
}

Value node_get_id(const CallbackInfo& info)
{
    NO_ARGS()
    auto id = zts_node_get_id();

    return BigInt::New(env, id);
}

Value node_stop(const CallbackInfo& info)
{
    NO_ARGS()
    int err = ZTS_ERR_OK;

    if ((err = zts_node_stop()) != ZTS_ERR_OK) {
        Error::New(env, "Unable to stop service.").ThrowAsJavaScriptException();
        VOID();
    }

    VOID();
}

Value node_free(const CallbackInfo& info)
{
    NO_ARGS()
    int err = ZTS_ERR_OK;

    if ((err = zts_node_free()) != ZTS_ERR_OK) {
        Error::New(env, "Unable to free service.").ThrowAsJavaScriptException();
        VOID();
    }

    VOID();
}

// ### net ###

Value net_join(const CallbackInfo& info)
{
    NB_ARGS(1)
    ARG_UINT64(0, net_id)

    int err;
    if ((err = zts_net_join(net_id)) != ZTS_ERR_OK) {
        Error::New(env, "Unable to join network.").ThrowAsJavaScriptException();
        VOID();
    }

    VOID();
}

Value net_leave(const CallbackInfo& info)
{
    NB_ARGS(1)
    ARG_UINT64(0, net_id)

    int err;
    if ((err = zts_net_leave(net_id)) != ZTS_ERR_OK) {
        Error::New(env, "Error leaving network.").ThrowAsJavaScriptException();
        VOID();
    }

    VOID();
}

Value net_transport_is_ready(const CallbackInfo& info)
{
    NB_ARGS(1)
    ARG_UINT64(0, net_id)

    return Boolean::New(env, zts_net_transport_is_ready(net_id));
}

// ### addr ###

Value addr_get_str(const CallbackInfo& info)
{
    NB_ARGS(2)
    ARG_UINT64(0, net_id)
    ARG_BOOLEAN(1, ipv6)

    auto family = ipv6 ? ZTS_AF_INET6 : ZTS_AF_INET;

    char addr[ZTS_IP_MAX_STR_LEN];

    int err;
    if ((err = zts_addr_get_str(net_id, family, addr, ZTS_IP_MAX_STR_LEN)) != ZTS_ERR_OK) {
        Error::New(env, "Unable to get ip address.").ThrowAsJavaScriptException();
        VOID();
    }

    return String::New(env, addr);
}

// ### bsd ###

Value bsd_socket(const CallbackInfo& info)
{
    NB_ARGS(3)
    ARG_BOOLEAN(0, ipv6)
    ARG_INT32(1, type)
    ARG_INT32(2, protocol)

    int fd;
    if ((fd = zts_bsd_socket(ipv6 ? ZTS_AF_INET6 : ZTS_AF_INET, type, protocol)) < 0) {
        Error::New(env, "Unable to start service.").ThrowAsJavaScriptException();
        VOID();
    }

    return Number::New(env, fd);
}

Value bsd_close(const CallbackInfo& info)
{
    NB_ARGS(1)
    ARG_INT32(0, fd)

    int err;
    if ((err = zts_bsd_close(fd)) < 0) {
        Error::New(env, "Error when sending.").ThrowAsJavaScriptException();
        VOID();
    }

    VOID();
}

Value bsd_send(const CallbackInfo& info)
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
    
    VOID();
}

Value bsd_recv(const CallbackInfo& info)
{
    NB_ARGS(4)
    ARG_INT32(0, fd)
    ARG_INT32(1, n)
    ARG_INT32(2, flags)
    ARG_FUNC(3, cb)

    auto worker = new BsdRecvWorker(cb, fd, n, flags);
    worker->Queue();

    VOID();
}

// ### no namespace socket stuff

Value bind(const CallbackInfo& info)
{
    NB_ARGS(3)
    ARG_INT32(0, fd)
    ARG_STRING(1, ipstr)
    ARG_INT32(2, port)

    int err;
    if ((err = zts_bind(fd, std::string(ipstr).c_str(), port)) < 0) {
        Error::New(env, "Error when binding.").ThrowAsJavaScriptException();
        VOID();
    }

    VOID();
}

Value listen(const CallbackInfo& info)
{
    NB_ARGS(2)
    ARG_INT32(0, fd)
    ARG_INT32(1, backlog)

    int err;
    if ((err = zts_listen(fd, backlog)) < 0) {
        Error::New(env, "Error when listening.").ThrowAsJavaScriptException();
        VOID();
    }

    VOID();
}

Value accept(const CallbackInfo& info)
{
    NB_ARGS(2)
    ARG_INT32(0, fd)
    ARG_FUNC(1, cb)

    auto worker = new AsyncLambda(cb, "accept", [fd]() {
        char remote_addr[ZTS_IP_MAX_STR_LEN];
        unsigned short port;
        return zts_accept(fd, remote_addr, ZTS_IP_MAX_STR_LEN, &port);
    }, []() {});
    worker->Queue();

    VOID();
}

Value connect(const CallbackInfo& info)
{
    NB_ARGS(5)
    ARG_INT32(0, fd)
    ARG_STRING(1, ipstr)
    ARG_INT32(2, port)
    ARG_INT32(3, timeout)
    ARG_FUNC(4, cb)

    auto worker = new AsyncLambda(cb, "connect", [=]() {
        return zts_connect(fd, ipstr.c_str(), port, timeout);
    }, [](){});
    worker->Queue();

    VOID();
}

Value shutdown_wr(const CallbackInfo& info) {
    NB_ARGS(1)
    ARG_INT32(0, fd)

    int err;
    if ((err = zts_shutdown_wr(fd)) < 0) {
        Error::New(env, "Error shutdown_wr socket.").ThrowAsJavaScriptException();
        VOID();
    }

    VOID();
}

// ### util ###

Value util_delay(const CallbackInfo& info)
{
    NB_ARGS(1)
    ARG_INT32(0, delay)

    zts_util_delay(delay);

    VOID();
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

    // no ns socket
    EXPORT(bind)
    EXPORT(listen)
    EXPORT(accept)
    EXPORT(connect)
    EXPORT(shutdown_wr)

    // util
    EXPORT(util_delay)

    return exports;
}

NODE_API_MODULE(hello, Init)