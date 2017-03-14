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

#include <algorithm>
#include <utility>
#include <dlfcn.h>
#include <sys/poll.h>
#include <stdint.h>
#include <utility>
#include <string>
#include <sys/resource.h>
#include <sys/syscall.h>

#include "tap.hpp"
#include "sdkutils.hpp"
#include "sdk.h"
#include "defs.h"
#include "debug.h"
#include "rpc.h"

#if defined(SDK_LWIP) 
	#include "lwip.hpp"
#elif defined(SDK_PICOTCP)
 	#include "picotcp.hpp"
#elif defined(SDK_JIP)
	#include "jip.hpp"
#endif

#include "Utils.hpp"
#include "OSUtils.hpp"
#include "Constants.hpp"
#include "Phy.hpp"

//#if !defined(__IOS__) && !defined(__ANDROID__) && !defined(__UNITY_3D__) && !defined(__XCODE__)
//    const ip_addr_t ip_addr_any = { IPADDR_ANY };
//#endif

namespace ZeroTier {
int NetconEthernetTap::sendReturnValue(int fd, int retval, int _errno)
{
	//#if !defined(USE_SOCKS_PROXY)
	//DEBUG_EXTRA("fd=%d, retval=%d, errno=%d", fd, retval, _errno);
	int sz = sizeof(char) + sizeof(retval) + sizeof(errno);
	char retmsg[sz];
	memset(&retmsg, 0, sizeof(retmsg));
	retmsg[0]=RPC_RETVAL;
	memcpy(&retmsg[1], &retval, sizeof(retval));
	memcpy(&retmsg[1]+sizeof(retval), &_errno, sizeof(_errno));
	return write(fd, &retmsg, sz);
	//#else
	//    return 1;
	//#endif
}
// Unpacks the buffer from an RPC command
void NetconEthernetTap::unloadRPC(void *data, pid_t &pid, pid_t &tid, 
	char (timestamp[RPC_TIMESTAMP_SZ]), char (CANARY[sizeof(uint64_t)]), char &cmd, void* &payload)
	{
	unsigned char *buf = (unsigned char*)data;
	memcpy(&pid, &buf[IDX_PID], sizeof(pid_t));
	memcpy(&tid, &buf[IDX_TID], sizeof(pid_t));
	memcpy(timestamp, &buf[IDX_TIME], RPC_TIMESTAMP_SZ);
	memcpy(&cmd, &buf[IDX_PAYLOAD], sizeof(char));
	memcpy(CANARY, &buf[IDX_PAYLOAD+1], CANARY_SZ);
}

/*------------------------------------------------------------------------------
-------------------------------- Tap Service  ----------------------------------
------------------------------------------------------------------------------*/

NetconEthernetTap::NetconEthernetTap(
	const char *homePath,
	const MAC &mac,
	unsigned int mtu,
	unsigned int metric,
	uint64_t nwid,
	const char *friendlyName,
	void (*handler)(void *,uint64_t,const MAC &,const MAC &,unsigned int,unsigned int,const void *,unsigned int),
	void *arg) :
		_homePath(homePath),
		_mac(mac),
		_mtu(mtu),
		_nwid(nwid),
		_handler(handler),
		_arg(arg),
		_phy(this,false,true),
		_unixListenSocket((PhySocket *)0),
		_enabled(true),
		_run(true)
{
	sockstate = -1;
    char sockPath[4096],stackPath[4096];
    Utils::snprintf(sockPath,sizeof(sockPath),"%s%snc_%.16llx",homePath,ZT_PATH_SEPARATOR_S,_nwid,ZT_PATH_SEPARATOR_S,(unsigned long long)nwid);
    _dev = sockPath; // in SDK mode, set device to be just the network ID

	// Load and initialize network stack library
    #if defined(SDK_LWIP)
		Utils::snprintf(stackPath,sizeof(stackPath),"%s%sliblwip.so",homePath,ZT_PATH_SEPARATOR_S);
		lwipstack = new lwIP_stack(stackPath);
		if(!lwipstack) {
			DEBUG_ERROR("unable to dynamically load a new instance of (%s) (searched ZeroTier home path)", stackPath);
			throw std::runtime_error("");
		}
		lwipstack->__lwip_init();
		DEBUG_EXTRA("network stack initialized (%p)", lwipstack);
	#elif defined(SDK_PICOTCP)            
		pico_frame_rxbuf_tot = 0;
		Utils::snprintf(stackPath,sizeof(stackPath),"%s%slibpicotcp.so",homePath,ZT_PATH_SEPARATOR_S);
		picostack = new picoTCP_stack(stackPath);
		if(!picostack) {
			DEBUG_ERROR("unable to dynamically load a new instance of (%s) (searched ZeroTier home path)", stackPath);
			throw std::runtime_error("");
		}
		picostack->__pico_stack_init();
		DEBUG_EXTRA("network stack initialized (%p)", picostack);
	#elif defined(SDK_JIP)
		Utils::snprintf(stackPath,sizeof(stackPath),"%s%slibjip.so",homePath,ZT_PATH_SEPARATOR_S);
		jipstack = new jip_stack(stackPath);
	#endif
	_unixListenSocket = _phy.unixListen(sockPath,(void *)this);

	chmod(sockPath, 0777); // To make the RPC socket available to all users

	if (!_unixListenSocket)
		DEBUG_ERROR("unable to bind to: path=%s", sockPath);
	else
		DEBUG_INFO("tap initialized on: path=%s", sockPath);
     _thread = Thread::start(this);
}

NetconEthernetTap::~NetconEthernetTap()
{
	_run = false;
	_phy.whack();
	_phy.whack(); // TODO: Rationale?
	Thread::join(_thread);
	_phy.close(_unixListenSocket,false);
	#if defined(SDK_LWIP)
		delete lwipstack;
	#endif
	#if defined(SDK_PICOTCP)
		delete picostack;
	#endif
	#if defined(SDK_JIP)
		delete jipstack;
	#endif
}

void NetconEthernetTap::setEnabled(bool en)
{
	_enabled = en;
}

bool NetconEthernetTap::enabled() const
{
	return _enabled;
}

bool NetconEthernetTap::addIp(const InetAddress &ip)
{
	// Initialize network stack's interface, assign addresses
    #if defined(SDK_LWIP)
		lwip_init_interface(this, ip);
	#elif defined(SDK_PICOTCP)
        picotap = this;
		pico_init_interface(this, ip);
	#elif defined(SDK_JIP)
		jip_init_interface(ip);
	#endif
	return true;
}

bool NetconEthernetTap::removeIp(const InetAddress &ip)
{
	Mutex::Lock _l(_ips_m);
	std::vector<InetAddress>::iterator i(std::find(_ips.begin(),_ips.end(),ip));
	if (i == _ips.end())
		return false;
	_ips.erase(i);
	if (ip.isV4()) {
		// TODO: De-register from network stacks
	}
	return true;
}

std::vector<InetAddress> NetconEthernetTap::ips() const
{
	Mutex::Lock _l(_ips_m);
	return _ips;
}

// Receive data from ZT tap service (virtual wire) and present it to network stack
// -----------------------------------------
// | TAP <-> MEM BUFFER <-> STACK <-> APP  |
// | |--------------->|                    | RX
// | APP <-> I/O BUFFER <-> STACK <-> TAP  |
// |                                       | 
// ----------------------------------------- 
void NetconEthernetTap::put(const MAC &from,const MAC &to,unsigned int etherType,const void *data,unsigned int len)
{
    // DEBUG_EXTRA("RX packet: len=%d, etherType=%d", len, etherType);
    // RX packet
    #if defined(SDK_LWIP)
		lwip_rx(this, from,to,etherType,data,len);
	#elif defined(SDK_PICOTCP)
		pico_rx(this, from,to,etherType,data,len);
	#elif defined(SDK_JIP)
		jip_rx(from,to,etherType,data,len);
	#endif
}

std::string NetconEthernetTap::deviceName() const
{
	return _dev;
}

void NetconEthernetTap::setFriendlyName(const char *friendlyName) {
}

void NetconEthernetTap::scanMulticastGroups(std::vector<MulticastGroup> &added,std::vector<MulticastGroup> &removed)
{
	std::vector<MulticastGroup> newGroups;
	Mutex::Lock _l(_multicastGroups_m);
	// TODO: get multicast subscriptions from LWIP
	std::vector<InetAddress> allIps(ips());
	for(std::vector<InetAddress>::iterator ip(allIps.begin());ip!=allIps.end();++ip)
		newGroups.push_back(MulticastGroup::deriveMulticastGroupForAddressResolution(*ip));

	std::sort(newGroups.begin(),newGroups.end());
	std::unique(newGroups.begin(),newGroups.end());

	for(std::vector<MulticastGroup>::iterator m(newGroups.begin());m!=newGroups.end();++m) {
		if (!std::binary_search(_multicastGroups.begin(),_multicastGroups.end(),*m))
			added.push_back(*m);
	}
	for(std::vector<MulticastGroup>::iterator m(_multicastGroups.begin());m!=_multicastGroups.end();++m) {
		if (!std::binary_search(newGroups.begin(),newGroups.end(),*m))
			removed.push_back(*m);
	}
	_multicastGroups.swap(newGroups);
}

void NetconEthernetTap::threadMain()
	throw()
{
	// Enter main thread loop for network stack
    #if defined(SDK_LWIP)
		lwip_loop(this);
	#elif defined(SDK_PICOTCP)
		pico_loop(this);
	#elif defined(SDK_JIP)
		jip_loop(this);
	#endif
}

Connection *NetconEthernetTap::getConnection(PhySocket *sock)
{
	for(size_t i=0;i<_Connections.size();++i) {
		if(_Connections[i]->sock == sock)
			return _Connections[i];
	}
	return NULL;
}

Connection *NetconEthernetTap::getConnection(struct pico_socket *sock)
{
	for(size_t i=0;i<_Connections.size();++i) {
		if(_Connections[i]->picosock == sock)
			return _Connections[i];
	}
	return NULL;
}

void NetconEthernetTap::closeConnection(PhySocket *sock)
{
    DEBUG_EXTRA("physock=%p", sock);
	Mutex::Lock _l(_close_m);
	// Here we assume _tcpconns_m is already locked by caller
	if(!sock) {
		DEBUG_EXTRA("invalid PhySocket");
		return;
	}
	// picoTCP
	#if defined(SDK_PICOTCP)
		pico_handleClose(sock);
	#endif
    Connection *conn = getConnection(sock);
    if(!conn)
        return;
    // lwIP
	#if defined(SDK_LWIP)
	    lwip_handleClose(this, sock, conn);
	#endif
	for(size_t i=0;i<_Connections.size();++i) {
		if(_Connections[i] == conn){
			_Connections.erase(_Connections.begin() + i);
			delete conn;
			break;
		}
	}
	if(!sock)
		return;
	close(_phy.getDescriptor(sock));
	_phy.close(sock, false);
}

void NetconEthernetTap::phyOnUnixClose(PhySocket *sock,void **uptr) {
    //DEBUG_EXTRA("physock=%p", sock);
	Mutex::Lock _l(_tcpconns_m);
    //closeConnection(sock);
}


// Receive data from ZT tap service and present it to network stack
// -----------------------------------------
// | TAP <-> MEM BUFFER <-> STACK <-> APP  |
// | |--------------->|                    | RX
// | APP <-> I/O BUFFER <-> STACK <-> TAP  |
// |                                       | 
// ----------------------------------------- 
void NetconEthernetTap::handleRead(PhySocket *sock,void **uptr,bool lwip_invoked)
{
	// DEBUG_EXTRA("handleRead(physock=%p): lwip_invoked = %d\n", sock, lwip_invoked);
	#if defined(SDK_PICOTCP)
		pico_handleRead(sock, uptr, lwip_invoked);
	#endif
	#if defined(SDK_LWIP)
		lwip_handleRead(this, sock, uptr, lwip_invoked);
	#endif
}

void NetconEthernetTap::phyOnUnixWritable(PhySocket *sock,void **uptr,bool lwip_invoked)
{
	handleRead(sock,uptr,lwip_invoked);
}

void NetconEthernetTap::phyOnUnixData(PhySocket *sock, void **uptr, void *data, ssize_t len)
{
    //DEBUG_EXTRA("physock=%p, len=%d", sock, (int)len);
	uint64_t CANARY_num;
	pid_t pid, tid;
	ssize_t wlen = len;
	char cmd, timestamp[20], CANARY[CANARY_SZ], padding[] = {PADDING};
	void *payload;
	unsigned char *buf = (unsigned char*)data;
	std::pair<PhySocket*, void*> sockdata;
	PhySocket *rpcSock;
	bool foundJob = false, detected_rpc = false;
	Connection *conn;
	// RPC
	char phrase[RPC_PHRASE_SZ];
	memset(phrase, 0, RPC_PHRASE_SZ);
	if(len == BUF_SZ) {
		memcpy(phrase, buf, RPC_PHRASE_SZ);
		if(strcmp(phrase, RPC_PHRASE) == 0)
			detected_rpc = true;
	}
	if(detected_rpc) {
		unloadRPC(data, pid, tid, timestamp, CANARY, cmd, payload);
		memcpy(&CANARY_num, CANARY, CANARY_SZ);
		// DEBUG_EXTRA(" RPC: physock=%p, (pid=%d, tid=%d, timestamp=%s, cmd=%d)", sock, pid, tid, timestamp, cmd);

		if(cmd == RPC_SOCKET) {				
			//DEBUG_INFO("RPC_SOCKET, physock=%p", sock);
			// Create new lwip socket and associate it with this sock
			struct socket_st socket_rpc;
			memcpy(&socket_rpc, &buf[IDX_PAYLOAD+STRUCT_IDX], sizeof(struct socket_st));
			Connection * new_conn;
			if((new_conn = handleSocket(sock, uptr, &socket_rpc))) {
				new_conn->pid = pid; // Merely kept to look up application path/names later, not strictly necessary
			}
		} else {
			jobmap[CANARY_num] = std::pair<PhySocket*, void*>(sock, data);
		}
		write(_phy.getDescriptor(sock), "z", 1); // RPC ACK byte to maintain order
	}
	// STREAM
	else {
		int data_start = -1, data_end = -1, canary_pos = -1, padding_pos = -1;
		// Look for padding
		std::string padding_pattern(padding, padding+PADDING_SZ);
		std::string buffer(buf, buf + len);
		padding_pos = buffer.find(padding_pattern);
		canary_pos = padding_pos-CANARY_SZ;
		// Grab token, next we'll use it to look up an RPC job
		if(canary_pos > -1) {
			memcpy(&CANARY_num, buf+canary_pos, CANARY_SZ);
			if(CANARY_num != 0) {
				// Find job
				sockdata = jobmap[CANARY_num];
				if(!sockdata.first) {
					return;
				}  else
					foundJob = true;
			}
		}

		conn = getConnection(sock);
		if(!conn)
			return;

		if(padding_pos == -1) { // [DATA]
			memcpy(&conn->txbuf[conn->txsz], buf, wlen);
		} else { // Padding found, implies a canary is present
			// [CANARY]
			if(len == CANARY_SZ+PADDING_SZ && canary_pos == 0) {
				wlen = 0; // Nothing to write
			} else {
				// [CANARY] + [DATA]
				if(len > CANARY_SZ+PADDING_SZ && canary_pos == 0) {
					wlen = len - CANARY_SZ+PADDING_SZ;
					data_start = padding_pos+PADDING_SZ;
					memcpy((&conn->txbuf)+conn->txsz, buf+data_start, wlen);
				}
				// [DATA] + [CANARY]
				if(len > CANARY_SZ+PADDING_SZ && canary_pos > 0 && canary_pos == len - CANARY_SZ+PADDING_SZ) {
					wlen = len - CANARY_SZ+PADDING_SZ;
					data_start = 0;
					memcpy((&conn->txbuf)+conn->txsz, buf+data_start, wlen);												
				}
				// [DATA] + [CANARY] + [DATA]
				if(len > CANARY_SZ+PADDING_SZ && canary_pos > 0 && len > (canary_pos + CANARY_SZ+PADDING_SZ)) {
					wlen = len - CANARY_SZ+PADDING_SZ;
					data_start = 0;
					data_end = padding_pos-CANARY_SZ;
					memcpy((&conn->txbuf)+conn->txsz, buf+data_start, (data_end-data_start)+1);
					memcpy((&conn->txbuf)+conn->txsz, buf+(padding_pos+PADDING_SZ), len-(canary_pos+CANARY_SZ+PADDING_SZ));
				}
			}
		}
		// Write data from stream
        if(wlen) {
        	/*
            if(conn->type == SOCK_STREAM) { // We only disable TCP "connections"
                int softmax = conn->type == SOCK_STREAM ? DEFAULT_TCP_RX_BUF_SZ : DEFAULT_UDP_RX_BUF_SZ;
                if(conn->txsz > softmax) {
                    _phy.setNotifyReadable(sock, false);
                    conn->disabled = true;
                }
                else if (conn->disabled) {
                    conn->disabled = false;
                    _phy.setNotifyReadable(sock, true);
                }
            }
            */
            conn->txsz += wlen;
            handleWrite(conn);
        }
	}
	// Process RPC if we have a corresponding jobmap entry
    if(foundJob) {
        rpcSock = sockdata.first;
        buf = (unsigned char*)sockdata.second;
		unloadRPC(buf, pid, tid, timestamp, CANARY, cmd, payload);
		// DEBUG_EXTRA(" RPC: physock=%p, (pid=%d, tid=%d, timestamp=%s, cmd=%d)", sock, pid, tid, timestamp, cmd);
		switch(cmd) {
			case RPC_BIND:
				//DEBUG_INFO("RPC_BIND, physock=%p", sock);
			    struct bind_st bind_rpc;
			    memcpy(&bind_rpc,  &buf[IDX_PAYLOAD+STRUCT_IDX], sizeof(struct bind_st));
			    handleBind(sock, rpcSock, uptr, &bind_rpc);
				break;
		  	case RPC_LISTEN:
		  		//DEBUG_INFO("RPC_LISTEN, physock=%p", sock);
			    struct listen_st listen_rpc;
			    memcpy(&listen_rpc,  &buf[IDX_PAYLOAD+STRUCT_IDX], sizeof(struct listen_st));
			    handleListen(sock, rpcSock, uptr, &listen_rpc);
				break;
		  	case RPC_GETSOCKNAME:
		  		//DEBUG_INFO("RPC_GETSOCKNAME, physock=%p", sock);
		  		struct getsockname_st getsockname_rpc;
		    	memcpy(&getsockname_rpc,  &buf[IDX_PAYLOAD+STRUCT_IDX], sizeof(struct getsockname_st));
		  		handleGetsockname(sock, rpcSock, uptr, &getsockname_rpc);
		  		break;
			case RPC_GETPEERNAME:
		  		//DEBUG_INFO("RPC_GETPEERNAME, physock=%p", sock);
		  		struct getsockname_st getpeername_rpc;
		    	memcpy(&getpeername_rpc,  &buf[IDX_PAYLOAD+STRUCT_IDX], sizeof(struct getsockname_st));
		  		handleGetpeername(sock, rpcSock, uptr, &getpeername_rpc);
		  		break;
			case RPC_CONNECT:
				//DEBUG_INFO("RPC_CONNECT, physock=%p", sock);
			    struct connect_st connect_rpc;
			    memcpy(&connect_rpc,  &buf[IDX_PAYLOAD+STRUCT_IDX], sizeof(struct connect_st));
			    handleConnect(sock, rpcSock, conn, &connect_rpc);
			    jobmap.erase(CANARY_num);
				return; // Keep open RPC, we'll use it once in nc_connected to send retval
		  	default:
				break;
		}
		Mutex::Lock _l(_tcpconns_m);
		closeConnection(sockdata.first); // close RPC after sending retval, no longer needed
		jobmap.erase(CANARY_num);
		return;
	}
}

/*------------------------------------------------------------------------------
----------------------------- RPC Handler functions ----------------------------
------------------------------------------------------------------------------*/

void NetconEthernetTap::handleGetsockname(PhySocket *sock, PhySocket *rpcSock, void **uptr, struct getsockname_st *getsockname_rpc)
{
	Mutex::Lock _l(_tcpconns_m);
	Connection *conn = getConnection(sock);
	if(conn->local_addr == NULL){
		DEBUG_EXTRA("no address info available. is it bound?");
		struct sockaddr_storage storage;
		memset(&storage, 0, sizeof(struct sockaddr_storage));
		write(_phy.getDescriptor(rpcSock), NULL, sizeof(struct sockaddr_storage));
		return;
	}
	write(_phy.getDescriptor(rpcSock), conn->local_addr, sizeof(struct sockaddr_storage));
}

void NetconEthernetTap::handleGetpeername(PhySocket *sock, PhySocket *rpcSock, void **uptr, struct getsockname_st *getsockname_rpc)
{
	Mutex::Lock _l(_tcpconns_m);
	Connection *conn = getConnection(sock);
	if(conn->peer_addr == NULL){
		DEBUG_EXTRA("no peer address info available. is it connected?");
		struct sockaddr_storage storage;
		memset(&storage, 0, sizeof(struct sockaddr_storage));
		write(_phy.getDescriptor(rpcSock), NULL, sizeof(struct sockaddr_storage));
		return;
	}
	write(_phy.getDescriptor(rpcSock), conn->peer_addr, sizeof(struct sockaddr_storage));
}
    
Connection * NetconEthernetTap::handleSocket(PhySocket *sock, void **uptr, struct socket_st* socket_rpc)
{
    DEBUG_ATTN("physock=%p, sock_type=%d", sock, socket_rpc->socket_type);
	#if defined(SDK_PICOTCP)
		return pico_handleSocket(sock, uptr, socket_rpc);
	#endif
	#if defined(SDK_LWIP) 
	    return lwip_handleSocket(this, sock, uptr, socket_rpc);
	#endif
    return NULL;
}

Connection * NetconEthernetTap::handleSocketProxy(PhySocket *sock, int socket_type)
{
	return NULL;
}
int NetconEthernetTap::handleConnectProxy(PhySocket *sock, struct sockaddr_in *rawAddr)
{
    return -1;
}

// Connect a stack's PCB/socket/Connection object to a remote host
void NetconEthernetTap::handleConnect(PhySocket *sock, PhySocket *rpcSock, Connection *conn, struct connect_st* connect_rpc)
{
    //DEBUG_ATTN("physock=%p", sock);
	Mutex::Lock _l(_tcpconns_m);
	#if defined(SDK_PICOTCP)
		pico_handleConnect(sock, rpcSock, conn, connect_rpc);		
	#endif
    #if defined(SDK_LWIP)
		lwip_handleConnect(this, sock, rpcSock, conn, connect_rpc);
	#endif
}

void NetconEthernetTap::handleBind(PhySocket *sock, PhySocket *rpcSock, void **uptr, struct bind_st *bind_rpc)
{
	Mutex::Lock _l(_tcpconns_m);
	if(!_ips.size()) {
		// We haven't been given an address yet. Binding at this stage is premature
		DEBUG_ERROR("cannot bind yet. ZT address hasn't been provided");
		sendReturnValue(_phy.getDescriptor(rpcSock), -1, ENOMEM);
		return;
	}
	#if defined(SDK_PICOTCP)
		pico_handleBind(sock,rpcSock,uptr,bind_rpc);
	#endif
	#if defined(SDK_LWIP)
		lwip_handleBind(this, sock, rpcSock, uptr, bind_rpc);
	#endif
}

void NetconEthernetTap::handleListen(PhySocket *sock, PhySocket *rpcSock, void **uptr, struct listen_st *listen_rpc)
{
	Mutex::Lock _l(_tcpconns_m);
  	#if defined(SDK_PICOTCP)
  		pico_handleListen(sock, rpcSock, uptr, listen_rpc);
  	#endif
	#if defined(SDK_LWIP)
 		lwip_handleListen(this, sock, rpcSock, uptr, listen_rpc); 		
	#endif
}

// Write to the network stack (and thus out onto the network)
void NetconEthernetTap::handleWrite(Connection *conn)
{
    #if defined(SDK_PICOTCP)
	    pico_handleWrite(conn);
    #endif
    #if defined(SDK_LWIP)
		lwip_handleWrite(this, conn);
	#endif
}

} // namespace ZeroTier

