/*********************************************************************
PicoTCP. Copyright (c) 2013 TASS Belgium NV. Some rights reserved.
See LICENSE and COPYING for usage.
Do not redistribute without a written permission by the Copyright
holders.

Author: Maxime Vincent, Daniele Lacamera
*********************************************************************/
#ifndef PICO_BSD_SOCKETS_H_
#define PICO_BSD_SOCKETS_H_

#include <stdint.h>
#include <signal.h>
#include "pico_defines.h"
#include "pico_constants.h"
#include "pico_config.h"
#include "pico_stack.h"
#include "pico_icmp4.h"
#include "pico_stack.h"
#include "pico_ipv4.h"
#include "pico_ipv6.h"
#include "pico_dns_client.h"
#include "pico_socket.h"

#define SOCKSIZE  16
#define SOCKSIZE6 28

struct pico_bsd_endpoint;
extern void   *picoLock;
extern void   *pico_signal_tick;

#if defined STDSOCKET || defined __socklen_t_defined
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netdb.h>
    #include <fcntl.h>
    #include <sys/poll.h>
    #include <sys/time.h>
    #ifdef __linux__
        #include <linux/tcp.h>
    #endif
    
    static inline int sockopt_get_name(int posix_name)
    {
        switch (posix_name) {
            case IP_MULTICAST_LOOP: return PICO_IP_MULTICAST_LOOP;
            case IP_MULTICAST_TTL: return PICO_IP_MULTICAST_TTL;
            case IP_MULTICAST_IF: return PICO_IP_MULTICAST_IF;
            case IP_ADD_MEMBERSHIP: return PICO_IP_ADD_MEMBERSHIP;
            case IP_DROP_MEMBERSHIP: return PICO_IP_DROP_MEMBERSHIP;
            case SO_RCVBUF   : return PICO_SOCKET_OPT_RCVBUF;
            case SO_SNDBUF   : return PICO_SOCKET_OPT_SNDBUF;
            case TCP_NODELAY : return PICO_TCP_NODELAY;
            case TCP_KEEPCNT   : return PICO_SOCKET_OPT_KEEPCNT;
            case TCP_KEEPIDLE   : return PICO_SOCKET_OPT_KEEPIDLE;
            case TCP_KEEPINTVL   : return PICO_SOCKET_OPT_KEEPINTVL;
        }
        return -1;
    }
    
    #define pico_fd_set     fd_set
    #define PICO_FD_SET     FD_SET
    #define PICO_FD_CLR     FD_CLR
    #define PICO_FD_ISSET   FD_ISSET
    #define PICO_FD_ZERO    FD_ZERO
    
    #undef  fcntl
    #define fcntl           pico_fcntl

