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

/**
 * @file
 *
 * lwIP network stack driver
 */

#ifndef ZT_LWIP_HPP
#define ZT_LWIP_HPP

#include "libztDefs.h"

#ifdef STACK_LWIP

namespace ZeroTier {
  class MAC;
  class Mutex;
  struct InetAddress;
}

//#include "lwip/err.h"

/**
 * @brief Initialize network stack semaphores, threads, and timers.
 *
 * @usage This is called during the initial setup of each VirtualTap but is only allowed to execute once
 * @return
 */
void lwip_driver_init();

/**
 * @brief Initialize and start the DNS client
 *
 * @usage Called after lwip_driver_init()
 * @return
 */
void lwip_dns_init();

/**
 * @brief Starts DHCP timers
 *
 * @usage lwip_driver_init()
 * @return
 */
void lwip_start_dhcp(void *netif);

/**
 * @brief Set up an interface in the network stack for the VirtualTap.
 *
 * @param
 * @param tapref Reference to VirtualTap that will be responsible for sending and receiving data
 * @param mac Virtual hardware address for this ZeroTier VirtualTap interface
 * @param ip Virtual IP address for this ZeroTier VirtualTap interface
 * @return
 */
void lwip_init_interface(void *tapref, const ZeroTier::MAC &mac, const ZeroTier::InetAddress &ip);

/**
 * @brief Called from the stack, outbound ethernet frames from the network stack enter the ZeroTier virtual wire here.
 *
 * @usage This shall only be called from the stack or the stack driver. Not the application thread.
 * @param netif Transmits an outgoing Ethernet fram from the network stack onto the ZeroTier virtual wire
 * @param p A pointer to the beginning of a chain pf struct pbufs
 * @return
 */
err_t lwip_eth_tx(struct netif *netif, struct pbuf *p);

/**
 * @brief Receives incoming Ethernet frames from the ZeroTier virtual wire
 *
 * @usage This shall be called from the VirtualTap's I/O thread (via VirtualTap::put())
 * @param tap Pointer to VirtualTap from which this data comes
 * @param from Origin address (virtual ZeroTier hardware address)
 * @param to Intended destination address (virtual ZeroTier hardware address)
 * @param etherType Protocol type
 * @param data Pointer to Ethernet frame
 * @param len Length of Ethernet frame
 * @return
 */
void lwip_eth_rx(VirtualTap *tap, const ZeroTier::MAC &from, const ZeroTier::MAC &to, unsigned int etherType,
	const void *data, unsigned int len);

/****************************************************************************/
/* Raw API driver                                                           */
/****************************************************************************/

#ifdef ZT_VIRTUAL_SOCKET

class VirtualSocket;

/**
 * Returns the number of TCP PCBs currently allocated
 */
int rd_lwip_num_current_tcp_pcbs();

/**
 * Returns the number of UDP PCBs currently allocated
 */
int rd_lwip_num_current_udp_pcbs();

/**
 * Returns the number of RAW PCBs currently allocated
 */
int rd_lwip_num_current_raw_pcbs();

/**
 * Returns the total number of PCBs of any time or state
 */
int rd_lwip_num_total_pcbs();

/**
 * Registers a DNS nameserver with the network stack
 */
int rd_lwip_add_dns_nameserver(struct sockaddr *addr);

/**
 * Un-registers a DNS nameserver from the network stack
 */
int rd_lwip_del_dns_nameserver(struct sockaddr *addr);

/**
 * Main stack loop
 */
void rd_lwip_loop(VirtualTap *tap);

/**
 * Creates a stack-specific "socket" or "VirtualSocket object"
 */
int rd_lwip_socket(void **pcb, int socket_family, int socket_type, int protocol);

/**
 * Connect to remote host via userspace network stack interface - Called from VirtualTap
 */
int rd_lwip_connect(VirtualSocket *vs, const struct sockaddr *addr, socklen_t addrlen);

/**
 * Bind to a userspace network stack interface - Called from VirtualTap
 */
int rd_lwip_bind(VirtualTap *tap, VirtualSocket *vs, const struct sockaddr *addr, socklen_t addrlen);

/**
 * Listen for incoming VirtualSockets - Called from VirtualTap
 */
int rd_lwip_listen(VirtualSocket *vs, int backlog);

/**
 * Accept an incoming VirtualSocket - Called from VirtualTap
 */
VirtualSocket* rd_lwip_accept(VirtualSocket *vs);

/**
 * Read from RX buffer to application - Called from VirtualTap
 */
int rd_lwip_read(VirtualSocket *vs, bool lwip_invoked);

/**
 * Write to userspace network stack - Called from VirtualTap
 */
int rd_lwip_write(VirtualSocket *vs, void *data, ssize_t len);

/**
 * Close a VirtualSocket - Called from VirtualTap
 */
int rd_lwip_close(VirtualSocket *vs);

/**
 * Shuts down some aspect of a VirtualSocket - Called from VirtualTap
 */
int rd_lwip_shutdown(VirtualSocket *vs, int how);

/**
 *  Sets a property of a socket
 */
int rd_lwip_setsockopt(VirtualSocket *vs, int level, int optname, const void *optval, socklen_t optlen);

/**
 *  Gets a property of a socket
 */
int rd_lwip_getsockopt(VirtualSocket *vs, int level, int optname, void *optval, socklen_t *optlen);

// --- Callbacks from network stack ---

#ifdef ZT_DRIVER_MODULE // only include these symbols if we're building the full driver

/**
 * Callback for handling received UDP packets (already processed by network stack)
 */
static err_t rd_lwip_cb_tcp_recved(void *arg, struct tcp_pcb *PCB, struct pbuf *p, err_t err);

/**
 * Callback for handling accepted connection
 */
static err_t rd_lwip_cb_accept(void *arg, struct tcp_pcb *newPCB, err_t err);

/**
 * Callback for handling received TCP packets (already processed by stack)
 */
static void rd_lwip_cb_udp_recved(void * arg, struct udp_pcb * upcb, struct pbuf * p, const ip_addr_t * addr, u16_t port);

/**
 * Callback for handling errors from within the network stack
 */
static void rd_lwip_cb_err(void *arg, err_t err);

/**
 * Callback for handling periodic background tasks
 */
static err_t rd_lwip_cb_poll(void* arg, struct tcp_pcb *PCB);

/**
 * Callback for handling confirmation of sent packets
 */
static err_t rd_lwip_cb_sent(void *arg, struct tcp_pcb *PCB, u16_t len);

/**
 * Callback for handling successful connections
 */
static err_t rd_lwip_cb_connected(void *arg, struct tcp_pcb *PCB, err_t err);

#endif // ZT_DRIVER_MODULE

#endif

#endif // ZT_VIRTUAL_SOCKET
#endif // STACK_LWIP
