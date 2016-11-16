    /*
 * ZeroTier One - Network Virtualization Everywhere
 * Copyright (C) 2011-2015  ZeroTier, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * --
 *
 * ZeroTier may be used and distributed under the terms of the GPLv3, which
 * are available at: http://www.gnu.org/licenses/gpl-3.0.html
 *
 * If you would like to embed ZeroTier into a commercial application or
 * redistribute it in a modified binary form, please contact ZeroTier Networks
 * LLC. Start here: http://www.zerotier.com/
 */

#if defined(SDK_INTERCEPT)

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <strings.h>
#include <pwd.h>
#include <errno.h>
#include <stdarg.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <sys/resource.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#if defined(__linux__)
    #include <linux/errno.h>
    #include <sys/syscall.h>
    #include <linux/net.h>
#endif

#include "sdk.h"
#include "debug.h"
#include "rpc.h"

pthread_key_t thr_id_key;

// externs common between SDK_Intercept and SDK_Socket from SDK.h
#if defined(__linux__)
    int (*realaccept4)(ACCEPT4_SIG) = 0;
    #if !defined(__ANDROID__)
        int (*realsyscall)(SYSCALL_SIG) = 0;
    #endif
#endif

#if !defined(__ANDROID__)
     int (*realbind)(BIND_SIG) = 0;
     int (*realsendmsg)(SENDMSG_SIG) = 0;
     ssize_t (*realsendto)(SENDTO_SIG) = 0;
     int (*realrecvmsg)(RECVMSG_SIG) = 0;
     int (*realrecvfrom)(RECVFROM_SIG) = 0;
#endif
     int (*realconnect)(CONNECT_SIG) = 0;
     int (*realaccept)(ACCEPT_SIG) = 0;
     int (*reallisten)(LISTEN_SIG) = 0;
     int (*realsocket)(SOCKET_SIG) = 0;
     int (*realsetsockopt)(SETSOCKOPT_SIG) = 0;
     int (*realgetsockopt)(GETSOCKOPT_SIG) = 0;
     int (*realclose)(CLOSE_SIG);
     int (*realgetsockname)(GETSOCKNAME_SIG) = 0;

    // ------------------------------------------------------------------------------
    // --------------------- Get Original socket API pointers -----------------------
    // ------------------------------------------------------------------------------

    extern void load_symbols()
    {
        DEBUG_EXTRA("");
#if defined(__linux__)
        realaccept4 = dlsym(RTLD_NEXT, "accept4");
    #if !defined(__ANDROID__)
        realsyscall = dlsym(RTLD_NEXT, "syscall");
    #endif
#endif
        realsetsockopt = (int(*)(SETSOCKOPT_SIG))dlsym(RTLD_NEXT, "setsockopt");
        realgetsockopt = (int(*)(GETSOCKOPT_SIG))dlsym(RTLD_NEXT, "getsockopt");
        realsocket = (int(*)(SOCKET_SIG))dlsym(RTLD_NEXT, "socket");
        realconnect = (int(*)(CONNECT_SIG))dlsym(RTLD_NEXT, "connect");
        realaccept = (int(*)(ACCEPT_SIG))dlsym(RTLD_NEXT, "accept");
        reallisten = (int(*)(LISTEN_SIG))dlsym(RTLD_NEXT, "listen");
        realclose = (int(*)(CLOSE_SIG))dlsym(RTLD_NEXT, "close");
        realgetsockname = (int(*)(GETSOCKNAME_SIG))dlsym(RTLD_NEXT, "getsockname");
    #if !defined(__ANDROID__)
        realbind = (int(*)(BIND_SIG))dlsym(RTLD_NEXT, "bind");
        realsendto = (ssize_t(*)(int, const void *, size_t, int, const struct sockaddr *, socklen_t))dlsym(RTLD_NEXT, "sendto");
        realrecvfrom = (int(*)(RECVFROM_SIG))dlsym(RTLD_NEXT, "recvfrom");
        realrecvmsg = (int(*)(RECVMSG_SIG))dlsym(RTLD_NEXT, "recvmsg");
    #endif
    }
        
    // ------------------------------------------------------------------------------
    // ------------------------------- Intercept Setup ------------------------------
    // ------------------------------------------------------------------------------
    // Return whether 'intercept' API is enabled for this thread

    bool check_intercept_enabled() {
        if(!realconnect){
            load_symbols();
        }
    #if defined(SDK_BUNDLED)
        // The reasoning for this check is that if you've built the SDK with SDK_BUNDLE=1, then 
        // you've included a full ZeroTier service in the same binary as your intercept, and we 
        // don't want to run ZeroTier network API calls through the intercept, so we must specify
        // which threads should be intercepted manually
        void *spec = pthread_getspecific(thr_id_key);
        int thr_id = spec != NULL ? *((int*)spec) : -1;
        return thr_id == INTERCEPT_ENABLED;
    #else
        return 1;
    #endif
    }

    // ------------------------------------------------------------------------------
    // ------------------------- connected_to_service() -----------------------------
    // ------------------------------------------------------------------------------
    // Check whether or not the socket is mapped to the service. We
    // need to know if this is a regular AF_LOCAL socket or an end of a socketpair
    // that the service uses. We don't want to keep state in the intercept, so
    // we simply ask the service via an RPC

    int connected_to_service(int sockfd)
    {
        struct sockaddr_storage addr;
        socklen_t len = sizeof addr;
        struct sockaddr_un * addr_un;
        getpeername(sockfd, (struct sockaddr*)&addr, &len);
        if (addr.ss_family == AF_LOCAL || addr.ss_family == AF_LOCAL) {
            addr_un = (struct sockaddr_un*)&addr;
            return strcmp(addr_un->sun_path, api_netpath) == 0;
        }
        DEBUG_ERROR("not connected to service");
        return 0;
    }
    
    // ------------------------------------------------------------------------------
    // ------------------------------------ sendto() --------------------------------
    // ------------------------------------------------------------------------------
    // int fd, const void *buf, size_t len, int flags, 
    // const struct sockaddr *addr, socklen_t addrlen

