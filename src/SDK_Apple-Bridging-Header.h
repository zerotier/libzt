//
//  Implementations located in src/SDK_XcodeWrapper.cpp
//

#ifndef Example_OSX_Bridging_Header_h
#define Example_OSX_Bridging_Header_h

int start_intercept();
void start_service(const char * path);
void start_service_and_rpc(const char * path, const char * nwid);
void join_network(const char * nwid);
void disable_intercept();
void enable_intercept();

#include <sys/socket.h>
#include "SDK_Signatures.h"

void zt_join_network(const char *nwid);
void zt_leave_network(const char *nwid);
void zt_get_addresses(const char *nwid, char * addrstr);

// Direct Call ZT API
// These functions will provide direct access to ZT-enabled sockets with no hassle
int zt_init_rpc(const char *path, const char *nwid);
int zt_connect(CONNECT_SIG);
int zt_bind(BIND_SIG);
int zt_accept(ACCEPT_SIG);
int zt_listen(LISTEN_SIG);
int zt_socket(SOCKET_SIG);
int zt_setsockopt(SETSOCKOPT_SIG);
int zt_getsockopt(GETSOCKOPT_SIG);
int zt_close(CLOSE_SIG);
int zt_getsockname(GETSOCKNAME_SIG);

#endif /* Example_OSX_Bridging_Header_h */



