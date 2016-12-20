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

#ifndef SDK_LWIPSTACK_H
#define SDK_LWIPSTACK_H

#if defined (SDK_LWIP)

#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/init.h"
#include "lwip/udp.h"
#include "lwip/tcp.h"
#include "lwip/priv/tcp_priv.h"

#include "Mutex.hpp"
#include "OSUtils.hpp"
#include "debug.h"

#include <stdio.h>
#include <dlfcn.h>

#include "tap.hpp"

namespace ZeroTier {
    class NetconEthernetTap;
    struct Connection;

    void lwip_init_interface(NetconEthernetTap *tap, const InetAddress &ip);
    void lwip_loop(NetconEthernetTap *tap);
    void lwip_rx(NetconEthernetTap *tap, const MAC &from,const MAC &to,unsigned int etherType,const void *data,unsigned int len);
    Connection *lwip_handleSocket(NetconEthernetTap *tap, PhySocket *sock, void **uptr, struct socket_st* socket_rpc);
    Connection * lwip_handleSocketProxy(NetconEthernetTap *tap, PhySocket *sock, int socket_type);
    void lwip_handleConnect(NetconEthernetTap *tap, PhySocket *sock, PhySocket *rpcSock, Connection *conn, struct connect_st* connect_rpc);
    int lwip_handleConnectProxy(NetconEthernetTap *tap, PhySocket *sock, struct sockaddr_in *rawAddr);
    void lwip_handleBind(NetconEthernetTap *tap, PhySocket *sock, PhySocket *rpcSock, void **uptr, struct bind_st *bind_rpc);
    void lwip_handleListen(NetconEthernetTap *tap, PhySocket *sock, PhySocket *rpcSock, void **uptr, struct listen_st *listen_rpc);
    void lwip_handleRead(NetconEthernetTap *tap, PhySocket *sock, void **uptr, bool lwip_invoked);
    void lwip_handleWrite(NetconEthernetTap *tap, Connection *conn);
    void lwip_handleClose(NetconEthernetTap *tap, PhySocket *sock, Connection *conn);



    err_t tapif_init(struct netif *netif);
    err_t low_level_output(struct netif *netif, struct pbuf *p);

    /*
     * Callback from LWIP for when data is available to be read from the network.
     *
     * Data is in the form of a linked list of struct pbufs, it is then recombined and
     * send to the client over the associated unix socket.
     *
     * @param associated service state object
     * @param allocated PCB
     * @param chain of pbufs
     * @param error code
     * @return ERR_OK if everything is ok, -1 otherwise
     *
     */
    err_t nc_recved(void *arg, struct tcp_pcb *PCB, struct pbuf *p, err_t err);

    /*
     * Callback from LWIP for when a connection has been accepted and the PCB has been
     * put into an ACCEPT state.
     *
     * A socketpair is created, one end is kept and wrapped into a PhySocket object
     * for use in the main ZT I/O loop, and one end is sent to the client. The client
     * is then required to tell the service what new file descriptor it has allocated
     * for this connection. After the mapping is complete, the accepted socket can be
     * used.
     *
     * @param associated service state object
     * @param newly allocated PCB
     * @param error code
     * @return ERR_OK if everything is ok, -1 otherwise
     *
     *   i := should be implemented in intercept lib
     *   I := is implemented in intercept lib
     *   X := is implemented in service
     *   ? := required treatment Unknown
     *   - := Not needed
     *
     *  [ ] EAGAIN or EWOULDBLOCK - The socket is marked nonblocking and no connections are present
     *                                                  to be accepted. POSIX.1-2001 allows either error to be returned for
     *                                                  this case, and does not require these constants to have the same value,
     *                                                  so a portable application should check for both possibilities.
     *  [I] EBADF - The descriptor is invalid.
     *  [I] ECONNABORTED - A connection has been aborted.
     *  [i] EFAULT - The addr argument is not in a writable part of the user address space.
     *  [-] EINTR - The system call was interrupted by a signal that was caught before a valid connection arrived; see signal(7).
     *  [I] EINVAL - Socket is not listening for connections, or addrlen is invalid (e.g., is negative).
     *  [I] EINVAL - (accept4()) invalid value in flags.
     *  [I] EMFILE - The per-process limit of open file descriptors has been reached.
     *  [ ] ENFILE - The system limit on the total number of open files has been reached.
     *  [ ] ENOBUFS, ENOMEM - Not enough free memory. This often means that the memory allocation is
     *                                              limited by the socket buffer limits, not by the system memory.
     *  [I] ENOTSOCK - The descriptor references a file, not a socket.
     *  [I] EOPNOTSUPP - The referenced socket is not of type SOCK_STREAM.
     *  [ ] EPROTO - Protocol error.
     *
     */
    err_t nc_accept(void *arg, struct tcp_pcb *newPCB, err_t err);

    
    err_t nc_recved_proxy(void *arg, struct tcp_pcb *PCB, struct pbuf *p, err_t err);
    void nc_udp_recved(void * arg, struct udp_pcb * upcb, struct pbuf * p, ip_addr_t * addr, u16_t port);

    
    /*
     * Callback from LWIP when an internal error is associtated with the given (arg)
     *
     * Since the PCB related to this error might no longer exist, only its perviously
     * associated (arg) is provided to us.
     *
     * @param associated service state object
     * @param error code
     *
     */
    void nc_err(void *arg, err_t err);

