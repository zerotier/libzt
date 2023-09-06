
#ifndef BINDING_IMPL_H_
#define BINDING_IMPL_H_

#include <napi.h>

void init_from_storage_impl(const Napi::CallbackInfo& info, Napi::String path);

void init_set_event_handler_impl(const Napi::CallbackInfo& info, Napi::Function callback);

void node_start_impl(const Napi::CallbackInfo& info);

Napi::Boolean node_is_online_impl(const Napi::CallbackInfo& info);

Napi::BigInt node_get_id_impl(const Napi::CallbackInfo& info);

void node_stop_impl(const Napi::CallbackInfo& info);

void node_free_impl(const Napi::CallbackInfo& info);

void net_join_impl(const Napi::CallbackInfo& info, Napi::BigInt nwid);

void net_leave_impl(const Napi::CallbackInfo& info, Napi::BigInt nwid);

Napi::Boolean net_transport_is_ready_impl(const Napi::CallbackInfo& info, Napi::BigInt nwid);

Napi::String addr_get_str_impl(const Napi::CallbackInfo& info, Napi::BigInt nwid, Napi::Boolean ipv6);

Napi::Number bsd_socket_impl(const Napi::CallbackInfo& info, Napi::Number family, Napi::Number type, Napi::Number protocol);

void bsd_close_impl(const Napi::CallbackInfo& info, Napi::Number fd);

void bsd_send_impl(const Napi::CallbackInfo& info, Napi::Number fd, Napi::Uint8Array data, Napi::Number flags, Napi::Function callback);

void bsd_recv_impl(const Napi::CallbackInfo& info, Napi::Number fd, Napi::Number len, Napi::Number flags, Napi::Function callback);

void bsd_sendto_impl(const Napi::CallbackInfo& info, Napi::Number fd, Napi::Uint8Array data, Napi::Number flags, Napi::String ipaddr, Napi::Number port, Napi::Function callback);

void bsd_recvfrom_impl(const Napi::CallbackInfo& info, Napi::Number fd, Napi::Number len, Napi::Number flags, Napi::Function callback);

void bind_impl(const Napi::CallbackInfo& info, Napi::Number fd, Napi::String ipAddr, Napi::Number port);

void listen_impl(const Napi::CallbackInfo& info, Napi::Number fd, Napi::Number backlog);

void accept_impl(const Napi::CallbackInfo& info, Napi::Number fd, Napi::Function callback);

void connect_impl(const Napi::CallbackInfo& info, Napi::Number fd, Napi::String ipAddr, Napi::Number port, Napi::Number timeout, Napi::Function callback);

void shutdown_rd_impl(const Napi::CallbackInfo& info, Napi::Number fd);

void shutdown_wr_impl(const Napi::CallbackInfo& info, Napi::Number fd);

Napi::Object getpeername_impl(const Napi::CallbackInfo& info, Napi::Number fd);

Napi::Object getsockname_impl(const Napi::CallbackInfo& info, Napi::Number fd);

void set_recv_timeout_impl(const Napi::CallbackInfo& info, Napi::Number fd, Napi::Number seconds, Napi::Number microseconds);

void set_send_timeout_impl(const Napi::CallbackInfo& info, Napi::Number fd, Napi::Number seconds, Napi::Number microseconds);

#endif // BINDING_IMPL_H_