#if !defined(__ANDROID__)
    ssize_t sendto(SENDTO_SIG)
    {
        DEBUG_INFO("fd=%d, len=%d", fd, (int)len);
        //if (!check_intercept_enabled())
            return realsendto(fd, buf, len, flags, addr, addrlen);
        return zts_sendto(fd, buf, len, flags, addr, addrlen);
    }
#endif

    // ------------------------------------------------------------------------------
    // ----------------------------------- sendmsg() --------------------------------
    // ------------------------------------------------------------------------------
    // int fd, const struct msghdr *msg, int flags

#if !defined(__ANDROID__)
    ssize_t sendmsg(SENDMSG_SIG)
    {
        DEBUG_INFO("fd=%d", fd);
        //if(!check_intercept_enabled())
            return realsendmsg(fd, msg, flags);
        zts_sendmsg(fd, msg, flags);
    }
#endif
    
    // ------------------------------------------------------------------------------
    // ---------------------------------- recvfrom() --------------------------------
    // ------------------------------------------------------------------------------
    // int fd, void *restrict buf, size_t len, int flags, struct sockaddr
    // *restrict addr, socklen_t *restrict addrlen

#if !defined(__ANDROID__)
    ssize_t recvfrom(RECVFROM_SIG)
    {
        DEBUG_INFO("fd=%d", fd);
        if(!check_intercept_enabled())
            return realrecvfrom(fd, buf, len, flags, addr, addrlen);
        return zts_recvfrom(fd, buf, len, flags, addr, addrlen);
    }
#endif

    // ------------------------------------------------------------------------------
    // ----------------------------------- recvmsg() --------------------------------
    // ------------------------------------------------------------------------------
    // int fd, struct msghdr *msg, int flags

#if !defined(__ANDROID__)
    ssize_t recvmsg(RECVMSG_SIG)
    {
        DEBUG_INFO("fd=%d", fd);
        //if(!check_intercept_enabled())
            return realrecvmsg(fd, msg, flags);
        return zts_recvmsg(fd, msg, flags);
    }