#else
    typedef int socklen_t;
    #define AF_INET     (PICO_PROTO_IPV4)
    #define AF_INET6    (PICO_PROTO_IPV6)
    #define SOCK_STREAM  (PICO_PROTO_TCP)
    #define SOCK_DGRAM   (PICO_PROTO_UDP)
    
    #define SOL_SOCKET (0x80)
    
    #define IP_MULTICAST_LOOP   (PICO_IP_MULTICAST_LOOP)
    #define IP_MULTICAST_TTL    (PICO_IP_MULTICAST_TTL)
    #define IP_MULTICAST_IF     (PICO_IP_MULTICAST_IF)
    #define IP_ADD_MEMBERSHIP   (PICO_IP_ADD_MEMBERSHIP)
    #define IP_DROP_MEMBERSHIP  (PICO_IP_DROP_MEMBERSHIP)
    #define SO_RCVBUF           (PICO_SOCKET_OPT_RCVBUF)
    #define SO_SNDBUF           (PICO_SOCKET_OPT_SNDBUF)
    #define TCP_NODELAY         (PICO_TCP_NODELAY)
    #define TCP_KEEPCNT         (PICO_SOCKET_OPT_KEEPCNT)
    #define TCP_KEEPIDLE        (PICO_SOCKET_OPT_KEEPIDLE)
    #define TCP_KEEPINTVL       (PICO_SOCKET_OPT_KEEPINTVL)
    #define SO_ERROR            (4103)
    #define SO_REUSEADDR        (2)
    #define sockopt_get_name(x) ((x))
    
    #define INET_ADDRSTRLEN    (16)
    #define INET6_ADDRSTRLEN   (46)
    
    struct sockaddr {
        uint16_t sa_family;
    }; 
    
    struct in_addr {
        uint32_t s_addr;
    };
    
    #define INADDR_ANY ((uint32_t)0U)
    
    struct in6_addr {
        uint8_t s6_addr[16];
    };
    
    struct __attribute__((packed)) sockaddr_in {
        uint16_t sin_family;
        uint16_t sin_port;
        struct in_addr sin_addr;
        uint8_t _pad[SOCKSIZE - 8];         
    };
    
    
    struct __attribute__((packed)) sockaddr_in6 {
        uint16_t sin6_family;
        uint16_t sin6_port;
        uint32_t sin6_flowinfo;
        struct in6_addr sin6_addr;
        uint32_t sin6_scope_id;
    };
    
    struct __attribute__((packed)) sockaddr_storage {
        uint16_t ss_family;
        uint8_t  _pad[(SOCKSIZE6 - sizeof(uint16_t))];
    };
    
    /* getaddrinfo */
    struct addrinfo {
        int              ai_flags;
        int              ai_family;
        int              ai_socktype;
        int              ai_protocol;
        socklen_t        ai_addrlen;
        struct sockaddr *ai_addr;
        char            *ai_canonname;
        struct addrinfo *ai_next;
    };
    
    
    /* hostent */
    struct hostent {
        char  *h_name;            /* official name of host */
        char **h_aliases;         /* alias list */
        int    h_addrtype;        /* host address type */
        int    h_length;          /* length of address */
        char **h_addr_list;       /* list of addresses */
    };
    #define h_addr h_addr_list[0] /* for backward compatibility */
    
    /* fd_set */
    #ifndef FD_SETSIZE
        #define FD_SETSIZE  64 /* 64 files max, 1 bit per file -> 64bits = 8 bytes */
    #endif
    
    struct pico_fd_set_s {
        uint8_t fds_bits[FD_SETSIZE/8];
    };
    
    typedef struct pico_fd_set_s pico_fd_set;
    #ifndef fd_set
    #define fd_set pico_fd_set
    #endif
    
    //typedef void sigset_t;
    
    #define   PICO_FD_SET(n, p)   ((p)->fds_bits[(n)/8] |=  (1u << ((n) % 8)))
    #define   PICO_FD_CLR(n, p)   ((p)->fds_bits[(n)/8] &= ~(1u << ((n) % 8)))
    #define   PICO_FD_ISSET(n, p) ((p)->fds_bits[(n)/8] &   (1u << ((n) % 8)))
    #define   PICO_FD_ZERO(p)     do{memset((p)->fds_bits, 0, sizeof(struct pico_fd_set_s));}while(0)
    
    /* Not socket related */
    #ifndef __time_t_defined
        typedef pico_time time_t;
        #define __time_t_defined
    #endif

    #if !defined _TIME_H && !defined _TIMEVAL_DEFINED && !defined _STRUCT_TIMEVAL
        struct timeval {
            time_t tv_sec;
            time_t tv_usec;
        };

        #if !defined __timespec_defined && !defined _SYS__TIMESPEC_H_
            struct timespec {
                long tv_sec;
                long tv_nsec;
            };
        #endif
        
        struct timezone {
            int tz_minuteswest;     /* minutes west of Greenwich */
            int tz_dsttime;         /* type of DST correction */
        };
        #define _TIMEVAL_DEFINED
    #endif
#endif /* STDSOCKET */

#ifndef SO_REUSEPORT
    #define SO_REUSEPORT    (15)
#endif

#ifndef FD_CLOEXEC
    #define FD_CLOEXEC	1
#endif

#ifndef F_DUPFD
    #define F_DUPFD 0
#endif

#ifndef F_GETFD
    #define F_GETFD 1
#endif

#ifndef F_SETFD
    #define F_SETFD 2 
#endif

#ifndef F_GETFL
    #define F_GETFL 3
#endif

#ifndef F_SETFL
    #define F_SETFL 4
#endif


#ifndef O_NONBLOCK
    #define O_NONBLOCK  0x4000
#endif

