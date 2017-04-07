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

#ifndef ZT_PICOTCP_HPP
#define ZT_PICOTCP_HPP


#include "pico_eth.h"
#include "pico_stack.h"
#include "pico_ipv4.h"
#include "pico_icmp4.h"
#include "pico_dev_tap.h"
#include "pico_protocol.h"
#include "pico_socket.h"
#include "pico_device.h"
#include "pico_ipv6.h"

#include "SocketTap.hpp"

/****************************************************************************/
/* PicoTCP API Signatures                                                   */
/****************************************************************************/

#define PICO_IPV4_TO_STRING_SIG char *ipbuf, const uint32_t ip
#define PICO_TAP_CREATE_SIG char *name
#define PICO_IPV4_LINK_ADD_SIG struct pico_device *dev, struct pico_ip4 address, struct pico_ip4 netmask
#define PICO_DEVICE_INIT_SIG struct pico_device *dev, const char *name, uint8_t *mac
#define PICO_STACK_RECV_SIG struct pico_device *dev, uint8_t *buffer, uint32_t len
#define PICO_ICMP4_PING_SIG char *dst, int count, int interval, int timeout, int size, void (*cb)(struct pico_icmp4_stats *)
#define PICO_TIMER_ADD_SIG pico_time expire, void (*timer)(pico_time, void *), void *arg
#define PICO_STRING_TO_IPV4_SIG const char *ipstr, uint32_t *ip
#define PICO_STRING_TO_IPV6_SIG const char *ipstr, uint8_t *ip
#define PICO_SOCKET_SETOPTION_SIG struct pico_socket *s, int option, void *value
#define PICO_SOCKET_SEND_SIG struct pico_socket *s, const void *buf, int len
#define PICO_SOCKET_SENDTO_SIG struct pico_socket *s, const void *buf, int len, void *dst, uint16_t remote_port
#define PICO_SOCKET_RECV_SIG struct pico_socket *s, void *buf, int len
#define PICO_SOCKET_RECVFROM_SIG struct pico_socket *s, void *buf, int len, void *orig, uint16_t *remote_port
#define PICO_SOCKET_OPEN_SIG uint16_t net, uint16_t proto, void (*wakeup)(uint16_t ev, struct pico_socket *s)
#define PICO_SOCKET_BIND_SIG struct pico_socket *s, void *local_addr, uint16_t *port
#define PICO_SOCKET_CONNECT_SIG struct pico_socket *s, const void *srv_addr, uint16_t remote_port
#define PICO_SOCKET_LISTEN_SIG struct pico_socket *s, const int backlog
#define PICO_SOCKET_READ_SIG struct pico_socket *s, void *buf, int len
#define PICO_SOCKET_WRITE_SIG struct pico_socket *s, const void *buf, int len
#define PICO_SOCKET_CLOSE_SIG struct pico_socket *s
#define PICO_SOCKET_SHUTDOWN_SIG struct pico_socket *s, int mode
#define PICO_SOCKET_ACCEPT_SIG struct pico_socket *s, void *orig, uint16_t *port
#define PICO_IPV6_LINK_ADD_SIG struct pico_device *dev, struct pico_ip6 address, struct pico_ip6 netmask

namespace ZeroTier
{
	class SocketTap;
	struct Connection;

	class picoTCP
	{		
	public:

		void pico_init_interface(ZeroTier::SocketTap *tap, const ZeroTier::InetAddress &ip);
		void pico_loop(SocketTap *tap);

		//int pico_eth_send(struct pico_device *dev, void *buf, int len);
		//int pico_eth_poll(struct pico_device *dev, int loop_score);
		static void pico_cb_tcp_read(SocketTap *tap, struct pico_socket *s);
		static void pico_cb_udp_read(SocketTap *tap, struct pico_socket *s);
		static void pico_cb_tcp_write(SocketTap *tap, struct pico_socket *s);
		static void pico_cb_socket_activity(uint16_t ev, struct pico_socket *s);

		void pico_rx(SocketTap *tap, const ZeroTier::MAC &from,const ZeroTier::MAC &to,unsigned int etherType,const void *data,unsigned int len);
		Connection *pico_handleSocket(ZeroTier::PhySocket *sock, void **uptr, struct socket_st* socket_rpc);
		void pico_handleWrite(Connection *conn);
		void pico_handleConnect(ZeroTier::PhySocket *sock, ZeroTier::PhySocket *rpcSock, Connection *conn, struct connect_st* connect_rpc);
		void pico_handleBind(ZeroTier::PhySocket *sock, ZeroTier::PhySocket *rpcSock, void **uptr, struct bind_st *bind_rpc);
		void pico_handleListen(ZeroTier::PhySocket *sock, ZeroTier::PhySocket *rpcSock, void **uptr, struct listen_st *listen_rpc);
		void pico_handleRead(ZeroTier::PhySocket *sock,void **uptr,bool lwip_invoked);
		void pico_handleClose(ZeroTier::PhySocket *sock);


	};
}

#endif