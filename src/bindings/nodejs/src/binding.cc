#include "ZeroTierSockets.h"

#include <napi.h>

using namespace Napi;

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

Value start_node(const CallbackInfo& info)
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

Object Init(Env env, Object exports)
{
    exports.Set("init_from_storage", Function::New(env, init_from_storage));
    exports.Set("init_set_event_handler", Function::New(env, init_set_event_handler));
    exports.Set("start_node", Function::New(env, start_node));
    exports.Set("node_is_online", Function::New(env, node_is_online));
    exports.Set("util_delay", Function::New(env, util_delay));
    exports.Set("net_join", Function::New(env, net_join));
    exports.Set("net_transport_is_ready", Function::New(env, net_transport_is_ready));
    exports.Set("addr_get_str", Function::New(env, addr_get_str));
    return exports;
}

NODE_API_MODULE(hello, Init)