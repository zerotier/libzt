//
//  Use this file to import your target's public headers that you would like to expose to Swift.
//

#ifndef Example_OSX_Bridging_Header_h
#define Example_OSX_Bridging_Header_h

int start_intercept();
void start_service(const char * path);
void join_network(const char * nwid);
void disable_intercept();
void enable_intercept();

#include <sys/socket.h>
#include "SDK_Signatures.h"

void zt_join_network(const char *nwid);
void zt_leave_network(const char *nwid);

// Direct Call ZT API
// These functions will provide direct access to ZT-enabled sockets with no hassle
int zts_init_rpc(const char *path, const char *nwid);
int zts_connect(CONNECT_SIG);
int zts_bind(BIND_SIG);
int zts_accept(ACCEPT_SIG);
int zts_listen(LISTEN_SIG);
int zts_socket(SOCKET_SIG);
int zts_setsockopt(SETSOCKOPT_SIG);
int zts_getsockopt(GETSOCKOPT_SIG);
int zts_close(CLOSE_SIG);
int zts_getsockname(GETSOCKNAME_SIG);


#endif /* Example_OSX_Bridging_Header_h */


