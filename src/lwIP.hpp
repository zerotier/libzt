/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2017  ZeroTier, Inc.  https://www.zerotier.com/
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
 * You can be released from the requirements of the license by purchasing
 * a commercial license. Buying such a license is mandatory as soon as you
 * develop commercial closed-source software that incorporates or links
 * directly against ZeroTier software without disclosing the source code
 * of your own application.
 */

// lwIP network stack driver

#ifndef ZT_LWIP_HPP
#define ZT_LWIP_HPP

#include <stdio.h>
#include <dlfcn.h>
#include <errno.h>

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

#include "libzt.h"
#include "VirtualTap.hpp"

struct tcp_pcb;
struct netif;

/** Table to quickly map an lwIP error (err_t) to a socket error
  * by using -err as an index */
static const int lwip_err_to_errno_table[] = {
  0,             /* ERR_OK          0      No error, everything OK. */
  ENOMEM,        /* ERR_MEM        -1      Out of memory error.     */
  ENOBUFS,       /* ERR_BUF        -2      Buffer error.            */
  EWOULDBLOCK,   /* ERR_TIMEOUT    -3      Timeout                  */
  EHOSTUNREACH,  /* ERR_RTE        -4      Routing problem.         */
  EINPROGRESS,   /* ERR_INPROGRESS -5      Operation in progress    */
  EINVAL,        /* ERR_VAL        -6      Illegal value.           */
  EWOULDBLOCK,   /* ERR_WOULDBLOCK -7      Operation would block.   */
  EADDRINUSE,    /* ERR_USE        -8      Address in use.          */
  EALREADY,      /* ERR_ALREADY    -9      Already connecting.      */
  EISCONN,       /* ERR_ISCONN     -10     Conn already established.*/
  ENOTCONN,      /* ERR_CONN       -11     Not connected.           */
  -1,            /* ERR_IF         -12     Low-level netif error    */
  ECONNABORTED,  /* ERR_ABRT       -13     Connection aborted.      */
  ECONNRESET,    /* ERR_RST        -14     Connection reset.        */
  ENOTCONN,      /* ERR_CLSD       -15     Connection closed.       */
  EIO            /* ERR_ARG        -16     Illegal argument.        */
};

#define ERR_TO_ERRNO_TABLE_SIZE LWIP_ARRAYSIZE(lwip_err_to_errno_table)

#define lwip_err_to_errno(err) \
  ((unsigned)(-(signed)(err)) < ERR_TO_ERRNO_TABLE_SIZE ? \
	lwip_err_to_errno_table[-(signed)(err)] : EIO)

#if defined(LIBZT_IPV4)
	//#define LWIP_NETIF_ADD_SIG struct netif *netif, ip_addr_t *ipaddr, ip_addr_t *netmask, ip_addr_t *gw, void *state, netif_init_fn init, netif_input_fn input
	#define LWIP_ETHARP_OUTPUT_SIG struct netif *netif, struct pbuf *q, const ip4_addr_t *ipaddr
#endif
#if defined(LIBZT_IPV6)
#include "lwip/ip6_addr.h"
#include "ethip6.h"
	#define LWIP_NETIF_ADD_SIG struct netif *netif, void *state, netif_init_fn init, netif_input_fn input
	#define LWIP_ETHIP6_OUTPUT_SIG struct netif *netif, struct pbuf *q, const ip6_addr_t *ip6addr
	#define LWIP_ETHARP_OUTPUT_SIG struct netif *netif, struct pbuf *q, const ip6_addr_t *ipaddr
	#define LWIP_NETIF_IP6_ADDR_SET_STATE_SIG struct netif* netif, s8_t addr_idx, u8_t state
	#define LWIP_NETIF_CREATE_IP6_LINKLOCAL_ADDRESS_SIG struct netif *netif, u8_t from_mac_48bit
#endif

