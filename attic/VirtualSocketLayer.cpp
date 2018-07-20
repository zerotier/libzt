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
 * VirtualSocket management layer
 */

#include "libztDefs.h"

#ifdef ZT_VIRTUAL_SOCKET

#ifdef __linux__
#include <net/if.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#endif

extern int errno;

#include "libzt.h"
#include "VirtualTap.h"
#include "RingBuffer.h"
#include "VirtualSocket.h"
#include "VirtualSocketLayer.h"
#include "VirtualBindingPair.h"
#include "ZT1Service.h"
#include "Utilities.h"

#ifdef STACK_LWIP
#include "lwIP.h"
//#include "lwip/sockets.h"
#endif

#ifdef STACK_PICO
#include "picoTCP.h"
#include "pico_stack.h"
#include "pico_socket.h"
#endif

#include <map>
#include <utility>

std::map<int, std::pair<VirtualSocket*,VirtualTap*>*> fdmap;
std::map<int, VirtualSocket*> unmap;

// externs from VirtualSocketLayer.h
//std::map<int, VirtualSocket*> unmap;
//std::map<int, std::pair<VirtualSocket*,VirtualTap*>*> fdmap;

// int socket_family, int socket_type, int protocol
int virt_socket(int socket_family, int socket_type, int protocol) {
	DEBUG_EXTRA();
	int err = errno = 0;
	if (socket_family < 0 || socket_type < 0 || protocol < 0) {
		errno = EINVAL;
		return -1;
	}
	if (socket_type == SOCK_SEQPACKET) {
		DEBUG_ERROR("SOCK_SEQPACKET not yet supported.");
		errno = EPROTONOSUPPORT; // seemingly closest match
		return -1;
	}
	if (socket_type == SOCK_RAW) {
		// VirtualSocket is only used to associate a socket with a VirtualTap, it has no other implication
		VirtualSocket *vs = new VirtualSocket();
		vs->socket_family = socket_family;
		vs->socket_type = socket_type;
		vs->protocol = protocol;
		add_unassigned_virt_socket(vs->app_fd, vs);
		return vs->app_fd;
	}
#if defined(STACK_PICO)
	struct pico_socket *p;
	err = rd_pico_socket(&p, socket_family, socket_type, protocol);
	if (err == false && p) {
		VirtualSocket *vs = new VirtualSocket();
		vs->socket_family = socket_family;
		vs->socket_type = socket_type;
		vs->pcb = p;
		add_unassigned_virt_socket(vs->app_fd, vs);
		err = vs->app_fd; // return one end of the socketpair
	}
	else {
		DEBUG_ERROR("failed to create pico_socket");
		errno = ENOMEM;
		err = -1;
	}
#endif
#if defined(STACK_LWIP)
	// TODO: check for max lwIP timers/sockets
	void *pcb;
	err = rd_lwip_socket(&pcb, socket_family, socket_type, protocol);
	if (pcb) {
		VirtualSocket *vs = new VirtualSocket();
		vs->socket_family = socket_family;
		vs->socket_type = socket_type;
		vs->pcb = pcb;
		add_unassigned_virt_socket(vs->app_fd, vs);
		// return one end of the socketpair for the app to use
		err = vs->app_fd;
	}
	else {
		DEBUG_ERROR("failed to create lwip pcb");
		errno = ENOMEM;
		err = -1;
	}
#endif

//#if defined(DEFAULT_VS_LINGER)
	/*
	if (socket_type == SOCK_STREAM) {
		linger lin;
		unsigned int y=sizeof(lin);
		lin.l_onoff=1;
		lin.l_linger=10;
		int fd = err;
		if ((err = virt_setsockopt(fd, SOL_SOCKET, SO_LINGER, (void*)(&lin), y)) < 0) {
			DEBUG_ERROR("error while setting default linger time on socket");
			errno = -1; // TODO
			return -1;
		}
		err = fd;
	}
	*/
//#endif
	return err;
}

