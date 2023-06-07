#include "ZeroTierSockets.h"

#include <napi.h>

using namespace Napi;


// ### init ###

Value init_from_storage(const CallbackInfo& info)
{
    Env env = info.Env();

    if (info.Length() < 1) {
        TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return env.Null();
    }
    auto configPath = std::string(info[0].ToString()).c_str();

    int err = ZTS_ERR_OK;
    if ((err = zts_init_from_storage(configPath)) != ZTS_ERR_OK) {
        Error::New(env, "Unable to set config path.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return env.Null();
}

ThreadSafeFunction event_callback;

void event_handler(void* msgPtr)
{
    event_callback.Acquire();

    zts_event_msg_t *msg = (zts_event_msg_t *)msgPtr;
    double event = msg->event_code;
    auto cb = [=]( Env env, Function jsCallback ) {
        jsCallback.Call({Number::New( env, event)});
    };

    event_callback.NonBlockingCall(cb);

    event_callback.Release();
}

Value init_set_event_handler(const CallbackInfo& info)
{
    Env env = info.Env();

    if (info.Length() < 1) {
        TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return env.Null();
    }
    if (! info[0].IsFunction()) {
        TypeError::New(env, "Callback should be a function").ThrowAsJavaScriptException();
        return env.Null();
    }
    auto cb = info[0].As<Function>();
    event_callback = ThreadSafeFunction::New(
        env,
        cb,
        "zts_event_listener",
        0,
        1
    );

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
    Env env = info.Env();

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

Value node_get_id(const CallbackInfo& info) {
    Env env = info.Env();

    auto id = zts_node_get_id();

    return BigInt::New(env, id);
}

Value node_stop(const CallbackInfo& info)
{
    Env env = info.Env();

    int err = ZTS_ERR_OK;

    if ((err = zts_node_stop()) != ZTS_ERR_OK) {
        Error::New(env, "Unable to stop service.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return env.Null();
}

Value node_free(const CallbackInfo& info)
{
    Env env = info.Env();

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
    Env env = info.Env();

    if (info.Length() < 1) {
        TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (! info[0].IsBigInt()) {
        TypeError::New(env, "Network Id should be a bigint").ThrowAsJavaScriptException();
        return env.Null();
    }

    bool lossless = true;
    auto net_id = info[0].As<BigInt>().Uint64Value(&lossless);

    int err;
    if ((err = zts_net_join(net_id)) != ZTS_ERR_OK) {
        Error::New(env, "Unable to join network.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return env.Null();
}

Value net_leave(const CallbackInfo& info)
{
    Env env = info.Env();

    if (info.Length() < 1) {
        TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (! info[0].IsBigInt()) {
        TypeError::New(env, "Network Id should be a bigint").ThrowAsJavaScriptException();
        return env.Null();
    }

    bool lossless = true;
    auto net_id = info[0].As<BigInt>().Uint64Value(&lossless);

    int err;
    if ((err = zts_net_leave(net_id)) != ZTS_ERR_OK) {
        Error::New(env, "Error leaving network.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return env.Null();
}

Value net_transport_is_ready(const CallbackInfo& info)
{
    Env env = info.Env();

    if (info.Length() < 1) {
        TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (! info[0].IsBigInt()) {
        TypeError::New(env, "Network Id should be a bigint").ThrowAsJavaScriptException();
        return env.Null();
    }

    bool lossless = true;
    auto net_id = info[0].As<BigInt>().Uint64Value(&lossless);

    return Boolean::New(env, zts_net_transport_is_ready(net_id));
}

// ### addr ###

Value addr_get_str(const CallbackInfo& info)
{
    Env env = info.Env();

    if (info.Length() < 2) {
        TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (! info[0].IsBigInt()) {
        TypeError::New(env, "Network should be a bigint.").ThrowAsJavaScriptException();
        return env.Null();
    }

    auto family = info[1].ToBoolean() ? ZTS_AF_INET6 : ZTS_AF_INET;

    bool lossless;
    auto net_id = info[0].As<BigInt>().Uint64Value(&lossless);

    char addr[ZTS_IP_MAX_STR_LEN];

    int err;
    if ((err = zts_addr_get_str(net_id, family, addr, ZTS_IP_MAX_STR_LEN)) != ZTS_ERR_OK) {
        Error::New(env, "Unable to get ip address.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return String::New(env, addr);
}

// ### bsd ###

Value bsd_socket(const CallbackInfo& info) {
    Env env = info.Env();

    if (info.Length() < 3) {
        TypeError::New(env, "Wrong number of arguments: family, type, protocol").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (! info[0].IsNumber() || !info[1].IsNumber() || ! info[2].IsNumber()) {
        TypeError::New(env, "Wrong type of arguments: family, type, protocol").ThrowAsJavaScriptException();
        return env.Null();
    }

    int family = info[0].As<Number>().Int32Value();
    int type = info[1].As<Number>().Int32Value();
    int protocol = info[2].As<Number>().Int32Value();

    int fd;
    if ((fd = zts_bsd_socket(family, type, protocol)) < 0) {
        Error::New(env, "Unable to start service.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return Number::New(env, fd);
}

Value bsd_close(const CallbackInfo& info) {
    Env env = info.Env();

    if (info.Length() < 1) {
        TypeError::New(env, "Wrong number of arguments: fd").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (! info[0].IsNumber()) {
        TypeError::New(env, "Wrong type of arguments: fd").ThrowAsJavaScriptException();
        return env.Null();
    }

    int fd = info[0].As<Number>().Int32Value();
    
    int err;
    if ((err = zts_close(fd)) < 0) {
        Error::New(env, "Error when sending.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return env.Null();
}


Value bsd_send(const CallbackInfo& info) {
    Env env = info.Env();

    if (info.Length() < 3) {
        TypeError::New(env, "Wrong number of arguments: fd, buffer, flags").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (! info[0].IsNumber() || !info[1].IsTypedArray() || ! info[2].IsNumber()) {
        TypeError::New(env, "Wrong type of arguments: fd, buffer, flags").ThrowAsJavaScriptException();
        return env.Null();
    }

    int fd = info[0].As<Number>().Int32Value();
    auto data = info[1].As<Uint8Array>();
    int flags = info[2].As<Number>().Int32Value();
    
    int bytesWritten;
    if ((bytesWritten = zts_send(fd, data.Data(), data.ByteLength(), flags)) < 0) {
        Error::New(env, "Error when sending.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return Number::New(env, bytesWritten);
}

Value bsd_recv(const CallbackInfo& info) {
    Env env = info.Env();

    if (info.Length() < 3) {
        TypeError::New(env, "Wrong number of arguments: fd, buffer, flags").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (! info[0].IsNumber() || !info[1].IsTypedArray() || ! info[2].IsNumber()) {
        TypeError::New(env, "Wrong type of arguments: fd, buffer, flags").ThrowAsJavaScriptException();
        return env.Null();
    }

    int fd = info[0].As<Number>().Int32Value();
    auto data = info[1].As<Uint8Array>();
    int flags = info[2].As<Number>().Int32Value();
    
    int bytesWritten;
    if ((bytesWritten = zts_recv(fd, data.Data(), data.ByteLength(), flags)) < 0) {
        Error::New(env, "Error when receiving.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return Number::New(env, bytesWritten);
}

// ### no namespace socket stuff

Value connect(const CallbackInfo& info) {
    Env env = info.Env();

    if (info.Length() < 4) {
        TypeError::New(env, "Wrong number of arguments: fd, ipstr, port, timeout").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (! info[0].IsNumber() || !info[1].IsString() || ! info[2].IsNumber() || ! info[3].IsNumber()) {
        TypeError::New(env, "Wrong type of arguments: fd, ipstr, port, timeout").ThrowAsJavaScriptException();
        return env.Null();
    }

    int fd = info[0].As<Number>().Int32Value();
    std::string ipstr = info[1].As<String>();
    int port = info[2].As<Number>().Int32Value();
    int timeout = info[3].As<Number>().Int32Value();

    int err;
    if ((err = zts_connect(fd, ipstr.c_str(), port, timeout)) != ZTS_ERR_OK) {
        Error::New(env, "Error when connecting.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return env.Null();
}

// ### util ###

Value util_delay(const CallbackInfo& info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return env.Null();
    }
    if (! info[0].IsNumber()) {
        Napi::TypeError::New(env, "Wrong argument").ThrowAsJavaScriptException();
        return env.Null();
    }
    auto delay = info[0].As<Napi::Number>().Uint32Value();

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

    // no ns socket
    exports.Set("connect", Function::New(env, connect));

    // util
    exports.Set("util_delay", Function::New(env, util_delay));
    
    return exports;
}

NODE_API_MODULE(hello, Init)