#include "ZeroTierSockets.h"
#include "async.cc"

#include <napi.h>

using namespace Napi;

#define NO_ARGS() Env env = info.Env();

#define NB_ARGS(N)                                                                                                     \
    Env env = info.Env();                                                                                              \
    if (info.Length() < N) {                                                                                           \
        TypeError::New(env, "Wrong number of arguments. Expected: " #N).ThrowAsJavaScriptException();                  \
        return env.Null();                                                                                             \
    }

#define ARG_FUNC(POS, NAME)                                                                                            \
    if (! info[POS].IsFunction()) {                                                                                    \
        TypeError::New(env, "Argument at position " #POS "should be a function.").ThrowAsJavaScriptException();        \
        return env.Null();                                                                                             \
    }                                                                                                                  \
    auto NAME = info[POS].As<Function>();

#define ARG_INT32(POS, NAME)                                                                                           \
    if (! info[POS].IsNumber()) {                                                                                      \
        TypeError::New(env, "Argument at position " #POS "should be a number.").ThrowAsJavaScriptException();          \
        return env.Null();                                                                                             \
    }                                                                                                                  \
    auto NAME = info[POS].As<Number>().Int32Value();

#define ARG_STRING(POS, NAME)  auto NAME = info[POS].ToString();
#define ARG_BOOLEAN(POS, NAME) auto NAME = info[POS].ToBoolean();

#define ARG_UINT64(POS, NAME)                                                                                          \
    if (! info[POS].IsBigInt()) {                                                                                      \
        TypeError::New(env, "Argument at position " #POS "should be a BigInt.").ThrowAsJavaScriptException();          \
        return env.Null();                                                                                             \
    }                                                                                                                  \
    bool lossless;                                                                                                     \
    auto NAME = info[POS].As<BigInt>().Uint64Value(&lossless);

#define ARG_UINT8ARRAY(POS, NAME)                                                                                      \
    if (! info[POS].IsTypedArray()) {                                                                                  \
        TypeError::New(env, "Argument at position " #POS "should be a Uint8Array.").ThrowAsJavaScriptException();      \
        return env.Null();                                                                                             \
    }                                                                                                                  \
    auto NAME = info[POS].As<Uint8Array>();

// ### init ###

Value init_from_storage(const CallbackInfo& info)
{
    NB_ARGS(1)
    ARG_STRING(0, configPath)

    int err = ZTS_ERR_OK;
    if ((err = zts_init_from_storage(std::string(configPath).c_str())) != ZTS_ERR_OK) {
        Error::New(env, "Unable to set config path.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return env.Null();
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
        return env.Null();
    }

    return env.Null();
}

// ### node ###

Value node_start(const CallbackInfo& info)
{
    NO_ARGS()
    int err = ZTS_ERR_OK;

    if ((err = zts_node_start()) != ZTS_ERR_OK) {
        Error::New(env, "Unable to start service.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return env.Null();
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
        return env.Null();
    }

    return env.Null();
}

Value node_free(const CallbackInfo& info)
{
    NO_ARGS()
    int err = ZTS_ERR_OK;

    if ((err = zts_node_free()) != ZTS_ERR_OK) {
        Error::New(env, "Unable to free service.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return env.Null();
}

// ### net ###

Value net_join(const CallbackInfo& info)
{
    NB_ARGS(1)
    ARG_UINT64(0, net_id)

    int err;
    if ((err = zts_net_join(net_id)) != ZTS_ERR_OK) {
        Error::New(env, "Unable to join network.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return env.Null();
}

Value net_leave(const CallbackInfo& info)
{
    NB_ARGS(1)
    ARG_UINT64(0, net_id)

    int err;
    if ((err = zts_net_leave(net_id)) != ZTS_ERR_OK) {
        Error::New(env, "Error leaving network.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return env.Null();
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
        return env.Null();
    }

    return String::New(env, addr);
}

// ### bsd ###

Value bsd_socket(const CallbackInfo& info)
{
    NB_ARGS(3)
    ARG_INT32(0, family)
    ARG_INT32(0, type)
    ARG_INT32(0, protocol)

    int fd;
    if ((fd = zts_bsd_socket(family, type, protocol)) < 0) {
        Error::New(env, "Unable to start service.").ThrowAsJavaScriptException();
        return env.Null();
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
        return env.Null();
    }

    return env.Null();
}

Value bsd_send(const CallbackInfo& info)
{
    NB_ARGS(3)
    ARG_INT32(0, fd)
    ARG_UINT8ARRAY(1, data)
    ARG_INT32(2, flags)

    int bytesWritten;
    if ((bytesWritten = zts_send(fd, data.Data(), data.ByteLength(), flags)) < 0) {
        Error::New(env, "Error when sending.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return Number::New(env, bytesWritten);
}

Value bsd_recv(const CallbackInfo& info)
{
    NB_ARGS(3)
    ARG_INT32(0, fd)
    ARG_UINT8ARRAY(1, data)
    ARG_INT32(2, flags)

    int bytes_received;
    if ((bytes_received = zts_recv(fd, data.Data(), data.ByteLength(), flags)) < 0) {
        Error::New(env, "Error when receiving.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return Number::New(env, bytes_received);
}

Value bsd_recv_cb(const CallbackInfo& info)
{
    NB_ARGS(4)
    ARG_INT32(0, fd)
    ARG_INT32(1, n)
    ARG_INT32(2, flags)
    ARG_FUNC(3, cb)

    auto worker = new BsdRecvWorker(cb, fd, n, flags);
    worker->Queue();

    return env.Null();
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
        return env.Null();
    }

    return env.Null();
}

Value listen(const CallbackInfo& info)
{
    NB_ARGS(2)
    ARG_INT32(0, fd)
    ARG_INT32(1, backlog)

    int err;
    if ((err = zts_listen(fd, backlog)) < 0) {
        Error::New(env, "Error when binding.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return env.Null();
}

Value accept(const CallbackInfo& info)
{
    NB_ARGS(1)
    ARG_INT32(0, fd)

    char remote_addr[ZTS_IP_MAX_STR_LEN];
    unsigned short port;

    int new_fd;
    if ((new_fd = zts_accept(fd, remote_addr, ZTS_IP_MAX_STR_LEN, &port)) < 0) {
        Error::New(env, "Error when binding.").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto obj = Object::New(env);
    obj["fd"] = Number::New(env, new_fd);
    obj["address"] = String::New(env, remote_addr);
    obj["port"] = Number::New(env, port);

    return obj;
}

Value connect(const CallbackInfo& info)
{
    NB_ARGS(4)
    ARG_INT32(0, fd)
    ARG_STRING(1, ipstr)
    ARG_INT32(2, port)
    ARG_INT32(3, timeout)

    int err;
    if ((err = zts_connect(fd, std::string(ipstr).c_str(), port, timeout)) != ZTS_ERR_OK) {
        Error::New(env, "Error when connecting.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return env.Null();
}

// ### util ###

Value util_delay(const CallbackInfo& info)
{
    NB_ARGS(1)
    ARG_INT32(0, delay)

    zts_util_delay(delay);

    return env.Null();
}

// NAPI initialiser

Object Init(Env env, Object exports)
{
    // init
    exports.Set("init_from_storage", Function::New(env, init_from_storage));
    exports.Set("init_set_event_handler", Function::New(env, init_set_event_handler));

    // node
    exports.Set("node_start", Function::New(env, node_start));
    exports.Set("node_is_online", Function::New(env, node_is_online));
    exports.Set("node_get_id", Function::New(env, node_get_id));
    exports.Set("node_stop", Function::New(env, node_stop));
    exports.Set("node_free", Function::New(env, node_free));

    // net
    exports.Set("net_join", Function::New(env, net_join));
    exports.Set("net_leave", Function::New(env, net_leave));
    exports.Set("net_transport_is_ready", Function::New(env, net_transport_is_ready));

    // addr
    exports.Set("addr_get_str", Function::New(env, addr_get_str));

    // bsd
    exports.Set("bsd_socket", Function::New(env, bsd_socket));
    exports.Set("bsd_close", Function::New(env, bsd_close));
    exports.Set("bsd_send", Function::New(env, bsd_send));
    exports.Set("bsd_recv", Function::New(env, bsd_recv));

    exports["bsd_recv_cb"] = Function::New(env, bsd_recv_cb);

    // no ns socket
    exports["bind"] = Function::New(env, bind);
    exports["listen"] = Function::New(env, listen);
    exports["accept"] = Function::New(env, accept);
    exports["connect"] = Function::New(env, connect);

    // util
    exports.Set("util_delay", Function::New(env, util_delay));

    return exports;
}

NODE_API_MODULE(hello, Init)