int virt_connect(int fd, const struct sockaddr *addr, socklen_t addrlen) {
	DEBUG_INFO("fd=%d",fd);
	int err = errno = 0;
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	VirtualSocket *vs = get_virt_socket(fd);
	if (vs == NULL) {
		DEBUG_ERROR("invalid socket, unable to locate VirtualSocket for fd=%d", fd);
		errno = EBADF;
		return -1;
	}
	struct pico_socket *ps = (struct pico_socket*)(vs->pcb);
	if (addr == NULL) {
		DEBUG_ERROR("invalid address for fd=%d", fd);
		errno = EINVAL;
		return -1;
	}
	if (addrlen <= 0) {
		DEBUG_ERROR("invalid address length for fd=%d", fd);
		errno = EINVAL;
		return -1;
	}
	// TODO: Handle bad address lengths, right now this call will still
	// succeed with a complete connect despite a bad address length.
	// DEBUG_EXTRA("fd = %d, %s : %d", fd, ipstr, ntohs(port));
	ZeroTier::InetAddress inet;
	sockaddr2inet(vs->socket_family, addr, &inet);
	VirtualTap *tap = getTapByAddr(&inet);
	if (tap == NULL) {
		DEBUG_ERROR("no route to host, could not find appropriate VirtualTap for fd=%d", fd);
		errno = ENETUNREACH;
		return -1;
	}

#if defined(STACK_PICO)
	// pointer to virtual tap we use in callbacks from the stack

	ps->priv = new VirtualBindingPair(tap, vs);
#endif
#if defined(STACK_LWIP)
#endif

	if ((err = tap->Connect(vs, addr, addrlen)) < 0) {
		DEBUG_ERROR("error while connecting socket");
		// errno will be set by tap->Connect
		return -1;
	}
	// assign this VirtualSocket to the tap we decided on
	tap->_VirtualSockets.push_back(vs);
	vs->tap = tap;
	vs->sock = tap->_phy.wrapSocket(vs->sdk_fd, vs);

	// TODO: Consolidate these calls
	del_unassigned_virt_socket(fd);
	add_assigned_virt_socket(tap, vs, fd);

	// save peer addr, for calls like getpeername
	memcpy(&(vs->peer_addr), addr, sizeof(vs->peer_addr));

	// Below will simulate BLOCKING/NON-BLOCKING behaviour

	// NOTE: pico_socket_connect() will return 0 if no error happens immediately, but that doesn't indicate
	// the connection was completed, for that we must wait for a callback from the stack. During that
	// callback we will place the VirtualSocket in a VS_STATE_UNHANDLED_CONNECTED state to signal
	// to the multiplexer logic that this connection is complete and a success value can be sent to the
	// user application

	int f_err, blocking = 1;
	if ((f_err = fcntl(fd, F_GETFL, 0)) < 0) {
		DEBUG_ERROR("fcntl error, err=%s, errno=%d", f_err, errno);
		// errno will be set by fcntl
		return -1;
	}
	blocking = !(f_err & O_NONBLOCK);
	if (blocking == false) {
		errno = EINPROGRESS; // can't connect immediately
		err = -1;
	}
	if (blocking == true) {
		bool complete = false;
		while (true) {
			// FIXME: locking and unlocking so often might cause significant performance overhead while outgoing VirtualSockets
			// are being established (also applies to accept())
			nanosleep((const struct timespec[]) {{0, (ZT_CONNECT_RECHECK_DELAY * 1000000)}}, NULL);
			tap->_tcpconns_m.lock();
			for (int i=0; i<tap->_VirtualSockets.size(); i++) {
#if defined(STACK_PICO)
				DEBUG_EXTRA("checking tap->_VirtualSockets[i]=%p", tap->_VirtualSockets[i]);
				if (tap->_VirtualSockets[i]->get_state() == PICO_ERR_ECONNRESET) {
					errno = ECONNRESET;
					DEBUG_ERROR("ECONNRESET");
					err = -1;
				}
#endif
#if defined(STACK_LWIP)
#endif
				if (tap->_VirtualSockets[i]->get_state() == VS_STATE_UNHANDLED_CONNECTED) {
					tap->_VirtualSockets[i]->set_state(VS_STATE_CONNECTED);
					complete = true;
				}
			}
			tap->_tcpconns_m.unlock();
			if (complete) {
				break;
			}
		}
	}
	return err;
}

int virt_bind(int fd, const struct sockaddr *addr, socklen_t addrlen) {
	int err = errno = 0;
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	VirtualSocket *vs = get_virt_socket(fd);
	if (vs == NULL) {
		DEBUG_ERROR("no VirtualSocket for fd=%d", fd);
		errno = ENOTSOCK;
		return -1;
	}
	struct pico_socket *ps = (struct pico_socket*)(vs->pcb);
	// detect local interface binds
	VirtualTap *tap = NULL;
	if (vs->socket_family == AF_INET) {
		struct sockaddr_in *in4 = (struct sockaddr_in *)addr;
		if (in4->sin_addr.s_addr == INADDR_ANY) {
			DEBUG_EXTRA("AF_INET, INADDR_ANY, binding to all interfaces");
			// grab first vtap
			if (vtaps.size()) {
				tap = (VirtualTap*)(vtaps[0]); // pick any vtap
			}
		}
		if (in4->sin_addr.s_addr == 0x7f000001) {
			DEBUG_EXTRA("127.0.0.1, will bind to appropriate vtap when connection is inbound");
			if (vtaps.size()) {
				tap = (VirtualTap*)(vtaps[0]); // pick any vtap
			}
		}
	}
	if (vs->socket_family == AF_INET6) {
		struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)addr;
		if (memcmp((void*)&(in6->sin6_addr), (void*)&(in6addr_any), sizeof(in6addr_any)) == 0) {
			DEBUG_EXTRA("AF_INET6, in6addr_any, binding to all interfaces");
			if (vtaps.size()) {
				tap = (VirtualTap*)(vtaps[0]);  // pick any vtap
			}
		}
	}

	ZeroTier::InetAddress inet;
	sockaddr2inet(vs->socket_family, addr, &inet);
	if (tap == NULL) {
		tap = getTapByAddr(&inet);
	}
	if (tap == NULL) {
		DEBUG_ERROR("no matching interface to bind to, could not find appropriate VirtualTap for fd=%d", fd);
		errno = ENETUNREACH;
		return -1;
	}
