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

#include "SDK_EthernetTap.hpp"

#include "Utils.hpp"
#include "OSUtils.hpp"
#include "Phy.hpp"

#include "SDK_LWIPStack.hpp"

#include "lwip/tcp_impl.h"
#include "netif/etharp.h"
#include "lwip/api.h"
#include "lwip/ip.h"
#include "lwip/ip_addr.h"
#include "lwip/ip_frag.h"
#include "lwip/tcp.h"
#include "lwip/init.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"
#include "lwip/netif.h"
#include "lwip/udp.h"

#include "SDK_Debug.c"
#include "SDK_ServiceSetup.hpp"

#if !defined(__IOS__) && !defined(__ANDROID__) && !defined(__UNITY_3D__) && !defined(__XCODE__)
    const ip_addr_t ip_addr_any = { IPADDR_ANY };
#endif

namespace ZeroTier {

// ---------------------------------------------------------------------------

static err_t tapif_init(struct netif *netif)
{
  // Actual init functionality is in addIp() of tap
  return ERR_OK;
}

/*
 * Outputs data from the pbuf queue to the interface
 */
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
  struct pbuf *q;
  char buf[ZT_MAX_MTU+32];
  char *bufptr;
  int totalLength = 0;

  ZeroTier::NetconEthernetTap *tap = (ZeroTier::NetconEthernetTap*)netif->state;
  bufptr = buf;
  // Copy data from each pbuf, one at a time
  for(q = p; q != NULL; q = q->next) {
    memcpy(bufptr, q->payload, q->len);
    bufptr += q->len;
    totalLength += q->len;
  }
  // [Send packet to network]
  // Split ethernet header and feed into handler
  struct eth_hdr *ethhdr;
  ethhdr = (struct eth_hdr *)buf;

  ZeroTier::MAC src_mac;
  ZeroTier::MAC dest_mac;
  src_mac.setTo(ethhdr->src.addr, 6);
  dest_mac.setTo(ethhdr->dest.addr, 6);

  tap->_handler(tap->_arg,tap->_nwid,src_mac,dest_mac,
    Utils::ntoh((uint16_t)ethhdr->type),0,buf + sizeof(struct eth_hdr),totalLength - sizeof(struct eth_hdr));
  return ERR_OK;
}
    
// ---------------------------------------------------------------------------

NetconEthernetTap::NetconEthernetTap(
	const char *homePath,
	const MAC &mac,
	unsigned int mtu,
	unsigned int metric,
	uint64_t nwid,
	const char *friendlyName,
	void (*handler)(void *,uint64_t,const MAC &,const MAC &,unsigned int,unsigned int,const void *,unsigned int),
	void *arg) :
  _nwid(nwid),
	_handler(handler),
	_arg(arg),
  _phy(this,false,true),
  _unixListenSocket((PhySocket *)0),
	_mac(mac),
	_homePath(homePath),
	_mtu(mtu),
	_enabled(true),
	_run(true)
{
    char sockPath[4096],lwipPath[4096];
    rpcCounter = -1;
    Utils::snprintf(sockPath,sizeof(sockPath),"%s%snc_%.16llx",homePath,ZT_PATH_SEPARATOR_S,_nwid,ZT_PATH_SEPARATOR_S,(unsigned long long)nwid);
    _dev = sockPath; // in SDK mode, set device to be just the network ID
    
	// Start SOCKS5 Proxy server
	// For use when traditional syscall hooking isn't available (ex. some APIs on iOS and Android)
	#if defined(USE_SOCKS_PROXY) || defined(__ANDROID__)
		StartProxy(sockPath);
	#endif

	Utils::snprintf(lwipPath,sizeof(lwipPath),"%s%sliblwip.so",homePath,ZT_PATH_SEPARATOR_S);
	
    lwipstack = new LWIPStack(lwipPath);
	if(!lwipstack)
		throw std::runtime_error("unable to dynamically load a new instance of liblwip.so (searched ZeroTier home path)");
	lwipstack->__lwip_init();
    
	_unixListenSocket = _phy.unixListen(sockPath,(void *)this);
	dwr(MSG_DEBUG, " NetconEthernetTap initialized on: %s\n", sockPath);
	//if (!_unixListenSocket)
	//	throw std::runtime_error(std::string("unable to bind to ")+sockPath);
     _thread = Thread::start(this);
}

NetconEthernetTap::~NetconEthernetTap()
{
	_run = false;
	_phy.whack();
	_phy.whack(); // TODO: Rationale?
	Thread::join(_thread);
	_phy.close(_unixListenSocket,false);
	delete lwipstack;
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
	Mutex::Lock _l(_ips_m);
	if (std::find(_ips.begin(),_ips.end(),ip) == _ips.end()) {
		_ips.push_back(ip);
		std::sort(_ips.begin(),_ips.end());

		if (ip.isV4()) {
			// Set IP
			static ip_addr_t ipaddr, netmask, gw;
			IP4_ADDR(&gw,127,0,0,1);
			ipaddr.addr = *((u32_t *)ip.rawIpData());
			netmask.addr = *((u32_t *)ip.netmask().rawIpData());

			// Set up the lwip-netif for LWIP's sake
			lwipstack->__netif_add(&interface,&ipaddr, &netmask, &gw, NULL, tapif_init, lwipstack->_ethernet_input);
			interface.state = this;
			interface.output = lwipstack->_etharp_output;
			_mac.copyTo(interface.hwaddr, 6);
			interface.mtu = _mtu;
			interface.name[0] = 't';
			interface.name[1] = 'p';
			interface.linkoutput = low_level_output;
			interface.hwaddr_len = 6;
			interface.flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP;
			lwipstack->__netif_set_default(&interface);
			lwipstack->__netif_set_up(&interface);
		}
	}
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
		// TODO: dealloc from LWIP
	}
	return true;
}

std::vector<InetAddress> NetconEthernetTap::ips() const
{
	Mutex::Lock _l(_ips_m);
	return _ips;
}

