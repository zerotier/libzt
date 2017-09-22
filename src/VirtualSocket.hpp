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

// General connection object used by VirtualTap and network stack drivers

#ifndef ZT_CONNECTION_HPP
#define ZT_CONNECTION_HPP

#include <ctime>
#include <sys/socket.h>

#if defined(STACK_PICO)
#include "pico_socket.h"
#endif
#if defined(STACK_LWIP)
#include "lwip/tcp.h"
#endif

#include "Phy.hpp"

#include "libzt.h"
#include "VirtualTap.hpp"
#include "RingBuffer.hpp"

#define VS_OK                         100
#define VS_SHOULD_STOP                101
#define VS_STOPPED                    102
#define VS_STATE_UNHANDLED_CONNECTED  103
#define VS_STATE_CONNECTED            104
#define VS_STATE_LISTENING            105

namespace ZeroTier {

	class VirtualTap;

	/**
	 * An abstraction of a socket that operates between the application-exposed platform-sockets 
	 * and the network stack's representation of a protocol control block. This object is used by
	 * the POSIX socket emulation layer and stack drivers.
	 */
	class VirtualSocket
	{
	private:
		int _state = VS_OK;
	public:
		RingBuffer<unsigned char> *TXbuf;
		RingBuffer<unsigned char> *RXbuf;
		Mutex _tx_m, _rx_m, _op_m;
		PhySocket *sock = NULL;

		// State control
		/**
		 * Sets the VirtualSocket's state value
		 */
		void set_state(int state) {
			// states may be set by application or by stack callbacks, thus this must be guarded
			_op_m.lock();
			_state = state; 
			//DEBUG_EXTRA("SET STATE = %d (vs=%p)", _state, this);
			_op_m.unlock();
		}
		/**
		 * Gets the VirtualSocket's state value
		 */
		int get_state() {
			//DEBUG_EXTRA("GET STATE = %d (vs=%p)", _state, this);
			return _state; 
		}
#if defined(STACK_PICO)
		struct pico_socket *picosock = NULL;
#endif
#if defined(STACK_LWIP)
		void *pcb = NULL; // Protocol Control Block
		/*
		  - TCP_WRITE_FLAG_COPY: indicates whether the new memory should be allocated
			for the data to be copied into. If this flag is not given, no new memory
			should be allocated and the data should only be referenced by pointer. This
			also means that the memory behind dataptr must not change until the data is
			ACKed by the remote host
		  - TCP_WRITE_FLAG_MORE: indicates that more data follows. If this is omitted,
			the PSH flag is set in the last segment created by this call to tcp_write.
			If this flag is given, the PSH flag is not set.
		*/
		// copy as default, processed via pointer reference if set to 0. See notes in lwip_cb_sent() and lwip_Write()
		int8_t copymode = TCP_WRITE_FLAG_COPY;
#endif

		struct sockaddr_storage local_addr; // address we've bound to locally
		struct sockaddr_storage peer_addr;  // address of connection call to remote host

		int socket_family = 0;
		int socket_type = 0;
		int protocol = 0;
		int app_fd = 0; // used by app for I/O
		int sdk_fd = 0; // used by lib for I/O

		std::queue<VirtualSocket*> _AcceptedConnections;
		VirtualTap *tap = NULL;
		std::time_t closure_ts = 0;

		VirtualSocket() {

			memset(&local_addr, 0, sizeof(sockaddr_storage));
			memset(&peer_addr,  0, sizeof(sockaddr_storage));

			// ringbuffer used for incoming and outgoing traffic between app, stack, stack drivers, and ZT
			TXbuf = new RingBuffer<unsigned char>(ZT_TCP_TX_BUF_SZ);
			RXbuf = new RingBuffer<unsigned char>(ZT_TCP_RX_BUF_SZ);

			// socketpair, I/O channel between app and stack drivers
			closure_ts = -1;
			ZT_PHY_SOCKFD_TYPE fdpair[2];
			if (socketpair(PF_LOCAL, SOCK_STREAM, 0, fdpair) < 0) {
				if (errno < 0) {
					DEBUG_ERROR("unable to create socketpair, errno=%d", errno);
					return;
				}
			}
			sdk_fd = fdpair[0];
			app_fd = fdpair[1];

			// set to non-blocking since these are used as the primary I/O channel
			if (fcntl(sdk_fd, F_SETFL, O_NONBLOCK) < 0) {
				DEBUG_ERROR("error while setting virtual socket to NONBLOCKING. exiting", errno);
				exit(0);
			}
		}
		~VirtualSocket() {
			close(app_fd);
			close(sdk_fd);
			delete TXbuf;
			delete RXbuf;
			TXbuf = RXbuf = NULL;
		}
	};

	/**
	 * A helper object for passing VirtualTap(s) and VirtualSocket(s) through the stack
	 */
	struct VirtualBindingPair
	{
	  VirtualTap *tap;
	  VirtualSocket *vs;
	  VirtualBindingPair(VirtualTap *_tap, VirtualSocket *_vs) : tap(_tap), vs(_vs) {}
	};
}
#endif
