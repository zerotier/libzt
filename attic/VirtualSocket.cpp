/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2018  ZeroTier, Inc.  https://www.zerotier.com/
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

#include "libztDefs.h"

#ifdef ZT_VIRTUAL_SOCKET

#include <ctime>
#include <sys/socket.h>

#include "Phy.hpp"

#include "libzt.h"
#include "libztDebug.h"
#include "VirtualSocket.h"
#include "VirtualTap.h"
#include "RingBuffer.h"

class VirtualTap;

void VirtualSocket::apply_state(int state) {
	// states may be set by application or by stack callbacks, thus this must be guarded
	_op_m.lock();
	_state &= state;
	_op_m.unlock();
}

void VirtualSocket::set_state(int state) {
	_op_m.lock();
	_state = state;
	_op_m.unlock();
}

int VirtualSocket::get_state() {
	return _state;
}

VirtualSocket::VirtualSocket() {
	DEBUG_EXTRA("this=%p",this);
	memset(&local_addr, 0, sizeof(sockaddr_storage));
	memset(&peer_addr,  0, sizeof(sockaddr_storage));

	// ringbuffer used for incoming and outgoing traffic between app, stack, stack drivers, and ZT
	TXbuf = new RingBuffer(ZT_TCP_TX_BUF_SZ);
	RXbuf = new RingBuffer(ZT_TCP_RX_BUF_SZ);

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

VirtualSocket::~VirtualSocket() {
	DEBUG_EXTRA("this=%p",this);
	close(app_fd);
	close(sdk_fd);
	delete TXbuf;
	delete RXbuf;
	TXbuf = RXbuf = NULL;
	//picosock->priv = NULL;
	pcb = NULL;
}

#endif // ZT_VIRTUAL_SOCKET