    /*
     * Callback from LWIP to do whatever work we might need to do.
     *
     * @param associated service state object
     * @param PCB we're polling on
     * @return ERR_OK if everything is ok, -1 otherwise
     *
     */
    err_t nc_poll(void* arg, struct tcp_pcb *PCB);

    /*
     * Callback from LWIP to signal that 'len' bytes have successfully been sent.
     * As a result, we should put our socket back into a notify-on-readability state
     * since there is now room on the PCB buffer to write to.
     *
     * NOTE: This could be used to track the amount of data sent by a connection.
     *
     * @param associated service state object
     * @param relevant PCB
     * @param length of data sent
     * @return ERR_OK if everything is ok, -1 otherwise
     *
     */
    err_t nc_sent(void *arg, struct tcp_pcb *PCB, u16_t len);

    /*
     * Callback from LWIP which sends a return value to the client to signal that
     * a connection was established for this PCB
     *
     * @param associated service state object
     * @param relevant PCB
     * @param error code
     * @return ERR_OK if everything is ok, -1 otherwise
     *
     */
    err_t nc_connected(void *arg, struct tcp_pcb *PCB, err_t err);
    err_t nc_connected_proxy(void *arg, struct tcp_pcb *PCB, err_t err);
}

#ifdef D_GNU_SOURCE
#define _GNU_SOURCE
#endif

struct tcp_pcb;

#if defined(SDK_IPV4)
    #define NETIF_ADD_SIG struct netif *netif, ip_addr_t *ipaddr, ip_addr_t *netmask, ip_addr_t *gw, void *state, netif_init_fn init, netif_input_fn input
    #define ETHARP_OUTPUT_SIG struct netif *netif, struct pbuf *q, const ip4_addr_t *ipaddr
#endif

#if defined(SDK_IPV6)
    #define NETIF_ADD_SIG struct netif *netif, void *state, netif_init_fn init, netif_input_fn input
    #define ETHIP6_OUTPUT_SIG struct netif *netif, struct pbuf *q, const ip6_addr_t *ip6addr
    #define ETHARP_OUTPUT_SIG struct netif *netif, struct pbuf *q, const ip6_addr_t *ipaddr
    #define NETIF_IP6_ADDR_SET_STATE_SIG struct netif* netif, s8_t addr_idx, u8_t state
    #define NETIF_CREATE_IP6_LINKLOCAL_ADDRESS_SIG struct netif *netif, u8_t from_mac_48bit
#endif

// lwip General Stack API
#define PBUF_FREE_SIG struct pbuf *p
#define PBUF_ALLOC_SIG pbuf_layer layer, u16_t length, pbuf_type type
#define LWIP_HTONS_SIG u16_t x
#define LWIP_NTOHS_SIG u16_t x

