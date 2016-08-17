//
//  Implementations located in src/SDK_XcodeWrapper.cpp
//

#ifndef Example_OSX_Bridging_Header_h
#define Example_OSX_Bridging_Header_h

#include <sys/socket.h>
#include "SDK_Signatures.h"

// ZT INTERCEPT/RPC CONTROLS
int zt_init_rpc(const char *path, const char *nwid);
int start_intercept();
void disable_intercept();
void enable_intercept();

// ZT SERVICE CONTROLS
void start_service(const char * path);
void stop_service();
void start_service_and_rpc(const char * path, const char * nwid);
void zt_join_network(const char *nwid);
void zt_leave_network(const char *nwid);
void zt_is_running(const char *nwid);
void zt_get_addresses(const char *nwid, char * addrstr);

// SOCKS5 PROXY CONTROLS
void zt_start_proxy_server(const char *nwid, struct sockaddr_storage addr);
void zt_stop_proxy_server(const char *nwid);
void zt_get_proxy_server_address(const char *nwid, struct sockaddr_storage addr);

// SOCKET API
int zt_connect(CONNECT_SIG);
int zt_bind(BIND_SIG);
int zt_accept(ACCEPT_SIG);
int zt_listen(LISTEN_SIG);
int zt_socket(SOCKET_SIG);
int zt_setsockopt(SETSOCKOPT_SIG);
int zt_getsockopt(GETSOCKOPT_SIG);
int zt_close(CLOSE_SIG);
int zt_getsockname(GETSOCKNAME_SIG);
int zt_getpeername(GETPEERNAME_SIG);

#endif /* Example_OSX_Bridging_Header_h */



