#include "impl.h"

#include "ZeroTierSockets.h"
#include "async.cc"
#include "asynclambda.h"

using namespace Napi;

#define ERROR(ERR, FUN)                                                                                                \
    if (ERR < 0) {                                                                                                     \
        auto error = Error::New(info.Env(), "Error during " FUN " call");                                              \
        error.Set(String::New(info.Env(), "code"), Number::New(info.Env(), ERR));                                      \
        throw error;                                                                                                   \
    }

#define CHECK_ERRNO(ERR, FUN)                                                                                          \
    if (ERR < 0) {                                                                                                     \
        auto error = Error::New(info.Env(), "Error during " FUN " call");                                              \
        error.Set(String::New(info.Env(), "code"), Number::New(info.Env(), ERR));                                      \
        error.Set(String::New(info.Env(), "errno"), Number::New(info.Env(), zts_errno));                               \
        throw error;                                                                                                   \
    }

void init_from_storage_impl(const Napi::CallbackInfo& info, Napi::String path)
{
    int err = zts_init_from_storage(std::string(path).c_str());
    ERROR(err, "init_from_storage")
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

void init_set_event_handler_impl(const Napi::CallbackInfo& info, Napi::Function callback)
{
    event_callback = ThreadSafeFunction::New(info.Env(), callback, "zts_event_listener", 0, 1);

    int err = zts_init_set_event_handler(&event_handler);
    ERROR(err, "init_set_event_handler")
}

void node_start_impl(const Napi::CallbackInfo& info)
{
    int err = zts_node_start();
    ERROR(err, "node_start")
}

Napi::Boolean node_is_online_impl(const Napi::CallbackInfo& info)
{
    return Boolean::New(info.Env(), zts_node_is_online());
}

Napi::BigInt node_get_id_impl(const Napi::CallbackInfo& info)
{
    auto id = zts_node_get_id();

    return BigInt::New(info.Env(), id);
}

void node_stop_impl(const Napi::CallbackInfo& info)
{
    int err = zts_node_stop();
    ERROR(err, "node_stop")
}

void node_free_impl(const Napi::CallbackInfo& info)
{
    int err = zts_node_free();
    ERROR(err, "node_free")
}

void net_join_impl(const Napi::CallbackInfo& info, Napi::BigInt nwid)
{
    bool lossless;
    uint64_t net_id = nwid.Uint64Value(&lossless);

    int err = zts_net_join(net_id);
    ERROR(err, "net_join")
}

void net_leave_impl(const Napi::CallbackInfo& info, Napi::BigInt nwid)
{
    bool lossless;
    uint64_t net_id = nwid.Uint64Value(&lossless);

    int err = zts_net_leave(net_id);
    ERROR(err, "net_leave")
}

Napi::Boolean net_transport_is_ready_impl(const Napi::CallbackInfo& info, Napi::BigInt nwid)
{
    bool lossless;
    uint64_t net_id = nwid.Uint64Value(&lossless);

    return Boolean::New(info.Env(), zts_net_transport_is_ready(net_id));
}

Napi::String addr_get_str_impl(const Napi::CallbackInfo& info, Napi::BigInt nwid, Napi::Boolean ipv6)
{
    bool lossless;
    uint64_t net_id = nwid.Uint64Value(&lossless);

    auto family = ipv6 ? ZTS_AF_INET6 : ZTS_AF_INET;

    char addr[ZTS_IP_MAX_STR_LEN];

    int err = zts_addr_get_str(net_id, family, addr, ZTS_IP_MAX_STR_LEN);
    ERROR(err, "addr_get_str")

    return String::New(info.Env(), addr);
}

Napi::Number
bsd_socket_impl(const Napi::CallbackInfo& info, Napi::Number family, Napi::Number type, Napi::Number protocol)
{
    int fd = zts_bsd_socket(family, type, protocol);
    CHECK_ERRNO(fd, "bsd_socket")

    return Number::New(info.Env(), fd);
}

void bsd_close_impl(const Napi::CallbackInfo& info, Napi::Number fd)
{
    int err = zts_bsd_close(fd);
    CHECK_ERRNO(err, "bsd_close")
}

void bsd_send_impl(
    const Napi::CallbackInfo& info,
    Napi::Number fd,
    Napi::Uint8Array data,
    Napi::Number flags,
    Napi::Function callback)
{
    int size = data.ByteLength();
    auto data_vec = new std::vector<uint8_t>();
    data_vec->insert(data_vec->begin(), data.Data(), data.Data() + size);

    int fdI = fd;
    int flagsI = flags;
    auto execute = [fdI, data_vec, size, flagsI]() { return zts_bsd_send(fdI, data_vec->data(), size, flagsI); };
    auto on_destroy = [data_vec]() { delete data_vec; };
    auto worker = new AsyncLambda(callback, "bsd_send", execute, on_destroy);
    worker->Queue();
}

void bsd_recv_impl(
    const Napi::CallbackInfo& info,
    Napi::Number fd,
    Napi::Number len,
    Napi::Number flags,
    Napi::Function callback)
{
    auto worker = new BsdRecvWorker(callback, fd, len.Int32Value(), flags);
    worker->Queue();
}

void bsd_sendto_impl(
    const Napi::CallbackInfo& info,
    Napi::Number fd,
    Napi::Uint8Array data,
    Napi::Number flags,
    Napi::String ipaddr,
    Napi::Number port,
    Napi::Function callback)
{
    int size = data.ByteLength();
    auto data_vec = new std::vector<uint8_t>();
    data_vec->insert(data_vec->begin(), data.Data(), data.Data() + size);

    struct zts_sockaddr_storage addr;
    zts_socklen_t addrlen = sizeof(struct zts_sockaddr_storage);
    zts_util_ipstr_to_saddr(std::string(ipaddr).c_str(), port.Int32Value(), (struct zts_sockaddr*)&addr, &addrlen);

    int fdI = fd;
    int flagsI = flags;
    auto execute = [fdI, data_vec, size, flagsI, addr, addrlen]() {
        return zts_bsd_sendto(fdI, data_vec->data(), size, flagsI, (struct zts_sockaddr*)&addr, addrlen);
    };
    auto on_destroy = [data_vec]() { delete data_vec; };
    auto worker = new AsyncLambda(callback, "bsd_send", execute, on_destroy);
    worker->Queue();
}

void bsd_recvfrom_impl(
    const Napi::CallbackInfo& info,
    Napi::Number fd,
    Napi::Number len,
    Napi::Number flags,
    Napi::Function callback)
{
    auto worker = new BsdRecvFromWorker(callback, fd, len.Int32Value(), flags);
    worker->Queue();
}

void bind_impl(const Napi::CallbackInfo& info, Napi::Number fd, Napi::String ipAddr, Napi::Number port)
{
    int err = zts_bind(fd, std::string(ipAddr).c_str(), port.Int32Value());
    CHECK_ERRNO(err, "bind")
}

void listen_impl(const Napi::CallbackInfo& info, Napi::Number fd, Napi::Number backlog)
{
    int err = zts_listen(fd, backlog);
    CHECK_ERRNO(err, "listen")
}

void accept_impl(const Napi::CallbackInfo& info, Napi::Number fd, Napi::Function callback)
{
    int fdI = fd;
    auto worker = new AsyncLambda(
        callback,
        "accept",
        [fdI]() {
            char remote_addr[ZTS_IP_MAX_STR_LEN];
            unsigned short port;
            return zts_accept(fdI, remote_addr, ZTS_IP_MAX_STR_LEN, &port);
        },
        []() {});
    worker->Queue();
}

void connect_impl(
    const Napi::CallbackInfo& info,
    Napi::Number fd,
    Napi::String ipAddr,
    Napi::Number port,
    Napi::Number timeout,
    Napi::Function callback)
{
    int fdI = fd;
    std::string ip = ipAddr;
    int portI = port;
    int timeoutI = timeout;
    auto worker = new AsyncLambda(
        callback,
        "connect",
        [=]() { return zts_connect(fdI, ip.c_str(), portI, timeoutI); },
        []() {});
    worker->Queue();
}

void shutdown_rd_impl(const Napi::CallbackInfo& info, Napi::Number fd)
{
    int err = zts_shutdown_rd(fd);
    CHECK_ERRNO(err, "shutdown_rd")
}

void shutdown_wr_impl(const Napi::CallbackInfo& info, Napi::Number fd)
{
    int err = zts_shutdown_wr(fd);
    CHECK_ERRNO(err, "shutdown_wr")
}

Napi::Object getpeername_impl(const Napi::CallbackInfo& info, Napi::Number fd)
{
    char addr[ZTS_IP_MAX_STR_LEN];
    unsigned short port;

    int err = zts_getpeername(fd, addr, ZTS_IP_MAX_STR_LEN, &port);
    CHECK_ERRNO(err, "getpeername")

    auto obj = Object::New(info.Env());
    obj["address"] = String::New(info.Env(), addr);
    obj["port"] = Number::New(info.Env(), port);

    return obj;
}

Napi::Object getsockname_impl(const Napi::CallbackInfo& info, Napi::Number fd)
{
    char addr[ZTS_IP_MAX_STR_LEN];
    unsigned short port;

    int err = zts_getsockname(fd, addr, ZTS_IP_MAX_STR_LEN, &port);
    CHECK_ERRNO(err, "getsockname")

    auto obj = Object::New(info.Env());
    obj["address"] = String::New(info.Env(), addr);
    obj["port"] = Number::New(info.Env(), port);

    return obj;
}

void set_recv_timeout_impl(
    const Napi::CallbackInfo& info,
    Napi::Number fd,
    Napi::Number seconds,
    Napi::Number microseconds)
{
    int err = zts_set_recv_timeout(fd, seconds, microseconds);
    CHECK_ERRNO(err, "set_recv_timeout")
}

void set_send_timeout_impl(
    const Napi::CallbackInfo& info,
    Napi::Number fd,
    Napi::Number seconds,
    Napi::Number microseconds)
{
    int err = zts_set_send_timeout(fd, seconds, microseconds);
    CHECK_ERRNO(err, "set_recv_timeout")
}