#define LWIP_PBUF_FREE_SIG struct pbuf *p
#define LWIP_PBUF_ALLOC_SIG pbuf_layer layer, u16_t length, pbuf_type type
#define LWIP_HTONS_SIG u16_t x
#define LWIP_NTOHS_SIG u16_t x
#define LWIP_UDP_NEW_SIG void
#define LWIP_UDP_CONNECT_SIG struct udp_pcb * pcb, const ip_addr_t * ipaddr, u16_t port
#define LWIP_UDP_SEND_SIG struct udp_pcb * pcb, struct pbuf * p
#define LWIP_UDP_SENDTO_SIG struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *dst_ip, u16_t dst_port
#define LWIP_UDP_RECV_SIG struct udp_pcb * pcb,  void (* recv)(void * arg, struct udp_pcb * upcb, struct pbuf * p, ip_addr_t * addr, u16_t port), void * recv_arg
#define LWIP_UDP_RECVED_SIG struct udp_pcb * pcb, u16_t len
#define LWIP_UDP_BIND_SIG struct udp_pcb * pcb, const ip_addr_t * ipaddr, u16_t port
#define LWIP_UDP_REMOVE_SIG struct udp_pcb *pcb
#define LWIP_TCP_WRITE_SIG struct tcp_pcb *pcb, const void *arg, u16_t len, u8_t apiflags
#define LWIP_TCP_SENT_SIG struct tcp_pcb * pcb, err_t (* sent)(void * arg, struct tcp_pcb * tpcb, u16_t len)
#define LWIP_TCP_NEW_SIG void
#define LWIP_TCP_RECV_SIG struct tcp_pcb * pcb, err_t (* recv)(void * arg, struct tcp_pcb * tpcb, struct pbuf * p, err_t err)
#define LWIP_TCP_RECVED_SIG struct tcp_pcb * pcb, u16_t len
#define LWIP_TCP_CONNECT_SIG struct tcp_pcb * pcb, const ip_addr_t * ipaddr, u16_t port, err_t (* connected)(void * arg, struct tcp_pcb * tpcb, err_t err)
#define LWIP_TCP_RECV_SIG struct tcp_pcb * pcb, err_t (* recv)(void * arg, struct tcp_pcb * tpcb, struct pbuf * p, err_t err)
#define LWIP_TCP_ERR_SIG struct tcp_pcb * pcb, void (* err)(void * arg, err_t err)
#define LWIP_TCP_POLL_SIG struct tcp_pcb * pcb, err_t (* poll)(void * arg, struct tcp_pcb * tpcb), u8_t interval
#define LWIP_TCP_ARG_SIG struct tcp_pcb * pcb, void * arg
#define LWIP_TCP_CLOSE_SIG struct tcp_pcb * pcb
#define LWIP_TCP_ABORT_SIG struct tcp_pcb * pcb
#define LWIP_TCP_OUTPUT_SIG struct tcp_pcb * pcb
#define LWIP_TCP_ACCEPT_SIG struct tcp_pcb * pcb, err_t (* accept)(void * arg, struct tcp_pcb * newpcb, err_t err)
#define LWIP_TCP_LISTEN_SIG struct tcp_pcb * pcb
#define LWIP_TCP_LISTEN_WITH_BACKLOG_SIG struct tcp_pcb * pcb, u8_t backlog
#define LWIP_TCP_BIND_SIG struct tcp_pcb * pcb, const ip_addr_t * ipaddr, u16_t port
#define LWIP_TCP_INPUT_SIG struct pbuf *p, struct netif *inp
#define LWIP_ETHERNET_INPUT_SIG struct pbuf *p, struct netif *netif
#define LWIP_IP_INPUT_SIG struct pbuf *p, struct netif *inp
#define LWIP_NETIF_SET_DEFAULT_SIG struct netif *netif
#define LWIP_NETIF_SET_UP_SIG struct netif *netif
#define LWIP_NETIF_POLL_SIG struct netif *netif
#define NETIF_SET_STATUS_CALLBACK struct netif *netif, netif_status_callback_fn status_callback
#define LWIP_TCP_SHUTDOWN_SIG struct tcp_pcb *pcb, int shut_rx, int shut_tx

#if defined(LIBZT_IPV4)
	extern "C" err_t etharp_output(LWIP_ETHARP_OUTPUT_SIG);
