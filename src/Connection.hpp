/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2016  ZeroTier, Inc.  https://www.zerotier.com/
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
 */

#ifndef ZT_CONNECTION_HPP
#define ZT_CONNECTION_HPP

#include <sys/socket.h>

// picoTCP
#include "pico_socket.h"

// ZT
#include "Phy.hpp"

// SDK
#include "ZeroTierSDK.h"
#include "SocketTap.hpp"
#include "RingBuffer.hpp"

namespace ZeroTier {

	/*
	* Connection object
	*/
	struct Connection
	{
		//circular_buffer<char> crbuf = circular_buffer<char>(ZT_TCP_RX_BUF_SZ);
		//circular_buffer<char> ctbuf = circular_buffer<char>(ZT_TCP_TX_BUF_SZ);


		int pid;
		PhySocket *sock;				
		struct pico_socket *picosock;

		// TODO: For getsockname, etc
		struct sockaddr_storage *local_addr; // Address we've bound to locally
		struct sockaddr_storage *peer_addr; // Address of connection call to remote host

		// RX/TX buffers
		int txsz, rxsz;
		unsigned char txbuf[ZT_TCP_TX_BUF_SZ];
		unsigned char rxbuf[ZT_TCP_RX_BUF_SZ];

		int data_sock;
		int socket_family, socket_type;

		int app_fd; // provided to app for I/O
		int sdk_fd; // provided to SDK for I/O

		std::queue<Connection*> _AcceptedConnections;
		SocketTap *tap; // Reference to SocketTap  
		int state; // See ZeroTierSDK.h for (ZT_SOCK_STATE_*)

		Connection() {
			// DEBUG_INFO("Connection() this = %p", this);
			ZT_PHY_SOCKFD_TYPE fdpair[2];
			if(socketpair(PF_LOCAL, SOCK_STREAM, 0, fdpair) < 0) {
				if(errno < 0) {
					DEBUG_ERROR("unable to create socketpair");
					return;
				}
			}
			sdk_fd = fdpair[0];
			app_fd = fdpair[1];
			if(ZT_SOCK_BEHAVIOR_LINGER) {
				struct linger so_linger;
				so_linger.l_onoff = true;
				so_linger.l_linger = ZT_SOCK_BEHAVIOR_LINGER_TIME;
		        if(zts_setsockopt(app_fd, SOL_SOCKET, SO_LINGER, &so_linger, sizeof so_linger) < 0) {
    				DEBUG_ERROR("error setsockopt (%d)", errno);
		        }
		    }
		}
		~Connection()
		{
			// DEBUG_INFO("~Connection() this = %p", this);
		}
	};

	/*
	 * A helper object for passing SocketTaps and Connections through the stack
	 */
	struct ConnectionPair
	{
	  SocketTap *tap;
	  Connection *conn;
	  ConnectionPair(SocketTap *_tap, Connection *conn) : tap(_tap), conn(conn) {}
	};
}
#endif