#if defined(STACK_PICO)
	// used in callbacks from network stack
	ps->priv = new VirtualBindingPair(tap, vs);
#endif
	tap->addVirtualSocket(vs);
	err = tap->Bind(vs, addr, addrlen);
	if (err == 0) { // success
		vs->tap = tap;
		del_unassigned_virt_socket(fd);
		add_assigned_virt_socket(tap, vs, fd);
	}
	return err;
}

int virt_listen(int fd, int backlog) {
	DEBUG_EXTRA("fd=%d", fd);
	int err = errno = 0;
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	VirtualSocket *vs = get_virt_socket(fd);
	if (vs == NULL) {
		DEBUG_ERROR("!vs");
		return -1;
	}
	//std::pair<VirtualSocket*, VirtualTap*> *p = get_assigned_virtual_pair(fd);
	_multiplexer_lock.lock();
	VirtualTap *tap = vs->tap;
	if (tap == NULL || vs == NULL) {
		DEBUG_ERROR("unable to locate tap interface for file descriptor");
		errno = EBADF;
		return -1;
	}
	backlog = backlog > 128 ? 128 : backlog; // See: /proc/sys/net/core/somaxconn
	err = tap->Listen(vs, backlog);
	vs->set_state(VS_STATE_LISTENING);
	_multiplexer_lock.unlock();
	return err;
}

int virt_accept(int fd, struct sockaddr *addr, socklen_t *addrlen) {
	int err = errno = 0;
	DEBUG_EXTRA("fd=%d", fd);
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	if (addr && *addrlen <= 0) {
		DEBUG_ERROR("invalid address length given");
		errno = EINVAL; // TODO, not actually a valid error for this function
		return -1;
	}
	if (virt_can_provision_new_socket(SOCK_STREAM) == false) {
		DEBUG_ERROR("cannot provision additional socket due to limitation of network stack");
		errno = EMFILE;
		return -1;
	}

	//std::pair<VirtualSocket*, VirtualTap*> *p = get_assigned_virtual_pair(fd);
	VirtualSocket *vs = get_virt_socket(fd);
	if (vs == NULL) {
		DEBUG_ERROR("!vs");
		return -1;
	}
	_multiplexer_lock.lock();
	//std::pair<VirtualSocket*, VirtualTap*> *p = get_assigned_virtual_pair(fd);
	VirtualTap *tap = vs->tap;
	// BLOCKING: loop and keep checking until we find a newly accepted VirtualSocket
	int f_err, blocking = 1;
	if ((f_err = fcntl(fd, F_GETFL, 0)) < 0) {
		DEBUG_ERROR("fcntl error, err = %s, errno = %d", f_err, errno);
		err = -1;
	}
	else {
		blocking = !(f_err & O_NONBLOCK);
	}
	VirtualSocket *accepted_vs;
	if (err == false) {
		if (blocking == false) { // non-blocking
			DEBUG_EXTRA("EWOULDBLOCK, assuming non-blocking mode");
			errno = EWOULDBLOCK;
			err = -1;
			accepted_vs = tap->Accept(vs);
		}
		else { // blocking
			while (true) {
				nanosleep((const struct timespec[]) {{0, (ZT_ACCEPT_RECHECK_DELAY * 1000000)}}, NULL);
				accepted_vs = tap->Accept(vs);
				if (accepted_vs)
					break; // accepted fd = err
			}
		}
		if (accepted_vs) {
			add_assigned_virt_socket(tap, accepted_vs, accepted_vs->app_fd);
			err = accepted_vs->app_fd;
		}
	}
	if (err > 0) {
		if (addr && *addrlen) {
			*addrlen = *addrlen < sizeof(accepted_vs->peer_addr) ? *addrlen : sizeof(accepted_vs->peer_addr);
			// copy address into provided address buffer and len buffer
			memcpy(addr, &(accepted_vs->peer_addr), *addrlen);
		}
	}
	return err;
}

#if defined(__linux__)
int virt_accept4(int fd, struct sockaddr *addr, socklen_t *addrlen, int flags)
{
	errno = 0;
	//DEBUG_INFO("fd=%d", fd);
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	if (flags & SOCK_NONBLOCK) {
		fcntl(fd, F_SETFL, O_NONBLOCK);
	}
	if (flags & SOCK_CLOEXEC) {
		fcntl(fd, F_SETFL, FD_CLOEXEC);
	}
	addrlen = !addr ? 0 : addrlen;
	return virt_accept(fd, addr, addrlen);
}
#endif