// lwIP UDP API
#define UDP_NEW_SIG void
#define UDP_CONNECT_SIG struct udp_pcb * pcb, ip_addr_t * ipaddr, u16_t port
#define UDP_SEND_SIG struct udp_pcb * pcb, struct pbuf * p
#define UDP_SENDTO_SIG struct udp_pcb *pcb, struct pbuf *p, ip_addr_t *dst_ip, u16_t dst_port
#define UDP_RECV_SIG struct udp_pcb * pcb, void (* recv)(void * arg, struct udp_pcb * upcb, struct pbuf * p, ip_addr_t * addr, u16_t port), void * recv_arg
#define UDP_RECVED_SIG struct udp_pcb * pcb, u16_t len
#define UDP_BIND_SIG struct udp_pcb * pcb, const ip_addr_t * ipaddr, u16_t port
#define UDP_REMOVE_SIG struct udp_pcb *pcb

// lwIP TCP API
#define TCP_WRITE_SIG struct tcp_pcb *pcb, const void *arg, u16_t len, u8_t apiflags
#define TCP_SENT_SIG struct tcp_pcb * pcb, err_t (* sent)(void * arg, struct tcp_pcb * tpcb, u16_t len)
#define TCP_NEW_SIG void
#define TCP_RECV_SIG struct tcp_pcb * pcb, err_t (* recv)(void * arg, struct tcp_pcb * tpcb, struct pbuf * p, err_t err)
#define TCP_RECVED_SIG struct tcp_pcb * pcb, u16_t len
#define TCP_SNDBUF_SIG struct tcp_pcb * pcb
#define TCP_CONNECT_SIG struct tcp_pcb * pcb, ip_addr_t * ipaddr, u16_t port, err_t (* connected)(void * arg, struct tcp_pcb * tpcb, err_t err)
#define TCP_RECV_SIG struct tcp_pcb * pcb, err_t (* recv)(void * arg, struct tcp_pcb * tpcb, struct pbuf * p, err_t err)
#define TCP_ERR_SIG struct tcp_pcb * pcb, void (* err)(void * arg, err_t err)
#define TCP_POLL_SIG struct tcp_pcb * pcb, err_t (* poll)(void * arg, struct tcp_pcb * tpcb), u8_t interval
#define TCP_ARG_SIG struct tcp_pcb * pcb, void * arg
#define TCP_CLOSE_SIG struct tcp_pcb * pcb
#define TCP_ABORT_SIG struct tcp_pcb * pcb
#define TCP_OUTPUT_SIG struct tcp_pcb * pcb
#define TCP_ACCEPT_SIG struct tcp_pcb * pcb, err_t (* accept)(void * arg, struct tcp_pcb * newpcb, err_t err)
#define TCP_LISTEN_SIG struct tcp_pcb * pcb
#define TCP_LISTEN_WITH_BACKLOG_SIG struct tcp_pcb * pcb, u8_t backlog
#define TCP_BIND_SIG struct tcp_pcb * pcb, const ip_addr_t * ipaddr, u16_t port
#define TCP_INPUT_SIG struct pbuf *p, struct netif *inp

// lwIP network stack interfaces
#define ETHERNET_INPUT_SIG struct pbuf *p, struct netif *netif
#define IP_INPUT_SIG struct pbuf *p, struct netif *inp
#define NETIF_SET_DEFAULT_SIG struct netif *netif
#define NETIF_SET_UP_SIG struct netif *netif
#define NETIF_POLL_SIG struct netif *netif

//#define NETIF_SET_ADDR_SIG struct netif *netif, const ip4_addr_t *ipaddr, const ip4_addr_t *netmask, const ip4_addr_t *gw

namespace ZeroTier {
    
    /**
     * Loads an instance of liblwip.so in a private memory arena
     *
     * This uses dlmopen() to load an instance of the LWIP stack into its
     * own private memory space. This is done to get around the stack's
     * lack of thread-safety or multi-instance support. The alternative
     * would be to massively refactor the stack so everything lives in a
     * state object instead of static memory space.
     */
    class lwIP_stack
    {
    public:
        
        void *_libref;
        
        void close() {
#if defined(__STATIC_STACK__)
            return;
#elif defined(__DYNAMIC_STACK__)
            dlclose(_libref);
#endif
        }

