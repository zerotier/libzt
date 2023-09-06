#include "impl.h"

// init_from_storage(path: string): void
Napi::Value init_from_storage(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments. Expected: 1").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto path = info[0].ToString();
    
    init_from_storage_impl(info, path);
    return env.Undefined();
}

// init_set_event_handler(callback: (event: number) => void): void
Napi::Value init_set_event_handler(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments. Expected: 1").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (! info[0].IsFunction()) {
        Napi::TypeError::New(env, "Argument at position 0 should be a function.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto callback = info[0].As<Napi::Function>();
    
    init_set_event_handler_impl(info, callback);
    return env.Undefined();
}

// node_start(): void
Napi::Value node_start(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    
    node_start_impl(info);
    return env.Undefined();
}

// node_is_online(): boolean
Napi::Value node_is_online(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    
    return node_is_online_impl(info);
}

// node_get_id(): bigint
Napi::Value node_get_id(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    
    return node_get_id_impl(info);
}

// node_stop(): void
Napi::Value node_stop(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    
    node_stop_impl(info);
    return env.Undefined();
}

// node_free(): void
Napi::Value node_free(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    
    node_free_impl(info);
    return env.Undefined();
}

// net_join(nwid: bigint): void
Napi::Value net_join(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments. Expected: 1").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (! info[0].IsBigInt()) {
        Napi::TypeError::New(env, "Argument at position 0 should be a BigInt.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto nwid = info[0].As<Napi::BigInt>();
    
    net_join_impl(info, nwid);
    return env.Undefined();
}

// net_leave(nwid: bigint): void
Napi::Value net_leave(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments. Expected: 1").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (! info[0].IsBigInt()) {
        Napi::TypeError::New(env, "Argument at position 0 should be a BigInt.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto nwid = info[0].As<Napi::BigInt>();
    
    net_leave_impl(info, nwid);
    return env.Undefined();
}

// net_transport_is_ready(nwid: bigint): boolean
Napi::Value net_transport_is_ready(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments. Expected: 1").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (! info[0].IsBigInt()) {
        Napi::TypeError::New(env, "Argument at position 0 should be a BigInt.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto nwid = info[0].As<Napi::BigInt>();
    
    return net_transport_is_ready_impl(info, nwid);
}

// addr_get_str(nwid: bigint, ipv6: boolean): string
Napi::Value addr_get_str(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Wrong number of arguments. Expected: 2").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (! info[0].IsBigInt()) {
        Napi::TypeError::New(env, "Argument at position 0 should be a BigInt.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto nwid = info[0].As<Napi::BigInt>();
    auto ipv6 = info[1].ToBoolean();
    
    return addr_get_str_impl(info, nwid, ipv6);
}

// bsd_socket(family: number, type: number, protocol: number): number
Napi::Value bsd_socket(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 3) {
        Napi::TypeError::New(env, "Wrong number of arguments. Expected: 3").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (! info[0].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 0 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto family = info[0].As<Napi::Number>();
    if (! info[1].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 1 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto type = info[1].As<Napi::Number>();
    if (! info[2].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 2 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto protocol = info[2].As<Napi::Number>();
    
    return bsd_socket_impl(info, family, type, protocol);
}

// bsd_close(fd: number): void
Napi::Value bsd_close(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments. Expected: 1").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (! info[0].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 0 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto fd = info[0].As<Napi::Number>();
    
    bsd_close_impl(info, fd);
    return env.Undefined();
}

// bsd_send(fd: number, data: Uint8Array, flags: number, callback: (err: ZtsError | undefined, bytesWritten: number) => void): void;
Napi::Value bsd_send(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 4) {
        Napi::TypeError::New(env, "Wrong number of arguments. Expected: 4").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (! info[0].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 0 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto fd = info[0].As<Napi::Number>();
    if (! info[1].IsTypedArray()) {
        Napi::TypeError::New(env, "Argument at position 1 should be a Uint8Array.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto data = info[1].As<Napi::Uint8Array>();
    if (! info[2].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 2 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto flags = info[2].As<Napi::Number>();
    if (! info[3].IsFunction()) {
        Napi::TypeError::New(env, "Argument at position 3 should be a function.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto callback = info[3].As<Napi::Function>();
    
    bsd_send_impl(info, fd, data, flags, callback);
    return env.Undefined();
}

// bsd_recv(fd: number, len: number, flags: number, callback: (err: ZtsError | null, data: Buffer) => void): void
Napi::Value bsd_recv(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 4) {
        Napi::TypeError::New(env, "Wrong number of arguments. Expected: 4").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (! info[0].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 0 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto fd = info[0].As<Napi::Number>();
    if (! info[1].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 1 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto len = info[1].As<Napi::Number>();
    if (! info[2].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 2 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto flags = info[2].As<Napi::Number>();
    if (! info[3].IsFunction()) {
        Napi::TypeError::New(env, "Argument at position 3 should be a function.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto callback = info[3].As<Napi::Function>();
    
    bsd_recv_impl(info, fd, len, flags, callback);
    return env.Undefined();
}

// bsd_sendto(fd: number, data: Uint8Array, flags: number, ipaddr: string, port: number, callback: (err: ZtsError | undefined, bytesWritten: number) => void): void
Napi::Value bsd_sendto(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 6) {
        Napi::TypeError::New(env, "Wrong number of arguments. Expected: 6").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (! info[0].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 0 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto fd = info[0].As<Napi::Number>();
    if (! info[1].IsTypedArray()) {
        Napi::TypeError::New(env, "Argument at position 1 should be a Uint8Array.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto data = info[1].As<Napi::Uint8Array>();
    if (! info[2].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 2 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto flags = info[2].As<Napi::Number>();
    auto ipaddr = info[3].ToString();
    if (! info[4].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 4 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto port = info[4].As<Napi::Number>();
    if (! info[5].IsFunction()) {
        Napi::TypeError::New(env, "Argument at position 5 should be a function.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto callback = info[5].As<Napi::Function>();
    
    bsd_sendto_impl(info, fd, data, flags, ipaddr, port, callback);
    return env.Undefined();
}

// bsd_recvfrom(fd: number, len: number, flags: number, callback: (err: ZtsError | undefined, data: Buffer, address: string, port: number) => void): void
Napi::Value bsd_recvfrom(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 4) {
        Napi::TypeError::New(env, "Wrong number of arguments. Expected: 4").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (! info[0].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 0 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto fd = info[0].As<Napi::Number>();
    if (! info[1].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 1 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto len = info[1].As<Napi::Number>();
    if (! info[2].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 2 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto flags = info[2].As<Napi::Number>();
    if (! info[3].IsFunction()) {
        Napi::TypeError::New(env, "Argument at position 3 should be a function.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto callback = info[3].As<Napi::Function>();
    
    bsd_recvfrom_impl(info, fd, len, flags, callback);
    return env.Undefined();
}

// bind(fd: number, ipAddr: string, port: number): void
Napi::Value bind(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 3) {
        Napi::TypeError::New(env, "Wrong number of arguments. Expected: 3").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (! info[0].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 0 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto fd = info[0].As<Napi::Number>();
    auto ipAddr = info[1].ToString();
    if (! info[2].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 2 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto port = info[2].As<Napi::Number>();
    
    bind_impl(info, fd, ipAddr, port);
    return env.Undefined();
}

// listen(fd: number, backlog: number): void
Napi::Value listen(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Wrong number of arguments. Expected: 2").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (! info[0].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 0 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto fd = info[0].As<Napi::Number>();
    if (! info[1].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 1 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto backlog = info[1].As<Napi::Number>();
    
    listen_impl(info, fd, backlog);
    return env.Undefined();
}

// accept(fd: number, callback: (err: ZtsError | null, fd: number) => void): void
Napi::Value accept(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 2) {
        Napi::TypeError::New(env, "Wrong number of arguments. Expected: 2").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (! info[0].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 0 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto fd = info[0].As<Napi::Number>();
    if (! info[1].IsFunction()) {
        Napi::TypeError::New(env, "Argument at position 1 should be a function.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto callback = info[1].As<Napi::Function>();
    
    accept_impl(info, fd, callback);
    return env.Undefined();
}

// connect(fd: number, ipAddr: string, port: number, timeout: number, callback: (err: ZtsError | null) => void): void
Napi::Value connect(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 5) {
        Napi::TypeError::New(env, "Wrong number of arguments. Expected: 5").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (! info[0].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 0 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto fd = info[0].As<Napi::Number>();
    auto ipAddr = info[1].ToString();
    if (! info[2].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 2 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto port = info[2].As<Napi::Number>();
    if (! info[3].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 3 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto timeout = info[3].As<Napi::Number>();
    if (! info[4].IsFunction()) {
        Napi::TypeError::New(env, "Argument at position 4 should be a function.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto callback = info[4].As<Napi::Function>();
    
    connect_impl(info, fd, ipAddr, port, timeout, callback);
    return env.Undefined();
}

// shutdown_rd(fd: number): void
Napi::Value shutdown_rd(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments. Expected: 1").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (! info[0].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 0 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto fd = info[0].As<Napi::Number>();
    
    shutdown_rd_impl(info, fd);
    return env.Undefined();
}

// shutdown_wr(fd: number): void
Napi::Value shutdown_wr(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    if (info.Length() < 1) {
        Napi::TypeError::New(env, "Wrong number of arguments. Expected: 1").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    if (! info[0].IsNumber()) {
        Napi::TypeError::New(env, "Argument at position 0 should be a number.").ThrowAsJavaScriptException();
        return env.Undefined();
    }
    auto fd = info[0].As<Napi::Number>();
    
    shutdown_wr_impl(info, fd);
    return env.Undefined();
}

// getpeername(fd: number): { port: number, address: string }
    Napi::Value getpeername(const Napi::CallbackInfo& info)
    {
        Napi::Env env = info.Env();
        if (info.Length() < 1) {
            Napi::TypeError::New(env, "Wrong number of arguments. Expected: 1").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        if (! info[0].IsNumber()) {
            Napi::TypeError::New(env, "Argument at position 0 should be a number.").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        auto fd = info[0].As<Napi::Number>();
        
        return getpeername_impl(info, fd);
    }
    
// getsockname(fd: number): { port: number, address: string }
    Napi::Value getsockname(const Napi::CallbackInfo& info)
    {
        Napi::Env env = info.Env();
        if (info.Length() < 1) {
            Napi::TypeError::New(env, "Wrong number of arguments. Expected: 1").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        if (! info[0].IsNumber()) {
            Napi::TypeError::New(env, "Argument at position 0 should be a number.").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        auto fd = info[0].As<Napi::Number>();
        
        return getsockname_impl(info, fd);
    }
    
    // set_recv_timeout(fd: number, seconds: number, microseconds: number): void
    Napi::Value set_recv_timeout(const Napi::CallbackInfo& info)
    {
        Napi::Env env = info.Env();
        if (info.Length() < 3) {
            Napi::TypeError::New(env, "Wrong number of arguments. Expected: 3").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        if (! info[0].IsNumber()) {
            Napi::TypeError::New(env, "Argument at position 0 should be a number.").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        auto fd = info[0].As<Napi::Number>();
        if (! info[1].IsNumber()) {
            Napi::TypeError::New(env, "Argument at position 1 should be a number.").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        auto seconds = info[1].As<Napi::Number>();
        if (! info[2].IsNumber()) {
            Napi::TypeError::New(env, "Argument at position 2 should be a number.").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        auto microseconds = info[2].As<Napi::Number>();
        
        set_recv_timeout_impl(info, fd, seconds, microseconds);
        return env.Undefined();
    }
    
    // set_send_timeout(fd: number, seconds: number, microseconds: number): void
    Napi::Value set_send_timeout(const Napi::CallbackInfo& info)
    {
        Napi::Env env = info.Env();
        if (info.Length() < 3) {
            Napi::TypeError::New(env, "Wrong number of arguments. Expected: 3").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        if (! info[0].IsNumber()) {
            Napi::TypeError::New(env, "Argument at position 0 should be a number.").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        auto fd = info[0].As<Napi::Number>();
        if (! info[1].IsNumber()) {
            Napi::TypeError::New(env, "Argument at position 1 should be a number.").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        auto seconds = info[1].As<Napi::Number>();
        if (! info[2].IsNumber()) {
            Napi::TypeError::New(env, "Argument at position 2 should be a number.").ThrowAsJavaScriptException();
            return env.Undefined();
        }
        auto microseconds = info[2].As<Napi::Number>();
        
        set_send_timeout_impl(info, fd, seconds, microseconds);
        return env.Undefined();
    }
    
    Napi::Object Init(Napi::Env env, Napi::Object exports)
    {
        exports["init_from_storage"] = Napi::Function::New(env, init_from_storage);
        exports["init_set_event_handler"] = Napi::Function::New(env, init_set_event_handler);
        exports["node_start"] = Napi::Function::New(env, node_start);
        exports["node_is_online"] = Napi::Function::New(env, node_is_online);
        exports["node_get_id"] = Napi::Function::New(env, node_get_id);
        exports["node_stop"] = Napi::Function::New(env, node_stop);
        exports["node_free"] = Napi::Function::New(env, node_free);
        exports["net_join"] = Napi::Function::New(env, net_join);
        exports["net_leave"] = Napi::Function::New(env, net_leave);
        exports["net_transport_is_ready"] = Napi::Function::New(env, net_transport_is_ready);
        exports["addr_get_str"] = Napi::Function::New(env, addr_get_str);
        exports["bsd_socket"] = Napi::Function::New(env, bsd_socket);
        exports["bsd_close"] = Napi::Function::New(env, bsd_close);
        exports["bsd_send"] = Napi::Function::New(env, bsd_send);
        exports["bsd_recv"] = Napi::Function::New(env, bsd_recv);
        exports["bsd_sendto"] = Napi::Function::New(env, bsd_sendto);
        exports["bsd_recvfrom"] = Napi::Function::New(env, bsd_recvfrom);
        exports["bind"] = Napi::Function::New(env, bind);
        exports["listen"] = Napi::Function::New(env, listen);
        exports["accept"] = Napi::Function::New(env, accept);
        exports["connect"] = Napi::Function::New(env, connect);
        exports["shutdown_rd"] = Napi::Function::New(env, shutdown_rd);
        exports["shutdown_wr"] = Napi::Function::New(env, shutdown_wr);
        exports["getpeername"] = Napi::Function::New(env, getpeername);
        exports["getsockname"] = Napi::Function::New(env, getsockname);
        exports["set_recv_timeout"] = Napi::Function::New(env, set_recv_timeout);
        exports["set_send_timeout"] = Napi::Function::New(env, set_send_timeout);
        
        return exports;
    }
    
    NODE_API_MODULE(ZTS, Init)
    