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
#include <sys/socket.h>
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

#include "SDK.h"
#include "SDK_Debug.h"
#include "SDK_RPC.h"

void dwr(int level, const char *fmt, ... );
pthread_key_t thr_id_key;
char *api_netpath;

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

    void load_symbols()
    {
        dwr(MSG_DEBUG_EXTRA,"load_symbols\n");
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
    // Return whether 'intercept' shim is enabled for this thread

    bool check_intercept_enabled_for_thread() {
        dwr(MSG_DEBUG_EXTRA, "check_intercept_enabled_for_thread()\n");
        if(!realconnect){
            load_symbols();
        }
    #if defined(SDK_BUNDLED)
        /* The reasoning for this check is that if you've built the SDK with SDK_BUNDLE=1, then 
        you've included a full ZeroTier service in the same binary as your intercept, and we 
        don't want to run ZeroTier network API calls through the intercept, so we must specify
        which threads should be intercepted manually */
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
        dwr(MSG_DEBUG,"connected_to_service(): Not connected to service\n");
        return 0;
    }
    
    // ------------------------------------------------------------------------------
    // ------------------------------------ sendto() --------------------------------
    // ------------------------------------------------------------------------------
    // int sockfd, const void *buf, size_t len, int flags, 
    // const struct sockaddr *addr, socklen_t addr_len

#if !defined(__ANDROID__)
    ssize_t sendto(SENDTO_SIG)
    {
        dwr(MSG_DEBUG, "sendto(%d, %d)\n", sockfd, len);
        //if (!check_intercept_enabled_for_thread())
            return realsendto(sockfd, buf, len, flags, addr, addr_len);
        return zt_sendto(sockfd, buf, len, flags, addr, addr_len);
    }
#endif

    // ------------------------------------------------------------------------------
    // ----------------------------------- sendmsg() --------------------------------
    // ------------------------------------------------------------------------------
    // int socket, const struct msghdr *message, int flags

#if !defined(__ANDROID__)
    ssize_t sendmsg(SENDMSG_SIG)
    {
        dwr(MSG_DEBUG, "sendmsg()\n");
        //if(!check_intercept_enabled_for_thread())
            return realsendmsg(socket, message, flags);
        zt_sendmsg(socket, message, flags);
    }
#endif
    
    // ------------------------------------------------------------------------------
    // ---------------------------------- recvfrom() --------------------------------
    // ------------------------------------------------------------------------------
    // int socket, void *restrict buffer, size_t length, int flags, struct sockaddr
    // *restrict address, socklen_t *restrict address_len

#if !defined(__ANDROID__)
    ssize_t recvfrom(RECVFROM_SIG)
    {
        dwr(MSG_DEBUG, "recvfrom(%d)\n", socket);
        //if(!check_intercept_enabled_for_thread())
            return realrecvfrom(socket, buffer, length, flags, address, address_len);
        return zt_recvfrom(socket, buffer, length, flags, address, address_len);
    }
#endif

    // ------------------------------------------------------------------------------
    // ----------------------------------- recvmsg() --------------------------------
    // ------------------------------------------------------------------------------
    // int socket, struct msghdr *message, int flags

#if !defined(__ANDROID__)
    ssize_t recvmsg(RECVMSG_SIG)
    {
        dwr(MSG_DEBUG, "recvmsg(%d)\n", socket);
        //if(!check_intercept_enabled_for_thread())
            return realrecvmsg(socket, message, flags);
        return zt_recvmsg(socket, message, flags);
    }
#endif

    // ------------------------------------------------------------------------------
    // --------------------------------- setsockopt() -------------------------------
    // ------------------------------------------------------------------------------
    // int socket, int level, int option_name, const void *option_value, 
    // socklen_t option_len

    int setsockopt(SETSOCKOPT_SIG)
    {
        dwr(MSG_DEBUG, "setsockopt(%d)\n", socket);
        if (!check_intercept_enabled_for_thread())
            return realsetsockopt(socket, level, option_name, option_value, option_len);
    #if defined(__linux__)
        if(level == SOL_IPV6 && option_name == IPV6_V6ONLY)
            return 0;
        if(level == SOL_IP && (option_name == IP_TTL || option_name == IP_TOS))
            return 0;
    #endif
        if(level == IPPROTO_TCP || (level == SOL_SOCKET && option_name == SO_KEEPALIVE))
            return 0;
        if(realsetsockopt(socket, level, option_name, option_value, option_len) < 0)
            perror("setsockopt():\n");
        return zt_setsockopt(socket, level, option_name, option_value, option_len);
    }

    // ------------------------------------------------------------------------------
    // --------------------------------- getsockopt() -------------------------------
    // ------------------------------------------------------------------------------
    // int sockfd, int level, int optname, void *optval, 
    // socklen_t *optlen 

    int getsockopt(GETSOCKOPT_SIG)
    {
        dwr(MSG_DEBUG, "getsockopt(%d)\n", sockfd);
        if (!check_intercept_enabled_for_thread() || !connected_to_service(sockfd))
            return realgetsockopt(sockfd, level, optname, optval, optlen);
        return zt_getsockopt(sockfd, level, optname, optval, optlen);
    }

    // ------------------------------------------------------------------------------
    // ----------------------------------- socket() ---------------------------------
    // ------------------------------------------------------------------------------
    // int socket_family, int socket_type, int protocol

    int socket(SOCKET_SIG)
    {
        dwr(MSG_DEBUG, "socket()\n");
        if (!check_intercept_enabled_for_thread() && socket_type) {
            int err = realsocket(socket_family, socket_type, protocol);
            if(err < 0) {
                perror("socket:\n");
            }
            else {
                dwr(MSG_DEBUG, " socket() = %d\n", err);
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
            dwr(MSG_DEBUG,"realsocket() = %d\n", err);
            return err;
        }
        return zt_socket(socket_family, socket_type, protocol);
    }

    // ------------------------------------------------------------------------------
    // ---------------------------------- connect() ---------------------------------
    // ------------------------------------------------------------------------------
    // int __fd, const struct sockaddr * __addr, socklen_t __len

    int connect(CONNECT_SIG)
    {
        dwr(MSG_DEBUG, "connect(%d)\n", __fd);
        struct sockaddr_in *connaddr;
        connaddr = (struct sockaddr_in *)__addr;
        if(__addr->sa_family == AF_LOCAL || __addr->sa_family == AF_UNIX) {
            struct sockaddr_storage storage;
            memcpy(&storage, __addr, __len);
            struct sockaddr_un *s_un = (struct sockaddr_un*)&storage;
            dwr(MSG_DEBUG, "connect(): address = %s\n", s_un->sun_path);
        }
        
        int port = connaddr->sin_port;
        int ip = connaddr->sin_addr.s_addr;
        unsigned char d[4];
        d[0] = ip & 0xFF;
        d[1] = (ip >>  8) & 0xFF;
        d[2] = (ip >> 16) & 0xFF;
        d[3] = (ip >> 24) & 0xFF;
        dwr(MSG_DEBUG,"connect(): %d.%d.%d.%d: %d\n", d[0],d[1],d[2],d[3], ntohs(port));
        
        if(!check_intercept_enabled_for_thread())
            return realconnect(__fd, __addr, __len);

        /* Check that this is a valid fd */
        if(fcntl(__fd, F_GETFD) < 0) {
            errno = EBADF;
            return -1;
        }
        /* Check that it is a socket */
        int sock_type;
        socklen_t sock_type_len = sizeof(sock_type);
        if(getsockopt(__fd, SOL_SOCKET, SO_TYPE, (void *) &sock_type, &sock_type_len) < 0) {
            errno = ENOTSOCK;
            return -1;
        }
    #if defined(__linux__)
        /* Check family */
        if (connaddr->sin_family < 0 || connaddr->sin_family >= NPROTO){
            errno = EAFNOSUPPORT;
            return -1;
        }
    #endif
        /* make sure we don't touch any standard outputs */
        if(__fd == 0 || __fd == 1 || __fd == 2)
            return(realconnect(__fd, __addr, __len));
        
        if(__addr != NULL && (connaddr->sin_family == AF_LOCAL
    #if defined(__linux__)
                              || connaddr->sin_family == PF_NETLINK
                              || connaddr->sin_family == AF_NETLINK
    #endif
                              || connaddr->sin_family == AF_UNIX)) {
            return realconnect(__fd, __addr, __len);
        }
        return zt_connect(__fd, __addr, __len);
    }

    // ------------------------------------------------------------------------------
    // ------------------------------------ bind() ----------------------------------
    // ------------------------------------------------------------------------------
    // int sockfd, const struct sockaddr *addr, socklen_t addrlen

    int bind(BIND_SIG)
    {
        dwr(MSG_DEBUG,"bind(%d)\n", sockfd);
        // make sure we don't touch any standard outputs
        if(sockfd == 0 || sockfd == 1 || sockfd == 2)
            return(realbind(sockfd, addr, addrlen));
        struct sockaddr_in *connaddr;
        connaddr = (struct sockaddr_in *)addr;

        if(connaddr->sin_family == AF_LOCAL
        #if defined(__linux__)
           || connaddr->sin_family == AF_NETLINK
        #endif
           || connaddr->sin_family == AF_UNIX) {
            int err = realbind(sockfd, addr, addrlen);
            dwr(MSG_DEBUG,"realbind, err = %d\n", err);
            return err;
        }
        int port = connaddr->sin_port;
        int ip = connaddr->sin_addr.s_addr;
        unsigned char d[4];
        d[0] = ip & 0xFF;
        d[1] = (ip >>  8) & 0xFF;
        d[2] = (ip >> 16) & 0xFF;
        d[3] = (ip >> 24) & 0xFF;
        dwr(MSG_DEBUG,"bind(): %d.%d.%d.%d: %d\n", d[0],d[1],d[2],d[3], ntohs(port));

        int sock_type;
        socklen_t sock_type_len = sizeof(sock_type);
        if(getsockopt(sockfd, SOL_SOCKET, SO_TYPE, (void *) &sock_type, &sock_type_len) < 0) {
            errno = ENOTSOCK;
            return -1;
        }

        // Otherwise, perform usual intercept logic
        if (!check_intercept_enabled_for_thread())
            return realbind(sockfd, addr, addrlen);

        // Check that this is a valid fd
        if(fcntl(sockfd, F_GETFD) < 0) {
            errno = EBADF;
            return -1;
        }
        // Check that it is a socket
        int opt = -1;
        socklen_t opt_len;
        if(getsockopt(sockfd, SOL_SOCKET, SO_TYPE, (void *) &opt, &opt_len) < 0) {
            errno = ENOTSOCK;
            return -1;
        }
        return zt_bind(sockfd, addr, addrlen);
    }

    // ------------------------------------------------------------------------------
    // ----------------------------------- accept4() --------------------------------
    // ------------------------------------------------------------------------------
    // int sockfd, struct sockaddr *addr, socklen_t *addrlen, int flags

#if defined(__linux__)
    int accept4(ACCEPT4_SIG) {
        dwr(MSG_DEBUG,"accept4(%d):\n", sockfd);
        return zt_accept4(sockfd, addr, addrlen, flags);
    }
#endif

    // ------------------------------------------------------------------------------
    // ----------------------------------- accept() ---------------------------------
    // ------------------------------------------------------------------------------
    // int sockfd struct sockaddr *addr, socklen_t *addrlen

    int accept(ACCEPT_SIG) {
        dwr(MSG_DEBUG,"accept(%d):\n", sockfd);
        if (!check_intercept_enabled_for_thread())
            return realaccept(sockfd, addr, addrlen);

        /* Check that this is a valid fd */
        if(fcntl(sockfd, F_GETFD) < 0) {
            return -1;
            errno = EBADF;
            dwr(MSG_DEBUG,"EBADF\n");
            return -1;
        }
        /* Check that it is a socket */
        int opt;
        socklen_t opt_len;
        if(getsockopt(sockfd, SOL_SOCKET, SO_TYPE, (void *) &opt, &opt_len) < 0) {
            errno = ENOTSOCK;
            dwr(MSG_DEBUG,"ENOTSOCK\n");
            return -1;
        }
        /* Check that this socket supports accept() */
        if(!(opt && (SOCK_STREAM | SOCK_SEQPACKET))) {
            errno = EOPNOTSUPP;
            dwr(MSG_DEBUG,"EOPNOTSUPP\n");
            return -1;
        }
        /* Check that we haven't hit the soft-limit file descriptors allowed */
        struct rlimit rl;
        getrlimit(RLIMIT_NOFILE, &rl);
        if(sockfd >= rl.rlim_cur){
            errno = EMFILE;
            dwr(MSG_DEBUG,"EMFILE\n");
            return -1;
        }
        /* Check address length */
        if(addrlen < 0) {
            errno = EINVAL;
            dwr(MSG_DEBUG,"EINVAL\n");
            return -1;
        }
        /* redirect calls for standard I/O descriptors to kernel */
        if(sockfd == 0 || sockfd == 1 || sockfd == 2){
            dwr(MSG_DEBUG,"realaccept():\n");
            return(realaccept(sockfd, addr, addrlen));
        }

        return zt_accept(sockfd, addr, addrlen);
    }

    // ------------------------------------------------------------------------------
    // ------------------------------------- listen()--------------------------------
    // ------------------------------------------------------------------------------
    // int sockfd, int backlog

    int listen(LISTEN_SIG)
    {
        dwr(MSG_DEBUG,"listen(%d):\n", sockfd);
        if (!check_intercept_enabled_for_thread())
            return reallisten(sockfd, backlog);
        
        int sock_type;
        socklen_t sock_type_len = sizeof(sock_type);
        /* Check that this is a valid fd */
        if(fcntl(sockfd, F_GETFD) < 0) {
            errno = EBADF;
            return -1;
        }
        /* Check that it is a socket */
        if(getsockopt(sockfd, SOL_SOCKET, SO_TYPE, (void *) &sock_type, &sock_type_len) < 0) {
            errno = ENOTSOCK;
            return -1;
        }
        /* Check that this socket supports accept() */
        if(!(sock_type && (SOCK_STREAM | SOCK_SEQPACKET))) {
            errno = EOPNOTSUPP;
            return -1;
        }
        /* make sure we don't touch any standard outputs */
        if(sockfd == 0 || sockfd == 1 || sockfd == 2)
            return reallisten(sockfd, backlog);
        
        if(!connected_to_service(sockfd)) {
            return reallisten(sockfd, backlog);
        }
        return zt_listen(sockfd, backlog);
    }

    // ------------------------------------------------------------------------------
    // ------------------------------------- close() --------------------------------
    // ------------------------------------------------------------------------------
    // int fd

    int close(CLOSE_SIG) {
        dwr(MSG_DEBUG, " close(%d)\n", fd);
        if(!check_intercept_enabled_for_thread()) { 
            return realclose(fd);
        }
        zt_close(fd);
    }

    // ------------------------------------------------------------------------------
    // -------------------------------- getsockname() -------------------------------
    // ------------------------------------------------------------------------------
    // int sockfd, struct sockaddr *addr, socklen_t *addrlen

    int getsockname(GETSOCKNAME_SIG)
    {
        dwr(MSG_DEBUG,"getsockname(%d):\n", sockfd);
    #if !defined(__IOS__)
        if (!check_intercept_enabled_for_thread())
            return realgetsockname(sockfd, addr, addrlen);
    #endif
        dwr(MSG_DEBUG,"getsockname(%d)\n", sockfd);
        if(!connected_to_service(sockfd)) {
            dwr(MSG_DEBUG,"getsockname(): not used by service\n");
            return realgetsockname(sockfd, addr, addrlen);
        }
        return zt_getsockname(sockfd, addr, addrlen);
    }


    // ------------------------------------------------------------------------------
    // ------------------------------------ syscall() -------------------------------
    // ------------------------------------------------------------------------------

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
        
        if (!check_intercept_enabled_for_thread())
            return realsyscall(number,a,b,c,d,e,f);
        dwr(MSG_DEBUG,"syscall(%u, ...)\n", number);
        
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

#endif // ZT_SDK_INTERCEPT