void NetconEthernetTap::put(const MAC &from,const MAC &to,unsigned int etherType,const void *data,unsigned int len)
{
    //dwr(MSG_DEBUG_EXTRA, "RX packet: len = %d\n", len);
	struct pbuf *p,*q;
	if (!_enabled)
		return;

	struct eth_hdr ethhdr;
	from.copyTo(ethhdr.src.addr, 6);
	to.copyTo(ethhdr.dest.addr, 6);
	ethhdr.type = Utils::hton((uint16_t)etherType);

	// We allocate a pbuf chain of pbufs from the pool.
	p = lwipstack->__pbuf_alloc(PBUF_RAW, len+sizeof(struct eth_hdr), PBUF_POOL);

	if (p != NULL) {
		const char *dataptr = reinterpret_cast<const char *>(data);
		// First pbuf gets ethernet header at start
		q = p;
		if (q->len < sizeof(ethhdr)) {
			dwr(MSG_ERROR,"_put(): Dropped packet: first pbuf smaller than ethernet header\n");
			return;
		}
		memcpy(q->payload,&ethhdr,sizeof(ethhdr));
		memcpy((char*)q->payload + sizeof(ethhdr),dataptr,q->len - sizeof(ethhdr));
		dataptr += q->len - sizeof(ethhdr);

		// Remaining pbufs (if any) get rest of data
		while ((q = q->next)) {
			memcpy(q->payload,dataptr,q->len);
			dataptr += q->len;
		}
	} else {
		dwr(MSG_ERROR,"put(): Dropped packet: no pbufs available\n");
		return;
	}

	{
		if(interface.input(p, &interface) != ERR_OK) {
			dwr(MSG_ERROR,"put(): Error while RXing packet (netif->input)\n");
		}
	}
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
	uint64_t prev_tcp_time = 0, prev_status_time = 0, prev_etharp_time = 0;

	// Main timer loop
	while (_run) {
		
		uint64_t now = OSUtils::now();
		uint64_t since_tcp = now - prev_tcp_time;
		uint64_t since_etharp = now - prev_etharp_time;
		uint64_t since_status = now - prev_status_time;
		uint64_t tcp_remaining = ZT_LWIP_TCP_TIMER_INTERVAL;
		uint64_t etharp_remaining = ARP_TMR_INTERVAL;

		// Connection prunning
		if (since_status >= STATUS_TMR_INTERVAL) {
			prev_status_time = now;
			for(size_t i=0;i<_Connections.size();++i) {
				if(!_Connections[i]->sock || _Connections[i]->type != SOCK_STREAM)
					continue;
				int fd = _phy.getDescriptor(_Connections[i]->sock);
				// dwr(MSG_DEBUG," tap_thread(): tcp\\jobs = {%d, %d}\n", _Connection.size(), jobmap.size());
				// If there's anything on the RX buf, set to notify in case we stalled
				if(_Connections[i]->rxsz > 0)
					_phy.setNotifyWritable(_Connections[i]->sock, true);
				fcntl(fd, F_SETFL, O_NONBLOCK);
				unsigned char tmpbuf[BUF_SZ];
				
				int n = read(fd,&tmpbuf,BUF_SZ);
				if(_Connections[i]->TCP_pcb->state == SYN_SENT) {
					dwr(MSG_DEBUG_EXTRA,"  tap_thread(): (sock=%p) state = SYN_SENT, should finish or be removed soon\n", 
						(void*)&(_Connections[i]->sock));
				}
				if((n < 0 && errno != EAGAIN) || (n == 0 && errno == EAGAIN)) {
					dwr(MSG_DEBUG," tap_thread(): closing sock (%x)\n", _Connections[i]->sock);
					closeConnection(_Connections[i]->sock);
				} else if (n > 0) {
					dwr(MSG_DEBUG," tap_thread(): data read during connection check (%d bytes)\n", n);
					phyOnUnixData(_Connections[i]->sock,_phy.getuptr(_Connections[i]->sock),&tmpbuf,n);
				}		
			}
		}
		// Main TCP/ETHARP timer section
		if (since_tcp >= ZT_LWIP_TCP_TIMER_INTERVAL) {
			prev_tcp_time = now;
            lwipstack->__tcp_tmr();
            // FIXME: could be removed or refactored?
            // Makeshift poll
			for(size_t i=0;i<_Connections.size();++i) {
				if(_Connections[i]->txsz > 0){
					handleWrite(_Connections[i]);
				}
			}
		} else {
			tcp_remaining = ZT_LWIP_TCP_TIMER_INTERVAL - since_tcp;
		}
		if (since_etharp >= ARP_TMR_INTERVAL) {
			prev_etharp_time = now;
            lwipstack->__etharp_tmr();
		
        } else {
			etharp_remaining = ARP_TMR_INTERVAL - since_etharp;
		}
		_phy.poll((unsigned long)std::min(tcp_remaining,etharp_remaining));
	}
    lwipstack->close();
}

Connection *NetconEthernetTap::getConnection(PhySocket *sock)
{
	for(size_t i=0;i<_Connections.size();++i) {
		if(_Connections[i]->sock == sock)
			return _Connections[i];
	}
	return NULL;
}

void NetconEthernetTap::closeConnection(PhySocket *sock)
{
    dwr(MSG_DEBUG, "closeConnection(%x):\n", sock);
	Mutex::Lock _l(_close_m);
	// Here we assume _tcpconns_m is already locked by caller
	if(!sock) {
		dwr(MSG_DEBUG,"closeConnection(): invalid PhySocket\n");
		return;
	}
	Connection *conn = getConnection(sock);
	if(!conn)
		return;
    if(conn->type==SOCK_DGRAM) {
        lwipstack->__udp_remove(conn->UDP_pcb);
        return;
    }
	if(conn->TCP_pcb && conn->TCP_pcb->state != CLOSED) {
		dwr(MSG_DEBUG,"closeConnection(conn=%p,sock=%p): PCB->state = %d\n", 
			(void*)&conn, (void*)&sock, conn->TCP_pcb->state);
		if(conn->TCP_pcb->state == SYN_SENT /*|| conn->TCP_pcb->state == CLOSE_WAIT*/) {
			dwr(MSG_DEBUG,"closeConnection(sock=%p): invalid PCB state for this operation. ignoring.\n", 
				(void*)&sock);
			return;
		}	
		dwr(MSG_DEBUG, "__tcp_close(...)\n");
		if(lwipstack->__tcp_close(conn->TCP_pcb) == ERR_OK) {
			// Unregister callbacks for this PCB
			lwipstack->__tcp_arg(conn->TCP_pcb, NULL);
		    lwipstack->__tcp_recv(conn->TCP_pcb, NULL);
		    lwipstack->__tcp_err(conn->TCP_pcb, NULL);
		    lwipstack->__tcp_sent(conn->TCP_pcb, NULL);
		    lwipstack->__tcp_poll(conn->TCP_pcb, NULL, 1);
		}
		else {
			dwr(MSG_ERROR,"closeConnection(sock=%p): error while calling tcp_close()\n", (void*)&sock);
		}
	}
	dwr(MSG_DEBUG, "Removing from _Connections\n");
	for(size_t i=0;i<_Connections.size();++i) {
		if(_Connections[i] == conn){
			_Connections.erase(_Connections.begin() + i);
			delete conn;
			break;
		}
	}
	if(!sock)
		return;
	dwr(MSG_DEBUG, "closing underlying socket\n");
	close(_phy.getDescriptor(sock));
	_phy.close(sock, false);
}

void NetconEthernetTap::phyOnUnixClose(PhySocket *sock,void **uptr) {
    dwr(MSG_DEBUG, "phyOnUnixClose(sock=%p):\n", (void*)&sock);
	Mutex::Lock _l(_tcpconns_m);
    closeConnection(sock);
}


