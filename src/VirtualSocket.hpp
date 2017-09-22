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

#define VS_STATE_INACTIVE             0x000000u // Default value for newly created VirtualSocket
#define VS_STATE_ACTIVE               0x000001u // VirtualSocket is RX'ing or TX'ing without issue 
#define VS_STATE_SHOULD_SHUTDOWN      0x000002u // Application, stack driver, or stack marked this VirtualSocket for death
#define VS_STATE_SHUTDOWN             0x000004u // VirtualSocket and underlying protocol control structures will not RX/TX
#define VS_STATE_CLOSED               0x000008u // VirtualSocket and underlying protocol control structures are closed
#define VS_STATE_UNHANDLED_CONNECTED  0x000010u // stack callback has received a connection but we haven't dealt with it
#define VS_STATE_CONNECTED            0x000020u // stack driver has akwnowledged new connection
#define VS_STATE_LISTENING            0x000040u // virtual socket is listening for incoming connections

#define VS_OPT_TCP_NODELAY            0x000000u // Nagle's algorithm
#define VS_OPT_SO_LINGER              0x000001u // VirtualSocket waits for data transmission before closure
/*
#define VS_RESERVED                   0x000002u // 
#define VS_RESERVED                   0x000004u //
#define VS_RESERVED                   0x000008u // 
#define VS_RESERVED                   0x000010u // 
#define VS_RESERVED                   0x000020u //
#define VS_RESERVED                   0x000040u //
*/
#define VS_OPT_FD_NONBLOCKING         0x000080u // Whether the VirtualSocket exhibits non-blocking behaviour
/*
#define VS_RESERVED                   0x000100u //
#define VS_RESERVED                   0x000200u //
#define VS_RESERVED                   0x000400u //
#define VS_RESERVED                   0x000800u //
#define VS_RESERVED                   0x001000u //
#define VS_RESERVED                   0x002000u //
#define VS_RESERVED                   0x004000u //
#define VS_RESERVED                   0x008000u //
#define VS_RESERVED                   0x010000u //
#define VS_RESERVED                   0x020000u //
#define VS_RESERVED                   0x040000u //
#define VS_RESERVED                   0x080000u //
#define VS_RESERVED                   0x100000u //
#define VS_RESERVED                   0x200000u //
#define VS_RESERVED                   0x400000u // 
#define VS_RESERVED                   0x800000u // 
*/

#define vs_is_nonblocking(vs)  (((vs)->optflags & VS_OPT_FD_NONBLOCKING) != 0)

namespace ZeroTier {

	class VirtualTap;

	/**
	 * An abstraction of a socket that operates between the application-exposed platform-sockets 
	 * and the network stack's representation of a protocol control structure. This object is used by
	 * the POSIX socket emulation layer and stack drivers.
	 */
	class VirtualSocket
	{
	private:
		int _state = VS_STATE_INACTIVE;
	public:
		RingBuffer<unsigned char> *TXbuf;
		RingBuffer<unsigned char> *RXbuf;
		Mutex _tx_m, _rx_m, _op_m;
		PhySocket *sock = NULL;

		/**
		 * Sets the VirtualSocket's state value
		 */
		void apply_state(int state) {
			// states may be set by application or by stack callbacks, thus this must be guarded
			_op_m.lock();
			_state &= state; 
#if defined (STACK_PICO)
			DEBUG_EXTRA("APPLY STATE=%d (state=%d, vs=%p, ps=%p)", _state, state, this, picosock);
#endif
#if defined (STACK_LWIP)
			DEBUG_EXTRA("APPLY STATE=%d (state=%d, vs=%p, pcb=%p)", _state, state, this, pcb);
#endif
			_op_m.unlock();
		}
		/**
		 * Sets the VirtualSocket's state value
		 */
		void set_state(int state) {
			// states may be set by application or by stack callbacks, thus this must be guarded
			_op_m.lock();
			_state = state; 
#if defined (STACK_PICO)
			DEBUG_EXTRA("SET STATE=%d (state=%d, vs=%p, ps=%p)", _state, state, this, picosock);
#endif
#if defined (STACK_LWIP)
			DEBUG_EXTRA("SET STATE=%d (state=%d, vs=%p, pcb=%p)", _state, state, this, pcb);
#endif			
			_op_m.unlock();
		}
		/**
		 * Gets the VirtualSocket's state value
		 */
		int get_state() {
#if defined (STACK_PICO)
			DEBUG_EXTRA("GET STATE=%d (vs=%p, ps=%p)", _state, this, picosock);
#endif
#if defined (STACK_LWIP)
			DEBUG_EXTRA("GET STATE=%d (vs=%p, pcb=%p)", _state, this, pcb);
#endif			
			return _state; 
		}
#if defined(STACK_PICO)
		struct pico_socket *picosock = NULL;
#endif
#if defined(STACK_LWIP)
		int32_t optflags = 0;
		int linger;
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

		VirtualSocket() {
			DEBUG_EXTRA("this=%p",this);
			memset(&local_addr, 0, sizeof(sockaddr_storage));
			memset(&peer_addr,  0, sizeof(sockaddr_storage));

			// ringbuffer used for incoming and outgoing traffic between app, stack, stack drivers, and ZT
			TXbuf = new RingBuffer<unsigned char>(ZT_TCP_TX_BUF_SZ);
			RXbuf = new RingBuffer<unsigned char>(ZT_TCP_RX_BUF_SZ);

			// socketpair, I/O channel between app and stack drivers
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
			DEBUG_EXTRA("this=%p",this);
			close(app_fd);
			close(sdk_fd);
			delete TXbuf;
			delete RXbuf;
			TXbuf = RXbuf = NULL;
#if defined(STACK_PICO)
			picosock->priv = NULL;
			picosock = NULL;
#endif			
#if defined(STACK_LWIP)
			pcb = NULL;
#endif
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
