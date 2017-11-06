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
 * Platform- and stack-agnostic implementation of a socket-like object
 */

#ifndef LIBZT_VIRTUALSOCKET_H
#define LIBZT_VIRTUALSOCKET_H

#include <queue>

#include "RingBuffer.h"
#include "libztDefs.h"
#include "VirtualTap.h"
#include "Mutex.hpp"

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

#ifdef __cplusplus
extern "C" {
#endif

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
		RingBuffer *TXbuf, *RXbuf;
		ZeroTier::Mutex _tx_m, _rx_m, _op_m;
		ZeroTier::PhySocket *sock = NULL;

		void *pcb = NULL; // Protocol Control Block

#if defined(STACK_LWIP)
		int32_t optflags = 0;
		int linger;
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
		int8_t copymode = 1; // TCP_WRITE_FLAG_COPY;
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

	/**
	 * Sets the VirtualSocket's state value
	 */
	void apply_state(int state);

	/**
	 * Sets the VirtualSocket's state value
	 */
	void set_state(int state);
	/**
	 * Gets the VirtualSocket's state value
	 */
	int get_state();

	/**
	 * default ctor
	 */
	VirtualSocket();

	/**
	 * dtor
	 */
	~VirtualSocket();
};

#ifdef __cplusplus
}
#endif

#endif // _H