int virt_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen)
{
	DEBUG_EXTRA("fd=%d, level=%d, optname=%d", fd, level, optname);
	int err = errno = 0;
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	VirtualSocket *vs = get_virt_socket(fd);
	if (vs == NULL) {
		DEBUG_ERROR("invalid fd=%d", fd);
		errno = EBADF;
		return -1;
	}
#if defined(STACK_PICO)
	err = rd_pico_setsockopt(vs, level, optname, optval, optlen);
#endif
#if defined(STACK_LWIP)
	err = rd_lwip_setsockopt(vs, level, optname, optval, optlen);
#endif
	return err;
}

int virt_getsockopt(int fd, int level, int optname, void *optval, socklen_t *optlen)
{
	DEBUG_EXTRA("fd=%d, level=%d, optname=%d", fd, level, optname);
	int err = errno = 0;
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	VirtualSocket *vs = get_virt_socket(fd);
	if (vs == NULL) {
		DEBUG_ERROR("invalid fd=%d", fd);
		errno = EBADF;
		return -1;
	}
#if defined(STACK_PICO)
	err = rd_pico_getsockopt(vs, level, optname, optval, optlen);
#endif
#if defined(STACK_LWIP)
	err = rd_lwip_getsockopt(vs, level, optname, optval, optlen);
#endif
	return err;
}

int virt_getsockname(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
	int err = errno = 0;
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	// TODO
	return err;
}

int virt_getpeername(int fd, struct sockaddr *addr, socklen_t *addrlen)
{
	DEBUG_INFO("fd=%d", fd);
	int err = errno = 0;
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	VirtualSocket *vs = get_virt_socket(fd);
	if (vs == NULL) {
		errno = ENOTCONN;
		return -1;
	}
	memcpy(addr, &(vs->peer_addr), sizeof(struct sockaddr_storage));
	return err;
}

int virt_gethostname(char *name, size_t len)
{
	errno = 0;
	return gethostname(name, len);
}

int virt_sethostname(const char *name, size_t len)
{
	errno = 0;
	return sethostname(name, len);
}

int virt_close(int fd)
{
	DEBUG_EXTRA("fd=%d", fd);
	int err = errno = 0;
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	VirtualSocket *vs = get_virt_socket(fd);
	if (vs == NULL) {
		DEBUG_ERROR("no vs found for fd=%d", fd);
		errno = EBADF;
		return -1;
	}
	del_virt_socket(fd);
	if (vs->tap) {
		vs->tap->Close(vs);
	}
	delete vs;
	vs = NULL;
	return err;
}

/*
int virt_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
	errno = 0;
	return poll(fds, nfds, timeout);
}
*/

int virt_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout)
{
	errno = 0;
	return select(nfds, readfds, writefds, exceptfds, timeout);
}

int virt_fcntl(int fd, int cmd, int flags)
{
	int err = errno = 0;
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	VirtualSocket *vs = get_virt_socket(fd);
	if (vs == NULL) {
		DEBUG_ERROR("invalid vs for fd=%d", fd);
		errno = EBADF;
		return -1;
	}
	err = fcntl(fd, cmd, flags);
	return err;
}

int virt_ioctl(int fd, unsigned long request, void *argp)
{
	int err = errno = -1;
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	else {
#if defined(__linux__)
		/*
		if (argp)
		{
			struct ifreq *ifr = (struct ifreq *)argp;
			VirtualTap *tap = getTapByName(ifr->ifr_name);
			if (tap == NULL) {
				DEBUG_ERROR("unable to locate tap interface with that name");
				err = -1;
				errno = EINVAL;
			}
			// index of VirtualTap interface
			if (request == SIOCGIFINDEX) {
				ifr->ifr_ifindex = tap->ifindex;
				err = 0;
			}
			// MAC addres or VirtualTap
			if (request == SIOCGIFHWADDR) {
				tap->_mac.copyTo(&(ifr->ifr_hwaddr.sa_data), sizeof(ifr->ifr_hwaddr.sa_data));
				err = 0;
			}
			// IP address of VirtualTap
			if (request == SIOCGIFADDR) {
				struct sockaddr_in *in4 = (struct sockaddr_in *)&(ifr->ifr_addr);
				memcpy(&(in4->sin_addr.s_addr), tap->_ips[0].rawIpData(), sizeof(ifr->ifr_addr));
				err = 0;
			}
		}
		else {
			DEBUG_INFO("!argp");
		}
		*/
#else
		// err = ioctl(fd, request, argp);
		err = -1;
#endif
	}
	return err;
}