        #if defined(SDK_IPV4)
                err_t (*_etharp_output)(ETHARP_OUTPUT_SIG);
        #endif
        
        #if defined(SDK_IPV6)
            err_t (*_ethip6_output)(ETHIP6_OUTPUT_SIG);
            void (*_nd6_tmr)(void);
            void (*_netif_ip6_addr_set_state)(NETIF_IP6_ADDR_SET_STATE_SIG);
            void (*_netif_create_ip6_linklocal_address)(NETIF_CREATE_IP6_LINKLOCAL_ADDRESS_SIG);
        #endif

        void (*_netif_init)(void);
        // void (*_netif_set_addr)(NETIF_SET_ADDR_SIG);

        void (*_lwip_init)();
        err_t (*_tcp_write)(TCP_WRITE_SIG);
        void (*_tcp_sent)(TCP_SENT_SIG);
        struct tcp_pcb * (*_tcp_new)(TCP_NEW_SIG);
        u16_t (*_tcp_sndbuf)(TCP_SNDBUF_SIG);
        err_t (*_tcp_connect)(TCP_CONNECT_SIG);
        struct udp_pcb * (*_udp_new)(UDP_NEW_SIG);
        err_t (*_udp_connect)(UDP_CONNECT_SIG);
        err_t (*_udp_send)(UDP_SEND_SIG);
        err_t (*_udp_sendto)(UDP_SENDTO_SIG);
        void (*_udp_recv)(UDP_RECV_SIG);
        void (*_udp_recved)(UDP_RECVED_SIG);
        err_t (*_udp_bind)(UDP_BIND_SIG);
        void (*_udp_remove)(UDP_REMOVE_SIG);
        void (*_tcp_recv)(TCP_RECV_SIG);
        void (*_tcp_recved)(TCP_RECVED_SIG);
        void (*_tcp_err)(TCP_ERR_SIG);
        void (*_tcp_poll)(TCP_POLL_SIG);
        void (*_tcp_arg)(TCP_ARG_SIG);
        err_t (*_tcp_close)(TCP_CLOSE_SIG);
        void (*_tcp_abort)(TCP_ABORT_SIG);
        err_t (*_tcp_output)(TCP_OUTPUT_SIG);
        void (*_tcp_accept)(TCP_ACCEPT_SIG);
        struct tcp_pcb * (*_tcp_listen)(TCP_LISTEN_SIG);
        struct tcp_pcb * (*_tcp_listen_with_backlog)(TCP_LISTEN_WITH_BACKLOG_SIG);
        err_t (*_tcp_bind)(TCP_BIND_SIG);
        void (*_etharp_tmr)(void);
        void (*_tcp_tmr)(void);
        u8_t (*_pbuf_free)(PBUF_FREE_SIG);
        struct pbuf * (*_pbuf_alloc)(PBUF_ALLOC_SIG);
        u16_t (*_lwip_htons)(LWIP_HTONS_SIG);
        u16_t (*_lwip_ntohs)(LWIP_NTOHS_SIG);
        err_t (*_ethernet_input)(ETHERNET_INPUT_SIG);
        void (*_tcp_input)(TCP_INPUT_SIG);
        err_t (*_ip_input)(IP_INPUT_SIG);
        void (*_netif_set_default)(NETIF_SET_DEFAULT_SIG);
        struct netif * (*_netif_add)(NETIF_ADD_SIG);
        void (*_netif_set_up)(NETIF_SET_UP_SIG);
        void (*_netif_poll)(NETIF_POLL_SIG);
        
        Mutex _lock;        
        Mutex _lock_mem;