void NetconEthernetTap::processReceivedData(PhySocket *sock,void **uptr,bool lwip_invoked)
{
	dwr(MSG_DEBUG,"processReceivedData(sock=%p): lwip_invoked = %d\n", 
		(void*)&sock, lwip_invoked);
	if(!lwip_invoked) {
		_tcpconns_m.lock();
		_rx_buf_m.lock();
	}
	Connection *conn = getConnection(sock);	
	if(conn && conn->rxsz) {
		long n = _phy.streamSend(conn->sock, conn->rxbuf, conn->rxsz);
		if(n > 0) {
			if(conn->rxsz-n > 0)
				memcpy(conn->rxbuf, conn->rxbuf+n, conn->rxsz-n);
		  	conn->rxsz -= n;
            if(conn->type==SOCK_DGRAM){
                conn->unread_udp_packet = false;
                _phy.setNotifyWritable(conn->sock, false);
            }
            //dwr(MSG_DEBUG, "phyOnUnixWritable(): tid = %d\n", pthread_mach_thread_np(pthread_self()));
            if(conn->type==SOCK_STREAM) { // Only acknolwedge receipt of TCP packets
                lwipstack->__tcp_recved(conn->TCP_pcb, n);
            	float max = conn->type == SOCK_STREAM ? (float)DEFAULT_TCP_TX_BUF_SZ : (float)DEFAULT_UDP_TX_BUF_SZ;
            	dwr(MSG_TRANSFER," RX <---    :: {TX: %.3f%%, RX: %.3f%%, sock=%x} :: %d bytes\n",
                	(float)conn->txsz / max, (float)conn->rxsz / max, conn->sock, n);
        	}
		} else {
			dwr(MSG_DEBUG," processReceivedData(): errno = %d, rxsz = %d\n", errno, conn->rxsz);
			_phy.setNotifyWritable(conn->sock, false);
		}
	}
    // If everything on the buffer has been written
    if(conn->rxsz == 0) {
        _phy.setNotifyWritable(conn->sock, false);
    }
	if(!lwip_invoked) {
		_tcpconns_m.unlock();
		_rx_buf_m.unlock();
	}
}


void NetconEthernetTap::phyOnUnixWritable(PhySocket *sock,void **uptr,bool lwip_invoked)
{
	dwr(MSG_DEBUG," phyOnUnixWritable(sock=%p): lwip_invoked = %d\n", (void*)&sock, lwip_invoked);
	processReceivedData(sock,uptr,lwip_invoked);
}

