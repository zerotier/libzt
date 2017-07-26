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

// General connection object used by SocketTap and network stack drivers

#ifndef ZT_CONNECTION_HPP
#define ZT_CONNECTION_HPP

#include <ctime>
#include <sys/socket.h>

#if defined(STACK_PICO)
#include "pico_socket.h"
#endif

#include "Phy.hpp"

#include "libzt.h"
#include "SocketTap.hpp"
#include "RingBuffer.hpp"

namespace ZeroTier {
	
	class SocketTap;

	struct Connection
	{
		int tot = 0;
		RingBuffer<unsigned char> *TXbuf;
		RingBuffer<unsigned char> *RXbuf;

		Mutex _tx_m, _rx_m;

		PhySocket *sock;	

#if defined(STACK_PICO)			
		struct pico_socket *picosock;
#endif
#if defined(STACK_LWIP)
		void *pcb;
#endif
		// TODO: For getsockname, etc
		struct sockaddr_storage *local_addr; // Address we've bound to locally
		struct sockaddr_storage *peer_addr;  // Address of connection call to remote host

		int socket_family, socket_type;

		int app_fd; // used by app for I/O
		int sdk_fd; // used by lib for I/O

		std::queue<Connection*> _AcceptedConnections;
		SocketTap *tap;
		int state;      // See libzt.h for (ZT_SOCK_STATE_*)

		// timestamp for closure event
		std::time_t closure_ts;

		Connection() {
			TXbuf = new RingBuffer<unsigned char>(ZT_TCP_TX_BUF_SZ);
			RXbuf = new RingBuffer<unsigned char>(ZT_TCP_RX_BUF_SZ);

			closure_ts = -1;
			ZT_PHY_SOCKFD_TYPE fdpair[2];
			if(socketpair(PF_LOCAL, SOCK_STREAM, 0, fdpair) < 0) {
				if(errno < 0) {
					DEBUG_ERROR("unable to create socketpair");
					return;
				}
			}
			sdk_fd = fdpair[0];
			app_fd = fdpair[1];
		}
		~Connection() { }
	};

	/*
	 * A helper object for passing SocketTap(s) and Connection(s) through the stack
	 */
	struct ConnectionPair
	{
	  SocketTap *tap;
	  Connection *conn;
	  ConnectionPair(SocketTap *_tap, Connection *conn) : tap(_tap), conn(conn) {}
	};
}
#endif