#endif

    // ------------------------------------------------------------------------------
    // --------------------------------- setsockopt() -------------------------------
    // ------------------------------------------------------------------------------
    // int fd, int level, int optname, const void *optval, socklen_t optlen

    int setsockopt(SETSOCKOPT_SIG)
    {
        DEBUG_INFO("fd=%d", fd);
        if (!check_intercept_enabled())
            return realsetsockopt(fd, level, optname, optval, optlen);
    #if defined(__linux__)
        if(level == SOL_IPV6 && optname == IPV6_V6ONLY)
            return 0;
        if(level == SOL_IP && (optname == IP_TTL || optname == IP_TOS))
            return 0;
    #endif
        if(level == IPPROTO_TCP || (level == SOL_SOCKET && optname == SO_KEEPALIVE))
            return 0;
        if(realsetsockopt(fd, level, optname, optval, optlen) < 0)
            perror("setsockopt():\n");
        return zts_setsockopt(fd, level, optname, optval, optlen);
    }

    // ------------------------------------------------------------------------------
    // --------------------------------- getsockopt() -------------------------------
    // ------------------------------------------------------------------------------
    // int fd, int level, int optname, void *optval, socklen_t *optlen 

    int getsockopt(GETSOCKOPT_SIG)
    {
        DEBUG_INFO("fd=%d", fd);
        if (!check_intercept_enabled() || !connected_to_service(fd))
            return realgetsockopt(fd, level, optname, optval, optlen);
        return zts_getsockopt(fd, level, optname, optval, optlen);
    }

    // ------------------------------------------------------------------------------
    // ----------------------------------- socket() ---------------------------------
    // ------------------------------------------------------------------------------
    // int socket_family, int socket_type, int protocol

    int socket(SOCKET_SIG)
    {   
        DEBUG_ATTN();
        if (!check_intercept_enabled() && socket_type) {
            int err = realsocket(socket_family, socket_type, protocol);
            if(err < 0) {
                perror("socket:\n");
            }
            else {
                DEBUG_INFO("err=%d", err);
                return err;
            }
        }
        // Check if local
        if(socket_family == AF_LOCAL
    #if defined(__linux__)
           || socket_family == AF_NETLINK
    #endif
           || socket_family == AF_UNIX) {
            int err = realsocket(socket_family, socket_type, protocol);
            DEBUG_BLANK("realsocket(): err=%d", err);
            return err;
        }
        return zts_socket(socket_family, socket_type, protocol);
    }

    // ------------------------------------------------------------------------------
    // ---------------------------------- connect() ---------------------------------
    // ------------------------------------------------------------------------------
    // int fd, const struct sockaddr *addr, socklen_t addrlen

    int connect(CONNECT_SIG)
    {
        DEBUG_ATTN("fd=%d", fd);
        struct sockaddr_in *connaddr;
        connaddr = (struct sockaddr_in *)addr;
        if(addr->sa_family == AF_LOCAL || addr->sa_family == AF_UNIX) {
            struct sockaddr_storage storage;
            memcpy(&storage, addr, addrlen);
            struct sockaddr_un *s_un = (struct sockaddr_un*)&storage;
            DEBUG_INFO("addr=%s", s_un->sun_path);
        }
        
        char addrstr[INET6_ADDRSTRLEN];
        if(addr->sa_family == AF_INET) {
            struct sockaddr_in *connaddr = (struct sockaddr_in *)addr;
            inet_ntop(AF_INET, &(connaddr->sin_addr), addrstr, INET_ADDRSTRLEN);    
            sprintf(addrstr, "%s:%d", addrstr, ntohs(connaddr->sin_port));
        }
        if(addr->sa_family == AF_INET6) {        
            struct sockaddr_in6 *connaddr6 = (struct sockaddr_in6 *)addr;
            inet_ntop(AF_INET6, &(connaddr6->sin6_addr), addrstr, INET6_ADDRSTRLEN);
            sprintf(addrstr, "%s:%d", addrstr, ntohs(connaddr6->sin6_port));
        }
        DEBUG_INFO("addr=%s", addrstr);
        
        if(!check_intercept_enabled())
            return realconnect(fd, addr, addrlen);

        // Check that this is a valid fd
        /*
        if(fcntl(fd, F_GETFD) < 0) {
            errno = EBADF;
            return -1;
        }
        */
        // Check that it is a socket 
        int sock_type;
        socklen_t sock_type_len = sizeof(sock_type);
        if(getsockopt(fd, SOL_SOCKET, SO_TYPE, (void *) &sock_type, &sock_type_len) < 0) {
            errno = ENOTSOCK;
            return -1;
        }
    #if defined(__linux__)
        // Check family
        if (connaddr->sin_family < 0 || connaddr->sin_family >= NPROTO){
            errno = EAFNOSUPPORT;
            return -1;
        }
    #endif
        // make sure we don't touch any standard outputs
        if(fd == 0 || fd == 1 || fd == 2)
            return(realconnect(fd, addr, addrlen));
        
        if(addr != NULL && (connaddr->sin_family == AF_LOCAL
    #if defined(__linux__)
                              || connaddr->sin_family == PF_NETLINK
                              || connaddr->sin_family == AF_NETLINK
    #endif
                              || connaddr->sin_family == AF_UNIX)) {
            return realconnect(fd, addr, addrlen);
        }
        return zts_connect(fd, addr, addrlen);
    }

    // ------------------------------------------------------------------------------
    // ------------------------------------ bind() ----------------------------------
    // ------------------------------------------------------------------------------
    // int fd, const struct sockaddr *addr, socklen_t addrlen

    int bind(BIND_SIG)
    {
        DEBUG_ATTN("fd=%d", fd);
        // make sure we don't touch any standard outputs
        if(fd == 0 || fd == 1 || fd == 2)
            return(realbind(fd, addr, addrlen));

        // TODO: Revisit
        char addrstr[INET6_ADDRSTRLEN];
        if(addr->sa_family == AF_INET) {
            struct sockaddr_in *connaddr = (struct sockaddr_in *)addr;
            inet_ntop(AF_INET, &(connaddr->sin_addr), addrstr, INET_ADDRSTRLEN);    
            sprintf(addrstr, "%s:%d", addrstr, ntohs(connaddr->sin_port));
        }
        if(addr->sa_family == AF_INET6) {        
            struct sockaddr_in6 *connaddr6 = (struct sockaddr_in6 *)addr;
            inet_ntop(AF_INET6, &(connaddr6->sin6_addr), addrstr, INET6_ADDRSTRLEN);
            sprintf(addrstr, "%s:%d", addrstr, ntohs(connaddr6->sin6_port));
        }
        DEBUG_INFO("addr=%s", addrstr);

        if(addr->sa_family == AF_LOCAL
        #if defined(__linux__)
           || addr->sa_family == AF_NETLINK
        #endif
           || addr->sa_family == AF_UNIX) {
            int err = realbind(fd, addr, addrlen);
            DEBUG_BLANK("realbind(): err=%d", err);
            return err;
        }

        int sock_type;
        socklen_t sock_type_len = sizeof(sock_type);
        if(getsockopt(fd, SOL_SOCKET, SO_TYPE, (void *) &sock_type, &sock_type_len) < 0) {
            errno = ENOTSOCK;
            return -1;
        }

        // Otherwise, perform usual intercept logic
        if (!check_intercept_enabled())
            return realbind(fd, addr, addrlen);

        // Check that this is a valid fd
        /*
        if(fcntl(fd, F_GETFD) < 0) {
            errno = EBADF;
            return -1;
        }
        */
        // Check that it is a socket
        int opt = -1;
        socklen_t opt_len;
        if(getsockopt(fd, SOL_SOCKET, SO_TYPE, (void *) &opt, &opt_len) < 0) {
            errno = ENOTSOCK;
            return -1;
        }
        return zts_bind(fd, addr, addrlen);
    }

    // ------------------------------------------------------------------------------
    // ----------------------------------- accept4() --------------------------------
    // ------------------------------------------------------------------------------
    // int fd, struct sockaddr *addr, socklen_t *addrlen, int flags