        lwIP_stack(const char* path) :
        _libref(NULL)
        {
#if defined(__ANDROID__) || defined(__UNITY_3D__)
    #define __STATIC_STACK__
#elif defined(__linux__)
    #define __DYNAMIC_STACK__
    // Dynamically load liblwip.so
    _libref = dlmopen(LM_ID_NEWLM, path, RTLD_NOW);
#elif defined(__APPLE__)
    #include "TargetConditionals.h"
    #if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
        #include "node/Mutex.hpp"
        #define __STATIC_STACK__
        // iOS Simulator or iOS device
        // Do nothing, symbols are statically-linked
    #elif TARGET_OS_MAC && !defined(SDK_BUNDLED)
        #define __DYNAMIC_STACK__
        // Dynamically load liblwip.so
        _libref = dlopen(path, RTLD_NOW);
    #elif TARGET_OS_MAC && defined(SDK_BUNDLED)
        #define __DYNAMIC_STACK__   // TODO: Implement static version
        // Dynamically load liblwip.so
        _libref = dlopen(path, RTLD_NOW);   
    #endif
#endif
            
#ifdef __STATIC_STACK__ // Set static references (for use in iOS)

            #if defined(SDK_IPV4)
                _etharp_output = (err_t(*)(ETHARP_OUTPUT_SIG))&etharp_output;
            #endif
                        
            #if defined(SDK_IPV6)
                _nd6_tmr = (void(*)(void))&nd6_tmr;
                _netif_ip6_addr_set_state = (void(*)(NETIF_IP6_ADDR_SET_STATE_SIG))&netif_ip6_addr_set_state;
                _netif_create_ip6_linklocal_address = (void(*)(NETIF_CREATE_IP6_LINKLOCAL_ADDRESS_SIG))&netif_create_ip6_linklocal_address;
                _ethip6_output = (err_t(*)(ETHIP6_OUTPUT_SIG))&ethip6_output;
            #endif
            
            _netif_init = (void(*)(void))&netif_init;
            _ethernet_input = (err_t(*)(ETHERNET_INPUT_SIG))&ethernet_input;
            _lwip_init = (void(*)(void))&lwip_init;
            _tcp_write = (err_t(*)(TCP_WRITE_SIG))&tcp_write;
            _tcp_sent = (void(*)(TCP_SENT_SIG))&tcp_sent;
            _tcp_new = (struct tcp_pcb*(*)(TCP_NEW_SIG))&tcp_new;
            _udp_new = (struct udp_pcb*(*)(UDP_NEW_SIG))&udp_new;
            _udp_connect = (err_t(*)(UDP_CONNECT_SIG))&udp_connect;
            _udp_send = (err_t(*)(UDP_SEND_SIG))&udp_send;
            _udp_sendto = (err_t(*)(UDP_SENDTO_SIG))&udp_sendto;
            _udp_recv = (void(*)(UDP_RECV_SIG))&udp_recv;
            _udp_bind = (err_t(*)(UDP_BIND_SIG))&udp_bind;
            _udp_remove = (void(*)(UDP_REMOVE_SIG))&udp_remove;
            _tcp_connect = (err_t(*)(TCP_CONNECT_SIG))&tcp_connect;
            _tcp_recv = (void(*)(TCP_RECV_SIG))&tcp_recv;
            _tcp_recved = (void(*)(TCP_RECVED_SIG))&tcp_recved;
            _tcp_err = (void(*)(TCP_ERR_SIG))&tcp_err;
            _tcp_poll = (void(*)(TCP_POLL_SIG))&tcp_poll;
            _tcp_arg = (void(*)(TCP_ARG_SIG))&tcp_arg;
            _tcp_close = (err_t(*)(TCP_CLOSE_SIG))&tcp_close;
            _tcp_abort = (void(*)(TCP_ABORT_SIG))&tcp_abort;
            _tcp_output = (err_t(*)(TCP_OUTPUT_SIG))&tcp_output;
            _tcp_accept = (void(*)(TCP_ACCEPT_SIG))&tcp_accept;
            _tcp_listen_with_backlog = (struct tcp_pcb*(*)(TCP_LISTEN_WITH_BACKLOG_SIG))&tcp_listen_with_backlog;
            _tcp_bind = (err_t(*)(TCP_BIND_SIG))&tcp_bind;
            _etharp_tmr = (void(*)(void))&etharp_tmr;
            _tcp_tmr = (void(*)(void))&tcp_tmr;
            _pbuf_free = (u8_t(*)(PBUF_FREE_SIG))&pbuf_free;
            _pbuf_alloc = (struct pbuf*(*)(PBUF_ALLOC_SIG))&pbuf_alloc;
            _lwip_htons = (u16_t(*)(LWIP_HTONS_SIG))&lwip_htons;
            _lwip_ntohs = (u16_t(*)(LWIP_NTOHS_SIG))&lwip_ntohs;
            _tcp_input = (void(*)(TCP_INPUT_SIG))&tcp_input;
            _ip_input = (err_t(*)(IP_INPUT_SIG))&ip_input;
            _netif_set_default = (void(*)(NETIF_SET_DEFAULT_SIG))&netif_set_default;
            _netif_add = (struct netif*(*)(NETIF_ADD_SIG))&netif_add;
            _netif_set_up = (void(*)(NETIF_SET_UP_SIG))&netif_set_up;

#endif
            
#ifdef __DYNAMIC_STACK__ // Use dynamically-loaded symbols (for use in normal desktop applications)
            
            if(_libref == NULL)
                DEBUG_ERROR("dlerror(): %s", dlerror());
            
            #if defined(SDK_IPV4)
                _etharp_output = (err_t(*)(ETHARP_OUTPUT_SIG))dlsym(_libref, "etharp_output");
            #endif

            #if defined(SDK_IPV6)
                _nd6_tmr = (void(*)(void))dlsym(_libref, "nd6_tmr");
                _netif_ip6_addr_set_state = (void(*)(NETIF_IP6_ADDR_SET_STATE_SIG))dlsym(_libref, "netif_ip6_addr_set_state");
                _netif_create_ip6_linklocal_address = (void(*)(NETIF_CREATE_IP6_LINKLOCAL_ADDRESS_SIG))dlsym(_libref, "netif_create_ip6_linklocal_address");
                _ethip6_output = (err_t(*)(ETHIP6_OUTPUT_SIG))dlsym(_libref, "ethip6_output");
            #endif

            _netif_init = (void(*)(void))dlsym(_libref, "netif_init");
            // _netif_set_addr = (void(*))(NETIF_SET_ADDR_SIG))dlsym(_libref, "netif_set_addr");

            _ethernet_input = (err_t(*)(ETHERNET_INPUT_SIG))dlsym(_libref, "ethernet_input");
            _lwip_init = (void(*)(void))dlsym(_libref, "lwip_init");
            _tcp_write = (err_t(*)(TCP_WRITE_SIG))dlsym(_libref, "tcp_write");
            _tcp_sent = (void(*)(TCP_SENT_SIG))dlsym(_libref, "tcp_sent");
            _tcp_new = (struct tcp_pcb*(*)(TCP_NEW_SIG))dlsym(_libref, "tcp_new");
            _udp_new = (struct udp_pcb*(*)(UDP_NEW_SIG))dlsym(_libref, "udp_new");
            _udp_connect = (err_t(*)(UDP_CONNECT_SIG))dlsym(_libref, "udp_connect");
            _udp_send = (err_t(*)(UDP_SEND_SIG))dlsym(_libref, "udp_send");
            _udp_sendto = (err_t(*)(UDP_SENDTO_SIG))dlsym(_libref, "udp_sendto");
            _udp_recv = (void(*)(UDP_RECV_SIG))dlsym(_libref, "udp_recv");
            _udp_bind = (err_t(*)(UDP_BIND_SIG))dlsym(_libref, "udp_bind");
            _udp_remove = (void(*)(UDP_REMOVE_SIG))dlsym(_libref, "udp_remove");
            _tcp_sndbuf = (u16_t(*)(TCP_SNDBUF_SIG))dlsym(_libref, "tcp_sndbuf");
            _tcp_connect = (err_t(*)(TCP_CONNECT_SIG))dlsym(_libref, "tcp_connect");
            _tcp_recv = (void(*)(TCP_RECV_SIG))dlsym(_libref, "tcp_recv");
            _tcp_recved = (void(*)(TCP_RECVED_SIG))dlsym(_libref, "tcp_recved");
            _tcp_err = (void(*)(TCP_ERR_SIG))dlsym(_libref, "tcp_err");
            _tcp_poll = (void(*)(TCP_POLL_SIG))dlsym(_libref, "tcp_poll");
            _tcp_arg = (void(*)(TCP_ARG_SIG))dlsym(_libref, "tcp_arg");
            _tcp_close = (err_t(*)(TCP_CLOSE_SIG))dlsym(_libref, "tcp_close");
            _tcp_abort = (void(*)(TCP_ABORT_SIG))dlsym(_libref, "tcp_abort");
            _tcp_output = (err_t(*)(TCP_OUTPUT_SIG))dlsym(_libref, "tcp_output");
            _tcp_accept = (void(*)(TCP_ACCEPT_SIG))dlsym(_libref, "tcp_accept");
            _tcp_listen = (struct tcp_pcb*(*)(TCP_LISTEN_SIG))dlsym(_libref, "tcp_listen");
            _tcp_listen_with_backlog = (struct tcp_pcb*(*)(TCP_LISTEN_WITH_BACKLOG_SIG))dlsym(_libref, "tcp_listen_with_backlog");
            _tcp_bind = (err_t(*)(TCP_BIND_SIG))dlsym(_libref, "tcp_bind");
            _etharp_tmr = (void(*)(void))dlsym(_libref, "etharp_tmr");
            _tcp_tmr = (void(*)(void))dlsym(_libref, "tcp_tmr");
            _pbuf_free = (u8_t(*)(PBUF_FREE_SIG))dlsym(_libref, "pbuf_free");
            _pbuf_alloc = (struct pbuf*(*)(PBUF_ALLOC_SIG))dlsym(_libref, "pbuf_alloc");
            _lwip_htons = (u16_t(*)(LWIP_HTONS_SIG))dlsym(_libref, "lwip_htons");
            _lwip_ntohs = (u16_t(*)(LWIP_NTOHS_SIG))dlsym(_libref, "lwip_ntohs");
            _tcp_input = (void(*)(TCP_INPUT_SIG))dlsym(_libref, "tcp_input");
            _ip_input = (err_t(*)(IP_INPUT_SIG))dlsym(_libref, "ip_input");
            _netif_set_default = (void(*)(NETIF_SET_DEFAULT_SIG))dlsym(_libref, "netif_set_default");
            _netif_add = (struct netif*(*)(NETIF_ADD_SIG))dlsym(_libref, "netif_add");
            _netif_set_up = (void(*)(NETIF_SET_UP_SIG))dlsym(_libref, "netif_set_up");
#endif
        }
        
