//
//  Implementations located in src/wrappers/swift/XcodeWrapper.cpp
//

#ifndef Example_OSX_IOS_Bridging_Header_h
#define Example_OSX_IOS_Bridging_Header_h

#include <sys/socket.h>
#include "signatures.h"

// ZT INTERCEPT/RPC CONTROLS
int zt_init_rpc(const char *path, const char *nwid);
int zt_start_intercept();
void zt_disable_intercept();
void zt_enable_intercept();

// ZT SERVICE CONTROLS
void zt_start_service(const char * path);
void zt_stop_service();
void zt_start_service_and_rpc(const char * path, const char * nwid);
bool zt_service_is_running();
void zt_join_network(const char *nwid);
void zt_leave_network(const char *nwid);
void zt_get_ipv4_address(const char *nwid, char *addrstr);
void zt_get_ipv6_address(const char *nwid, char *addrstr);


// SOCKS5 PROXY CONTROLS
void zt_start_proxy_server(const char *nwid, struct sockaddr_storage addr);
void zt_stop_proxy_server(const char *nwid);
void zt_proxy_is_running(const char *nwid);
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
int zt_recvfrom(RECVFROM_SIG);
int zt_fcntl(FCNTL_SIG);
int zt_sendto(SENDTO_SIG);

#endif /* Example_OSX_IOS_Bridging_Header_h */