#if defined(__linux__)
    int accept4(ACCEPT4_SIG) {
        DEBUG_ATTN("fd=%d", fd);
        return zts_accept4(fd, addr, addrlen, flags);
    }
#endif

    // ------------------------------------------------------------------------------
    // ----------------------------------- accept() ---------------------------------
    // ------------------------------------------------------------------------------
    // int fd struct sockaddr *addr, socklen_t *addrlen

    int accept(ACCEPT_SIG) {
        DEBUG_ATTN("fd=%d", fd);
        if (!check_intercept_enabled())
            return realaccept(fd, addr, addrlen);

        // Check that this is a valid fd
        if(fcntl(fd, F_GETFD) < 0) {
            return -1;
            errno = EBADF;
            DEBUG_ERROR("EBADF");
            return -1;
        }
        /*
        int opt;
        socklen_t opt_len;
        if(getsockopt(fd, SOL_SOCKET, SO_TYPE, (void *) &opt, &opt_len) < 0) {
            errno = ENOTSOCK;
            return -1;
        }
        // Check that this socket supports accept()
        if((opt != SOCK_STREAM) && (opt != SOCK_SEQPACKET)) {
            errno = EOPNOTSUPP;
            return -1;
        }
        */
        // Check that we haven't hit the soft-limit file descriptors allowed
        struct rlimit rl;
        getrlimit(RLIMIT_NOFILE, &rl);
        if(fd >= rl.rlim_cur){
            errno = EMFILE;
            DEBUG_ERROR("EMFILE");
            return -1;
        }
        // Check address length
        if(addrlen < 0) {
            errno = EINVAL;
            DEBUG_ERROR("EINVAL");
            return -1;
        }
        // redirect calls for standard I/O descriptors to kernel
        if(fd == 0 || fd == 1 || fd == 2){
            DEBUG_BLANK("realaccept(): ");
            return(realaccept(fd, addr, addrlen));
        }

        return zts_accept(fd, addr, addrlen);
    }

    // ------------------------------------------------------------------------------
    // ------------------------------------- listen()--------------------------------
    // ------------------------------------------------------------------------------
    // int fd, int backlog

    int listen(LISTEN_SIG)
    {
        DEBUG_ATTN("fd=%d", fd);
        if (!check_intercept_enabled() || !connected_to_service(fd))
            return reallisten(fd, backlog);
        // make sure we don't touch any standard outputs
        if(fd == 0 || fd == 1 || fd == 2)
            return reallisten(fd, backlog);
        return zts_listen(fd, backlog);
    }

    // ------------------------------------------------------------------------------
    // ------------------------------------- close() --------------------------------
    // ------------------------------------------------------------------------------
    // int fd

    int close(CLOSE_SIG) {
        DEBUG_EXTRA("fd=%d", fd);
        if(!check_intercept_enabled())
            return realclose(fd);
        return zts_close(fd);
    }

    // ------------------------------------------------------------------------------
    // -------------------------------- getsockname() -------------------------------
    // ------------------------------------------------------------------------------
    // int fd, struct sockaddr *addr, socklen_t *addrlen

    int getsockname(GETSOCKNAME_SIG)
    {
        DEBUG_INFO("fd=%d", fd);
    #if !defined(__IOS__)
        if (!check_intercept_enabled())
            return realgetsockname(fd, addr, addrlen);
    #endif
        DEBUG_INFO("fd=%d", fd);
        if(!connected_to_service(fd)) {
            DEBUG_ERROR("fd=%d not used by service", fd);
            return realgetsockname(fd, addr, addrlen);
        }
        return zts_getsockname(fd, addr, addrlen);
    }

    // ------------------------------------------------------------------------------
    // ------------------------------------ syscall() -------------------------------
    // ------------------------------------------------------------------------------
    // long number, ...