void NetconEthernetTap::phyOnUnixData(PhySocket *sock,void **uptr,void *data,unsigned long len)
{
    dwr(MSG_DEBUG, "phyOnUnixData(%p), len = %d\n", (void*)&sock, len);
	uint64_t CANARY_num;
	pid_t pid, tid;
	int rpcCount, wlen = len;
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
		unloadRPC(data, pid, tid, rpcCount, timestamp, CANARY, cmd, payload);
		memcpy(&CANARY_num, CANARY, CANARY_SZ);
		dwr(MSG_DEBUG," <sock=%p> RPC: (pid=%d, tid=%d, rpcCount=%d, timestamp=%s, cmd=%d)\n", 
			(void*)&sock, pid, tid, rpcCount, timestamp, cmd);

		if(cmd == RPC_SOCKET) {				
			dwr(MSG_DEBUG,"  <sock=%p> RPC_SOCKET\n", (void*)&sock);
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
					dwr(MSG_DEBUG," <sock=%p> unable to locate job entry for %llu\n", (void*)&sock, CANARY_num);
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
            conn->txsz += wlen;
            handleWrite(conn);
        }
	}
	// Process RPC if we have a corresponding jobmap entry
    if(foundJob) {
        rpcSock = sockdata.first;
        buf = (unsigned char*)sockdata.second;
		unloadRPC(buf, pid, tid, rpcCount, timestamp, CANARY, cmd, payload);
		dwr(MSG_DEBUG," <sock=%p> RPC: (pid=%d, tid=%d, rpcCount=%d, timestamp=%s, cmd=%d)\n", 
			(void*)&sock, pid, tid, rpcCount, timestamp, cmd);

		switch(cmd) {
			case RPC_BIND:
			    struct bind_st bind_rpc;
			    memcpy(&bind_rpc,  &buf[IDX_PAYLOAD+STRUCT_IDX], sizeof(struct bind_st));
			    handleBind(sock, rpcSock, uptr, &bind_rpc);
				break;
		  	case RPC_LISTEN:
			    struct listen_st listen_rpc;
			    memcpy(&listen_rpc,  &buf[IDX_PAYLOAD+STRUCT_IDX], sizeof(struct listen_st));
			    handleListen(sock, rpcSock, uptr, &listen_rpc);
				break;
		  	case RPC_GETSOCKNAME:
		  		struct getsockname_st getsockname_rpc;
		    	memcpy(&getsockname_rpc,  &buf[IDX_PAYLOAD+STRUCT_IDX], sizeof(struct getsockname_st));
		  		handleGetsockname(sock, rpcSock, uptr, &getsockname_rpc);
		  		break;
			case RPC_CONNECT:
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

int NetconEthernetTap::sendReturnValue(PhySocket *sock, int retval, int _errno = 0){
    dwr(MSG_DEBUG," sendReturnValue(sock=%p)\n", (void*)&sock);
	return sendReturnValue(_phy.getDescriptor(sock), retval, _errno);
}
int NetconEthernetTap::sendReturnValue(int fd, int retval, int _errno = 0)
{
//#if !defined(USE_SOCKS_PROXY)
	dwr(MSG_DEBUG," sendReturnValue(): fd = %d, retval = %d, errno = %d\n", fd, retval, _errno);
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

void NetconEthernetTap::unloadRPC(void *data, pid_t &pid, pid_t &tid, 
	int &rpcCount, char (timestamp[RPC_TIMESTAMP_SZ]), char (CANARY[sizeof(uint64_t)]), char &cmd, void* &payload)
{
	unsigned char *buf = (unsigned char*)data;
	memcpy(&pid, &buf[IDX_PID], sizeof(pid_t));
	memcpy(&tid, &buf[IDX_TID], sizeof(pid_t));
	memcpy(&rpcCount, &buf[IDX_COUNT], sizeof(int));
	memcpy(timestamp, &buf[IDX_TIME], RPC_TIMESTAMP_SZ);
	memcpy(&cmd, &buf[IDX_PAYLOAD], sizeof(char));
	memcpy(CANARY, &buf[IDX_PAYLOAD+1], CANARY_SZ);
}

/*------------------------------------------------------------------------------
--------------------------------- LWIP callbacks -------------------------------
------------------------------------------------------------------------------*/

err_t NetconEthernetTap::nc_accept(void *arg, struct tcp_pcb *newPCB, err_t err)
{
	dwr(MSG_DEBUG, "nc_accept(pcb=%p)\n", (void*)&newPCB);
	Larg *l = (Larg*)arg;
    Mutex::Lock _l(l->tap->_tcpconns_m);
	Connection *conn = l->conn;
	NetconEthernetTap *tap = l->tap;

    if(!conn)
        return -1;
    if(conn->type==SOCK_DGRAM)
        return -1;
	if(!conn->sock)
		return -1;
	int fd = tap->_phy.getDescriptor(conn->sock);

  	if(conn) {
	  	// create new socketpair
	  	ZT_PHY_SOCKFD_TYPE fds[2];
		if(socketpair(PF_LOCAL, SOCK_STREAM, 0, fds) < 0) {
			if(errno < 0) {
				l->tap->sendReturnValue(conn, -1, errno);
				dwr(MSG_ERROR," nc_accept(): unable to create socketpair\n");
				return ERR_MEM;
			}
		}
		// create and populate new Connection
		Connection *newTcpConn = new Connection();
		l->tap->_Connections.push_back(newTcpConn);
		newTcpConn->TCP_pcb = newPCB;
		newTcpConn->type = SOCK_STREAM;
		newTcpConn->sock = tap->_phy.wrapSocket(fds[0], newTcpConn);

		if(sock_fd_write(fd, fds[1]) < 0)
	  		return -1;
	    tap->lwipstack->__tcp_arg(newPCB, new Larg(tap, newTcpConn));
	    tap->lwipstack->__tcp_recv(newPCB, nc_recved);
	    tap->lwipstack->__tcp_err(newPCB, nc_err);
	    tap->lwipstack->__tcp_sent(newPCB, nc_sent);
	    tap->lwipstack->__tcp_poll(newPCB, nc_poll, 1);
	    if(conn->TCP_pcb->state == LISTEN) {
	    	dwr(MSG_DEBUG," nc_accept(): can't call tcp_accept() on LISTEN socket (pcb = %x)\n", conn->TCP_pcb);
	    	return ERR_OK;
	    }
	    tcp_accepted(conn->TCP_pcb); // Let lwIP know that it can queue additional incoming connections
		return ERR_OK;
  	} else
  		dwr(MSG_ERROR," nc_accept(): can't locate Connection object for PCB.\n");
  	return -1;
}
    
void NetconEthernetTap::nc_udp_recved(void * arg, struct udp_pcb * upcb, struct pbuf * p, struct ip_addr * addr, u16_t port)
{
    Larg *l = (Larg*)arg;
    dwr(MSG_DEBUG, "nc_udp_recved(conn=%p,pcb=%p,port=%d)\n", (void*)&(l->conn), (void*)&upcb, port);
    int tot = 0;
    struct pbuf* q = p;
    Mutex::Lock _l2(l->tap->_rx_buf_m);
    // Cycle through pbufs and write them to the RX buffer
    // The RX "buffer" will be emptied via phyOnUnixWritable()
    if(p) {
        l->conn->rxsz = 0;
        memset(l->conn->rxbuf, 0, DEFAULT_UDP_RX_BUF_SZ);
    }
    while(p != NULL) {
        if(p->len <= 0)
            break;
        int len = p->len;
        memcpy(l->conn->rxbuf + (l->conn->rxsz), p->payload, len);
        l->conn->rxsz += len;
        p = p->next;
        tot += len;
    }
    if(tot) {
        dwr(MSG_DEBUG, " nc_udp_recved(): tot = %d, rxsz = %d\n", tot, l->conn->rxsz);
        l->conn->unread_udp_packet = true;
        l->tap->phyOnUnixWritable(l->conn->sock, NULL, true);
        l->tap->_phy.setNotifyWritable(l->conn->sock, true);
    }
    l->tap->lwipstack->__pbuf_free(q);
}
   

err_t NetconEthernetTap::nc_recved(void *arg, struct tcp_pcb *PCB, struct pbuf *p, err_t err)
{
    Larg *l = (Larg*)arg;
    dwr(MSG_DEBUG, "nc_recved(conn=%p,pcb=%p)\n", (void*)&(l->conn), (void*)&PCB);
	int tot = 0;
  	struct pbuf* q = p;
	Mutex::Lock _l(l->tap->_tcpconns_m);

	if(!l->conn) {
		dwr(MSG_ERROR," nc_recved(): no connection\n");
		return ERR_OK; 
	}
	if(p == NULL) {
		if(l->conn->TCP_pcb->state == CLOSE_WAIT){
			l->tap->closeConnection(l->conn->sock);
			return ERR_ABRT;
		}
		return err;
	}
	Mutex::Lock _l2(l->tap->_rx_buf_m);
	// Cycle through pbufs and write them to the RX buffer
	// The RX buffer will be emptied via phyOnUnixWritable()
	while(p != NULL) {
		if(p->len <= 0)
			break;
		int avail = DEFAULT_TCP_RX_BUF_SZ - l->conn->rxsz;
		int len = p->len;
		if(avail < len)
			dwr(MSG_ERROR," nc_recved(): not enough room (%d bytes) on RX buffer\n", avail);
		memcpy(l->conn->rxbuf + (l->conn->rxsz), p->payload, len);
		l->conn->rxsz += len;
		p = p->next;
		tot += len;
	}
	if(tot) {
		#if defined(USE_SOCKS_PROXY)
			l->tap->phyOnTcpWritable(l->conn->sock, NULL, true);
		#else
			l->tap->phyOnUnixWritable(l->conn->sock, NULL, true);
		#endif
	}
	l->tap->lwipstack->__pbuf_free(q);
	return ERR_OK;
}

err_t NetconEthernetTap::nc_sent(void* arg, struct tcp_pcb *PCB, u16_t len)
{
    dwr(MSG_DEBUG, "nc_sent(pcb=%p)\n", (void*)&PCB);
	Larg *l = (Larg*)arg;
	Mutex::Lock _l(l->tap->_tcpconns_m);
	if(l->conn->probation && l->conn->txsz == 0){
		l->conn->probation = false; // TX buffer now empty, removing from probation
	}
	if(l && l->conn && len && !l->conn->probation) {
        int softmax = l->conn->type == SOCK_STREAM ? DEFAULT_TCP_TX_BUF_SZ : DEFAULT_UDP_TX_BUF_SZ;
		if(l->conn->txsz < softmax) {
			l->tap->_phy.setNotifyReadable(l->conn->sock, true);
            l->tap->_phy.whack();
		}
	}
	return ERR_OK;
}

err_t NetconEthernetTap::nc_connected_proxy(void *arg, struct tcp_pcb *PCB, err_t err)
{
    dwr(MSG_DEBUG, "nc_connected_proxy(pcb=%p)\n", (void*)&PCB);
    return ERR_OK;
}
    
err_t NetconEthernetTap::nc_connected(void *arg, struct tcp_pcb *PCB, err_t err)
{
    dwr(MSG_DEBUG, "nc_connected(pcb=%p)\n", (void*)&PCB);
	Larg *l = (Larg*)arg;
	if(l && l->conn)
		l->tap->sendReturnValue(l->tap->_phy.getDescriptor(l->conn->rpcSock), ERR_OK);
	return ERR_OK;
}

err_t NetconEthernetTap::nc_poll(void* arg, struct tcp_pcb *PCB)
{
	return ERR_OK;
}

void NetconEthernetTap::nc_err(void *arg, err_t err)
{
	dwr(MSG_DEBUG,"nc_err() = %d\n", err);
	Larg *l = (Larg*)arg;
	Mutex::Lock _l(l->tap->_tcpconns_m);

	if(!l->conn)
		dwr(MSG_ERROR," nc_err(): connection is NULL!\n");
	int fd = l->tap->_phy.getDescriptor(l->conn->sock);

	switch(err)
	{
		case ERR_MEM:
		  dwr(MSG_ERROR," nc_err(): ERR_MEM->ENOMEM\n");
			l->tap->sendReturnValue(fd, -1, ENOMEM);
			break;
		case ERR_BUF:
			dwr(MSG_ERROR," nc_err(): ERR_BUF->ENOBUFS\n");
			l->tap->sendReturnValue(fd, -1, ENOBUFS);
			break;
		case ERR_TIMEOUT:
			dwr(MSG_ERROR," nc_err(): ERR_TIMEOUT->ETIMEDOUT\n");
			l->tap->sendReturnValue(fd, -1, ETIMEDOUT);
			break;
		case ERR_RTE:
			dwr(MSG_ERROR," nc_err(): ERR_RTE->ENETUNREACH\n");
			l->tap->sendReturnValue(fd, -1, ENETUNREACH);
			break;
		case ERR_INPROGRESS:
			dwr(MSG_ERROR," nc_err(): ERR_INPROGRESS->EINPROGRESS\n");
			l->tap->sendReturnValue(fd, -1, EINPROGRESS);
			break;
		case ERR_VAL:
			dwr(MSG_ERROR," nc_err(): ERR_VAL->EINVAL\n");
			l->tap->sendReturnValue(fd, -1, EINVAL);
			break;
		case ERR_WOULDBLOCK:
			dwr(MSG_ERROR," nc_err(): ERR_WOULDBLOCK->EWOULDBLOCK\n");
			l->tap->sendReturnValue(fd, -1, EWOULDBLOCK);
			break;
		case ERR_USE:
			dwr(MSG_ERROR," nc_err(): ERR_USE->EADDRINUSE\n");
			l->tap->sendReturnValue(fd, -1, EADDRINUSE);
			break;
		case ERR_ISCONN:
			dwr(MSG_ERROR," nc_err(): ERR_ISCONN->EISCONN\n");
			l->tap->sendReturnValue(fd, -1, EISCONN);
			break;
		case ERR_ABRT:
			dwr(MSG_ERROR," nc_err(): ERR_ABRT->ECONNREFUSED\n");
			l->tap->sendReturnValue(fd, -1, ECONNREFUSED);
			break;

			// FIXME: Below are errors which don't have a standard errno correlate

		case ERR_RST:
			l->tap->sendReturnValue(fd, -1, -1);
			break;
		case ERR_CLSD:
			l->tap->sendReturnValue(fd, -1, -1);
			break;
		case ERR_CONN:
			l->tap->sendReturnValue(fd, -1, -1);
			break;
		case ERR_ARG:
			l->tap->sendReturnValue(fd, -1, -1);
			break;
		case ERR_IF:
			l->tap->sendReturnValue(fd, -1, -1);
			break;
		default:
			break;
	}
	dwr(MSG_ERROR," nc_err(): closing connection\n");
	l->tap->closeConnection(l->conn);
}

/*------------------------------------------------------------------------------
----------------------------- RPC Handler functions ----------------------------
------------------------------------------------------------------------------*/

void NetconEthernetTap::handleGetsockname(PhySocket *sock, PhySocket *rpcSock, void **uptr, struct getsockname_st *getsockname_rpc)
{
	Mutex::Lock _l(_tcpconns_m);
	Connection *conn = getConnection(sock);
	char retmsg[sizeof(struct sockaddr_storage)];
	memset(&retmsg, 0, sizeof(retmsg));
	if ((conn)&&(conn->addr))
    	memcpy(&retmsg, conn->addr, sizeof(struct sockaddr_storage));
	write(_phy.getDescriptor(rpcSock), &retmsg, sizeof(struct sockaddr_storage));
}

void NetconEthernetTap::handleBind(PhySocket *sock, PhySocket *rpcSock, void **uptr, struct bind_st *bind_rpc)
{
	Mutex::Lock _l(_tcpconns_m);
	struct sockaddr_in *rawAddr = (struct sockaddr_in *) &bind_rpc->addr;
	int err, port = lwipstack->__lwip_ntohs(rawAddr->sin_port);
	ip_addr_t connAddr;
	connAddr.addr = *((u32_t *)_ips[0].rawIpData());
	Connection *conn = getConnection(sock);
    dwr(MSG_DEBUG," handleBind(sock=%p,fd=%d,port=%d)\n", (void*)&sock, bind_rpc->sockfd, port);
    if(conn) {
        if(conn->type == SOCK_DGRAM) {
        	// FIXME: Review why compliation through JNI+NDK toolchain comaplains about this
       		#if defined(__ANDROID__)
            	err = lwipstack->__udp_bind(conn->UDP_pcb, NULL, port);
            #else
				err = lwipstack->__udp_bind(conn->UDP_pcb, IP_ADDR_ANY, port);
            #endif
            if(err == ERR_USE) // port in use
                sendReturnValue(rpcSock, -1, EADDRINUSE);
            else {
                lwipstack->__udp_recv(conn->UDP_pcb, nc_udp_recved, new Larg(this, conn));
                conn->port = conn->UDP_pcb->local_port;
  				// Update port to assigned port
                struct sockaddr_in addr_in;
                memcpy(&addr_in, &bind_rpc->addr, sizeof(addr_in));
                addr_in.sin_port = Utils::ntoh(conn->UDP_pcb->local_port);
                memcpy(&conn->addr, &addr_in, sizeof(addr_in));
                sendReturnValue(rpcSock, ERR_OK, ERR_OK); // Success
            }
            return;
        }
        else if (conn->type == SOCK_STREAM) {
            if(conn->TCP_pcb->state == CLOSED){
                err = lwipstack->__tcp_bind(conn->TCP_pcb, &connAddr, port);
                int ip = rawAddr->sin_addr.s_addr;
                unsigned char d[4];
                d[0] = ip & 0xFF;
                d[1] = (ip >>  8) & 0xFF;
                d[2] = (ip >> 16) & 0xFF;
                d[3] = (ip >> 24) & 0xFF;
                dwr(MSG_DEBUG," handleBind(): %d.%d.%d.%d : %d\n", d[0],d[1],d[2],d[3], port);

                if(err != ERR_OK) {
                    dwr(MSG_ERROR," handleBind(): err = %d\n", err);
                    if(err == ERR_USE)
                        sendReturnValue(rpcSock, -1, EADDRINUSE);
                    if(err == ERR_MEM)
                        sendReturnValue(rpcSock, -1, ENOMEM);
                    if(err == ERR_BUF)
                        sendReturnValue(rpcSock, -1, ENOMEM);
                } else {
                    conn->addr = (struct sockaddr_storage *) &bind_rpc->addr;
                    sendReturnValue(rpcSock, ERR_OK, ERR_OK); // Success
                }
            } else {
                dwr(MSG_ERROR," handleBind(): PCB (conn=%p,pcb=%p) not in CLOSED state. Ignoring BIND request.\n", 
                	(void*)&conn, (void*)&conn->TCP_pcb);
                sendReturnValue(rpcSock, -1, EINVAL);
            }
        }
	} else {
		dwr(MSG_ERROR," handleBind(): unable to locate Connection.\n");
		sendReturnValue(rpcSock, -1, EBADF);
	}
}

void NetconEthernetTap::handleListen(PhySocket *sock, PhySocket *rpcSock, void **uptr, struct listen_st *listen_rpc)
{
	dwr(MSG_DEBUG, "handleListen(sock=%p)\n", (void*)&sock);
	Mutex::Lock _l(_tcpconns_m);
	Connection *conn = getConnection(sock);
    
    if(conn->type==SOCK_DGRAM)
        return;
	if(!conn) {
		dwr(MSG_ERROR," handleListen(): unable to locate Connection.\n");
		sendReturnValue(rpcSock, -1, EBADF);
		return;
	}
	if(conn->TCP_pcb->state == LISTEN) {
		dwr(MSG_ERROR," handleListen(): PCB is already in listening state.\n");
		sendReturnValue(rpcSock, ERR_OK, ERR_OK);
		return;
	}
	struct tcp_pcb* listeningPCB;

#ifdef TCP_LISTEN_BACKLOG
		listeningPCB = lwipstack->__tcp_listen_with_backlog(conn->TCP_pcb, listen_rpc->backlog);
#else
		listeningPCB = lwipstack->__tcp_listen(conn->pcb);
#endif
	if(listeningPCB != NULL) {
    	conn->TCP_pcb = listeningPCB;
    	lwipstack->__tcp_accept(listeningPCB, nc_accept);
		lwipstack->__tcp_arg(listeningPCB, new Larg(this, conn));
		fcntl(_phy.getDescriptor(conn->sock), F_SETFL, O_NONBLOCK);
		conn->listening = true;
		sendReturnValue(rpcSock, ERR_OK, ERR_OK);
		return;
  	}
  sendReturnValue(rpcSock, -1, -1);
}
    
Connection * NetconEthernetTap::handleSocketProxy(PhySocket *sock, int socket_type)
{
    Connection *conn = getConnection(sock);
    if(!conn){
        dwr(MSG_DEBUG, "handleSocketProxy(sock=%p): Unable to locate Connection object for this PhySocket\n", (void*)&sock);
        return NULL;
    }
	dwr(MSG_DEBUG, "handleSocketProxy(sock=%p)\n", (void*)&sock);
    struct udp_pcb *new_udp_PCB = NULL;
    struct tcp_pcb *new_tcp_PCB = NULL;
    if(socket_type == SOCK_DGRAM) {
        dwr(MSG_DEBUG, " handleSocketProxy(): SOCK_DGRAM\n");
        Mutex::Lock _l(_tcpconns_m);
        new_udp_PCB = lwipstack->__udp_new();
    }
    else if(socket_type == SOCK_STREAM) {
        dwr(MSG_DEBUG, " handleSocketProxy(): SOCK_STREAM\n");
        Mutex::Lock _l(_tcpconns_m);
        new_tcp_PCB = lwipstack->__tcp_new();
    }
    if(new_udp_PCB || new_tcp_PCB) {
        conn->sock = sock;
        conn->type = socket_type;
        if(conn->type == SOCK_DGRAM) conn->UDP_pcb = new_udp_PCB;
        if(conn->type == SOCK_STREAM) conn->TCP_pcb = new_tcp_PCB;
        dwr(MSG_DEBUG, " handleSocketProxy(): Updated sock=%p\n", (void*)&sock);
        return conn;
    }
	dwr(MSG_ERROR," handleSocketProxy(): Memory not available for new PCB\n");
	return NULL;
}

Connection * NetconEthernetTap::handleSocket(PhySocket *sock, void **uptr, struct socket_st* socket_rpc)
{
    dwr(MSG_DEBUG, "handleSocket(sock=%p)\n", (void*)&sock);
    struct udp_pcb *new_udp_PCB = NULL;
    struct tcp_pcb *new_tcp_PCB = NULL;
    if(socket_rpc->socket_type == SOCK_DGRAM) {
        dwr(MSG_DEBUG, "handleSocket(): SOCK_DGRAM\n");
        Mutex::Lock _l(_tcpconns_m);
        new_udp_PCB = lwipstack->__udp_new();
    }
    else if(socket_rpc->socket_type == SOCK_STREAM) {
        dwr(MSG_DEBUG, "handleSocket(): SOCK_STREAM\n");
        Mutex::Lock _l(_tcpconns_m);
        new_tcp_PCB = lwipstack->__tcp_new();
    }
    if(new_udp_PCB || new_tcp_PCB) {
        Connection * newConn = new Connection();
        *uptr = newConn;
        newConn->type = socket_rpc->socket_type;
        newConn->sock = sock;
        if(newConn->type == SOCK_DGRAM) newConn->UDP_pcb = new_udp_PCB;
        if(newConn->type == SOCK_STREAM) newConn->TCP_pcb = new_tcp_PCB;
        _Connections.push_back(newConn);
        return newConn;
    }
	dwr(MSG_ERROR," handleSocket(): Memory not available for new PCB\n");
	sendReturnValue(_phy.getDescriptor(sock), -1, ENOMEM);
	return NULL;
}

int NetconEthernetTap::handleConnectProxy(PhySocket *sock, struct sockaddr_in *rawAddr)
{
    dwr(MSG_DEBUG, "handleConnectProxy(%p)\n", (void*)&sock);
    Mutex::Lock _l(_tcpconns_m);
    int port = rawAddr->sin_port;
	ip_addr_t connAddr = convert_ip(rawAddr);
    int err = 0;

    Connection *conn = getConnection(sock);
    if(!conn) {
    	dwr(MSG_DEBUG, " handleConnectProxy(): Unable to locate Connection object for (sock=%p)\n", (void*)&sock);
    	return -1;
    }
    if(conn->type == SOCK_DGRAM) {
        // Generates no network traffic
        if((err = lwipstack->__udp_connect(conn->UDP_pcb,&connAddr,port)) < 0)
            dwr(MSG_DEBUG, "handleConnectProxy(): Error while connecting to with UDP\n");
        lwipstack->__udp_recv(conn->UDP_pcb, nc_udp_recved, new Larg(this, conn));
        errno = ERR_OK;
        return 0;
    }
	if(conn != NULL) {
		lwipstack->__tcp_sent(conn->TCP_pcb, nc_sent);
		lwipstack->__tcp_recv(conn->TCP_pcb, nc_recved);
		lwipstack->__tcp_err(conn->TCP_pcb, nc_err);
		lwipstack->__tcp_poll(conn->TCP_pcb, nc_poll, APPLICATION_POLL_FREQ);
		lwipstack->__tcp_arg(conn->TCP_pcb, new Larg(this, conn));
        
		int ip = rawAddr->sin_addr.s_addr;
		unsigned char d[4];
		d[0] = ip & 0xFF;
		d[1] = (ip >>  8) & 0xFF;
		d[2] = (ip >> 16) & 0xFF;
		d[3] = (ip >> 24) & 0xFF;
		dwr(MSG_DEBUG," handleConnectProxy(): %d.%d.%d.%d: %d\n", d[0],d[1],d[2],d[3], port);	
		dwr(MSG_DEBUG," handleConnectProxy(): pcb->state = %x\n", conn->TCP_pcb->state);
		if(conn->TCP_pcb->state != CLOSED) {
			dwr(MSG_DEBUG," handleConnectProxy(): PCB != CLOSED, cannot connect using this PCB\n");
			errno = EAGAIN;
			return -1;
		}
		if((err = lwipstack->__tcp_connect(conn->TCP_pcb,&connAddr,port,nc_connected_proxy)) < 0)
		{
			if(err == ERR_ISCONN) {
				errno = EISCONN; // Already in connected state
				return -1;
			} if(err == ERR_USE) {
				errno = EADDRINUSE; // Already in use
				return -1;
			} if(err == ERR_VAL) {
				errno = EINVAL; // Invalid ipaddress parameter
				return -1;
			} if(err == ERR_RTE) {
				errno = ENETUNREACH; // No route to host
				return -1;
			} if(err == ERR_BUF) {
				errno = EAGAIN; // No more ports available
				return -1;
			}
			if(err == ERR_MEM) {
				/* Can occur for the following reasons: tcp_enqueue_flags()

				1) tcp_enqueue_flags is always called with either SYN or FIN in flags.
				  We need one available snd_buf byte to do that.
				  This means we can't send FIN while snd_buf==0. A better fix would be to
				  not include SYN and FIN sequence numbers in the snd_buf count.

				2) Cannot allocate new pbuf
				3) Cannot allocate new TCP segment

				*/
				errno = EAGAIN; // FIXME: Doesn't describe the problem well, but closest match
				return -1;
			}
			// We should only return a value if failure happens immediately
			// Otherwise, we still need to wait for a callback from lwIP.
			// - This is because an ERR_OK from tcp_connect() only verifies
			//   that the SYN packet was enqueued onto the stack properly,
			//   that's it!
			// - Most instances of a retval for a connect() should happen
			//   in the nc_connect() and nc_err() callbacks!
			dwr(MSG_ERROR," handleConnectProxy(): unable to connect\n");
			errno = EAGAIN;
			return -1;
		}
		// Everything seems to be ok, but we don't have enough info to retval
		conn->listening=true;
        return 0;
	} else {
		dwr(MSG_ERROR," handleConnectProxy(): could not locate PCB based on their fd\n");
		errno = EBADF;
		return -1;
	}
    return -1;
}

void NetconEthernetTap::handleConnect(PhySocket *sock, PhySocket *rpcSock, Connection *conn, struct connect_st* connect_rpc)
{
    dwr(MSG_DEBUG, "handleConnect(%p)\n", (void*)&sock);
    Mutex::Lock _l(_tcpconns_m);
	struct sockaddr_in *rawAddr = (struct sockaddr_in *) &connect_rpc->__addr;
	int port = lwipstack->__lwip_ntohs(rawAddr->sin_port);
	ip_addr_t connAddr = convert_ip(rawAddr);
    int err = 0;
    
    if(conn->type == SOCK_DGRAM) {
        // Generates no network traffic
        if((err = lwipstack->__udp_connect(conn->UDP_pcb,&connAddr,port)) < 0)
            dwr(MSG_DEBUG, "handleConnect(): Error while connecting to with UDP\n");
        lwipstack->__udp_recv(conn->UDP_pcb, nc_udp_recved, new Larg(this, conn));
        sendReturnValue(rpcSock, 0, ERR_OK);
        return;
    }
	if(conn != NULL) {
		lwipstack->__tcp_sent(conn->TCP_pcb, nc_sent);
		lwipstack->__tcp_recv(conn->TCP_pcb, nc_recved);
		lwipstack->__tcp_err(conn->TCP_pcb, nc_err);
		lwipstack->__tcp_poll(conn->TCP_pcb, nc_poll, APPLICATION_POLL_FREQ);
		lwipstack->__tcp_arg(conn->TCP_pcb, new Larg(this, conn));
        
		int ip = rawAddr->sin_addr.s_addr;
		unsigned char d[4];
		d[0] = ip & 0xFF;
		d[1] = (ip >>  8) & 0xFF;
		d[2] = (ip >> 16) & 0xFF;
		d[3] = (ip >> 24) & 0xFF;
		dwr(MSG_DEBUG," handleConnect(): %d.%d.%d.%d: %d\n", d[0],d[1],d[2],d[3], port);	
		dwr(MSG_DEBUG," handleConnect(): pcb->state = %x\n", conn->TCP_pcb->state);
		if(conn->TCP_pcb->state != CLOSED) {
			dwr(MSG_DEBUG," handleConnect(): PCB != CLOSED, cannot connect using this PCB\n");
			sendReturnValue(rpcSock, -1, EAGAIN);
			return;
		}
		if((err = lwipstack->__tcp_connect(conn->TCP_pcb,&connAddr,port,nc_connected)) < 0)
		{
			if(err == ERR_ISCONN) {
				sendReturnValue(rpcSock, -1, EISCONN); // Already in connected state
				return;
			} if(err == ERR_USE) {
				sendReturnValue(rpcSock, -1, EADDRINUSE); // Already in use
				return;
			} if(err == ERR_VAL) {
				sendReturnValue(rpcSock, -1, EINVAL); // Invalid ipaddress parameter
				return;
			} if(err == ERR_RTE) {
				sendReturnValue(rpcSock, -1, ENETUNREACH); // No route to host
				return;
			} if(err == ERR_BUF) {
				sendReturnValue(rpcSock, -1, EAGAIN); // No more ports available
				return;
			}
			if(err == ERR_MEM) {
				/* Can occur for the following reasons: tcp_enqueue_flags()

				1) tcp_enqueue_flags is always called with either SYN or FIN in flags.
				  We need one available snd_buf byte to do that.
				  This means we can't send FIN while snd_buf==0. A better fix would be to
				  not include SYN and FIN sequence numbers in the snd_buf count.

				2) Cannot allocate new pbuf
				3) Cannot allocate new TCP segment

				*/
				sendReturnValue(rpcSock, -1, EAGAIN); // FIXME: Doesn't describe the problem well, but closest match
				return;
			}

			// We should only return a value if failure happens immediately
			// Otherwise, we still need to wait for a callback from lwIP.
			// - This is because an ERR_OK from tcp_connect() only verifies
			//   that the SYN packet was enqueued onto the stack properly,
			//   that's it!
			// - Most instances of a retval for a connect() should happen
			//   in the nc_connect() and nc_err() callbacks!
			dwr(MSG_ERROR," handleConnect(): unable to connect\n");
			sendReturnValue(rpcSock, -1, EAGAIN);
		}
		// Everything seems to be ok, but we don't have enough info to retval
		conn->listening=true;
		conn->rpcSock=rpcSock; // used for return value from lwip CB
	} else {
		dwr(MSG_ERROR," handleConnect(): could not locate PCB based on their fd\n");
		sendReturnValue(rpcSock, -1, EBADF);
	}
}

void NetconEthernetTap::handleWrite(Connection *conn)
{
    dwr(MSG_DEBUG, "handleWrite(conn=%p)\n", (void*)&conn);
	if(!conn || (!conn->TCP_pcb && !conn->UDP_pcb)) {
		dwr(MSG_ERROR," handleWrite(): invalid connection\n");
		return;
	}
    if(conn->type == SOCK_DGRAM) {
        if(!conn->UDP_pcb) {
            dwr(MSG_DEBUG, " handleWrite(): type = SOCK_DGRAM, invalid UDP_pcb\n");
            return;
        }
        // TODO: Packet re-assembly hasn't yet been tested with lwIP so UDP packets are limited to MTU-sized chunks
        int udp_trans_len = conn->txsz < ZT_UDP_DEFAULT_PAYLOAD_MTU ? conn->txsz : ZT_UDP_DEFAULT_PAYLOAD_MTU;
        
        dwr(MSG_DEBUG_EXTRA, " handleWrite(): Allocating pbuf chain of size (%d) for UDP packet, TXSZ = %d\n", udp_trans_len, conn->txsz);
        struct pbuf * pb = lwipstack->__pbuf_alloc(PBUF_TRANSPORT, udp_trans_len, PBUF_POOL);
        if(!pb){
            dwr(MSG_DEBUG, " handleWrite(): unable to allocate new pbuf of size (%d)\n", conn->txsz);
            return;
        }
        memcpy(pb->payload, conn->txbuf, udp_trans_len);
        int err = lwipstack->__udp_send(conn->UDP_pcb, pb);
        
        if(err == ERR_MEM) {
            dwr(MSG_DEBUG, " handleWrite(): Error sending packet. Out of memory\n");
        } else if(err == ERR_RTE) {
            dwr(MSG_DEBUG, " handleWrite(): Could not find route to destinations address\n");
        } else if(err != ERR_OK) {
            dwr(MSG_DEBUG, " handleWrite(): Error sending packet - %d\n", err);
        } else {
            int buf_remaining = (conn->txsz)-udp_trans_len;
            if(buf_remaining)
                memmove(&conn->txbuf, (conn->txbuf+udp_trans_len), buf_remaining);
            conn->txsz -= udp_trans_len;
            int max = conn->type == SOCK_STREAM ? DEFAULT_TCP_TX_BUF_SZ : DEFAULT_UDP_TX_BUF_SZ;
            dwr(MSG_TRANSFER," TX --->    :: {TX: %.3f%%, RX: %.3f%%, sock=%x} :: %d bytes\n",
                (float)conn->txsz / (float)max, (float)conn->rxsz / max, conn->sock, udp_trans_len);
        }
        lwipstack->__pbuf_free(pb);
        return;
    }
    else if(conn->type == SOCK_STREAM) {
        if(!conn->TCP_pcb) {
            dwr(MSG_DEBUG, " handleWrite(): type = SOCK_STREAM, invalid TCP_pcb\n");
            return;
        }
        // How much we are currently allowed to write to the connection
        int sndbuf = conn->TCP_pcb->snd_buf;
        int err, sz, r;
    
        if(!sndbuf) {
            // PCB send buffer is full, turn off readability notifications for the
            // corresponding PhySocket until nc_sent() is called and confirms that there is
            // now space on the buffer
            if(!conn->probation) {
                dwr(MSG_DEBUG," handleWrite(): sndbuf == 0, LWIP stack is full\n");
                _phy.setNotifyReadable(conn->sock, false);
                conn->probation = true;
            }
            return;
        }
        if(conn->txsz <= 0)
            return; // Nothing to write
        if(!conn->listening)
            lwipstack->__tcp_output(conn->TCP_pcb);

        if(conn->sock) {
            r = conn->txsz < sndbuf ? conn->txsz : sndbuf;
            // Writes data pulled from the client's socket buffer to LWIP. This merely sends the
            // data to LWIP to be enqueued and eventually sent to the network.
            if(r > 0) {
                err = lwipstack->__tcp_write(conn->TCP_pcb, &conn->txbuf, r, TCP_WRITE_FLAG_COPY);
                lwipstack->__tcp_output(conn->TCP_pcb);
                if(err != ERR_OK) {
                    dwr(MSG_ERROR," handleWrite(): error while writing to PCB, (err = %d)\n", err);
                    if(err == -1)
                        dwr(MSG_DEBUG," handleWrite(): out of memory\n");
                    return;
                } else {
                    sz = (conn->txsz)-r;
                    if(sz)
                        memmove(&conn->txbuf, (conn->txbuf+r), sz);
                    conn->txsz -= r;
                    int max = conn->type == SOCK_STREAM ? DEFAULT_TCP_TX_BUF_SZ : DEFAULT_UDP_TX_BUF_SZ;
                    dwr(MSG_TRANSFER," TX --->    :: {TX: %.3f%%, RX: %.3f%%, sock=%p} :: %d bytes\n",
                        (float)conn->txsz / (float)max, (float)conn->rxsz / max, (void*)&conn->sock, r);
                    return;
                }
            }
        }
    }
}

} // namespace ZeroTier