        ~lwIP_stack()
        {
            if (_libref)
                dlclose(_libref);
        }
        
        #if defined(SDK_IPV4)
            inline struct netif * __netif_add(NETIF_ADD_SIG) throw() { Mutex::Lock _l(_lock); return _netif_add(netif,ipaddr,netmask,gw,state,init,input); }
        #endif
                
        #if defined(SDK_IPV6)
            inline struct netif * __netif_add(NETIF_ADD_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _netif_add(netif,state,init,input); }
            inline void __nd6_tmr(void) throw() { /*DEBUG_STACK();*/ Mutex::Lock _l(_lock); _nd6_tmr(); }
            inline void __netif_ip6_addr_set_state(NETIF_IP6_ADDR_SET_STATE_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); _netif_ip6_addr_set_state(netif, addr_idx, state);  }
            inline void __netif_create_ip6_linklocal_address(NETIF_CREATE_IP6_LINKLOCAL_ADDRESS_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); _netif_create_ip6_linklocal_address(netif, from_mac_48bit); }
            inline err_t __ethip6_output(ETHIP6_OUTPUT_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _ethip6_output(netif,q,ip6addr); }
        #endif
        
        inline void __netif_init(void) throw() { Mutex::Lock _l(_lock); _netif_init(); }
        // inline void __netif_set_addr(NETIF_SET_ADDR_SIG) throw() { Mutex::Lock _l(_lock); _netif_set_addr(netif, ipaddr, netmask, gw); }
        inline void __lwip_init() throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _lwip_init(); }
        inline err_t __tcp_write(TCP_WRITE_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _tcp_write(pcb,arg,len,apiflags); }
        inline void __tcp_sent(TCP_SENT_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _tcp_sent(pcb,sent); }
        inline struct tcp_pcb * __tcp_new(TCP_NEW_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _tcp_new(); }
        inline struct udp_pcb * __udp_new(UDP_NEW_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _udp_new(); }
        inline err_t __udp_connect(UDP_CONNECT_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _udp_connect(pcb,ipaddr,port); }
        inline err_t __udp_send(UDP_SEND_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _udp_send(pcb,p); }
        inline err_t __udp_sendto(UDP_SENDTO_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _udp_sendto(pcb,p,dst_ip,dst_port); }
        inline void __udp_recv(UDP_RECV_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _udp_recv(pcb,recv,recv_arg); }
        inline err_t __udp_bind(UDP_BIND_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _udp_bind(pcb,ipaddr,port); }
        inline void __udp_remove(UDP_REMOVE_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _udp_remove(pcb); }
        inline u16_t __tcp_sndbuf(TCP_SNDBUF_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _tcp_sndbuf(pcb); }
        inline err_t __tcp_connect(TCP_CONNECT_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _tcp_connect(pcb,ipaddr,port,connected); }
        inline void __tcp_recv(TCP_RECV_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _tcp_recv(pcb,recv); }
        inline void __tcp_recved(TCP_RECVED_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _tcp_recved(pcb,len); }
        inline void __tcp_err(TCP_ERR_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _tcp_err(pcb,err); }
        inline void __tcp_poll(TCP_POLL_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _tcp_poll(pcb,poll,interval); }
        inline void __tcp_arg(TCP_ARG_SIG) throw() { /*DEBUG_STACK();*/ Mutex::Lock _l(_lock); return _tcp_arg(pcb,arg); }
        inline err_t __tcp_close(TCP_CLOSE_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _tcp_close(pcb); }
        inline void __tcp_abort(TCP_ABORT_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _tcp_abort(pcb); }
        inline err_t __tcp_output(TCP_OUTPUT_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _tcp_output(pcb); }
        inline void __tcp_accept(TCP_ACCEPT_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _tcp_accept(pcb,accept); }
        inline struct tcp_pcb * __tcp_listen(TCP_LISTEN_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _tcp_listen(pcb); }
        inline struct tcp_pcb * __tcp_listen_with_backlog(TCP_LISTEN_WITH_BACKLOG_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _tcp_listen_with_backlog(pcb,backlog); }
        inline err_t __tcp_bind(TCP_BIND_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _tcp_bind(pcb,ipaddr,port); }
        inline void __etharp_tmr(void) throw() { /*DEBUG_STACK();*/ Mutex::Lock _l(_lock); return _etharp_tmr(); }
        inline void __tcp_tmr(void) throw() { /*DEBUG_STACK();*/ Mutex::Lock _l(_lock); return _tcp_tmr(); }
        inline u8_t __pbuf_free(PBUF_FREE_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _pbuf_free(p); }
        inline struct pbuf * __pbuf_alloc(PBUF_ALLOC_SIG) throw() { /*DEBUG_STACK();*/ Mutex::Lock _l(_lock_mem); return _pbuf_alloc(layer,length,type); }
        inline u16_t __lwip_htons(LWIP_HTONS_SIG) throw() { /*DEBUG_STACK();*/ Mutex::Lock _l(_lock); return _lwip_htons(x); }
        inline u16_t __lwip_ntohs(LWIP_NTOHS_SIG) throw() { /*DEBUG_STACK();*/ Mutex::Lock _l(_lock); return _lwip_ntohs(x); }
        //inline err_t __etharp_output(ETHARP_OUTPUT_SIG) throw() { Mutex::Lock _l(_lock); return _etharp_output(netif,q,ipaddr); }
        inline err_t __ethernet_input(ETHERNET_INPUT_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _ethernet_input(p,netif); }
        inline void __tcp_input(TCP_INPUT_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _tcp_input(p,inp); }
        inline err_t __ip_input(IP_INPUT_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _ip_input(p,inp); }
        inline void __netif_set_default(NETIF_SET_DEFAULT_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _netif_set_default(netif); }
        inline void __netif_set_up(NETIF_SET_UP_SIG) throw() { DEBUG_STACK(); Mutex::Lock _l(_lock); return _netif_set_up(netif); }
};
    
} // namespace ZeroTier

#endif

#endif // SDK_LWIP
