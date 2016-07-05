//
//  NetconWrapper.cpp
//  Netcon-iOS
//
//  Created by Joseph Henry on 2/14/16.
//  Copyright © 2016 ZeroTier. All rights reserved.
//

#include "SDK.h"
#include "SDK_XcodeWrapper.hpp"
#include "SDK_Signatures.h"
#include <sys/socket.h>

#define INTERCEPT_ENABLED   111
#define INTERCEPT_DISABLED  222

#include "SDK_ServiceSetup.hpp"

// Starts a service at the specified path
extern "C" int start_service(const char * path) {
    init_service(INTERCEPT_DISABLED, path);
    return 1;
}

// Joins a network
extern "C" void zt_join_network(const char * nwid){
    join_network(nwid); // Instruct ZeroTier service to join network
    // zt_init_rpc(nwid); // Tells the RPC code where to contact the ZeroTier service
}

// Leaves a network
extern "C" void zt_leave_network(const char * nwid){
    leave_network(nwid);
}

// Explicit ZT API wrappers
extern "C" int zts_socket(SOCKET_SIG) {
    return zt_socket(socket_family, socket_type, protocol);
}
extern "C" int zts_connect(CONNECT_SIG) {
    return zt_connect(__fd, __addr, __len);
}
extern "C" int zt_bind(BIND_SIG){
    return zt_bind(sockfd, addr, addrlen);
}
extern "C" int zt_accept(ACCEPT_SIG) {
    return zt_accept(sockfd, addr, addrlen);
}
extern "C" int zt_listen(LISTEN_SIG) {
    return zt_listen(sockfd, backlog);
}
extern "C" int zt_setsockopt(SETSOCKOPT_SIG) {
    return zt_setsockopt(socket, level, option_name, option_value, option_len);
}
extern "C" int zt_getsockopt(GETSOCKOPT_SIG) {
    return zt_getsockopt(sockfd, level, optname, optval, optlen);
}
extern "C" int zt_close(CLOSE_SIG) {
    return zt_close(fd);
}
extern "C" int zt_getsockname(GETSOCKNAME_SIG) {
    return zt_getsockname(sockfd, addr, addrlen);
}