#endif
#if defined(LIBZT_IPV6)
	extern "C" void nd6_tmr(void);
	//extern "C" void netif_ip6_addr_set_state(LWIP_NETIF_IP6_ADDR_SET_STATE_SIG);
	extern "C" void netif_create_ip6_linklocal_address(LWIP_NETIF_CREATE_IP6_LINKLOCAL_ADDRESS_SIG);
	extern "C" err_t ethip6_output(LWIP_ETHIP6_OUTPUT_SIG);
#endif

extern "C" void lwip_init();
extern "C" err_t ethernet_input(LWIP_ETHERNET_INPUT_SIG);
extern "C" void netif_poll(LWIP_NETIF_POLL_SIG);

//extern "C" err_t etharp_output(LWIP_ETHARP_OUTPUT_SIG);
//extern "C" err_t ethernet_input(LWIP_ETHERNET_INPUT_SIG);
extern "C" void netif_set_up(LWIP_NETIF_SET_UP_SIG);
extern "C" void netif_set_default(LWIP_NETIF_SET_DEFAULT_SIG);
//extern "C" struct netif *netif_add(LWIP_NETIF_ADD_SIG);
extern "C" err_t tapif_init(struct netif *netif);
//extern "C" err_t low_level_output(struct netif *netif, struct pbuf *p);
extern "C" err_t tcp_write(LWIP_TCP_WRITE_SIG);
extern "C" void tcp_sent(LWIP_TCP_SENT_SIG);
extern "C" struct tcp_pcb *tcp_new(LWIP_TCP_NEW_SIG);
//u16_t tcp_sndbuf(struct tcp_pcb * pcb);
extern "C" err_t tcp_connect(LWIP_TCP_CONNECT_SIG);
extern "C" struct udp_pcb *udp_new(LWIP_UDP_NEW_SIG);
extern "C" err_t udp_connect(LWIP_UDP_CONNECT_SIG);
extern "C" err_t udp_send(LWIP_UDP_SEND_SIG);
extern "C" err_t udp_sendto(LWIP_UDP_SENDTO_SIG);
//extern "C" void udp_recv(LWIP_UDP_RECV_SIG);
extern "C" void udp_recved(LWIP_UDP_RECVED_SIG);
extern "C" err_t udp_bind(LWIP_UDP_BIND_SIG);
extern "C" void udp_remove(LWIP_UDP_REMOVE_SIG);
extern "C" void tcp_recv(LWIP_TCP_RECV_SIG);
extern "C" void tcp_recved(LWIP_TCP_RECVED_SIG);
extern "C" void tcp_err(LWIP_TCP_ERR_SIG);
extern "C" void tcp_poll(LWIP_TCP_POLL_SIG);
extern "C" void tcp_arg(LWIP_TCP_ARG_SIG);
extern "C" err_t tcp_close(LWIP_TCP_CLOSE_SIG);
extern "C" void tcp_abort(LWIP_TCP_ABORT_SIG);
extern "C" err_t tcp_output(LWIP_TCP_OUTPUT_SIG);
extern "C" void tcp_accept(LWIP_TCP_ACCEPT_SIG);
//extern "C" struct tcp_pcb *tcp_listen(LWIP_TCP_LISTEN_SIG);
extern "C" struct tcp_pcb *tcp_listen_with_backlog(LWIP_TCP_LISTEN_WITH_BACKLOG_SIG);
extern "C" err_t tcp_bind(LWIP_TCP_BIND_SIG);
extern "C" void etharp_tmr(void);
extern "C" void tcp_tmr(void);
extern "C" u8_t pbuf_free(LWIP_PBUF_FREE_SIG);
extern "C" struct pbuf *pbuf_alloc(LWIP_PBUF_ALLOC_SIG);
extern "C" u16_t lwip_htons(LWIP_HTONS_SIG);
extern "C" u16_t lwip_ntohs(LWIP_NTOHS_SIG);
extern "C" void tcp_input(LWIP_TCP_INPUT_SIG);
extern "C" err_t ip_input(LWIP_IP_INPUT_SIG);
extern "C" err_t tcp_shutdown(LWIP_TCP_SHUTDOWN_SIG);
//extern "C" void netif_set_status_callback(NETIF_SET_STATUS_CALLBACK);

namespace ZeroTier {

	class VirtualTap;
	struct VirtualSocket;

