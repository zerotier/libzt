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

/* This file implements the libzt library API, it talks to the network
stack driver and core ZeroTier service to create a socket-like interface
for applications to use. See also: include/libzt.h */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdint.h>

#if defined(__APPLE__)
#include <net/ethernet.h>
#include <net/if.h>
#endif
#if defined(__linux__)
#include <netinet/ether.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#endif

#include <pthread.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>

#if defined(STACK_PICO)
#include "pico_stack.h"
#endif
#if defined(STACK_LWIP)
#include "lwIP.hpp"
#endif

#include "OneService.hpp"
#include "Utils.hpp"
#include "OSUtils.hpp"
#include "InetAddress.hpp"
#include "ZeroTierOne.h"

#include "Utilities.hpp"
#include "VirtualTap.hpp"
#include "libzt.h"

#ifdef __cplusplus
extern "C" {
#endif

namespace ZeroTier {

	static ZeroTier::OneService *zt1Service;

	std::string homeDir; // Platform-specific dir we *must* use internally
	std::string netDir;  // Where network .conf files are to be written

#if defined(STACK_PICO)
	picoTCP *picostack = NULL;
#endif
#if defined(STACK_LWIP)
	lwIP *lwipstack = NULL;
#endif

	/*
	 * VirtualSockets that have been created but not bound to a VirtualTap interface yet
	 */
	std::map<int, VirtualSocket*> unmap;

	/*
	 * For fast lookup of VirtualSockets and VirtualTaps via given file descriptor
	 */
	std::map<int, std::pair<VirtualSocket*,VirtualTap*>*> fdmap;

	/*
	 * Virtual tap interfaces, one per virtual network
	 */
	std::vector<void*> vtaps;

	ZeroTier::Mutex _vtaps_lock;
	ZeroTier::Mutex _multiplexer_lock;
}

/****************************************************************************/
/* SDK Socket API - Language Bindings are written in terms of these         */
/****************************************************************************/

void zts_start(const char *path)
{
	if(ZeroTier::zt1Service)
		return;
#if defined(STACK_PICO)
	if(ZeroTier::picostack)
		return;
	ZeroTier::picostack = new ZeroTier::picoTCP();
	pico_stack_init();
#endif
#if defined(STACK_LWIP)
	ZeroTier::lwipstack = new ZeroTier::lwIP();
	lwip_init();
#endif
	if(path)
		ZeroTier::homeDir = path;
	pthread_t service_thread;
	pthread_create(&service_thread, NULL, zts_start_service, NULL);
}

void zts_simple_start(const char *path, const char *nwid)
{
	zts_start(path);
	while(!zts_running()) {
		nanosleep((const struct timespec[]){{0, (ZT_API_CHECK_INTERVAL * 1000000)}}, NULL);
	}
	zts_join(nwid);
	while(!zts_has_address(nwid)) {
		nanosleep((const struct timespec[]){{0, (ZT_API_CHECK_INTERVAL * 1000000)}}, NULL);
	}
}

void zts_stop() {
	if(ZeroTier::zt1Service) { 
		ZeroTier::zt1Service->terminate();
		dismantleTaps();
	}
}

void zts_join(const char * nwid) {
	if(ZeroTier::zt1Service) {
		std::string confFile = ZeroTier::zt1Service->givenHomePath() + "/networks.d/" + nwid + ".conf";
		if(!ZeroTier::OSUtils::mkdir(ZeroTier::netDir)) {
			DEBUG_ERROR("unable to create: %s", ZeroTier::netDir.c_str());
			handle_general_failure();
		}
		if(!ZeroTier::OSUtils::writeFile(confFile.c_str(), "")) {
			DEBUG_ERROR("unable to write network conf file: %s", confFile.c_str());
			handle_general_failure();
		}
		ZeroTier::zt1Service->join(nwid);
	}
	// provide ZTO service reference to virtual taps
	// TODO: This might prove to be unreliable, but it works for now
	for(int i=0;i<ZeroTier::vtaps.size(); i++) {
		ZeroTier::VirtualTap *s = (ZeroTier::VirtualTap*)ZeroTier::vtaps[i];
		s->zt1ServiceRef=(void*)ZeroTier::zt1Service;
	}
}

void zts_join_soft(const char * filepath, const char * nwid) { 
	std::string net_dir = std::string(filepath) + "/networks.d/";
	std::string confFile = net_dir + std::string(nwid) + ".conf";
	if(!ZeroTier::OSUtils::mkdir(net_dir)) {
		DEBUG_ERROR("unable to create: %s", net_dir.c_str());
		handle_general_failure();
	}
	if(!ZeroTier::OSUtils::fileExists(confFile.c_str(),false)) {
		if(!ZeroTier::OSUtils::writeFile(confFile.c_str(), "")) {
			DEBUG_ERROR("unable to write network conf file: %s", confFile.c_str());
			handle_general_failure();
		}
	}
}

void zts_leave(const char * nwid) { 
	if(ZeroTier::zt1Service)
		ZeroTier::zt1Service->leave(nwid);
}

void zts_leave_soft(const char * filepath, const char * nwid) {
	std::string net_dir = std::string(filepath) + "/networks.d/";
	ZeroTier::OSUtils::rm((net_dir + nwid + ".conf").c_str()); 
}

void zts_get_homepath(char *homePath, int len) { 
	if(ZeroTier::homeDir.length()) {
		memset(homePath, 0, len);
		memcpy(homePath, ZeroTier::homeDir.c_str(), len < ZeroTier::homeDir.length() ? len : ZeroTier::homeDir.length());
	}
}

void zts_core_version(char *ver) {
	int major, minor, revision;
	ZT_version(&major, &minor, &revision);
	sprintf(ver, "%d.%d.%d", major, minor, revision);
}

void zts_lib_version(char *ver) {
	sprintf(ver, "%d.%d.%d", ZT_LIB_VERSION_MAJOR, ZT_LIB_VERSION_MINOR, ZT_LIB_VERSION_REVISION);
}

int zts_get_device_id(char *devID) { 
	if(ZeroTier::zt1Service) {
		char id[ZT_ID_LEN];
		sprintf(id, "%lx",ZeroTier::zt1Service->getNode()->address());
		memcpy(devID, id, ZT_ID_LEN);
		return 0;
	}
	else // Service isn't online, try to read ID from file
	{
		std::string fname("identity.public");
		std::string fpath(ZeroTier::homeDir);

		if(ZeroTier::OSUtils::fileExists((fpath + ZT_PATH_SEPARATOR_S + fname).c_str(),false)) {
			std::string oldid;
			ZeroTier::OSUtils::readFile((fpath + ZT_PATH_SEPARATOR_S + fname).c_str(),oldid);
			memcpy(devID, oldid.c_str(), ZT_ID_LEN); // first 10 bytes of file
			return 0;
		}
	}
	return -1;
}

int zts_running() { 
	return !ZeroTier::zt1Service ? false : ZeroTier::zt1Service->isRunning(); 
}

int zts_has_ipv4_address(const char *nwid)
{
	char ipv4_addr[INET_ADDRSTRLEN];
	memset(ipv4_addr, 0, INET_ADDRSTRLEN);
	zts_get_ipv4_address(nwid, ipv4_addr, INET_ADDRSTRLEN);
	return strcmp(ipv4_addr, "\0");
}

int zts_has_ipv6_address(const char *nwid)
{
	char ipv6_addr[INET6_ADDRSTRLEN];
	memset(ipv6_addr, 0, INET6_ADDRSTRLEN);
	zts_get_ipv6_address(nwid, ipv6_addr, INET6_ADDRSTRLEN);
	return strcmp(ipv6_addr, "\0");
}

int zts_has_address(const char *nwid)
{
	return zts_has_ipv4_address(nwid) || zts_has_ipv6_address(nwid);
}

void zts_get_ipv4_address(const char *nwid, char *addrstr, const int addrlen)
{
	if(ZeroTier::zt1Service) {
		uint64_t nwid_int = strtoull(nwid, NULL, 16);
		ZeroTier::VirtualTap *tap = getTapByNWID(nwid_int);
		if(tap && tap->_ips.size()){ 
			for(int i=0; i<tap->_ips.size(); i++) {
				if(tap->_ips[i].isV4()) {
					char ipbuf[INET_ADDRSTRLEN];
					std::string addr = tap->_ips[i].toString(ipbuf);
					int len = addrlen < addr.length() ? addrlen : addr.length();
					memset(addrstr, 0, len);
					memcpy(addrstr, addr.c_str(), len); 
					return;
				}
			}
		}
	}
	else
		memcpy(addrstr, "\0", 1);
}

void zts_get_ipv6_address(const char *nwid, char *addrstr, const int addrlen)
{
	if(ZeroTier::zt1Service) {
		uint64_t nwid_int = strtoull(nwid, NULL, 16);
		ZeroTier::VirtualTap *tap = getTapByNWID(nwid_int);
		if(tap && tap->_ips.size()){ 
			for(int i=0; i<tap->_ips.size(); i++) {
				if(tap->_ips[i].isV6()) {
					char ipbuf[INET6_ADDRSTRLEN];
					std::string addr = tap->_ips[i].toString(ipbuf);
					int len = addrlen < addr.length() ? addrlen : addr.length();
					memset(addrstr, 0, len);
					memcpy(addrstr, addr.c_str(), len); 
					return;
				}
			}
		}
	}
	else
		memcpy(addrstr, "\0", 1);
}

void zts_get_6plane_addr(char *addr, const char *nwid, const char *devID)
{
	ZeroTier::InetAddress _6planeAddr = ZeroTier::InetAddress::makeIpv66plane(
		ZeroTier::Utils::hexStrToU64(nwid),ZeroTier::Utils::hexStrToU64(devID));
	char ipbuf[INET6_ADDRSTRLEN];
	memcpy(addr, _6planeAddr.toIpString(ipbuf), 40);
}

void zts_get_rfc4193_addr(char *addr, const char *nwid, const char *devID)
{
	ZeroTier::InetAddress _6planeAddr = ZeroTier::InetAddress::makeIpv6rfc4193(
		ZeroTier::Utils::hexStrToU64(nwid),ZeroTier::Utils::hexStrToU64(devID));
	char ipbuf[INET6_ADDRSTRLEN];
	memcpy(addr, _6planeAddr.toIpString(ipbuf), 40);
}

unsigned long zts_get_peer_count() {
	if(ZeroTier::zt1Service)
		return ZeroTier::zt1Service->getNode()->peers()->peerCount;
	else
		return 0;
}

int zts_get_peer_address(char *peer, const char *devID) {
	if(ZeroTier::zt1Service) {
		ZT_PeerList *pl = ZeroTier::zt1Service->getNode()->peers();
		// uint64_t addr;
		for(int i=0; i<pl->peerCount; i++) {
			// ZT_Peer *p = &(pl->peers[i]);
			// DEBUG_INFO("peer[%d] = %lx", i, p->address);
		}
		return pl->peerCount;
	}
	else
		return -1;
}

void zts_enable_http_control_plane()
{
	// TODO
}

void zts_disable_http_control_plane()
{
	// TODO
}

/****************************************************************************/
/* VirtualTap Multiplexer Functionality                                      */
/* - This section of the API is used to implement the general socket        */
/*   controls. Basically this is designed to handle socket provisioning     */
/*   requests when no VirtualTap is yet initialized, and as a way to         */
/*   determine which VirtualTap is to be used for a particular connect() or  */ 
/*   bind() call. This enables multi-network support                        */
/****************************************************************************/

/*

Darwin:

	[  ] [EACCES]           Permission to create a socket of the specified type and/or protocol is denied.
	[  ] [EAFNOSUPPORT]     The specified address family is not supported.
	[--] [EMFILE]           The per-process descriptor table is full.
	[NA] [ENFILE]           The system file table is full.
	[  ] [ENOBUFS]          Insufficient buffer space is available.  The socket cannot be created until sufficient resources are freed.
	[  ] [ENOMEM]           Insufficient memory was available to fulfill the request.
	[--] [EPROTONOSUPPORT]  The protocol type or the specified protocol is not supported within this domain.
	[  ] [EPROTOTYPE]       The socket type is not supported by the protocol.
*/

// int socket_family, int socket_type, int protocol
int zts_socket(ZT_SOCKET_SIG) {
	int err = errno = 0;
	if(socket_family < 0 || socket_type < 0 || protocol < 0) {
		errno = EINVAL;
		return -1;
	}
	if(!ZeroTier::zt1Service) {
		DEBUG_ERROR("cannot create socket, no service running. call zts_start() first.");
		errno = EMFILE; // could also be ENFILE
		return -1;
	}
	if(socket_type == SOCK_SEQPACKET) {
		DEBUG_ERROR("SOCK_SEQPACKET not yet supported.");
		errno = EPROTONOSUPPORT; // seemingly closest match
		return -1;
	}

	if(socket_type == SOCK_RAW)
	{
		// VirtualSocket is only used to associate a socket with a VirtualTap, it has no other implication
		ZeroTier::VirtualSocket *vs = new ZeroTier::VirtualSocket();
		vs->socket_family = socket_family;
		vs->socket_type = socket_type;
		vs->protocol = protocol;
		add_unassigned_virtual_socket(vs->app_fd, vs);
		return vs->app_fd;
	}

#if defined(STACK_PICO)
	struct pico_socket *p;
	err = ZeroTier::picostack->pico_Socket(&p, socket_family, socket_type, protocol);
	if(!err && p) {
		ZeroTier::VirtualSocket *vs = new ZeroTier::VirtualSocket();
		vs->socket_family = socket_family;
		vs->socket_type = socket_type;
		vs->picosock = p;
		add_unassigned_virtual_socket(vs->app_fd, vs);
		err = vs->app_fd; // return one end of the socketpair
	}
	else {
		DEBUG_ERROR("failed to create pico_socket");
		err = -1;
	}
#endif

#if defined(STACK_LWIP)
	// TODO: check for max lwIP timers/sockets
	void *pcb;
	err = ZeroTier::lwipstack->lwip_Socket(&pcb, socket_family, socket_type, protocol);
	if(pcb) {
		ZeroTier::VirtualSocket *vs = new ZeroTier::VirtualSocket();
		vs->socket_family = socket_family;
		vs->socket_type = socket_type;
		vs->pcb = pcb;
		add_unassigned_virtual_socket(vs->app_fd, vs);
		err = vs->app_fd; // return one end of the socketpair
	}
	else {
		DEBUG_ERROR("failed to create lwip pcb");
		err = -1;
	}
#endif
	return err;
}


/*

Darwin:

	[  ] [EACCES]           The destination address is a broadcast address and the socket option SO_BROADCAST is not set.
	[  ] [EADDRINUSE]       The address is already in use.
	[  ] [EADDRNOTAVAIL]    The specified address is not available on this machine.
	[  ] [EAFNOSUPPORT]     Addresses in the specified address family cannot be used with this socket.
	[  ] [EALREADY]         The socket is non-blocking and a previous VirtualSocket attempt has not yet been completed.
	[--] [EBADF]            socket is not a valid descriptor.
	[  ] [ECONNREFUSED]     The attempt to connect was ignored (because the target is not listening for VirtualSockets) or explicitly rejected.
	[  ] [EFAULT]           The address parameter specifies an area outside the process address space.
	[  ] [EHOSTUNREACH]     The target host cannot be reached (e.g., down, disconnected).
	[--] [EINPROGRESS]      The socket is non-blocking and the VirtualSocket cannot be completed immediately.  
							It is possible to select(2) for completion by selecting the socket for writing.
	[NA] [EINTR]            Its execution was interrupted by a signal.
	[  ] [EINVAL]           An invalid argument was detected (e.g., address_len is not valid for the address family, the specified address family is invalid).
	[  ] [EISCONN]          The socket is already connected.
	[  ] [ENETDOWN]         The local network interface is not functioning.
	[--] [ENETUNREACH]      The network isn't reachable from this host.
	[  ] [ENOBUFS]          The system call was unable to allocate a needed memory buffer.
	[  ] [ENOTSOCK]         socket is not a file descriptor for a socket.
	[  ] [EOPNOTSUPP]       Because socket is listening, no VirtualSocket is allowed.
	[  ] [EPROTOTYPE]       address has a different type than the socket that is bound to the specified peer address.
	[  ] [ETIMEDOUT]        VirtualSocket establishment timed out without establishing a VirtualSocket.
	[  ] [ECONNRESET]       Remote host reset the VirtualSocket request.

Linux:

	[  ] [EACCES]           For UNIX domain sockets, which are identified by pathname: Write permission is denied on the socket file, 
							or search permission is denied for one of the directories in the path prefix. (See also path_resolution(7).)
	[  ] [EACCES, EPERM]    The user tried to connect to a broadcast address without having the socket broadcast flag enabled or the 
							VirtualSocket request failed because of a local firewall rule.
	[  ] [EADDRINUSE]       Local address is already in use.
	[  ] [EAFNOSUPPORT]     The passed address didn't have the correct address family in its sa_family field.
	[  ] [EAGAIN]           No more free local ports or insufficient entries in the routing cache. For AF_INET see the description 
							of /proc/sys/net/ipv4/ip_local_port_range ip(7) for information on how to increase the number of local ports.
	[  ] [EALREADY]         The socket is nonblocking and a previous VirtualSocket attempt has not yet been completed.
	[  ] [EBADF]            The file descriptor is not a valid index in the descriptor table.
	[  ] [ECONNREFUSED]     No-one listening on the remote address.
	[  ] [EFAULT]           The socket structure address is outside the user's address space.
	[  ] [EINPROGRESS]      The socket is nonblocking and the VirtualSocket cannot be completed immediately. It is possible to select(2) or 
							poll(2) for completion by selecting the socket for writing. After select(2) indicates writability, use getsockopt(2) 
							to read the SO_ERROR option at level SOL_SOCKET to determine whether connect() completed successfully (SO_ERROR is zero) 
							or unsuccessfully (SO_ERROR is one of the usual error codes listed here, explaining the reason for the failure).
	[  ] [EINTR]            The system call was interrupted by a signal that was caught; see signal(7).
	[  ] [EISCONN]          The socket is already connected.
	[  ] [ENETUNREACH]      Network is unreachable.
	[  ] [ENOTSOCK]         The file descriptor is not associated with a socket.
	[  ] [ETIMEDOUT]        Timeout while attempting VirtualSocket. The server may be too busy to accept new VirtualSockets. Note that for 
							IP sockets the timeout may be very long when syncookies are enabled on the server.

*/

int zts_connect(ZT_CONNECT_SIG) {
	int err = errno = 0;
	if(fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	if(!ZeroTier::zt1Service) {
		DEBUG_ERROR("service not started. call zts_start(path) first");
		errno = EBADF;
		return -1;
	}
	ZeroTier::VirtualSocket *vs = get_virtual_socket(fd);
	if(!vs) {
		DEBUG_ERROR("invalid socket, unable to locate VirtualSocket for fd=%d", fd);
		errno = EBADF;
		return -1;
	}
	if(!addr) {
		DEBUG_ERROR("invalid address for fd=%d", fd);
		errno = EINVAL;
		return -1;
	}
	if(addrlen <= 0) {
		DEBUG_ERROR("invalid address length for fd=%d", fd);
		errno = EINVAL;
		return -1;
	}
	// TODO: Handle bad address lengths, right now this call will still 
	// succeed with a complete connect despite a bad address length. 

	// DEBUG_EXTRA("fd = %d, %s : %d", fd, ipstr, ntohs(port));
	ZeroTier::InetAddress inet;
	sockaddr2inet(vs->socket_family, addr, &inet);
	ZeroTier::VirtualTap *tap = getTapByAddr(&inet);
	if(!tap) {
		DEBUG_ERROR("no route to host, could not find appropriate VirtualTap for fd=%d", fd);
		errno = ENETUNREACH;
		return -1;
	}

#if defined(STACK_PICO)
	// pointer to virtual tap we use in callbacks from the stack
	vs->picosock->priv = new ZeroTier::VirtualBindingPair(tap, vs); 
#endif
#if defined(STACK_LWIP)
#endif

	if((err = tap->Connect(vs, addr, addrlen)) < 0) {
		DEBUG_ERROR("error while connecting socket");
		// errno will be set by tap->Connect
		return -1;
	}
	// assign this VirtualSocket to the tap we decided on
	tap->_VirtualSockets.push_back(vs); 
	vs->tap = tap;
	vs->sock = tap->_phy.wrapSocket(vs->sdk_fd, vs);  

	// TODO: Consolidate these calls
	del_unassigned_virtual_socket(fd);
	add_assigned_virtual_socket(tap, vs, fd);

	// save peer addr, for calls like getpeername
	memcpy(&(vs->peer_addr), addr, sizeof(vs->peer_addr));

	// Below will simulate BLOCKING/NON-BLOCKING behaviour

	// NOTE: pico_socket_connect() will return 0 if no error happens immediately, but that doesn't indicate
	// the connection was completed, for that we must wait for a callback from the stack. During that
	// callback we will place the VirtualSocket in a ZT_SOCK_STATE_UNHANDLED_CONNECTED state to signal 
	// to the multiplexer logic that this connection is complete and a success value can be sent to the
	// user application

	int f_err, blocking = 1;
	if ((f_err = fcntl(fd, F_GETFL, 0)) < 0) {
		DEBUG_ERROR("fcntl error, err=%s, errno=%d", f_err, errno);
		// errno will be set by fcntl
		return -1;
	} 
	blocking = !(f_err & O_NONBLOCK);
	if(!blocking) {
		errno = EINPROGRESS; // can't connect immediately
		err = -1;
	}
	if(blocking) {
		bool complete = false;
		while(true)
		{
			// FIXME: locking and unlocking so often might cause a performance bottleneck while outgoing VirtualSockets
			// are being established (also applies to accept())
			nanosleep((const struct timespec[]){{0, (ZT_CONNECT_RECHECK_DELAY * 1000000)}}, NULL);
			tap->_tcpconns_m.lock();
			for(int i=0; i<tap->_VirtualSockets.size(); i++)
			{
#if defined(STACK_PICO)
				if(tap->_VirtualSockets[i]->state == PICO_ERR_ECONNRESET) {   
					errno = ECONNRESET;
					DEBUG_ERROR("ECONNRESET");
					err = -1;
				}
#endif
#if defined(STACK_LWIP)
#endif
				if(tap->_VirtualSockets[i]->state == ZT_SOCK_STATE_UNHANDLED_CONNECTED) {
					tap->_VirtualSockets[i]->state = ZT_SOCK_STATE_CONNECTED;
					complete = true;
				}
			}
			tap->_tcpconns_m.unlock();
			if(complete)
				break;
		}
	}
	return err;
}

/*

Darwin:

	[--] [EBADF]            S is not a valid descriptor.
	[--] [ENOTSOCK]         S is not a socket.
	[--] [EADDRNOTAVAIL]    The specified address is not available from the local
							machine.
	[  ] [EADDRINUSE]       The specified address is already in use.
	[  ] [EINVAL]           The socket is already bound to an address.
	[  ] [EACCES]           The requested address is protected, and the current
							user has inadequate permission to access it.
	[  ] [EFAULT]           The name parameter is not in a valid part of the user
							address space.
*/
int zts_bind(ZT_BIND_SIG) {
	int err = errno = 0;
	if(fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	if(!ZeroTier::zt1Service) {
		DEBUG_ERROR("service not started. call zts_start(path) first");        
		errno = EBADF;
		return -1;
	}
	ZeroTier::VirtualSocket *vs = get_virtual_socket(fd);
	if(!vs) {
		DEBUG_ERROR("no VirtualSocket for fd=%d", fd);
		errno = ENOTSOCK;
		return -1;
	}
	
	// detect local interface binds 
	ZeroTier::VirtualTap *tap = NULL;
	
	if(vs->socket_family == AF_INET) {
		struct sockaddr_in *in4 = (struct sockaddr_in *)addr;
		if(in4->sin_addr.s_addr == INADDR_ANY) {
			DEBUG_EXTRA("AF_INET, INADDR_ANY, binding to all interfaces");
			// grab first vtap
			if(ZeroTier::vtaps.size()) {
				tap = (ZeroTier::VirtualTap*)(ZeroTier::vtaps[0]); // pick any vtap
			}
		}
		if(in4->sin_addr.s_addr == 0x7f000001) {
			DEBUG_EXTRA("127.0.0.1, will bind to appropriate vtap when connection is inbound");
			if(ZeroTier::vtaps.size()) {
				tap = (ZeroTier::VirtualTap*)(ZeroTier::vtaps[0]); // pick any vtap
			}
		}
	}
	if(vs->socket_family == AF_INET6) {
		struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)addr;
		if(memcmp((void*)&(in6->sin6_addr), (void*)&(in6addr_any), sizeof(in6addr_any)) == 0) {
			DEBUG_EXTRA("AF_INET6, in6addr_any, binding to all interfaces");
			if(ZeroTier::vtaps.size()) {
				tap = (ZeroTier::VirtualTap*)(ZeroTier::vtaps[0]);  // pick any vtap
			}
		}
	}

	ZeroTier::InetAddress inet;
	sockaddr2inet(vs->socket_family, addr, &inet);
	if(!tap)
		tap = getTapByAddr(&inet);

	if(!tap) {
		DEBUG_ERROR("no matching interface to bind to, could not find appropriate VirtualTap for fd=%d", fd);
		errno = ENETUNREACH;
		return -1;
	}
#if defined(STACK_PICO) 
	// used in callbacks from network stack
	vs->picosock->priv = new ZeroTier::VirtualBindingPair(tap, vs);
#endif
	tap->addVirtualSocket(vs);
	err = tap->Bind(vs, addr, addrlen);
	vs->tap = tap;
	if(err == 0) { // success
		del_unassigned_virtual_socket(fd);
		add_assigned_virtual_socket(tap, vs, fd);
	}
	return err;
}

/*

Darwin:

	[--] [EACCES]           The current process has insufficient privileges.
	[--] [EBADF]            The argument socket is not a valid file descriptor.
	[--] [EDESTADDRREQ]     The socket is not bound to a local address and the protocol does not support listening on an unbound socket.
	[  ] [EINVAL]           socket is already connected.
	[  ] [ENOTSOCK]         The argument socket does not reference a socket.
	[  ] [EOPNOTSUPP]       The socket is not of a type that supports the operation listen().

Linux:

	[  ] [EADDRINUSE]       Another socket is already listening on the same port.
	[--] [EBADF]            The argument sockfd is not a valid descriptor.
	[  ] [ENOTSOCK]         The argument sockfd is not a socket.
	[  ] [EOPNOTSUPP]       The socket is not of a type that supports the listen() operation.
*/
int zts_listen(ZT_LISTEN_SIG) {
	int err = errno = 0;
	if(fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	if(!ZeroTier::zt1Service) {
		DEBUG_ERROR("service not started. call zts_start(path) first");
		errno = EACCES;
		return -1;
	}
	ZeroTier::_multiplexer_lock.lock();
	std::pair<ZeroTier::VirtualSocket*, ZeroTier::VirtualTap*> *p = ZeroTier::fdmap[fd];
	if(!p) {
		DEBUG_ERROR("unable to locate VirtualSocket pair. did you bind?");
		errno = EDESTADDRREQ;
		return -1;
	}
	ZeroTier::VirtualSocket *vs = p->first;
	ZeroTier::VirtualTap *tap = p->second;
	if(!tap || !vs) {
		DEBUG_ERROR("unable to locate tap interface for file descriptor");
		errno = EBADF;
		return -1;
	}
	backlog = backlog > 128 ? 128 : backlog; // See: /proc/sys/net/core/somaxconn
	err = tap->Listen(vs, backlog);
	vs->state = ZT_SOCK_STATE_LISTENING;
	ZeroTier::_multiplexer_lock.unlock();
	return err;
}

/*

Darwin:

	[--] [EBADF]            The descriptor is invalid.
	[  ] [ENOTSOCK]         The descriptor references a file, not a socket.
	[  ] [EOPNOTSUPP]       The referenced socket is not of type SOCK_STREAM.
	[  ] [EFAULT]           The addr parameter is not in a writable part of the
							user address space.
	[--] [EWOULDBLOCK]      The socket is marked non-blocking and no VirtualSockets
							are present to be accepted.
	[--] [EMFILE]           The per-process descriptor table is full.
	[  ] [ENFILE]           The system file table is full.
*/
int zts_accept(ZT_ACCEPT_SIG) {
	int err = errno = 0;
	//DEBUG_EXTRA("fd=%d", fd);
	if(fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	// +1 since we'll be creating a new pico_socket when we accept the VirtualSocket
	if(!can_provision_new_socket()) {
		DEBUG_ERROR("cannot provision additional socket due to limitation of network stack");
		errno = EMFILE;
		return -1;
	}
	ZeroTier::_multiplexer_lock.lock();
	std::pair<ZeroTier::VirtualSocket*, ZeroTier::VirtualTap*> *p = ZeroTier::fdmap[fd];
	if(!p) {
		DEBUG_ERROR("unable to locate VirtualSocket pair (did you zts_bind())?");
		errno = EBADF;
		err = -1;
	}
	else {
		ZeroTier::VirtualSocket *vs = p->first;
		ZeroTier::VirtualTap *tap = p->second;

		// BLOCKING: loop and keep checking until we find a newly accepted VirtualSocket
		int f_err, blocking = 1;
		if ((f_err = fcntl(fd, F_GETFL, 0)) < 0) {
			DEBUG_ERROR("fcntl error, err = %s, errno = %d", f_err, errno);
			err = -1;
		} 
		else {
			blocking = !(f_err & O_NONBLOCK);
		}

		if(!err) {
			ZeroTier::VirtualSocket *accepted_vs;
			if(!blocking) { // non-blocking
				DEBUG_EXTRA("EWOULDBLOCK, not a real error, assuming non-blocking mode");
				errno = EWOULDBLOCK;
				err = -1;
				accepted_vs = tap->Accept(vs);
			}
			else { // blocking
				while(true) {
					nanosleep((const struct timespec[]){{0, (ZT_ACCEPT_RECHECK_DELAY * 1000000)}}, NULL);
					accepted_vs = tap->Accept(vs);
					if(accepted_vs)
						break; // accepted fd = err
				}
			}
			if(accepted_vs) {
				ZeroTier::fdmap[accepted_vs->app_fd] = new std::pair<ZeroTier::VirtualSocket*,ZeroTier::VirtualTap*>(accepted_vs, tap);
				err = accepted_vs->app_fd;
			}
		}
		if(!err) {
			// copy address into provided address buffer and len buffer
			memcpy(addr, &(vs->peer_addr), sizeof(struct sockaddr));
			*addrlen = sizeof(vs->peer_addr);
		}
	}
	ZeroTier::_multiplexer_lock.unlock();
	return err;
}


/*
Linux accept() (and accept4()) passes already-pending network errors on the new socket as an error code from accept(). This behavior differs from other BSD socket implementations. For reliable operation the application should detect the network errors defined for the protocol after accept() and treat them like EAGAIN by retrying. In the case of TCP/IP, these are ENETDOWN, EPROTO, ENOPROTOOPT, EHOSTDOWN, ENONET, EHOSTUNREACH, EOPNOTSUPP, and ENETUNREACH.
Errors

	[  ] [EAGAIN or EWOULDBLOCK]   The socket is marked nonblocking and no VirtualSockets are present to be accepted. POSIX.1-2001 allows either error to be returned for this case, and does not require these constants to have the same value, so a portable application should check for both possibilities.
	[--] [EBADF]                   The descriptor is invalid.
	[  ] [ECONNABORTED]            A VirtualSocket has been aborted.
	[  ] [EFAULT]                  The addr argument is not in a writable part of the user address space.
	[NA] [EINTR]                   The system call was interrupted by a signal that was caught before a valid VirtualSocket arrived; see signal(7).
	[  ] [EINVAL]                  Socket is not listening for VirtualSockets, or addrlen is invalid (e.g., is negative).
	[  ] [EINVAL]                  (accept4()) invalid value in flags.
	[  ] [EMFILE]                  The per-process limit of open file descriptors has been reached.
	[  ] [ENFILE]                  The system limit on the total number of open files has been reached.
	[  ] [ENOBUFS, ENOMEM]         Not enough free memory. This often means that the memory allocation is limited by the socket buffer limits, not by the system memory.
	[  ] [ENOTSOCK]                The descriptor references a file, not a socket.
	[  ] [EOPNOTSUPP]              The referenced socket is not of type SOCK_STREAM.
	[  ] [EPROTO]                  Protocol error.

In addition, Linux accept() may fail if:

EPERM Firewall rules forbid VirtualSocket.



	SOCK_NONBLOCK   Set the O_NONBLOCK file status flag on the new open
				   file description.  Using this flag saves extra calls
				   to fcntl(2) to achieve the same result.

	SOCK_CLOEXEC    Set the close-on-exec (FD_CLOEXEC) flag on the new
				   file descriptor.  See the description of the
				   O_CLOEXEC flag in open(2) for reasons why this may be
				   useful.

	int fd, struct sockaddr *addr, socklen_t *addrlen, int flags
*/
#if defined(__linux__)
	int zts_accept4(ZT_ACCEPT4_SIG)
	{
		errno = 0;
		//DEBUG_INFO("fd=%d", fd);
		if(fd < 0 || fd >= ZT_MAX_SOCKETS) {
			errno = EBADF;
			return -1;
		}
		if(flags & SOCK_NONBLOCK) {
			fcntl(fd, F_SETFL, O_NONBLOCK);
		}
		if(flags & SOCK_CLOEXEC) {
			fcntl(fd, F_SETFL, FD_CLOEXEC);
		}
		addrlen = !addr ? 0 : addrlen;
		return zts_accept(fd, addr, addrlen);
	}
#endif


/*
	[--] [EBADF]            The argument s is not a valid descriptor.
	[  ] [ENOTSOCK]         The argument s is a file, not a socket.
	[--] [ENOPROTOOPT]      The option is unknown at the level indicated.
	[  ] [EFAULT]           The address pointed to by optval is not in a valid
							part of the process address space.  For getsockopt(),
							this error may also be returned if optlen is not in a
							valid part of the process address space.
	[  ] [EDOM]             The argument value is out of bounds.
*/
int zts_setsockopt(ZT_SETSOCKOPT_SIG)
{
	int err = errno = 0;
#if defined(STACK_PICO)
	//DEBUG_INFO("fd=%d", fd);
	if(fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	// Disable Nagle's algorithm
	struct pico_socket *p = NULL;
	err = zts_get_pico_socket(fd, &p);
	if(p) {
		int value = 1;
		if((err = pico_socket_setoption(p, PICO_TCP_NODELAY, &value)) < 0) {
			if(err == PICO_ERR_EINVAL) {
				DEBUG_ERROR("error while disabling Nagle's algorithm");
				errno = ENOPROTOOPT;
				return -1;
			}
		}

	}
	err = setsockopt(fd, level, optname, optval, optlen);
	return err;
#endif
	return 0;
}

/*
	[--] [EBADF]            The argument s is not a valid descriptor.
	[  ] [ENOTSOCK]         The argument s is a file, not a socket.
	[  ] [ENOPROTOOPT]      The option is unknown at the level indicated.
	[  ] [EFAULT]           The address pointed to by optval is not in a valid
							part of the process address space.  For getsockopt(),
							this error may also be returned if optlen is not in a
							valid part of the process address space.
	[  ] [EDOM]             The argument value is out of bounds.

*/
int zts_getsockopt(ZT_GETSOCKOPT_SIG)
{
	int err = errno = 0;
	if(fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	err = getsockopt(fd, level, optname, optval, optlen);
	return err;
}

/*
	[--] [EBADF]            The argument s is not a valid descriptor.
	[  ] [ENOTSOCK]         The argument s is a file, not a socket.
	[  ] [ENOBUFS]          Insufficient resources were available in the system to
							perform the operation.
	[  ] [EFAULT]           The name parameter points to memory not in a valid
							part of the process address space.
*/
int zts_getsockname(ZT_GETSOCKNAME_SIG)
{
	int err = errno = 0;
	if(fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	// TODO
	return err;
}

/*

Linux:

	[--] [EBADF]            The argument s is not a valid descriptor.
	[  ] [ENOTSOCK]         The argument s is a file, not a socket.
	[--] [ENOTCONN]         The socket is not connected.
	[  ] [ENOBUFS]          Insufficient resources were available in the system to
							perform the operation.
	[  ] [EFAULT]           The name parameter points to memory not in a valid
							part of the process address space.
*/
int zts_getpeername(ZT_GETPEERNAME_SIG)
{
	int err = errno = 0;
	if(fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	ZeroTier::VirtualSocket *vs = get_virtual_socket(fd);
	if(!vs) {
		errno = ENOTCONN;
		return -1;
	}
	memcpy(addr, &(vs->peer_addr), sizeof(struct sockaddr_storage));
	return err;
}

/*

Linux:

	[  ] [EFAULT]			name is an invalid address.
	[  ] [EINVAL] 			len is negative or, for sethostname(), len is larger than the
							maximum allowed size.
	[  ] [ENAMETOOLONG]		(glibc gethostname()) len is smaller than the actual size.
							(Before version 2.1, glibc uses EINVAL for this case.)
	[  ] [EPERM]  			For sethostname(), the caller did not have the CAP_SYS_ADMIN
							capability in the user namespace associated with its UTS
							namespace (see namespaces(7)).
*/

int zts_gethostname(ZT_GETHOSTNAME_SIG)
{
	errno = 0;
	return gethostname(name, len);
}

/*

Linux:

	[  ] [EFAULT]			name is an invalid address.
	[  ] [EINVAL] 			len is negative or, for sethostname(), len is larger than the
							maximum allowed size.
	[  ] [ENAMETOOLONG]		(glibc gethostname()) len is smaller than the actual size.
							(Before version 2.1, glibc uses EINVAL for this case.)
	[  ] [EPERM]  			For sethostname(), the caller did not have the CAP_SYS_ADMIN
							capability in the user namespace associated with its UTS
							namespace (see namespaces(7)).
*/

int zts_sethostname(ZT_SETHOSTNAME_SIG)
{
	errno = 0;
	return sethostname(name, len);
}

/*

Linux:

	See: http://yarchive.net/comp/linux/close_return_value.html

Linux / Darwin:

	[--] [EBADF]            fildes is not a valid, active file descriptor.
	[NA] [EINTR]            Its execution was interrupted by a signal.
	[  ] [EIO]              A previously-uncommitted write(2) encountered an input/output error.
*/

int zts_close(ZT_CLOSE_SIG)
{
	int err = errno = 0;
	if(fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	if(!ZeroTier::zt1Service) {
		DEBUG_ERROR("cannot close socket. service not started. call zts_start(path) first");
		errno = EBADF;
		return -1;
	}
	ZeroTier::VirtualSocket *vs = get_virtual_socket(fd);
	if(!vs) {
		DEBUG_ERROR("no vs found for fd=%d", fd);
		errno = EBADF;
		return -1;
	}
	if(vs->tap) {
		vs->tap->Close(vs);
	}
	delete vs;
	vs = NULL;
	del_virtual_socket(fd);
	return err;

	/*
		// check if socket is blocking
		int f_err, blocking = 1;
		if ((f_err = fcntl(fd, F_GETFL, 0)) < 0) {
			DEBUG_ERROR("fcntl error, err = %s, errno = %d", f_err, errno);
			err = -1;
		} 
		else {
			blocking = !(f_err & O_NONBLOCK);
		}
		if(blocking) {
			for(int i=0; i<ZT_SDK_CLTIME; i++) {
				if(vs->TXbuf->count() == 0)
					break;
				usleep(ZT_API_CHECK_INTERVAL * 1000);
			}
		}
	*/
}

int zts_poll(ZT_POLL_SIG)
{
	errno = 0;
	return poll(fds, nfds, timeout);
}

int zts_select(ZT_SELECT_SIG)
{
	errno = 0;
	return select(nfds, readfds, writefds, exceptfds, timeout);
}

int zts_fcntl(ZT_FCNTL_SIG)
{
	int err = errno = 0;
	if(fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	else {
		err = fcntl(fd, cmd, flags);
	}
	return err;
}

/*
	[  ] [BADF]             fd is not a valid file descriptor.
	[  ] [EFAULT]           argp references an inaccessible memory area.
	[  ] [EINVAL]           request or argp is not valid.
	[  ] [ENOTTY]           The specified request does not apply to the kind of object that the file descriptor fd references.
*/

int zts_ioctl(ZT_IOCTL_SIG)
{
	int err = errno = 0;
	if(fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	else {
#if defined(__linux__)
		if(argp)
		{
			struct ifreq *ifr = (struct ifreq *)argp;
			ZeroTier::VirtualTap *tap = getTapByName(ifr->ifr_name);
			if(!tap) {
				DEBUG_ERROR("unable to locate tap interface with that name");
				err = -1;
				errno = EINVAL;
			}
			// index of VirtualTap interface
			if(request == SIOCGIFINDEX) {
				ifr->ifr_ifindex = tap->ifindex;
				err = 0;
			}
			// MAC addres or VirtualTap
			if(request == SIOCGIFHWADDR) {
				tap->_mac.copyTo(&(ifr->ifr_hwaddr.sa_data), sizeof(ifr->ifr_hwaddr.sa_data));
				err = 0;
			}
			// IP address of VirtualTap
			if(request == SIOCGIFADDR) {
				struct sockaddr_in *in4 = (struct sockaddr_in *)&(ifr->ifr_addr);
				memcpy(&(in4->sin_addr.s_addr), tap->_ips[0].rawIpData(), sizeof(ifr->ifr_addr));
				err = 0;
			}
		}
		else
		{
			DEBUG_INFO("!argp");
		}
#else
		err = ioctl(fd, request, argp);
#endif
	}
	return err;
}


/*

Linux:

	[  ] [EAGAIN or EWOULDBLOCK] 	The socket is marked nonblocking and the requested operation would block. 
									POSIX.1-2001 allows either error to be returned for this case, and does not 
									require these constants to have the same value, so a portable application 
									should check for both possibilities.
	[--] [EBADF]                 	An invalid descriptor was specified.
	[  ] [ECONNRESET]            	VirtualSocket reset by peer.
	[  ] [EDESTADDRREQ]          	The socket is not VirtualSocket-mode, and no peer address is set.
	[  ] [EFAULT]                	An invalid user space address was specified for an argument.
	[  ] [EINTR]                 	A signal occurred before any data was transmitted; see signal(7).
	[  ] [EINVAL]                	Invalid argument passed.
	[  ] [EISCONN]               	The VirtualSocket-mode socket was connected already but a recipient was 
									specified. (Now either this error is returned, or the recipient 
									specification is ignored.)
	[  ] [EMSGSIZE]              	The socket type requires that message be sent atomically, and the size 
									of the message to be sent made this impossible.
	[  ] [ENOBUFS]               	The output queue for a network interface was full. This generally indicates 
									that the interface has stopped sending, but may be caused by transient congestion. 
									(Normally, this does not occur in Linux. Packets are just silently 
									dropped when a device queue overflows.)
	[  ] [ENOMEM]                	No memory available.
	[  ] [ENOTCONN]              	The socket is not connected, and no target has been given.
	[  ] [ENOTSOCK]              	The argument sockfd is not a socket.
	[  ] [EOPNOTSUPP]            	Some bit in the flags argument is inappropriate for the socket type.
	[  ] [EPIPE]                 	The local end has been shut down on a VirtualSocket oriented socket. 
									In this case the process will also receive a SIGPIPE unless MSG_NOSIGNAL is set.

	ZT_SENDTO_SIG int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen
*/

ssize_t zts_sendto(ZT_SENDTO_SIG)
{
	//DEBUG_TRANS("fd=%d", fd);
	int err = errno = 0;
	if(fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	if(len == 0) {
		return 0;
	}
	if(len > ZT_SOCKET_MSG_BUF_SZ) {
		DEBUG_ERROR("msg is too long to be sent atomically (len=%d)", len);
		errno = EMSGSIZE;
		return -1;
	}
	if(!buf) {
		DEBUG_ERROR("msg buf is null");
		errno = EINVAL;
		return -1;
	}
	ZeroTier::VirtualSocket *vs = get_virtual_socket(fd);
	if(!vs) {
		DEBUG_ERROR("no vs found for fd=%x", fd);
		handle_general_failure();
		errno = EBADF;
		return -1;
	}

	ZeroTier::InetAddress iaddr;
	ZeroTier::VirtualTap *tap;

	if(vs->socket_type == SOCK_DGRAM) 
	{
		if(vs->socket_family == AF_INET) 
		{
			char ipstr[INET_ADDRSTRLEN];
			memset(ipstr, 0, INET_ADDRSTRLEN);
			inet_ntop(AF_INET, 
				(const void *)&((struct sockaddr_in *)addr)->sin_addr.s_addr, ipstr, INET_ADDRSTRLEN);
			iaddr.fromString(ipstr);
		}
		if(vs->socket_family == AF_INET6) 
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
		if(!tap) {
			DEBUG_INFO("SOCK_DGRAM, tap not found");
			errno = EDESTADDRREQ; // TODO: double check this is the best errno to report
			return -1;
		}
		// write
		if((err = tap->SendTo(vs, buf, len, flags, addr, addrlen)) < 0) {
			DEBUG_ERROR("error while attempting to sendto");
			errno = EINVAL; // TODO: Not correct, but what else could we use?
		}
	}
	if(vs->socket_type == SOCK_RAW) 
	{
		struct sockaddr_ll *socket_address = (struct sockaddr_ll *)addr;
		ZeroTier::VirtualTap *tap = getTapByIndex(socket_address->sll_ifindex);
		if(tap) {
			DEBUG_INFO("found interface of ifindex=%d", tap->ifindex);
			err = tap->Write(vs, (void*)buf, len);
		}
		else {
			DEBUG_ERROR("unable to locate tap of ifindex=%d", socket_address->sll_ifindex);
			err = -1;
			errno = EINVAL;
		}
		//err = sendto(fd, buf, len, flags, addr, addrlen);
	}
	return err;
}

/*
	Linux:

	[  ] EACCES						(For UNIX domain sockets, which are identified by pathname)
									Write permission is denied on the destination socket file, or
									search permission is denied for one of the directories the
									path prefix.  (See path_resolution(7).)

									(For UDP sockets) An attempt was made to send to a
									network/broadcast address as though it was a unicast address.
	[  ] EAGAIN or EWOULDBLOCK		The socket is marked nonblocking and the requested operation
									would block.  POSIX.1-2001 allows either error to be returned
									for this case, and does not require these constants to have
									the same value, so a portable application should check for
									both possibilities.
	[  ] EAGAIN 					(Internet domain datagram sockets) The socket referred to by
									sockfd had not previously been bound to an address and, upon
									attempting to bind it to an ephemeral port, it was determined
									that all port numbers in the ephemeral port range are
									currently in use.  See the discussion of
									/proc/sys/net/ipv4/ip_local_port_range in ip(7).
	[--] EBADF  					sockfd is not a valid open file descriptor.
	[  ] ECONNRESET 				Connection reset by peer.
	[--] EDESTADDRREQ 				The socket is not connection-mode, and no peer address is set.
	[  ] EFAULT 					An invalid user space address was specified for an argument.
	[  ] EINTR  					A signal occurred before any data was transmitted
	[--] EINVAL 					Invalid argument passed.
	[  ] EISCONN					The connection-mode socket was connected already but a
									recipient was specified.  (Now either this error is returned,
									or the recipient specification is ignored.)
	[  ] EMSGSIZE					The socket type requires that message be sent atomically, and
									the size of the message to be sent made this impossible.
	[  ] ENOBUFS					The output queue for a network interface was full.  This
									generally indicates that the interface has stopped sending,
									but may be caused by transient congestion.  (Normally, this
									does not occur in Linux.  Packets are just silently dropped
									when a device queue overflows.)
	[  ] ENOMEM 					No memory available.
	[  ] ENOTCONN					The socket is not connected, and no target has been given.
	[  ] ENOTSOCK					The file descriptor sockfd does not refer to a socket.
	[  ] EOPNOTSUPP					Some bit in the flags argument is inappropriate for the socket
									type.
	[  ] EPIPE  					The local end has been shut down on a connection oriented
									socket.  In this case, the process will also receive a SIGPIPE
									unless MSG_NOSIGNAL is set.

	ZT_SEND_SIG int fd, const void *buf, size_t len, int flags
*/

ssize_t zts_send(ZT_SEND_SIG)
{
	// DEBUG_TRANS("fd=%d", fd);
	int err = errno = 0;
	if(fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	if(len == 0) {
		return 0;
	}
	if(len > ZT_SOCKET_MSG_BUF_SZ) {
		DEBUG_ERROR("msg is too long to be sent atomically (len=%d)", len);
		errno = EMSGSIZE;
		return -1;
	}

	ZeroTier::VirtualSocket *vs = get_virtual_socket(fd);
	if(!vs) {
		DEBUG_ERROR("invalid vs for fd=%d", fd);
		errno = EBADF;
		return -1;
	}
	if(vs->socket_type != SOCK_STREAM) {
		DEBUG_ERROR("the socket is not connection-mode, and no peer address is set for fd=%d", fd);
		errno = EDESTADDRREQ;
		return -1;
	}
	if(flags & MSG_DONTROUTE) {
		DEBUG_INFO("MSG_DONTROUTE not implemented yet");
		errno = EINVAL;
		return -1;
	}
	if(flags & MSG_DONTWAIT) {
		// The stack drivers and stack are inherently non-blocking by design, but we 
		// still need to modify the unix pipe connecting them to the application:
		fcntl(fd, F_SETFL, O_NONBLOCK);
	}
	if(flags & MSG_EOR) {
		DEBUG_INFO("MSG_EOR not implemented yet");
		errno = EINVAL;
		return -1;
	}
	if(flags & MSG_OOB) {
		DEBUG_INFO("MSG_OOB not implemented yet");
		errno = EINVAL;
		return -1;
	}
#if defined(__linux__)
	if(flags & MSG_CONFIRM) {
		DEBUG_INFO("MSG_CONFIRM not implemented yet");
		errno = EINVAL;
		return -1;
	}
	if(flags & MSG_MORE) {
		DEBUG_INFO("MSG_MORE not implemented yet");
		errno = EINVAL;
		return -1;
	}
	if(flags & MSG_NOSIGNAL) {
		DEBUG_INFO("MSG_NOSIGNAL not implemented yet");
		errno = EINVAL;
		return -1;
	}
#endif

	err = write(fd, buf, len);

	// restore "per-call" flags

	if(flags & MSG_DONTWAIT) {
		int saved_flags = fcntl(fd, F_GETFL);
		if(fcntl(fd, F_SETFL, saved_flags & ~O_NONBLOCK) < 0) { // mask out the blocking flag
			handle_general_failure();
			return -1;
		}
	}
	return err;
}

// TODO
ssize_t zts_sendmsg(ZT_SENDMSG_SIG)
{
	DEBUG_TRANS("fd=%d", fd);	
	int err = errno = 0;
	if(fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	else {
		err = sendmsg(fd, msg, flags);
	}
	return err;
}

/*

	Linux:

	   These are some standard errors generated by the socket layer.
	   Additional errors may be generated and returned from the underlying
	   protocol modules; see their manual pages.

	[  ] EAGAIN or EWOULDBLOCK		The socket is marked nonblocking and the receive operation
									would block, or a receive timeout had been set and the timeout
									expired before data was received.  
	[--] EBADF  					The argument sockfd is an invalid file descriptor.
	[  ] ECONNREFUSED
									A remote host refused to allow the network connection
									(typically because it is not running the requested service).
	[  ] EFAULT 					The receive buffer pointer(s) point outside the process's
									address space.
	[  ] EINTR  					The receive was interrupted by delivery of a signal before any
									data were available; see signal(7).
	[--] EINVAL 					Invalid argument passed.
	[  ] ENOMEM 					Could not allocate memory for recvmsg().
	[  ] ENOTCONN 					The socket is associated with a connection-oriented protocol and has not been connected (see connect(2) and accept(2)).
	[  ] ENOTSOCK 					The file descriptor sockfd does not refer to a socket.

	ZT_RECV_SIG int fd, void *buf, size_t len, int flags
*/

ssize_t zts_recv(ZT_RECV_SIG)
{
	DEBUG_TRANS("fd=%d", fd);
	int err = errno = 0;
	ZeroTier::VirtualSocket *vs = get_virtual_socket(fd);
	if(!vs) {
		DEBUG_ERROR("invalid vs for fd=%d", fd);
		errno = EBADF;
		return -1;
	}
	if(vs->socket_type != SOCK_STREAM) {
		DEBUG_ERROR("the socket is not connection-mode, and no peer address is set for fd=%d", fd);
		errno = EDESTADDRREQ;
		return -1;
	}
	if(vs->state != ZT_SOCK_STATE_CONNECTED) {
		DEBUG_ERROR("the socket is not in a connected state, fd=%d", fd);
		errno = ENOTCONN;
		return -1;
	}
	if(flags & MSG_DONTWAIT) {
		// The stack drivers and stack are inherently non-blocking by design, but we 
		// still need to modify the unix pipe connecting them to the application:
		fcntl(fd, F_SETFL, O_NONBLOCK);
	}
	if(flags & MSG_OOB) {
		DEBUG_INFO("MSG_OOB not implemented yet");
		errno = EINVAL;
		return -1;
	}
	if(flags & MSG_TRUNC) {
		DEBUG_INFO("MSG_TRUNC not implemented yet");
		errno = EINVAL;
		return -1;
	}
	if(flags & MSG_WAITALL) {
		DEBUG_INFO("MSG_WAITALL not implemented yet");
		errno = EINVAL;
		return -1;
	}
#if defined(__linux__)
	if(flags & MSG_ERRQUEUE) {
		DEBUG_INFO("MSG_ERRQUEUE not implemented yet");
		errno = EINVAL;
		return -1;
	}
#endif

	if(flags & MSG_PEEK) {
		// MSG_PEEK doesn't require any special stack-related machinery so we can just
		// pass it to a regular recv() call with no issue
		err = recv(fd, buf, len, MSG_PEEK);
	}

	// restore "per-call" flags

	if(flags & MSG_DONTWAIT) {
		int saved_flags = fcntl(fd, F_GETFL);
		if(fcntl(fd, F_SETFL, saved_flags & ~O_NONBLOCK) < 0) { // mask out the blocking flag
			handle_general_failure();
			return -1;
		}
	}
	return err;
}

/*
	Linux:

	[  ] [EAGAIN or EWOULDBLOCK] 	The socket is marked nonblocking and the receive operation 
									would block, or a receive timeout had been set and the 
									timeout expired before data was received. POSIX.1-2001 
									allows either error to be returned for this case, and does 
									not require these constants to have the same value, so a 
									portable application should check for both possibilities.
	[  ] [EBADF]					The argument sockfd is an invalid descriptor.
	[  ] [ECONNREFUSED] 			A remote host refused to allow the network connection 
									(typically because it is not running the requested service).
	[  ] [EFAULT] 					The receive buffer pointer(s) point outside the process's 
									address space.
	[  ] [EINTR] 					The receive was interrupted by delivery of a signal before any 
									data were available; see signal(7).
	[  ] [EINVAL] 					Invalid argument passed.
	[  ] [ENOMEM] 					Could not allocate memory for recvmsg().
	[  ] [ENOTCONN] 				The socket is associated with a connection-oriented protocol 
									and has not been connected (see connect(2) and accept(2)).
	[  ] [ENOTSOCK] 				The argument sockfd does not refer to a socket.

	ZT_RECVFROM_SIG int fd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen
*/

ssize_t zts_recvfrom(ZT_RECVFROM_SIG)
{
	//DEBUG_TRANS("fd=%d", fd);
	int32_t r = 0;
	errno = 0;
	if(fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	if(len == 0) {
		return 0;
	}
	if(!buf) {
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
	if(r != sizeof(int32_t)){
		//DEBUG_ERROR("invalid datagram, PEEK, r=%d", r);
		errno = EIO; // TODO: test for this
		return -1;
	}
	// read of sizeof(int32_t) for the length of the datagram (including address)
	r = read(fd, msg_ptr, sizeof(int32_t));
	// copy to length variable
	memcpy(&udp_msg_len, msg_ptr, sizeof(int32_t));
	msg_ptr+=sizeof(int32_t);

	if(udp_msg_len <= 0) {
		DEBUG_ERROR("invalid datagram");
		errno = EIO; // TODO: test for this
		return -1;
	}
	// there is a datagram to read, so let's read it
	// zero remainder of buffer
	memset(msg_ptr, 0, ZT_SOCKET_MSG_BUF_SZ- sizeof(int32_t));
	if((r = read(fd, msg_ptr, udp_msg_len)) < 0) {
		DEBUG_ERROR("invalid datagram");
		errno = EIO; // TODO: test for this
		return -1;
	}
	// get address
	if(addr) {
		if(*addrlen < sizeof(struct sockaddr_storage)) {
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
ssize_t zts_recvmsg(ZT_RECVMSG_SIG)
{
	//DEBUG_TRANS("fd=%d", fd);
	int err = errno = 0;
	if(fd < 0 || fd >= ZT_MAX_SOCKETS) {
		errno = EBADF;
		return -1;
	}
	else {
		err = recvmsg(fd, msg, flags);
	}
	return err;
}

int zts_read(ZT_READ_SIG) {
	//DEBUG_TRANS("fd=%d", fd);
	return read(fd, buf, len);
}

int zts_write(ZT_WRITE_SIG) {
	//DEBUG_TRANS("fd=%d", fd);
	return write(fd, buf, len);
}

int zts_shutdown(ZT_SHUTDOWN_SIG)
{
	/*
	int err = errno = 0;
#if defined(STACK_PICO)
	DEBUG_INFO("fd = %d", fd);
 
	int mode = 0;
	if(how == SHUT_RD) mode = PICO_SHUT_RD;
	if(how == SHUT_WR) mode = PICO_SHUT_WR;
	if(how == SHUT_RDWR) mode = PICO_SHUT_RDWR;

	if(fd < 0) {
		errno = EBADF;
		err = -1;
	}
	else
	{
		if(!ZeroTier::zt1Service) {
			DEBUG_ERROR("cannot shutdown socket. service not started. call zts_start(path) first");
			errno = EBADF;
			err = -1;
		}
		else
		{
			ZeroTier::_multiplexer_lock.lock();
			// First, look for for unassigned VirtualSockets
			ZeroTier::VirtualSocket *vs = ZeroTier::unmap[fd];
			// Since we found an unassigned VirtualSocket, we don't need to consult the stack or tap
			// during closure - it isn't yet stitched into the clockwork
			if(vs) // unassigned
			{
				DEBUG_ERROR("unassigned shutdown");
				
				//   PICO_SHUT_RD
				//   PICO_SHUT_WR
				//   PICO_SHUT_RDWR
			
				if((err = pico_socket_shutdown(vs->picosock, mode)) < 0)
					DEBUG_ERROR("error calling pico_socket_shutdown()");
				DEBUG_ERROR("vs=%p", vs);
				delete vs;
				vs = NULL;
				ZeroTier::unmap.erase(fd);
				// FIXME: Is deleting this correct behaviour?
			}
			else // assigned
			{
				std::pair<ZeroTier::VirtualSocket*, ZeroTier::VirtualTap*> *p = ZeroTier::fdmap[fd];
				if(!p) 
				{
					DEBUG_ERROR("unable to locate VirtualSocket pair.");
					errno = EBADF;
					err = -1;
				}
				else // found everything, begin closure
				{
					vs = p->first;
					int f_err, blocking = 1;
					if ((f_err = fcntl(fd, F_GETFL, 0)) < 0) {
						DEBUG_ERROR("fcntl error, err = %s, errno = %d", f_err, errno);
						err = -1;
					} 
					else {
						blocking = !(f_err & O_NONBLOCK);
					}
					if(blocking) {
						DEBUG_INFO("blocking, waiting for write operations before shutdown...");
						for(int i=0; i<ZT_SDK_CLTIME; i++) {
							if(vs->TXbuf->count() == 0)
								break;
							nanosleep((const struct timespec[]){{0, (ZT_API_CHECK_INTERVAL * 1000000)}}, NULL);

						}
					}

					if((err = pico_socket_shutdown(vs->picosock, mode)) < 0)
						DEBUG_ERROR("error calling pico_socket_shutdown()");
				}
			}
			ZeroTier::_multiplexer_lock.unlock();
		}
	}
	return err;
#endif
*/
	return 0;
}

int zts_add_dns_nameserver(struct sockaddr *addr)
{
	ZeroTier::VirtualTap *vtap = getAnyTap();
	if(vtap){
		return vtap->add_DNS_Nameserver(addr);
	}
	DEBUG_ERROR("unable to locate virtual tap to add DNS nameserver");
	return -1;
}

int zts_del_dns_nameserver(struct sockaddr *addr)
{
	ZeroTier::VirtualTap *vtap = getAnyTap();
	if(vtap){
		return vtap->del_DNS_Nameserver(addr);
	}
	DEBUG_ERROR("unable to locate virtual tap to remove DNS nameserver");
	return -1;
}

/****************************************************************************/
/* SDK Socket API (Java Native Interface JNI)                               */
/* JNI naming convention: Java_PACKAGENAME_CLASSNAME_METHODNAME             */
/****************************************************************************/


#if defined(SDK_JNI)

namespace ZeroTier {

	#include <jni.h>

	JNIEXPORT void JNICALL Java_zerotier_ZeroTier_ztjni_1start(JNIEnv *env, jobject thisObj, jstring path) {
		if(path) {
			homeDir = env->GetStringUTFChars(path, NULL);
			zts_start(homeDir.c_str());
		}
	}
	// Shuts down ZeroTier service and SOCKS5 Proxy server
	JNIEXPORT void JNICALL Java_zerotier_ZeroTier_ztjni_1stop(JNIEnv *env, jobject thisObj) {
		if(ZeroTier::zt1Service)
			zts_stop();
	}

	// Returns whether the ZeroTier service is running
	JNIEXPORT jboolean JNICALL Java_zerotier_ZeroTier_ztjni_1running(
		JNIEnv *env, jobject thisObj) 
	{
		return  zts_running();
	}
	// Returns path for ZT config/data files    
	JNIEXPORT jstring JNICALL Java_zerotier_ZeroTier_ztjni_1homepath(
		JNIEnv *env, jobject thisObj) 
	{
		// TODO: fix, should copy into given arg
		// return (*env).NewStringUTF(zts_get_homepath());
		return (*env).NewStringUTF("");
	}
	// Join a network
	JNIEXPORT void JNICALL Java_zerotier_ZeroTier_ztjni_1join(
		JNIEnv *env, jobject thisObj, jstring nwid) 
	{
		const char *nwidstr;
		if(nwid) {
			nwidstr = env->GetStringUTFChars(nwid, NULL);
			zts_join(nwidstr);
		}
	}
	// Leave a network
	JNIEXPORT void JNICALL Java_zerotier_ZeroTier_ztjni_1leave(
		JNIEnv *env, jobject thisObj, jstring nwid) 
	{
		const char *nwidstr;
		if(nwid) {
			nwidstr = env->GetStringUTFChars(nwid, NULL);
			zts_leave(nwidstr);
		}
	}
	// FIXME: Re-implemented to make it play nicer with the C-linkage required for Xcode integrations
	// Now only returns first assigned address per network. Shouldn't normally be a problem
	JNIEXPORT jobject JNICALL Java_zerotier_ZeroTier_ztjni_1get_1ipv4_1address(
		JNIEnv *env, jobject thisObj, jstring nwid) 
	{
		const char *nwid_str = env->GetStringUTFChars(nwid, NULL);
		char address_string[INET_ADDRSTRLEN];
		memset(address_string, 0, INET_ADDRSTRLEN);
		zts_get_ipv4_address(nwid_str, address_string, INET_ADDRSTRLEN);
		jclass clazz = (*env).FindClass("java/util/ArrayList");
		jobject addresses = (*env).NewObject(clazz, (*env).GetMethodID(clazz, "<init>", "()V"));        
		jstring _str = (*env).NewStringUTF(address_string);
		env->CallBooleanMethod(addresses, env->GetMethodID(clazz, "add", "(Ljava/lang/Object;)Z"), _str);
		return addresses;
	}

	JNIEXPORT jobject JNICALL Java_zerotier_ZeroTier_ztjni_1get_1ipv6_1address(
		JNIEnv *env, jobject thisObj, jstring nwid) 
	{
		const char *nwid_str = env->GetStringUTFChars(nwid, NULL);
		char address_string[INET6_ADDRSTRLEN];
		memset(address_string, 0, INET6_ADDRSTRLEN);
		zts_get_ipv6_address(nwid_str, address_string, INET6_ADDRSTRLEN);
		jclass clazz = (*env).FindClass("java/util/ArrayList");
		jobject addresses = (*env).NewObject(clazz, (*env).GetMethodID(clazz, "<init>", "()V"));        
		jstring _str = (*env).NewStringUTF(address_string);
		env->CallBooleanMethod(addresses, env->GetMethodID(clazz, "add", "(Ljava/lang/Object;)Z"), _str);
		return addresses;
	}

	// Returns the device is in integer form
	JNIEXPORT jint Java_zerotier_ZeroTier_ztjni_1get_1device_1id() 
	{
		return zts_get_device_id(NULL); // TODO
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1send(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len, int flags)
	{
		jbyte *body = (*env).GetByteArrayElements((_jbyteArray *)buf, 0);
		char * bufp = (char *)malloc(sizeof(char)*len);
		memcpy(bufp, body, len);
		(*env).ReleaseByteArrayElements((_jbyteArray *)buf, body, 0);
		int written_bytes = zts_write(fd, body, len);
		return written_bytes;
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1sendto(
		JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len, jint flags, jobject ztaddr)
	{
		struct sockaddr_in addr;
		jclass cls = (*env).GetObjectClass( ztaddr);
		jfieldID f = (*env).GetFieldID( cls, "port", "I");
		addr.sin_port = htons((*env).GetIntField( ztaddr, f));
		f = (*env).GetFieldID( cls, "_rawAddr", "J");
		addr.sin_addr.s_addr = (*env).GetLongField( ztaddr, f);
		addr.sin_family = AF_INET;
		//LOGV("zt_sendto(): fd = %d\naddr = %s\nport=%d", fd, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
		// TODO: Optimize this
		jbyte *body = (*env).GetByteArrayElements((_jbyteArray *)buf, 0);
		char * bufp = (char *)malloc(sizeof(char)*len);
		memcpy(bufp, body, len);
		(*env).ReleaseByteArrayElements((_jbyteArray *)buf, body, 0);
		// "connect" and send buffer contents
		int sent_bytes = zts_sendto(fd, body, len, flags, (struct sockaddr *)&addr, sizeof(addr));
		return sent_bytes;
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1recvfrom(
		JNIEnv *env, jobject thisObj, jint fd, jbyteArray buf, jint len, jint flags, jobject ztaddr)
	{
		struct sockaddr_in addr;
		jbyte *body = (*env).GetByteArrayElements( buf, 0);
		unsigned char buffer[ZT_SDK_MTU];
		int payload_offset = sizeof(int32_t) + sizeof(struct sockaddr_storage);
		int rxbytes = zts_recvfrom(fd, &buffer, len, flags, (struct sockaddr *)&addr, (socklen_t *)sizeof(struct sockaddr_storage));
		if(rxbytes > 0)
			memcpy(body, (jbyte*)buffer + payload_offset, rxbytes);
		(*env).ReleaseByteArrayElements( buf, body, 0);
		// Update fields of Java ZTAddress object
		jfieldID fid;
		jclass cls = (*env).GetObjectClass( ztaddr);
		fid = (*env).GetFieldID( cls, "port", "I");
		(*env).SetIntField( ztaddr, fid, addr.sin_port);
		fid = (*env).GetFieldID( cls,"_rawAddr", "J");
		(*env).SetLongField( ztaddr, fid,addr.sin_addr.s_addr);        
		return rxbytes;
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1write(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len)
	{
		jbyte *body = (*env).GetByteArrayElements((_jbyteArray *)buf, 0);
		char * bufp = (char *)malloc(sizeof(char)*len);
		memcpy(bufp, body, len);
		(*env).ReleaseByteArrayElements((_jbyteArray *)buf, body, 0);
		int written_bytes = zts_write(fd, body, len);
		return written_bytes;
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1read(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len)
	{
		jbyte *body = (*env).GetByteArrayElements((_jbyteArray *)buf, 0);
		int read_bytes = read(fd, body, len);
		(*env).ReleaseByteArrayElements((_jbyteArray *)buf, body, 0);
		return read_bytes;
	}    

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1setsockopt(
		JNIEnv *env, jobject thisObj, jint fd, jint level, jint optname, jint optval, jint optlen) {
		return zts_setsockopt(fd, level, optname, (const void*)optval, optlen);
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1getsockopt(JNIEnv *env, jobject thisObj, jint fd, jint level, jint optname, jint optval, jint optlen) {
		return zts_getsockopt(fd, level, optname, (void*)optval, (socklen_t *)optlen);
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1socket(JNIEnv *env, jobject thisObj, jint family, jint type, jint protocol) {
		return zts_socket(family, type, protocol);
	}
	
	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1connect(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port) {
		struct sockaddr_in addr;
		const char *str = (*env).GetStringUTFChars( addrstr, 0);
		addr.sin_addr.s_addr = inet_addr(str);
		addr.sin_family = AF_INET;
		addr.sin_port = htons( port );
		(*env).ReleaseStringUTFChars( addrstr, str);
		return zts_connect(fd, (struct sockaddr *)&addr, sizeof(addr));
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1bind(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port) {
		struct sockaddr_in addr;
		const char *str = (*env).GetStringUTFChars( addrstr, 0);
		DEBUG_INFO("fd=%d, addr=%s, port=%d", fd, str, port);
		addr.sin_addr.s_addr = inet_addr(str);
		addr.sin_family = AF_INET;
		addr.sin_port = htons( port );
		(*env).ReleaseStringUTFChars( addrstr, str);
		return zts_bind(fd, (struct sockaddr *)&addr, sizeof(addr));
	}

#if defined(__linux__)
	 JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1accept4(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port, jint flags) {
		struct sockaddr_in addr;
		char *str;
		// = env->GetStringUTFChars(addrstr, NULL);
		(*env).ReleaseStringUTFChars( addrstr, str);
		addr.sin_addr.s_addr = inet_addr(str);
		addr.sin_family = AF_INET;
		addr.sin_port = htons( port );
		return zts_accept4(fd, (struct sockaddr *)&addr, sizeof(addr), flags);
	}
#endif

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1accept(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port) {
		struct sockaddr_in addr;
		// TODO: Send addr info back to Javaland
		addr.sin_addr.s_addr = inet_addr("");
		addr.sin_family = AF_INET;
		addr.sin_port = htons( port );
		return zts_accept(fd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr));    
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1listen(JNIEnv *env, jobject thisObj, jint fd, int backlog) {
		return zts_listen(fd, backlog);
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1close(JNIEnv *env, jobject thisObj, jint fd) {
		return zts_close(fd);
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1getsockname(JNIEnv *env, jobject thisObj, jint fd, jobject ztaddr) {
		struct sockaddr_in addr;
		int err = zts_getsockname(fd, (struct sockaddr *)&addr, (socklen_t *)sizeof(struct sockaddr));
		jfieldID fid;
		jclass cls = (*env).GetObjectClass(ztaddr);
		fid = (*env).GetFieldID( cls, "port", "I");
		(*env).SetIntField( ztaddr, fid, addr.sin_port);
		fid = (*env).GetFieldID( cls,"_rawAddr", "J");
		(*env).SetLongField( ztaddr, fid,addr.sin_addr.s_addr);        
		return err;    
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1getpeername(JNIEnv *env, jobject thisObj, jint fd, jobject ztaddr) {
		struct sockaddr_in addr;
		int err = zts_getpeername(fd, (struct sockaddr *)&addr, (socklen_t *)sizeof(struct sockaddr));
		jfieldID fid;
		jclass cls = (*env).GetObjectClass( ztaddr);
		fid = (*env).GetFieldID( cls, "port", "I");
		(*env).SetIntField( ztaddr, fid, addr.sin_port);
		fid = (*env).GetFieldID( cls,"_rawAddr", "J");
		(*env).SetLongField( ztaddr, fid,addr.sin_addr.s_addr);        
		return err;
	}

	JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1fcntl(JNIEnv *env, jobject thisObj, jint fd, jint cmd, jint flags) {
		return zts_fcntl(fd,cmd,flags);
	}
}
#endif

/****************************************************************************/
/* SDK Socket API Helper functions --- DON'T CALL THESE DIRECTLY            */
/****************************************************************************/

#if defined(STACK_PICO)
int zts_get_pico_socket(int fd, struct pico_socket **s)
{
	int err = 0;
	if(!ZeroTier::zt1Service) {
		DEBUG_ERROR("cannot locate socket. service not started. call zts_start(path) first");
		errno = EBADF;
		err = -1;
	}
	else {
		ZeroTier::_multiplexer_lock.lock();
		// First, look for for unassigned VirtualSockets
		ZeroTier::VirtualSocket *vs = ZeroTier::unmap[fd];
		if(vs)
		{
			*s = vs->picosock;
			err = 1; // unassigned
		}
		else
		{
			std::pair<ZeroTier::VirtualSocket*, ZeroTier::VirtualTap*> *p = ZeroTier::fdmap[fd];
			if(!p) 
			{
				DEBUG_ERROR("unable to locate VirtualSocket pair.");
				errno = EBADF;
				err = -1;
			}
			else
			{
				*s = p->first->picosock;
				err = 0; // assigned
			}
		}
		ZeroTier::_multiplexer_lock.unlock();
	}
	return err;
}
#endif

bool can_provision_new_socket()
{
#if defined(STACK_PICO)
	if(pico_ntimers()+1 >= PICO_MAX_TIMERS) {
		return false;
	}
	return true;
#endif
#if defined(STACK_LWIP)
	// TODO: Add check here (see lwipopts.h)
	return true;
#endif
#if defined(NO_STACK)
	return true; // always true since there's no network stack timer limitation
#endif
}

int zts_nsockets()
{
	ZeroTier::_multiplexer_lock.unlock();
	int num = ZeroTier::unmap.size() + ZeroTier::fdmap.size();
	ZeroTier::_multiplexer_lock.unlock(); 
	return num;
}

int zts_maxsockets()
{
#if defined(STACK_PICO)
	// TODO: This is only an approximation
	return PICO_MAX_TIMERS - 10;
#endif
#if defined(STACK_LWIP)
	return 32;
#endif
#if defined(NO_STACK)
	return 1024; // arbitrary
#endif
}

/****************************************************************************/
/* ZeroTier Core helper functions for libzt - DON'T CALL THESE DIRECTLY     */
/****************************************************************************/

std::vector<ZT_VirtualNetworkRoute> *zts_get_network_routes(char *nwid)
{
	uint64_t nwid_int = strtoull(nwid, NULL, 16);
	return ZeroTier::zt1Service->getRoutes(nwid_int);
}

ZeroTier::VirtualTap *getTapByNWID(uint64_t nwid)
{
	ZeroTier::_vtaps_lock.lock();
	ZeroTier::VirtualTap *s, *tap = nullptr;    
	for(int i=0; i<ZeroTier::vtaps.size(); i++) {
		s = (ZeroTier::VirtualTap*)ZeroTier::vtaps[i];
		if(s->_nwid == nwid) { tap = s; }
	}
	ZeroTier::_vtaps_lock.unlock();
	return tap;
}

ZeroTier::VirtualTap *getTapByAddr(ZeroTier::InetAddress *addr)
{
	ZeroTier::_vtaps_lock.lock();
	ZeroTier::VirtualTap *s, *tap = nullptr; 
	//char ipbuf[64], ipbuf2[64], ipbuf3[64];
	for(int i=0; i<ZeroTier::vtaps.size(); i++) {
		s = (ZeroTier::VirtualTap*)ZeroTier::vtaps[i];
		// check address schemes
		for(int j=0; j<s->_ips.size(); j++) {
			if((s->_ips[j].isV4() && addr->isV4()) || (s->_ips[j].isV6() && addr->isV6())) {
				//DEBUG_INFO("looking at tap %s, <addr=%s> --- for <%s>", s->_dev.c_str(), s->_ips[j].toString(ipbuf), addr->toIpString(ipbuf2));
				if(s->_ips[j].isEqualPrefix(addr) 
					|| s->_ips[j].ipsEqual(addr) 
					|| s->_ips[j].containsAddress(addr)
					|| (addr->isV6() && ipv6_in_subnet(&s->_ips[j], addr)) 
					) 
				{
					//DEBUG_INFO("selected tap %s, <addr=%s>", s->_dev.c_str(), s->_ips[j].toString(ipbuf));
					ZeroTier::_vtaps_lock.unlock();
					return s;
				}
			}
		}
		// check managed routes
		if(!tap) {
			std::vector<ZT_VirtualNetworkRoute> *managed_routes = ZeroTier::zt1Service->getRoutes(s->_nwid);
			ZeroTier::InetAddress target, nm, via;
			for(int i=0; i<managed_routes->size(); i++){
				target = managed_routes->at(i).target;
				nm = target.netmask();
				via = managed_routes->at(i).via;
				if(target.containsAddress(addr)) {
					// DEBUG_INFO("chose tap with route <target=%s, nm=%s, via=%s>", target.toString(ipbuf), nm.toString(ipbuf2), via.toString(ipbuf3));
					ZeroTier::_vtaps_lock.unlock();
					return s;
				}
			}
		}
	}
	ZeroTier::_vtaps_lock.unlock();
	return tap;
}

ZeroTier::VirtualTap *getTapByName(char *ifname)
{
	ZeroTier::_vtaps_lock.lock();
	ZeroTier::VirtualTap *s, *tap = nullptr;  
	for(int i=0; i<ZeroTier::vtaps.size(); i++) {
		s = (ZeroTier::VirtualTap*)ZeroTier::vtaps[i];
		if(!strcmp(s->_dev.c_str(), ifname)) {
			tap = s;
		}
	}
	ZeroTier::_vtaps_lock.unlock();
	return tap;
}

ZeroTier::VirtualTap *getTapByIndex(int index)
{
	ZeroTier::_vtaps_lock.lock();
	ZeroTier::VirtualTap *s, *tap = nullptr;    
	for(int i=0; i<ZeroTier::vtaps.size(); i++) {
		s = (ZeroTier::VirtualTap*)ZeroTier::vtaps[i];
		if(s->ifindex == index) {
			tap = s;
		}
	}
	ZeroTier::_vtaps_lock.unlock();
	return tap;
}

ZeroTier::VirtualTap *getAnyTap()
{
	ZeroTier::_vtaps_lock.lock();
	ZeroTier::VirtualTap *vtap = NULL;
	if(ZeroTier::vtaps.size()) {
		vtap = (ZeroTier::VirtualTap *)ZeroTier::vtaps[0];
	}
	ZeroTier::_vtaps_lock.unlock();
	return vtap;
}

/****************************************************************************/
/* VirtualSocket / VirtualTap helper functions - DON'T CALL THESE DIRECTLY  */
/****************************************************************************/

ZeroTier::VirtualSocket *get_virtual_socket(int fd)
{
	ZeroTier::_multiplexer_lock.lock();
	// try to locate in unmapped set
	ZeroTier::VirtualSocket *vs = ZeroTier::unmap[fd];
	if(!vs) {
		// if not, try to find in mapped set (bind to vtap has been performed)
		std::pair<ZeroTier::VirtualSocket*, ZeroTier::VirtualTap*> *p = ZeroTier::fdmap[fd];
		if(p) {
			vs = p->first;
		}
	}
	ZeroTier::_multiplexer_lock.unlock();
	return vs;
}

void del_virtual_socket(int fd)
{
	ZeroTier::_multiplexer_lock.lock();
	ZeroTier::unmap.erase(fd);
	ZeroTier::fdmap.erase(fd);
	ZeroTier::_multiplexer_lock.unlock();
}

void add_unassigned_virtual_socket(int fd, ZeroTier::VirtualSocket *vs)
{
	ZeroTier::_multiplexer_lock.lock();
	ZeroTier::unmap[fd] = vs;
	ZeroTier::_multiplexer_lock.unlock();
}

void del_unassigned_virtual_socket(int fd)
{
	ZeroTier::_multiplexer_lock.lock();
	ZeroTier::unmap.erase(fd);
	ZeroTier::_multiplexer_lock.unlock();
}

void add_assigned_virtual_socket(ZeroTier::VirtualTap *tap, ZeroTier::VirtualSocket *vs, int fd)
{
	ZeroTier::_multiplexer_lock.lock();
	ZeroTier::fdmap[fd] = new std::pair<ZeroTier::VirtualSocket*,ZeroTier::VirtualTap*>(vs, tap);
	ZeroTier::_multiplexer_lock.unlock();
}

void del_assigned_virtual_socket(ZeroTier::VirtualTap *tap, ZeroTier::VirtualSocket *vs, int fd)
{
	ZeroTier::_multiplexer_lock.lock();
	ZeroTier::fdmap.erase(fd);
	ZeroTier::_multiplexer_lock.unlock();
}

void dismantleTaps()
{
	ZeroTier::_vtaps_lock.lock();
	for(int i=0; i<ZeroTier::vtaps.size(); i++) { 
		DEBUG_ERROR("ZeroTier::vtapsf[i]=%p", ZeroTier::vtaps[i]);
		delete (ZeroTier::VirtualTap*)ZeroTier::vtaps[i]; 
		ZeroTier::vtaps[i] = NULL;
	}
	ZeroTier::vtaps.clear();
	ZeroTier::_vtaps_lock.unlock();
}

int zts_get_device_id_from_file(const char *filepath, char *devID) {
	std::string fname("identity.public");
	std::string fpath(filepath);

	if(ZeroTier::OSUtils::fileExists((fpath + ZT_PATH_SEPARATOR_S + fname).c_str(),false)) {
		std::string oldid;
		ZeroTier::OSUtils::readFile((fpath + ZT_PATH_SEPARATOR_S + fname).c_str(),oldid);
		memcpy(devID, oldid.c_str(), 10); // first 10 bytes of file
		return 0;
	}
	return -1;
}

// Starts a ZeroTier service in the background
void *zts_start_service(void *thread_id) {

	DEBUG_INFO("homeDir=%s", ZeroTier::homeDir.c_str());
	// Where network .conf files will be stored
	ZeroTier::netDir = ZeroTier::homeDir + "/networks.d";
	ZeroTier::zt1Service = (ZeroTier::OneService *)0;
	
	// Construct path for network config and supporting service files
	if (ZeroTier::homeDir.length()) {
		std::vector<std::string> hpsp(ZeroTier::OSUtils::split(ZeroTier::homeDir.c_str(),
			ZT_PATH_SEPARATOR_S,"",""));
		std::string ptmp;
		if (ZeroTier::homeDir[0] == ZT_PATH_SEPARATOR)
			ptmp.push_back(ZT_PATH_SEPARATOR);
		for(std::vector<std::string>::iterator pi(hpsp.begin());pi!=hpsp.end();++pi) {
			if (ptmp.length() > 0)
				ptmp.push_back(ZT_PATH_SEPARATOR);
			ptmp.append(*pi);
			if ((*pi != ".")&&(*pi != "..")) {
				if (!ZeroTier::OSUtils::mkdir(ptmp)) {
					DEBUG_ERROR("home path does not exist, and could not create");
					handle_general_failure();
					perror("error\n");
				}
			}
		}
	}
	else {
		DEBUG_ERROR("homeDir is empty, could not construct path");
		handle_general_failure();
		return NULL;
	}

	// Generate random port for new service instance
	unsigned int randp = 0;
	ZeroTier::Utils::getSecureRandom(&randp,sizeof(randp));
	// TODO: Better port random range selection
	int servicePort = 9000 + (randp % 1000);
	for(;;) {
		ZeroTier::zt1Service = ZeroTier::OneService::newInstance(ZeroTier::homeDir.c_str(),servicePort);
		switch(ZeroTier::zt1Service->run()) {
			case ZeroTier::OneService::ONE_STILL_RUNNING: 
			case ZeroTier::OneService::ONE_NORMAL_TERMINATION:
				break;
			case ZeroTier::OneService::ONE_UNRECOVERABLE_ERROR:
				DEBUG_ERROR("ZTO service port = %d", servicePort);
				DEBUG_ERROR("fatal error: %s",ZeroTier::zt1Service->fatalErrorMessage().c_str());
				break;
			case ZeroTier::OneService::ONE_IDENTITY_COLLISION: {
				delete ZeroTier::zt1Service;
				ZeroTier::zt1Service = (ZeroTier::OneService *)0;
				std::string oldid;
				ZeroTier::OSUtils::readFile((ZeroTier::homeDir + ZT_PATH_SEPARATOR_S 
					+ "identity.secret").c_str(),oldid);
				if (oldid.length()) {
					ZeroTier::OSUtils::writeFile((ZeroTier::homeDir + ZT_PATH_SEPARATOR_S 
						+ "identity.secret.saved_after_collision").c_str(),oldid);
					ZeroTier::OSUtils::rm((ZeroTier::homeDir + ZT_PATH_SEPARATOR_S 
						+ "identity.secret").c_str());
					ZeroTier::OSUtils::rm((ZeroTier::homeDir + ZT_PATH_SEPARATOR_S 
						+ "identity.public").c_str());
				}
			}   
			continue; // restart!
		}
		break; // terminate loop -- normally we don't keep restarting
	}
	delete ZeroTier::zt1Service;
	ZeroTier::zt1Service = (ZeroTier::OneService *)0;
	return NULL;
}

void handle_general_failure() {
#ifdef ZT_EXIT_ON_GENERAL_FAIL
	DEBUG_ERROR("exiting (ZT_EXIT_ON_GENERAL_FAIL==1)");
	//exit(-1);
#endif
}

#ifdef __cplusplus
}
#endif