#if !defined(__ANDROID__)
#if defined(__linux__)
    long syscall(SYSCALL_SIG)
    {
        va_list ap;
        uintptr_t a,b,c,d,e,f;
        va_start(ap, number);
        a=va_arg(ap, uintptr_t);
        b=va_arg(ap, uintptr_t);
        c=va_arg(ap, uintptr_t);
        d=va_arg(ap, uintptr_t);
        e=va_arg(ap, uintptr_t);
        f=va_arg(ap, uintptr_t);
        va_end(ap);
        
        if (!check_intercept_enabled())
            return realsyscall(number,a,b,c,d,e,f);
        DEBUG_INFO("number=%ld", number);
        
#if defined(__i386__)
        // TODO: Implement for 32-bit systems: syscall(__NR_socketcall, 18, args);
        // args[0] = (unsigned long) fd;
        // args[1] = (unsigned long) addr;
        // args[2] = (unsigned long) addrlen;
        // args[3] = (unsigned long) flags;
#else
        if(number == __NR_accept4) {
            int sockfd = a;
            struct sockaddr * addr = (struct sockaddr*)b;
            socklen_t * addrlen = (socklen_t*)c;
            int flags = d;
            int old_errno = errno;
            int err = accept4(sockfd, addr, addrlen, flags);
            errno = old_errno;
            err = err == -EBADF ? -EAGAIN : err;
            return err;
        }
#endif
        return realsyscall(number,a,b,c,d,e,f);
    }

#endif
#endif

#endif // _SDK_INTERCEPT_