	class lwIP
	{
	public:

		/*
		 * Set up an interface in the network stack for the VirtualTap
		 */
		void lwip_init_interface(VirtualTap *tap, const InetAddress &ip);

		/*
		 * Returns the number of TCP PCBs currently allocated
		 */
		static int lwip_num_current_tcp_pcbs();

		/*
		 * Returns the number of UDP PCBs currently allocated
		 */
		static int lwip_num_current_udp_pcbs();

		/*
		 * Returns the number of RAW PCBs currently allocated
		 */
		static int lwip_num_current_raw_pcbs();

		/*
		 * Returns the total number of PCBs of any time or state
 		 */
		int lwip_num_total_pcbs();

		/*
		 * Registers a DNS nameserver with the network stack
		 */
		int lwip_add_dns_nameserver(struct sockaddr *addr);

		/*
		 * Un-registers a DNS nameserver from the network stack
		 */
		int lwip_del_dns_nameserver(struct sockaddr *addr);

		/*
		 * Main stack loop
		 */
		void lwip_loop(VirtualTap *tap);

		/*
		 * Packets from the ZeroTier virtual wire enter the stack here
		 */
		void lwip_eth_rx(VirtualTap *tap, const MAC &from,const MAC &to,unsigned int etherType,const void *data,unsigned int len);

		/*
		 * Creates a stack-specific "socket" or "VirtualSocket object"
		 */
		int lwip_Socket(void **pcb, int socket_family, int socket_type, int protocol);

		/*
		 * Connect to remote host via userspace network stack interface - Called from VirtualTap
		 */
		int lwip_Connect(VirtualSocket *vs, const struct sockaddr *addr, socklen_t addrlen);

		/*
		 * Bind to a userspace network stack interface - Called from VirtualTap
		 */
		int lwip_Bind(VirtualTap *tap, VirtualSocket *vs, const struct sockaddr *addr, socklen_t addrlen);

		/*
		 * Listen for incoming VirtualSockets - Called from VirtualTap
		 */
		int lwip_Listen(VirtualSocket *vs, int backlog);

		/*
		 * Accept an incoming VirtualSocket - Called from VirtualTap
		 */
		VirtualSocket* lwip_Accept(VirtualSocket *vs);

		/*
		 * Read from RX buffer to application - Called from VirtualTap
		 */
		int lwip_Read(VirtualSocket *vs, bool lwip_invoked);

		/*
		 * Write to userspace network stack - Called from VirtualTap
		 */
		int lwip_Write(VirtualSocket *vs, void *data, ssize_t len);

		/*
		 * Close a VirtualSocket - Called from VirtualTap
		 */
		int lwip_Close(VirtualSocket *vs);

		/*
		 * Shuts down some aspect of a VirtualSocket - Called from VirtualTap
		 */
		int lwip_Shutdown(VirtualSocket *vs, int how);

		// --- Callbacks from network stack ---

		//static void netif_status_callback(struct netif *nif);

		/*
		 * Callback for handling received UDP packets (already processed by network stack)
		 */
		static err_t lwip_cb_tcp_recved(void *arg, struct tcp_pcb *PCB, struct pbuf *p, err_t err);

		/*
		 * Callback for handling accepted connection
		 */
		static err_t lwip_cb_accept(void *arg, struct tcp_pcb *newPCB, err_t err);

		/*
		 * Callback for handling received TCP packets (already processed by stack)
		 */
		static void lwip_cb_udp_recved(void * arg, struct udp_pcb * upcb, struct pbuf * p, const ip_addr_t * addr, u16_t port);

		/*
		 * Callback for handling errors from within the network stack
		 */
		static void lwip_cb_err(void *arg, err_t err);

		/*
		 * Callback for handling periodic background tasks
		 */
		static err_t lwip_cb_poll(void* arg, struct tcp_pcb *PCB);

		/*
		 * Callback for handling confirmation of sent packets
		 */
		static err_t lwip_cb_sent(void *arg, struct tcp_pcb *PCB, u16_t len);

		/*
		 * Callback for handling successful connections
		 */
		static err_t lwip_cb_connected(void *arg, struct tcp_pcb *PCB, err_t err);
	};
}

#endif
