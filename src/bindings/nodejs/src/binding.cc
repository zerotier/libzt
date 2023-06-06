#include "ZeroTierSockets.h"

#include <napi.h>

Napi::Value InitFromStorage(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return env.Null();
    }
    auto configPath = std::string(info[0].ToString()).c_str();

    int err = ZTS_ERR_OK;
    if ((err = zts_init_from_storage(configPath)) != ZTS_ERR_OK) {
        Napi::Error::New(env, "Unable to set config path.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return env.Null();
}

void on_zts_event(void* msgPtr)
{
    zts_event_msg_t* msg = (zts_event_msg_t*)msgPtr;

    printf("ZTS EVENT %d\n", msg->event_code);
}

Napi::Value StartNode(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    int err = ZTS_ERR_OK;

    if ((err = zts_init_set_event_handler(&on_zts_event)) != ZTS_ERR_OK) {
        Napi::Error::New(env, "Unable to start service.").ThrowAsJavaScriptException();
        return env.Null();
    }

    if ((err = zts_node_start()) != ZTS_ERR_OK) {
        Napi::Error::New(env, "Unable to start service.").ThrowAsJavaScriptException();
        return env.Null();
    }

    while (! zts_node_is_online()) {
        zts_util_delay(50);
    }

    return env.Null();
}

Napi::Value JoinNetwork(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (! info[0].IsBigInt()) {
        Napi::TypeError::New(env, "Network should be a bigint").ThrowAsJavaScriptException();
        return env.Null();
    }

    bool lossless = true;
    auto net_id = info[0].As<Napi::BigInt>().Uint64Value(&lossless);
    
    int err;
    if ((err = zts_net_join(net_id)) != ZTS_ERR_OK) {
        Napi::Error::New(env, "Unable to join network.").ThrowAsJavaScriptException();
        return env.Null();
    }

    while(!  zts_net_transport_is_ready(net_id)) {
        zts_util_delay(50);
    }

    return env.Null();
}

Napi::Value GetNetworkIPv4Addr(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (! info[0].IsBigInt()) {
        Napi::TypeError::New(env, "Network should be a bigint").ThrowAsJavaScriptException();
        return env.Null();
    }

    bool lossless;
    auto net_id = info[0].As<Napi::BigInt>().Uint64Value(&lossless);

    char addr[ZTS_IP_MAX_STR_LEN];
    
    int err;
    if ((err = zts_addr_get_str(net_id, ZTS_AF_INET, addr, ZTS_IP_MAX_STR_LEN)) != ZTS_ERR_OK) {
        Napi::Error::New(env, "Unable to get ipv4 address.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return Napi::String::New(env, addr);
}

Napi::Value GetNetworkIPv6Addr(const Napi::CallbackInfo& info) {
    Napi::Env env = info.Env();

    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments").ThrowAsJavaScriptException();
        return env.Null();
    }

    if (! info[0].IsBigInt()) {
        Napi::TypeError::New(env, "Network should be a bigint").ThrowAsJavaScriptException();
        return env.Null();
    }

    bool lossless;
    auto net_id = info[0].As<Napi::BigInt>().Uint64Value(&lossless);

    char addr[ZTS_IP_MAX_STR_LEN];
    
    int err;
    if ((err = zts_addr_get_str(net_id, ZTS_AF_INET6, addr, ZTS_IP_MAX_STR_LEN)) != ZTS_ERR_OK) {
        Napi::Error::New(env, "Unable to get ipv6 address.").ThrowAsJavaScriptException();
        return env.Null();
    }

    return Napi::String::New(env, addr);
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    exports.Set(Napi::String::New(env, "initFromStorage"), Napi::Function::New(env, InitFromStorage));
    exports.Set(Napi::String::New(env, "startNode"), Napi::Function::New(env, StartNode));
    exports.Set(Napi::String::New(env, "joinNetwork"), Napi::Function::New(env, JoinNetwork));
    exports.Set(Napi::String::New(env, "getNetworkIPv4Addr"), Napi::Function::New(env, GetNetworkIPv4Addr));
    exports.Set(Napi::String::New(env, "getNetworkIPv6Addr"), Napi::Function::New(env, GetNetworkIPv6Addr));
    return exports;
}

NODE_API_MODULE(hello, Init)