#ifndef _SYS_POLL_H
    #define POLLIN      0x001       /* There is data to read.  */
    #define POLLPRI     0x002       /* There is urgent data to read.  */
    #define POLLOUT     0x004       /* Writing now will not block.  */
    #define POLLRDNORM 0x040       /* Normal data may be read.  */
    #define POLLRDBAND 0x080       /* Priority data may be read.  */
    #define POLLWRNORM 0x100       /* Writing now will not block.  */
    #define POLLWRBAND 0x200       /* Priority data may be written.  */
    
    #define POLLMSG    0x400
    #define POLLREMOVE 0x1000
    #define POLLRDHUP  0x2000
    
    #define POLLERR     0x008       /* Error condition.  */
    #define POLLHUP     0x010       /* Hung up.  */
    #define POLLNVAL    0x020       /* Invalid polling request.  */
    
    typedef unsigned long int nfds_t;
    
    struct pollfd {
        int fd;
        uint16_t events;
        uint16_t revents;
    };
#endif

int pico_newsocket(int domain, int type, int proto);
int pico_bind(int sd, struct sockaddr * local_addr, socklen_t socklen);
int pico_listen(int sd, int backlog);
int pico_connect(int sd, struct sockaddr *_saddr, socklen_t socklen);
int pico_isconnected(int sd);
int pico_accept(int sd, struct sockaddr *_orig, socklen_t *socklen);
int pico_sendto(int sd, void * buf, int len, int flags, struct sockaddr *_dst, socklen_t socklen);
int pico_recvfrom(int sd, void * buf, int len, int flags, struct sockaddr *_addr, socklen_t *socklen);
int pico_write(int sd, void * buf, int len);
int pico_send(int sd, void * buf, int len, int flags);
int pico_read(int sd, void * buf, int len);
int pico_recv(int sd, void * buf, int len, int flags);
int pico_close(int sd);
int pico_shutdown(int sd, int how);
int pico_getsockname(int sd, struct sockaddr * local_addr, socklen_t *socklen);
int pico_getpeername(int sd, struct sockaddr * remote_addr, socklen_t *socklen);
int pico_fcntl(int sd, int cmd, int arg);
int pico_join_multicast_group(int sd, const char *address, const char *local);

#ifdef PICO_SUPPORT_DNS_CLIENT
    struct hostent *pico_gethostbyname(const char *name);
    
    /* getaddrinfo */
    int pico_getaddrinfo(const char *node, const char *service,
                           const struct addrinfo *hints,
                           struct addrinfo **res);
    
    void pico_freeaddrinfo(struct addrinfo *res);
#endif

int pico_setsockopt          (int sockfd, int level, int optname, const void *optval, socklen_t optlen); 
int pico_getsockopt          (int sockfd, int level, int optname, void *optval, socklen_t *optlen);

int pico_select              (int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
int pico_pselect             (int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, const struct timespec *timeout, 
                                    const sigset_t *sigmask);

int pico_poll                (struct pollfd *pfd, nfds_t npfd, int timeout);
int pico_ppoll               (struct pollfd *pfd, nfds_t npfd, const struct timespec *timeout_ts, const sigset_t *sigmask);

#ifndef PICO_SUPPORT_SNTP_CLIENT
    struct pico_timeval
    {
        pico_time tv_sec;
        pico_time tv_msec;
    };
    int pico_settimeofday(struct timeval *tv, struct timezone *tz);
#endif

int pico_gettimeofday(struct timeval *tv, struct timezone *tz);
long XTIME(void);
#define XGMTIME gmtime

/* arpa/inet.h */
const char *pico_inet_ntop   (int af, const void *src, char *dst, socklen_t size);
char       *pico_inet_ntoa   (struct in_addr in);

/* Non-POSIX */
void                        pico_bsd_init(void);
void                        pico_bsd_deinit(void);
void                        pico_bsd_stack_tick(void);
void                        pico_bsd_stack_tick_timeout(int timeout_ms);
uint16_t                    pico_bsd_select(struct pico_bsd_endpoint *ep);

#ifdef REPLACE_STDCALLS
    #include "pico_bsd_syscalls.h"
#endif

#endif /* PICO_BSD_SOCKETS_H_ */