ssize_t virt_sendto(int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen)
{
	//DEBUG_TRANS("fd=%d", fd);
	int err = errno = 0;
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	if (len == 0) {
		return 0;
	}
	if (len > ZT_SOCKET_MSG_BUF_SZ) {
		DEBUG_ERROR("msg is too long to be sent atomically (len=%d)", len);
		errno = EMSGSIZE;
		return -1;
	}
	if (buf == NULL) {
		DEBUG_ERROR("msg buf is null");
		errno = EINVAL;
		return -1;
	}
	VirtualSocket *vs = get_virt_socket(fd);
	if (vs == NULL) {
		DEBUG_ERROR("no vs found for fd=%x", fd);
		errno = EBADF;
		return -1;
	}
	ZeroTier::InetAddress iaddr;
	VirtualTap *tap;

	if (vs->socket_type == SOCK_DGRAM) {
		if (vs->socket_family == AF_INET)
		{
			char ipstr[INET_ADDRSTRLEN];
			memset(ipstr, 0, INET_ADDRSTRLEN);
			inet_ntop(AF_INET,
				(const void *)&((struct sockaddr_in *)addr)->sin_addr.s_addr, ipstr, INET_ADDRSTRLEN);
			iaddr.fromString(ipstr);
		}
		if (vs->socket_family == AF_INET6)
		{
			char ipstr[INET6_ADDRSTRLEN];
			memset(ipstr, 0, INET6_ADDRSTRLEN);
			inet_ntop(AF_INET6,
				(const void *)&((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr, ipstr, INET6_ADDRSTRLEN);
			// TODO: This is a hack, determine a proper way to do this
			char addrstr[INET6_ADDRSTRLEN];
			sprintf(addrstr, "%s%s", ipstr, std::string("/40").c_str());
			iaddr.fromString(addrstr);
		}
		// get tap
		tap = getTapByAddr(&iaddr);
		if (tap == NULL) {
			DEBUG_INFO("SOCK_DGRAM, tap not found");
			errno = EDESTADDRREQ; // TODO: double check this is the best errno to report
			return -1;
		}
		// write
		if ((err = tap->SendTo(vs, buf, len, flags, addr, addrlen)) < 0) {
			DEBUG_ERROR("error while attempting to sendto");
			errno = EINVAL; // TODO: Not correct, but what else could we use?
		}
	}
	if (vs->socket_type == SOCK_RAW)
	{
		struct sockaddr_ll *socket_address = (struct sockaddr_ll *)addr;
		VirtualTap *tap = getTapByIndex(socket_address->sll_ifindex);
		if (tap) {
			DEBUG_INFO("found interface of ifindex=%d", tap->ifindex);
			err = tap->Write(vs, (void*)buf, len);
		}
		else {
			DEBUG_ERROR("unable to locate tap of ifindex=%d", socket_address->sll_ifindex);
			err = -1;
			errno = EINVAL;
		}
	}
	return err;
}

ssize_t virt_send(int fd, const void *buf, size_t len, int flags)
{
	// DEBUG_TRANS("fd=%d", fd);
	int err = errno = 0;
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	if (len == 0) {
		return 0;
	}
	if (len > ZT_SOCKET_MSG_BUF_SZ) {
		DEBUG_ERROR("msg is too long to be sent atomically (len=%d)", len);
		errno = EMSGSIZE;
		return -1;
	}
	VirtualSocket *vs = get_virt_socket(fd);
	if (vs == NULL) {
		DEBUG_ERROR("invalid vs for fd=%d", fd);
		errno = EBADF;
		return -1;
	}
	if (vs->socket_type != SOCK_STREAM) {
		DEBUG_ERROR("the socket is not connection-mode, and no peer address is set for fd=%d", fd);
		errno = EDESTADDRREQ;
		return -1;
	}
	if (flags & MSG_DONTROUTE) {
		DEBUG_INFO("MSG_DONTROUTE not implemented yet");
		errno = EINVAL;
		return -1;
	}
	if (flags & MSG_DONTWAIT) {
		// The stack drivers and stack are inherently non-blocking by design, but we
		// still need to modify the unix pipe connecting them to the application:
		fcntl(fd, F_SETFL, O_NONBLOCK);
	}
	if (flags & MSG_EOR) {
		DEBUG_INFO("MSG_EOR not implemented yet");
		errno = EINVAL;
		return -1;
	}
	if (flags & MSG_OOB) {
		DEBUG_INFO("MSG_OOB not implemented yet");
		errno = EINVAL;
		return -1;
	}
#if defined(__linux__)
	if (flags & MSG_CONFIRM) {
		DEBUG_INFO("MSG_CONFIRM not implemented yet");
		errno = EINVAL;
		return -1;
	}
	if (flags & MSG_MORE) {
		DEBUG_INFO("MSG_MORE not implemented yet");
		errno = EINVAL;
		return -1;
	}
	if (flags & MSG_NOSIGNAL) {
		DEBUG_INFO("MSG_NOSIGNAL not implemented yet");
		errno = EINVAL;
		return -1;
	}
#endif

	err = write(fd, buf, len);
	// restore "per-call" flags

	if (flags & MSG_DONTWAIT) {
		int saved_flags = fcntl(fd, F_GETFL);
		if (fcntl(fd, F_SETFL, saved_flags & ~O_NONBLOCK) < 0) { // mask out the blocking flag
			return -1;
		}
	}
	return err;
}

// TODO
ssize_t virt_sendmsg(int fd, const struct msghdr *msg, int flags)
{
	DEBUG_TRANS("fd=%d", fd);
	int err = errno = 0;
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	else {
		err = sendmsg(fd, msg, flags);
	}
	return err;
}

ssize_t virt_recv(int fd, void *buf, size_t len, int flags)
{
	DEBUG_TRANS("fd=%d", fd);
	int err = errno = 0;
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	VirtualSocket *vs = get_virt_socket(fd);
	if (vs == NULL) {
		DEBUG_ERROR("invalid vs for fd=%d", fd);
		errno = EBADF;
		return -1;
	}
	if (vs->socket_type != SOCK_STREAM) {
		DEBUG_ERROR("the socket is not connection-mode, and no peer address is set for fd=%d", fd);
		errno = EDESTADDRREQ;
		return -1;
	}
	if (vs->get_state() != VS_STATE_CONNECTED) {
		DEBUG_ERROR("the socket is not in a connected state, fd=%d", fd);
		errno = ENOTCONN;
		return -1;
	}
	if (flags & MSG_DONTWAIT) {
		// The stack drivers and stack are inherently non-blocking by design, but we
		// still need to modify the unix pipe connecting them to the application:
		fcntl(fd, F_SETFL, O_NONBLOCK);
	}
	if (flags & MSG_OOB) {
		DEBUG_INFO("MSG_OOB not implemented yet");
		errno = EINVAL;
		return -1;
	}
	if (flags & MSG_TRUNC) {
		DEBUG_INFO("MSG_TRUNC not implemented yet");
		errno = EINVAL;
		return -1;
	}
	if (flags & MSG_WAITALL) {
		DEBUG_INFO("MSG_WAITALL not implemented yet");
		errno = EINVAL;
		return -1;
	}
#if defined(__linux__)
	if (flags & MSG_ERRQUEUE) {
		DEBUG_INFO("MSG_ERRQUEUE not implemented yet");
		errno = EINVAL;
		return -1;
	}
#endif

	if (flags & MSG_PEEK) {
		// MSG_PEEK doesn't require any special stack-related machinery so we can just
		// pass it to a regular recv() call with no issue
		err = recv(fd, buf, len, MSG_PEEK);
	}

	// restore "per-call" flags

	if (flags & MSG_DONTWAIT) {
		int saved_flags = fcntl(fd, F_GETFL);
		if (fcntl(fd, F_SETFL, saved_flags & ~O_NONBLOCK) < 0) { // mask out the blocking flag
			return -1;
		}
	}
	return err;
}

ssize_t virt_recvfrom(int fd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen)
{
	//DEBUG_TRANS("fd=%d", fd);
	int32_t r = 0;
	errno = 0;
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	if (len == 0) {
		return 0;
	}
	if (buf == NULL) {
		DEBUG_ERROR("buf is null");
		errno = EINVAL;
		return -1;
	}
	char udp_msg_buf[ZT_SOCKET_MSG_BUF_SZ];
	char *msg_ptr = udp_msg_buf;
	memset(msg_ptr, 0, sizeof(int32_t)); // zero only len portion

	int32_t udp_msg_len = 0;

	// PEEK at the buffer and see if we can read a length, if not, err out
	r = recv(fd, msg_ptr, sizeof(int32_t), MSG_PEEK);
	if (r != sizeof(int32_t)) {
		errno = EIO; // TODO: test for this
		return -1;
	}
	// read of sizeof(int32_t) for the length of the datagram (including address)
	r = read(fd, msg_ptr, sizeof(int32_t));
	// copy to length variable
	memcpy(&udp_msg_len, msg_ptr, sizeof(int32_t));
	msg_ptr+=sizeof(int32_t);

	if (udp_msg_len <= 0) {
		DEBUG_ERROR("invalid datagram");
		errno = EIO; // TODO: test for this
		return -1;
	}
	// there is a datagram to read, so let's read it
	// zero remainder of buffer
	memset(msg_ptr, 0, ZT_SOCKET_MSG_BUF_SZ- sizeof(int32_t));
	if ((r = read(fd, msg_ptr, udp_msg_len)) < 0) {
		DEBUG_ERROR("invalid datagram");
		errno = EIO; // TODO: test for this
		return -1;
	}
	// get address
	if (addr) {
		if (*addrlen < sizeof(struct sockaddr_storage)) {
			DEBUG_ERROR("invalid address length provided");
			errno = EINVAL;
			return -1;
		}
		*addrlen = sizeof(struct sockaddr_storage);
		memcpy(addr, msg_ptr, *addrlen);
	}
	msg_ptr+=sizeof(struct sockaddr_storage);
	// get payload
	int32_t payload_sz = udp_msg_len - *addrlen;
	int32_t write_sz = len < payload_sz ? len : payload_sz;
	memcpy(buf, msg_ptr, write_sz);
	return write_sz;
}

// TODO
ssize_t virt_recvmsg(int fd, struct msghdr *msg,int flags)
{
	//DEBUG_TRANS("fd=%d", fd);
	int err = errno = 0;
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	else {
		err = recvmsg(fd, msg, flags);
	}
	return err;
}

int virt_read(int fd, void *buf, size_t len) {
	DEBUG_TRANS("fd=%d", fd);
	errno = 0;
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	VirtualSocket *vs = get_virt_socket(fd);
	if (vs == NULL) {
		DEBUG_ERROR("invalid vs for fd=%d", fd);
		errno = EBADF;
		return -1;
	}
	return read(fd, buf, len);
}

int virt_write(int fd, const void *buf, size_t len) {
	DEBUG_TRANS("fd=%d", fd);
	errno = 0;
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	VirtualSocket *vs = get_virt_socket(fd);
	if (vs == NULL) {
		DEBUG_ERROR("invalid vs for fd=%d", fd);
		errno = EBADF;
		return -1;
	}
	return write(fd, buf, len);
}

int virt_shutdown(int fd, int how)
{
	int err = errno = 0;
	if (fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	if (how != SHUT_RD && how != SHUT_WR && how != SHUT_RDWR) {
		errno = EINVAL;
		return -1;
	}
	VirtualSocket *vs = get_virt_socket(fd);
	if (vs == NULL) {
		DEBUG_ERROR("invalid vs for fd=%d", fd);
		errno = EBADF;
		return -1;
	}
	if (vs->get_state() != VS_STATE_CONNECTED || vs->socket_type != SOCK_STREAM) {
		DEBUG_ERROR("the socket is either not in a connected state, or isn't connection-based, fd=%d", fd);
		errno = ENOTCONN;
		return -1;
	}
	if (vs->tap) {
		err = vs->tap->Shutdown(vs, how);
	}
	return err;
}

/****************************************************************************/
/* Socket API Helper functions --- DON'T CALL THESE DIRECTLY                */
/****************************************************************************/

bool virt_can_provision_new_socket(int socket_type)
{
	int can = false;
#if defined(STACK_PICO)
	return !(pico_ntimers()+1 > PICO_MAX_TIMERS);
#endif
#if defined(STACK_LWIP)
	if (socket_type == SOCK_STREAM) {
		return !(rd_lwip_num_current_tcp_pcbs()+1 > MEMP_NUM_TCP_PCB);
	}
	if (socket_type == SOCK_DGRAM) {
		return !(rd_lwip_num_current_udp_pcbs()+1 > MEMP_NUM_UDP_PCB);
	}
	if (socket_type == SOCK_RAW) {
		return !(rd_lwip_num_current_raw_pcbs()+1 > MEMP_NUM_RAW_PCB);
	}
	can = true;
#endif
#if defined(NO_STACK)
	// always true since there's no network stack timer/memory limitation
	can = true;
#endif
	return can;
}

int virt_num_active_sockets()
{
	_multiplexer_lock.lock();
	int num = unmap.size() + fdmap.size();
	_multiplexer_lock.unlock();
	return num;
}

int virt_maxsockets(int socket_type)
{
	int max = 0;
#if defined(STACK_PICO)
	// TODO: This is only an approximation
	// TODO: distinquish by type
	max = PICO_MAX_TIMERS - 10;
#endif
#if defined(STACK_LWIP)
	if (socket_type == SOCK_STREAM) {
		max = MEMP_NUM_TCP_PCB;
	}
	if (socket_type == SOCK_DGRAM) {
		max = MEMP_NUM_UDP_PCB;
	}
#endif
#if defined(NO_STACK)
	// arbitrary
#if defined(__linux__)
	max = RLIMIT_NOFILE;
#endif
#if defined(__APPLE__)
	max = 1024;
#endif
#endif
	return max;
}

/****************************************************************************/
/* VirtualSocket / VirtualTap helper functions - DON'T CALL THESE DIRECTLY  */
/****************************************************************************/

VirtualSocket *get_virt_socket(int fd)
{
	DEBUG_EXTRA("fd=%d", fd);
	_multiplexer_lock.lock();
	// try to locate in unmapped set
	VirtualSocket *vs = unmap[fd];
	if (vs == NULL) {
		// if not, try to find in mapped set (bind to vtap has been performed)
		std::pair<VirtualSocket*, VirtualTap*> *p = fdmap[fd];
		if (p) {
			vs = p->first;
		}
		else {
			DEBUG_ERROR("unable to locate virtual socket");
		}
	}
	_multiplexer_lock.unlock();
	return vs;
}

int del_virt_socket(int fd)
{
	DEBUG_EXTRA("fd=%d", fd);
	int err = 0;
	_multiplexer_lock.lock();
	try {
		std::map<int, VirtualSocket*>::iterator un_iter = unmap.find(fd);
		if (un_iter != unmap.end()) {
			unmap.erase(un_iter);
		}
		std::map<int, std::pair<VirtualSocket*,VirtualTap*>*>::iterator fd_iter = fdmap.find(fd);
		if (fd_iter != fdmap.end()) {
			fdmap.erase(fd_iter);
		}
	}
	catch( ... ) {
		DEBUG_ERROR("unable to remove virtual socket");
		err = -1;
	}
	_multiplexer_lock.unlock();
	return err;
}

int add_unassigned_virt_socket(int fd, VirtualSocket *vs)
{
	DEBUG_EXTRA("fd=%d, vs=%p", fd, vs);
	int err = 0;
	_multiplexer_lock.lock();
	try {
		std::map<int, VirtualSocket*>::iterator un_iter = unmap.find(fd);
		if (un_iter == unmap.end()) {
			unmap[fd] = vs;
		}
		else {
			DEBUG_ERROR("fd=%d already contained in <fd:vs> map", fd);
		}
	}
	catch( ... ) {
		DEBUG_ERROR("unable to add virtual socket");
		err = -1;
	}
	_multiplexer_lock.unlock();
	return err;
}

int del_unassigned_virt_socket(int fd)
{
	DEBUG_EXTRA("fd=%d", fd);
	int err = 0;
	_multiplexer_lock.lock();
	try {
		std::map<int, VirtualSocket*>::iterator un_iter = unmap.find(fd);
		if (un_iter != unmap.end()) {
			unmap.erase(un_iter);
		}
	}
	catch( ... ) {
		DEBUG_ERROR("unable to remove virtual socket");
		err = -1;
	}
	_multiplexer_lock.unlock();
	return err;
}

int add_assigned_virt_socket(void *tap_ptr, VirtualSocket *vs, int fd)
{
	VirtualTap *tap = (VirtualTap*)tap_ptr;
	DEBUG_EXTRA("tap=%p, vs=%p, fd=%d", tap, vs, fd);
	int err = 0;
	_multiplexer_lock.lock();
	try {
		std::map<int, std::pair<VirtualSocket*,VirtualTap*>*>::iterator fd_iter;
		fd_iter = fdmap.find(fd);
		if (fd_iter == fdmap.end()) {
			fdmap[fd] = new std::pair<VirtualSocket*,VirtualTap*>(vs, tap);
		}
		else {
			DEBUG_ERROR("fd=%d already contained in <fd,<vs,vt>> map", fd);
		}
	}
	catch( ... ) {
		DEBUG_ERROR("unable to add virtual socket");
		err = -1;
	}
	_multiplexer_lock.unlock();
	return err;
}

int del_assigned_virt_socket(VirtualTap *tap, VirtualSocket *vs, int fd)
{
	DEBUG_EXTRA("tap=%p, vs=%p, fd=%d", tap, vs, fd);
	int err = 0;
	_multiplexer_lock.lock();
	try {
		std::map<int, std::pair<VirtualSocket*,VirtualTap*>*>::iterator fd_iter;
		fd_iter = fdmap.find(fd);
		if (fd_iter != fdmap.end()) {
			fdmap.erase(fd_iter);
		}
	}
	catch( ... ) {
		DEBUG_ERROR("unable to remove virtual socket");
		err = -1;
	}
	_multiplexer_lock.unlock();
	return err;
}

/*
std::pair<VirtualSocket*, VirtualTap*> *get_assigned_virtual_pair(int fd)
{
	_multiplexer_lock.lock();
	std::pair<VirtualSocket*, VirtualTap*> *p = fdmap[fd];
	_multiplexer_lock.unlock();
	return p;
}
*/

void disableTaps()
{
	_vtaps_lock.lock();
	for (int i=0; i<vtaps.size(); i++) {
		DEBUG_EXTRA("vt=%p", vtaps[i]);
		((VirtualTap*)vtaps[i])->_enabled = false;
	}
	_vtaps_lock.unlock();
}

void sockaddr2inet(int socket_family, const struct sockaddr *addr, ZeroTier::InetAddress *inet)
{
	char ipstr[INET6_ADDRSTRLEN];
	memset(ipstr, 0, INET6_ADDRSTRLEN);
	if (socket_family == AF_INET) {
		inet_ntop(AF_INET,
			(const void *)&((struct sockaddr_in *)addr)->sin_addr.s_addr, ipstr, INET_ADDRSTRLEN);
		inet->fromString(ipstr);
	}
	if (socket_family == AF_INET6) {
		inet_ntop(AF_INET6,
			(const void *)&((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr, ipstr, INET6_ADDRSTRLEN);
		char addrstr[64];
		sprintf(addrstr, "%s", ipstr);
		inet->fromString(addrstr);
	}
}

#endif // ZT_VIRTUAL_SOCKET
