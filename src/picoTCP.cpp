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



#include "pico_eth.h"
#include "pico_stack.h"
#include "pico_ipv4.h"
#include "pico_icmp4.h"
#include "pico_dev_tap.h"
#include "pico_protocol.h"
#include "pico_socket.h"
#include "pico_device.h"
#include "pico_ipv6.h"

#include "ZeroTierSDK.h"
#include "SocketTap.hpp"
#include "picoTCP.hpp"

#include "Utils.hpp"
#include "OSUtils.hpp"
#include "Mutex.hpp"
#include "Constants.hpp"
#include "Phy.hpp"

// stack locks
ZeroTier::Mutex _lock;        
ZeroTier::Mutex _lock_mem;

struct pico_socket;
struct pico_device;

extern "C" int pico_stack_init(void);
extern "C" void pico_stack_tick(void);

int pico_ipv4_to_string(PICO_IPV4_TO_STRING_SIG);
extern "C" int pico_ipv4_link_add(PICO_IPV4_LINK_ADD_SIG);
extern "C" int pico_device_init(PICO_DEVICE_INIT_SIG);
int pico_stack_recv(PICO_STACK_RECV_SIG);
int pico_icmp4_ping(PICO_ICMP4_PING_SIG);
extern "C" int pico_string_to_ipv4(PICO_STRING_TO_IPV4_SIG);
extern "C" int pico_string_to_ipv6(PICO_STRING_TO_IPV6_SIG);
int pico_socket_setoption(PICO_SOCKET_SETOPTION_SIG);
uint32_t pico_timer_add(PICO_TIMER_ADD_SIG);
int pico_socket_send(PICO_SOCKET_SEND_SIG);
int pico_socket_sendto(PICO_SOCKET_SENDTO_SIG);
int pico_socket_recv(PICO_SOCKET_RECV_SIG);
extern "C" int pico_socket_recvfrom(PICO_SOCKET_RECVFROM_SIG);
extern "C" struct pico_socket * pico_socket_open(PICO_SOCKET_OPEN_SIG);
int pico_socket_bind(PICO_SOCKET_BIND_SIG);
int pico_socket_connect(PICO_SOCKET_CONNECT_SIG);
extern "C" int pico_socket_listen(PICO_SOCKET_LISTEN_SIG);
int pico_socket_read(PICO_SOCKET_READ_SIG);
extern "C" int pico_socket_write(PICO_SOCKET_WRITE_SIG);
extern "C" int pico_socket_close(PICO_SOCKET_CLOSE_SIG);
int pico_socket_shutdown(PICO_SOCKET_SHUTDOWN_SIG);
struct pico_socket * pico_socket_accept(PICO_SOCKET_ACCEPT_SIG);
extern "C" struct pico_ipv6_link * pico_ipv6_link_add(PICO_IPV6_LINK_ADD_SIG);


namespace ZeroTier {

	// Reference to the tap interface
	// This is needed due to the fact that there's a lot going on in the tap interface
	// that needs to be updated on each of the network stack's callbacks and not every
	// network stack provides a mechanism for storing a reference to the tap.
	//
	// In future releases this will be replaced with a new structure of static pointers that 
	// will make it easier to maintain multiple active tap interfaces

	struct pico_device picodev;
	SocketTap * picotap;

    int pico_eth_send(struct pico_device *dev, void *buf, int len);
    int pico_eth_poll(struct pico_device *dev, int loop_score);

    // Initialize network stack's interfaces and assign addresses
	void picoTCP::pico_init_interface(SocketTap *tap, const InetAddress &ip)
	{
		if (std::find(tap->_ips.begin(),tap->_ips.end(),ip) == tap->_ips.end()) {
			tap->_ips.push_back(ip);
			std::sort(tap->_ips.begin(),tap->_ips.end());
		#if defined(SDK_IPV4)
			if(ip.isV4())
			{
			    struct pico_ip4 ipaddr, netmask;
			    ipaddr.addr = *((uint32_t *)ip.rawIpData());
			    netmask.addr = *((uint32_t *)ip.netmask().rawIpData());
			    uint8_t mac[PICO_SIZE_ETH];
			    tap->_mac.copyTo(mac, PICO_SIZE_ETH);
			    DEBUG_ATTN("mac = %s", tap->_mac.toString().c_str());
			    picodev.send = pico_eth_send; // tx
			    picodev.poll = pico_eth_poll; // rx
			    picodev.mtu = tap->_mtu;
			    if( 0 != pico_device_init(&(picodev), "p0", mac)) {
			        DEBUG_ERROR("device init failed");
			        return;
			    }
			    pico_ipv4_link_add(&(picodev), ipaddr, netmask);
			    // DEBUG_INFO("device initialized as ipv4_addr = %s", ipv4_str);
			   	// pico_icmp4_ping("10.8.8.1", 20, 1000, 10000, 64, cb_ping);
			}
		#elif defined(SDK_IPV6)
			if(ip.isV6())
			{
				struct pico_ip6 ipaddr, netmask;
				char ipv6_str[INET6_ADDRSTRLEN], nm_str[INET6_ADDRSTRLEN];
				inet_ntop(AF_INET6, ip.rawIpData(), ipv6_str, INET6_ADDRSTRLEN);
				inet_ntop(AF_INET6, ip.netmask().rawIpData(), nm_str, INET6_ADDRSTRLEN);
		    	pico_string_to_ipv6(ipv6_str, ipaddr.addr);
		    	pico_string_to_ipv6(nm_str, netmask.addr);
			    pico_ipv6_link_add(&(picodev), ipaddr, netmask);
			    picodev.send = pico_eth_send; // tx
			    picodev.poll = pico_eth_poll; // rx
			    uint8_t mac[PICO_SIZE_ETH];
			    tap->_mac.copyTo(mac, PICO_SIZE_ETH);
			    DEBUG_ATTN("mac = %s", tap->_mac.toString().c_str());
			    if( 0 != pico_device_init(&(picodev), "p0", mac)) {
			        DEBUG_ERROR("device init failed");
			        return;
			    }
			    DEBUG_ATTN("device initialized as ipv6_addr = %s", ipv6_str);
			}
		#endif
		}
	}
	
	// Main stack loop
	void picoTCP::pico_loop(SocketTap *tap)
	{
		while(tap->_run)
		{
			tap->_phy.poll(ZT_PHY_POLL_INTERVAL); // in ms
	        pico_stack_tick();
		}
	}

	// RX packets from [ZT->STACK] onto RXBUF
	// Also notify the tap service that data can be read:
	// [RXBUF -> (ZTSOCK->APP)]
   	// -----------------------------------------
	// | TAP <-> MEM BUFFER <-> STACK <-> APP  |
    // |                                       | 
    // | APP <-> I/O BUFFER <-> STACK <-> TAP  |
    // |         |<-----------------|          | RX
    // ----------------------------------------- 
    // After this step, buffer will be emptied periodically by pico_handleRead()
	void picoTCP::pico_cb_tcp_read(ZeroTier::SocketTap *tap, struct pico_socket *s)
	{
		Connection *conn = tap->getConnection(s);
		if(conn) {
			int r;				
			uint16_t port = 0;
			union {
		        struct pico_ip4 ip4;
		        struct pico_ip6 ip6;
		    } peer;

			do {
				int avail = DEFAULT_TCP_RX_BUF_SZ - conn->rxsz;
				if(avail) {
		            r = pico_socket_recvfrom(s, conn->rxbuf + (conn->rxsz), SDK_MTU, (void *)&peer.ip4.addr, &port);
		            // DEBUG_ATTN("received packet (%d byte) from %08X:%u", r, long_be2(peer.ip4.addr), short_be(port));
		            tap->_phy.setNotifyWritable(conn->sock, true);
		            if (r > 0)
		                conn->rxsz += r;
	        	}
	        	else
	        		DEBUG_ERROR("not enough space left on I/O RX buffer for pico_socket(%p)", s);
            }
        	while(r > 0);
        	return;
        }
		DEBUG_ERROR("invalid connection");
	}

	// RX packets from the stack onto internal buffer
	// Also notifies the tap service that data can be read
   	// -----------------------------------------
	// | TAP <-> MEM BUFFER <-> STACK <-> APP  |
    // |                                       | 
    // | APP <-> I/O BUFFER <-> STACK <-> TAP  |
    // |         |<-----------------|          | RX
    // ----------------------------------------- 
    // After this step, buffer will be emptied periodically by pico_handleRead()
    // Read payload is encapsulated as such:
    //
    // [addr|payload_len|payload]
    //
	void picoTCP::pico_cb_udp_read(SocketTap *tap, struct pico_socket *s)
	{
		Connection *conn = tap->getConnection(s);
		if(conn) {

			uint16_t port = 0;
			union {
		        struct pico_ip4 ip4;
		        struct pico_ip6 ip6;
		    } peer;

		    char tmpbuf[SDK_MTU];
        	unsigned char *addr_pos, *sz_pos, *payload_pos;
			struct sockaddr_in addr_in;
			addr_in.sin_addr.s_addr = peer.ip4.addr;
        	addr_in.sin_port = port;
			
        	// RX
        	int r = pico_socket_recvfrom(s, tmpbuf, SDK_MTU, (void *)&peer.ip4.addr, &port);
            //DEBUG_FLOW(" [  RXBUF <- STACK] Receiving (%d) from stack, copying to receving buffer", r);

			// Mutex::Lock _l2(tap->_rx_buf_m);
			// struct sockaddr_in6 addr_in6;
        	// addr_in6.sin6_addr.s6_addr;
        	// addr_in6.sin6_port = Utils::ntoh(s->remote_port);
			// DEBUG_ATTN("remote_port=%d, local_port=%d", s->remote_port, Utils::ntoh(s->local_port));
            tap->_rx_buf_m.lock(); 
			if(conn->rxsz == DEFAULT_UDP_RX_BUF_SZ) { // if UDP buffer full
                //DEBUG_FLOW(" [  RXBUF <- STACK] UDP RX buffer full. Discarding oldest payload segment");
                memmove(conn->rxbuf, conn->rxbuf + SDK_MTU, DEFAULT_UDP_RX_BUF_SZ - SDK_MTU);
                addr_pos = conn->rxbuf + (DEFAULT_UDP_RX_BUF_SZ - SDK_MTU); // TODO:
                sz_pos = addr_pos + sizeof(struct sockaddr_storage);
                conn->rxsz -= SDK_MTU;
            }
            else {
                addr_pos = conn->rxbuf + conn->rxsz; // where we'll prepend the size of the address
                sz_pos = addr_pos + sizeof(struct sockaddr_storage);
            }
            payload_pos = addr_pos + sizeof(struct sockaddr_storage) + sizeof(r);
           	memcpy(addr_pos, &addr_in, sizeof(struct sockaddr_storage));

        	memcpy(payload_pos, tmpbuf, r); // write payload to app's socket

            // Adjust buffer size
			if(r) {
	            conn->rxsz += SDK_MTU;
	            memcpy(sz_pos, &r, sizeof(r));
	        }
	        if (r < 0) {
            	DEBUG_ERROR("unable to read from picosock=%p", s);
    		} 
	        tap->_rx_buf_m.unlock(); 

	        // TODO: Revisit logic
	        if(r)
	            tap->phyOnUnixWritable(conn->sock, NULL, true);
        	//DEBUG_EXTRA(" Copied onto rxbuf (%d) from stack socket", r);
        	return;
		}
	}

	// TX packets from internal buffer to network
	void picoTCP::pico_cb_tcp_write(SocketTap *tap, struct pico_socket *s)
	{
		Connection *conn = tap->getConnection(s);
		if(!conn)
			DEBUG_ERROR("invalid connection");
		if(!conn->txsz)
			return;
		// Only called from a locked context, no need to lock anything
		if(conn->txsz > 0) {
			int r, max_write_len = conn->txsz < SDK_MTU ? conn->txsz : SDK_MTU;
			if((r = pico_socket_write(s, &conn->txbuf, max_write_len)) < 0) {
				DEBUG_ERROR("unable to write to picosock=%p", s);
				return;
			}
			int sz = (conn->txsz)-r;
            if(sz)
                memmove(&conn->txbuf, (conn->txbuf+r), sz);
            conn->txsz -= r;

            #if DEBUG_LEVEL >= MSG_TRANSFER
            	int max = conn->type == SOCK_STREAM ? DEFAULT_TCP_TX_BUF_SZ : DEFAULT_UDP_TX_BUF_SZ;
            	DEBUG_TRANS("[TCP TX] --->    :: {TX: %.3f%%, RX: %.3f%%, physock=%p} :: %d bytes",
                	(float)conn->txsz / (float)max, (float)conn->rxsz / max, conn->sock, r);
        	#endif

            return;
		}
	}

	// Main callback for TCP connections
	void picoTCP::pico_cb_socket_activity(uint16_t ev, struct pico_socket *s)
    {
    	int err;
        Mutex::Lock _l(picotap->_tcpconns_m);
        Connection *conn = picotap->getConnection(s);
        if(!conn) {
        	DEBUG_ERROR("invalid connection");
        }
        // Accept connection (analogous to lwip_nc_accept)
        if (ev & PICO_SOCK_EV_CONN) {
            DEBUG_INFO("connection established with server, picosock=%p",(conn->picosock));
            uint32_t peer;
			uint16_t port;
            struct pico_socket *client = pico_socket_accept(s, &peer, &port);
            if(!client) {
				DEBUG_EXTRA("unable to accept conn. (event might not be incoming, not necessarily an error), picosock=%p", (conn->picosock));
			}
			ZT_PHY_SOCKFD_TYPE fds[2];
			if(socketpair(PF_LOCAL, SOCK_STREAM, 0, fds) < 0) {
				if(errno < 0) {
					// FIXME: Return a value to the client
					//tap->sendReturnValue(conn, -1, errno);
					DEBUG_ERROR("unable to create socketpair");
					return;
				}
			}
			Connection *newTcpConn = new Connection();
			picotap->_Connections.push_back(newTcpConn);
			newTcpConn->type = SOCK_STREAM;
			newTcpConn->sock = picotap->_phy.wrapSocket(fds[0], newTcpConn);
			newTcpConn->picosock = client;
			int fd = picotap->_phy.getDescriptor(conn->sock);
			if(sock_fd_write(fd, fds[1]) < 0) {
				DEBUG_ERROR("error sending new fd to client application");
			}
			DEBUG_EXTRA("conn=%p, physock=%p, listen_picosock=%p, new_picosock=%p, fd=%d", newTcpConn, newTcpConn->sock, s, client, fds[1]);
        }
        if (ev & PICO_SOCK_EV_FIN) {
            DEBUG_INFO("socket closed. exit normally. picosock=%p\n\n", s);
            //pico_timer_add(2000, compare_results, NULL);
        }
        if (ev & PICO_SOCK_EV_ERR) {
            DEBUG_INFO("socket error received" /*, strerror(pico_err)*/);
        }
        if (ev & PICO_SOCK_EV_CLOSE) {
            err = pico_socket_close(s);
            DEBUG_INFO("socket closure = %d, picosock=%p", err, s);
            if(err==0) {
            	picotap->closeConnection(conn->sock);
            }
            return;
        }
        // Read from picoTCP socket
        if (ev & PICO_SOCK_EV_RD) {
        	if(conn->type==SOCK_STREAM)
            	pico_cb_tcp_read(picotap, s);
        	if(conn->type==SOCK_DGRAM)
        		pico_cb_udp_read(picotap, s);
        }
        // Write to picoTCP socket
        if (ev & PICO_SOCK_EV_WR) {
            pico_cb_tcp_write(picotap, s);
        }
    }

    // Called when an incoming ping is received
    /*
    static void pico_cb_ping(struct pico_icmp4_stats *s)
    {   
    	DEBUG_INFO();
        char host[30];
        pico_ipv4_to_string(host, s->dst.addr);
        if (s->err == 0) {
            printf("%lu bytes from %s: icmp_req=%lu ttl=%lu time=%lu ms\n", s->size,
                    host, s->seq, s->ttl, (long unsigned int)s->time);
        } else {
            printf("PING %lu to %s: Error %d\n", s->seq, host, s->err);
        }
    }
	*/

    // Called from the stack, sends data to the tap device (in our case, the ZeroTier service)
   	// -----------------------------------------
	// | TAP <-> MEM BUFFER <-> STACK <-> APP  |
    // | |<-------------------------|          | TX 
    // | APP <-> I/O BUFFER <-> STACK <-> TAP  |
    // |                                       |
    // -----------------------------------------     
   	int pico_eth_send(struct pico_device *dev, void *buf, int len)
    {
        struct pico_eth_hdr *ethhdr;
        ethhdr = (struct pico_eth_hdr *)buf;

        MAC src_mac;
        MAC dest_mac;
        src_mac.setTo(ethhdr->saddr, 6);
        dest_mac.setTo(ethhdr->daddr, 6);

        picotap->_handler(picotap->_arg,NULL,picotap->_nwid,src_mac,dest_mac,
            Utils::ntoh((uint16_t)ethhdr->proto),0, ((char*)buf) + sizeof(struct pico_eth_hdr),len - sizeof(struct pico_eth_hdr));
        return len;
    }

    // Receives data from the tap device and encapsulates it into a ZeroTier ethernet frame and places it in a locked memory buffer
   	// -----------------------------------------
	// | TAP <-> MEM BUFFER <-> STACK <-> APP  |
    // | |--------------->|                    | RX 
    // | APP <-> I/O BUFFER <-> STACK <-> TAP  |
    // |                                       |
    // ----------------------------------------- 
    // It will then periodically be transfered into the network stack via pico_eth_poll()
    void picoTCP::pico_rx(SocketTap *tap, const MAC &from,const MAC &to,unsigned int etherType,const void *data,unsigned int len)
	{
		// Since picoTCP only allows the reception of frames from within the polling function, we
		// must enqueue each frame into a memory structure shared by both threads. This structure will
		Mutex::Lock _l(tap->_pico_frame_rxbuf_m);

		// assemble new eth header
		struct pico_eth_hdr ethhdr;
		from.copyTo(ethhdr.saddr, 6);
		to.copyTo(ethhdr.daddr, 6);
		ethhdr.proto = Utils::hton((uint16_t)etherType);
		int newlen = len + sizeof(int) + sizeof(struct pico_eth_hdr);

		int mylen;
		while(newlen > (MAX_PICO_FRAME_RX_BUF_SZ-tap->pico_frame_rxbuf_tot) && ethhdr.proto == 56710)
		{
			mylen = 0;
			//DEBUG_FLOW(" [ ZTWIRE -> FBUF ] not enough space left on RX frame buffer, dropping oldest packet in buffer");
			/*
        	memcpy(&mylen, picotap->pico_frame_rxbuf, sizeof(len));
			memmove(tap->pico_frame_rxbuf, tap->pico_frame_rxbuf + mylen, MAX_PICO_FRAME_RX_BUF_SZ-mylen); // shift buffer
			picotap->pico_frame_rxbuf_tot-=mylen;
			*/
			memset(tap->pico_frame_rxbuf,0,MAX_PICO_FRAME_RX_BUF_SZ);
			picotap->pico_frame_rxbuf_tot=0;
		}
		memcpy(tap->pico_frame_rxbuf + tap->pico_frame_rxbuf_tot, &newlen, sizeof(newlen));                      // size of frame + meta		
		memcpy(tap->pico_frame_rxbuf + tap->pico_frame_rxbuf_tot + sizeof(newlen), &ethhdr, sizeof(ethhdr));     // new eth header
		memcpy(tap->pico_frame_rxbuf + tap->pico_frame_rxbuf_tot + sizeof(newlen) + sizeof(ethhdr), data, len);  // frame data
		tap->pico_frame_rxbuf_tot += newlen;
		DEBUG_FLOW(" [ ZTWIRE -> FBUF ] Move FRAME(sz=%d) into FBUF(sz=%d), data_len=%d", newlen, picotap->pico_frame_rxbuf_tot, len);
	}

	// Called periodically by the stack, this removes data from the locked memory buffer (FBUF) and feeds it into the stack.
	// A maximum of 'loop_score' frames can be processed in each call
   	// -----------------------------------------
	// | TAP <-> MEM BUFFER <-> STACK <-> APP  |
    // |         |----------------->|          | RX 
    // | APP <-> I/O BUFFER <-> STACK <-> TAP  |
    // |                                       |
    // ----------------------------------------- 
    int pico_eth_poll(struct pico_device *dev, int loop_score)
    {
        // OPTIMIZATION: The copy logic and/or buffer structure should be reworked for better performance after the BETA
        // SocketTap *tap = (SocketTap*)netif->state;
        Mutex::Lock _l(picotap->_pico_frame_rxbuf_m);
        unsigned char frame[SDK_MTU];
        int len;
        while (picotap->pico_frame_rxbuf_tot > 0 && loop_score > 0) {
        	//DEBUG_FLOW(" [   FBUF -> STACK] Frame buffer SZ=%d", picotap->pico_frame_rxbuf_tot);
            memset(frame, 0, sizeof(frame));
            len = 0;
            memcpy(&len, picotap->pico_frame_rxbuf, sizeof(len)); // get frame len
            if(len >= 0) {
            	//DEBUG_FLOW(" [   FBUF -> STACK]   Moving FRAME of size (%d) from FBUF(sz=%d) into stack",len, picotap->pico_frame_rxbuf_tot-len);
            	memcpy(frame, picotap->pico_frame_rxbuf + sizeof(len), len-(sizeof(len)) ); // get frame data
            	memmove(picotap->pico_frame_rxbuf, picotap->pico_frame_rxbuf + len, MAX_PICO_FRAME_RX_BUF_SZ-len); // shift buffer
            	pico_stack_recv(dev, (uint8_t*)frame, (len-sizeof(len))); 
                picotap->pico_frame_rxbuf_tot-=len;
            }
            else {
            	DEBUG_ERROR("Skipping frame of size (%d)",len);
            	exit(0);
            }
            loop_score--;
        }
        return loop_score;
    }

    // Creates a new pico_socket and Connection object to represent a new connection to be.
    Connection *picoTCP::pico_handleSocket(PhySocket *sock, void **uptr, struct socket_st* socket_rpc)
    {
    	struct pico_socket * psock;
    	int protocol, protocol_version;

		#if defined(SDK_IPV4)
			protocol_version = PICO_PROTO_IPV4;
		#elif defined(SDK_IPV6)
			protocol_version = PICO_PROTO_IPV6;
		#endif
		if(socket_rpc->socket_type == SOCK_DGRAM) {
			protocol = PICO_PROTO_UDP;
			psock = pico_socket_open(protocol_version, protocol, &pico_cb_socket_activity);
		}
		if(socket_rpc->socket_type == SOCK_STREAM) {
			protocol = PICO_PROTO_TCP;
			psock = pico_socket_open(protocol_version, protocol, &pico_cb_socket_activity);
		}

		if(psock) {
			DEBUG_ATTN("physock=%p, picosock=%p", sock, psock);
			Connection * newConn = new Connection();
	        *uptr = newConn;
	        newConn->type = socket_rpc->socket_type;
	        newConn->sock = sock;
			
			/*
			int res = 0;
			int sendbuff = UNIX_SOCK_BUF_SIZE;
			socklen_t optlen = sizeof(sendbuff);

			res = setsockopt(picotap->_phy.getDescriptor(sock), SOL_SOCKET, SO_RCVBUF, &sendbuff, sizeof(sendbuff));
			if(res == -1)
				//DEBUG_ERROR("Error while setting RX buffer limits");
			res = setsockopt(picotap->_phy.getDescriptor(sock), SOL_SOCKET, SO_SNDBUF, &sendbuff, sizeof(sendbuff));
			if(res == -1)
				//DEBUG_ERROR("Error while setting TX buffer limits");

			// Get buffer size
			// optlen = sizeof(sendbuff);
			// res = getsockopt(picotap->_phy.getDescriptor(sock), SOL_SOCKET, SO_SNDBUF, &sendbuff, &optlen);
			// DEBUG_INFO("buflen=%d", sendbuff);
			*/
			
			newConn->local_addr = NULL;
			newConn->picosock = psock;
	        picotap->_Connections.push_back(newConn);
	        memset(newConn->rxbuf, 0, DEFAULT_UDP_RX_BUF_SZ);
	        return newConn;
		}
		else
			DEBUG_ERROR("failed to create pico_socket");
		return NULL;
    }

    // Writes data from the I/O buffer to the network stack
   	// -----------------------------------------
	// | TAP <-> MEM BUFFER <-> STACK <-> APP  |
    // |                                       | 
    // | APP <-> I/O BUFFER <-> STACK <-> TAP  |
    // |         |----------------->|          | TX
    // ----------------------------------------- 
    void picoTCP::pico_handleWrite(Connection *conn)
    {
		if(!conn || !conn->picosock) {
			DEBUG_ERROR(" invalid connection");
			return;
		}

		int max, r, max_write_len = conn->txsz < SDK_MTU ? conn->txsz : SDK_MTU;
	    if((r = pico_socket_write(conn->picosock, &conn->txbuf, max_write_len)) < 0) {
	    	DEBUG_ERROR("unable to write to picosock=%p, r=%d", (conn->picosock), r);
	    	return;
	    }
	   
	    // TODO: Errors

	    /*
	 	if(pico_err == PICO_ERR_EINVAL)
	 		DEBUG_ERROR("PICO_ERR_EINVAL - invalid argument");
		if(pico_err == PICO_ERR_EIO)
			DEBUG_ERROR("PICO_ERR_EIO - input/output error");
		if(pico_err == PICO_ERR_ENOTCONN)
			DEBUG_ERROR("PICO_ERR_ENOTCONN - the socket is not connected");
		if(pico_err == PICO_ERR_ESHUTDOWN)
			DEBUG_ERROR("PICO_ERR_ESHUTDOWN - cannot send after transport endpoint shutdown");
		if(pico_err == PICO_ERR_EADDRNOTAVAIL)
			DEBUG_ERROR("PICO_ERR_EADDRNOTAVAIL - address not available");
		if(pico_err == PICO_ERR_EHOSTUNREACH)
			DEBUG_ERROR("PICO_ERR_EHOSTUNREACH - host is unreachable");
		if(pico_err == PICO_ERR_ENOMEM)
			DEBUG_ERROR("PICO_ERR_ENOMEM - not enough space");
		if(pico_err == PICO_ERR_EAGAIN)
			DEBUG_ERROR("PICO_ERR_EAGAIN - resource temporarily unavailable");
		*/

	    // adjust buffer
	    int sz = (conn->txsz)-r;
	   	if(sz)
	   		memmove(&conn->txbuf, (conn->txbuf+r), sz);
		conn->txsz -= r;
	   	
	   	if(conn->type == SOCK_STREAM) {
	   		max = DEFAULT_TCP_TX_BUF_SZ;
	    	DEBUG_TRANS("[TCP TX] --->    :: {TX: %.3f%%, RX: %.3f%%, physock=%p} :: %d bytes",
	    		(float)conn->txsz / (float)max, (float)conn->rxsz / max, conn->sock, r);
	    }
	   	if(conn->type == SOCK_DGRAM) {
	   		max = DEFAULT_UDP_TX_BUF_SZ;
	    	DEBUG_TRANS("[UDP TX] --->    :: {TX: %.3f%%, RX: %.3f%%, physock=%p} :: %d bytes",
	    		(float)conn->txsz / (float)max, (float)conn->rxsz / max, conn->sock, r);
	    }
    }

    // Instructs the stack to connect to a remote host
    void picoTCP::pico_handleConnect(PhySocket *sock, PhySocket *rpcSock, Connection *conn, struct connect_st* connect_rpc)
    {
		if(conn->picosock) {
			struct sockaddr_in *addr = (struct sockaddr_in *) &connect_rpc->addr;
			int ret;
			// TODO: Rewrite this
			#if defined(SDK_IPV4)
				struct pico_ip4 zaddr;
    			struct sockaddr_in *in4 = (struct sockaddr_in*)&connect_rpc->addr;
    			char ipv4_str[INET_ADDRSTRLEN];
    			inet_ntop(AF_INET, &(in4->sin_addr), ipv4_str, INET_ADDRSTRLEN);
				pico_string_to_ipv4(ipv4_str, &(zaddr.addr));
				//DEBUG_ATTN("addr=%s:%d", ipv4_str, Utils::ntoh(addr->sin_port));
				ret = pico_socket_connect(conn->picosock, &zaddr, addr->sin_port);
			#elif defined(SDK_IPV6) // "fd56:5799:d8f6:1238:8c99:9322:30ce:418a"
				struct pico_ip6 zaddr;
				struct sockaddr_in6 *in6 = (struct sockaddr_in6*)&connect_rpc->addr;
				char ipv6_str[INET6_ADDRSTRLEN];
				inet_ntop(AF_INET6, &(in6->sin6_addr), ipv6_str, INET6_ADDRSTRLEN);
		    	pico_string_to_ipv6(ipv6_str, zaddr.addr);
		    	//DEBUG_ATTN("addr=%s:%d", ipv6_str, Utils::ntoh(addr->sin_port));
				ret = pico_socket_connect(conn->picosock, &zaddr, addr->sin_port);
			#endif
			
			memcpy(&(conn->peer_addr), &connect_rpc->addr, sizeof(struct sockaddr_storage));

			if(ret == PICO_ERR_EPROTONOSUPPORT)
				DEBUG_ERROR("PICO_ERR_EPROTONOSUPPORT");
			if(ret == PICO_ERR_EINVAL)
				DEBUG_ERROR("PICO_ERR_EINVAL");
			if(ret == PICO_ERR_EHOSTUNREACH)
				DEBUG_ERROR("PICO_ERR_EHOSTUNREACH");

	        picotap->sendReturnValue(picotap->_phy.getDescriptor(rpcSock), 0, ERR_OK);
		}
    }

    // Instructs the stack to bind to a given address
    void picoTCP::pico_handleBind(PhySocket *sock, PhySocket *rpcSock, void **uptr, struct bind_st *bind_rpc)
    {
    	Connection *conn = picotap->getConnection(sock);
    	if(!sock) {
    		DEBUG_ERROR("invalid connection");
    		return;
    	}
		struct sockaddr_in *addr = (struct sockaddr_in *) &bind_rpc->addr;
    	int ret;
    	// TODO: Rewrite this
		#if defined(SDK_IPV4)
			struct pico_ip4 zaddr;
			struct sockaddr_in *in4 = (struct sockaddr_in*)&bind_rpc->addr;
			char ipv4_str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(in4->sin_addr), ipv4_str, INET_ADDRSTRLEN);
			pico_string_to_ipv4(ipv4_str, &(zaddr.addr));
			DEBUG_ATTN("addr=%s:%d, physock=%p, picosock=%p", ipv4_str, Utils::ntoh(addr->sin_port), sock, (conn->picosock));
			ret = pico_socket_bind(conn->picosock, &zaddr, (uint16_t*)&(addr->sin_port));
		#elif defined(SDK_IPV6)
			struct pico_ip6 zaddr;
			struct sockaddr_in6 *in6 = (struct sockaddr_in6*)&bind_rpc->addr;
			char ipv6_str[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, &(in6->sin6_addr), ipv6_str, INET6_ADDRSTRLEN);
	    	pico_string_to_ipv6(ipv6_str, zaddr.addr);
	    	DEBUG_ATTN("addr=%s:%d, physock=%p, picosock=%p", ipv6_str, Utils::ntoh(addr->sin_port), sock, (conn->picosock));
			ret = pico_socket_bind(conn->picosock, &zaddr, (uint16_t*)&(addr->sin_port));
		#endif
		if(ret < 0) {
			DEBUG_ERROR("unable to bind pico_socket(%p), err=%d", (conn->picosock), ret);
			if(ret == PICO_ERR_EINVAL) {
				DEBUG_ERROR("PICO_ERR_EINVAL - invalid argument");
				picotap->sendReturnValue(picotap->_phy.getDescriptor(rpcSock), -1, EINVAL);
			} 
			if(ret == PICO_ERR_ENOMEM) {
				DEBUG_ERROR("PICO_ERR_ENOMEM - not enough space");
				picotap->sendReturnValue(picotap->_phy.getDescriptor(rpcSock), -1, ENOMEM);
			} 
			if(ret == PICO_ERR_ENXIO) {
				DEBUG_ERROR("PICO_ERR_ENXIO - no such device or address");
				picotap->sendReturnValue(picotap->_phy.getDescriptor(rpcSock), -1, ENXIO);
			}
		}
		picotap->sendReturnValue(picotap->_phy.getDescriptor(rpcSock), ERR_OK, ERR_OK); // success
    }

    // Puts a pico_socket into a listening state to receive incoming connection requests
    void picoTCP::pico_handleListen(PhySocket *sock, PhySocket *rpcSock, void **uptr, struct listen_st *listen_rpc)
    {
    	Connection *conn = picotap->getConnection(sock);
    	DEBUG_ATTN("physock=%p, conn=%p, picosock=%p", sock, conn, conn->picosock);
    	if(!sock || !conn) {
    		DEBUG_ERROR("invalid connection");
    		return;
    	}
    	int ret, backlog = 100;
    	if((ret = pico_socket_listen(conn->picosock, backlog)) < 0)
    	{
    		if(ret == PICO_ERR_EINVAL) {
    			DEBUG_ERROR("PICO_ERR_EINVAL - invalid argument");
    			picotap->sendReturnValue(picotap->_phy.getDescriptor(rpcSock), -1, EINVAL);
    		}
			if(ret == PICO_ERR_EISCONN) {
				DEBUG_ERROR("PICO_ERR_EISCONN - socket is connected");
				picotap->sendReturnValue(picotap->_phy.getDescriptor(rpcSock), -1, EISCONN);
			}
    	}
    	picotap->sendReturnValue(picotap->_phy.getDescriptor(rpcSock), ERR_OK, ERR_OK); // success
    }

    // Feeds data into the local app socket from the I/O buffer associated with the "connection"
    // [ (APP<-ZTSOCK) <- RXBUF ]
   	// -----------------------------------------
	// | TAP <-> MEM BUFFER <-> STACK <-> APP  |
    // |                                       | 
    // | APP <-> I/O BUFFER <-> STACK <-> TAP  |
    // | |<---------------|                    | RX
    // ----------------------------------------- 
    void picoTCP::pico_handleRead(PhySocket *sock,void **uptr,bool lwip_invoked)
    {
        if(!lwip_invoked) {
        	// The stack thread writes to RXBUF as well
            picotap->_tcpconns_m.lock();
            picotap->_rx_buf_m.lock(); 
        }
        int tot = 0, n = -1, write_attempts = 0;
		
		Connection *conn = picotap->getConnection(sock);
		if(conn && conn->rxsz) {	

			//	
			if(conn->type==SOCK_DGRAM) {
				// Try to write SDK_MTU-sized chunk to app socket
				while(tot < SDK_MTU) {
					write_attempts++;
					n = picotap->_phy.streamSend(conn->sock, (conn->rxbuf)+tot, SDK_MTU);
					tot += n;
					DEBUG_FLOW(" [ ZTSOCK <- RXBUF] wrote = %d, errno=%d", n, errno);
					// If socket is unavailable, attempt to write N times before giving up
					if(errno==35) {
						if(write_attempts == 1024) {
							n = SDK_MTU; // say we wrote it, even though we didn't (drop packet)
							tot = SDK_MTU;
						}
					}
				}
				int payload_sz, addr_sz_offset = sizeof(struct sockaddr_storage);
				memcpy(&payload_sz, conn->rxbuf + addr_sz_offset, sizeof(int));
				struct sockaddr_storage addr;
				memcpy(&addr, conn->rxbuf, addr_sz_offset);
				// adjust buffer
				//DEBUG_FLOW(" [ ZTSOCK <- RXBUF] Copying data from receiving buffer to ZT-controlled app socket (n=%d, payload_sz=%d)", n, payload_sz);
				if(conn->rxsz-n > 0) { // If more remains on buffer
					memcpy(conn->rxbuf, conn->rxbuf+SDK_MTU, conn->rxsz - SDK_MTU);
					//DEBUG_FLOW(" [ ZTSOCK <- RXBUF]   Data(%d) still on buffer, moving it up by one MTU", conn->rxsz-n);
					////memset(conn->rxbuf, 0, DEFAULT_UDP_RX_BUF_SZ);
					////conn->rxsz=SDK_MTU;
				}
			  	conn->rxsz -= SDK_MTU;
			}
			//
			if(conn->type==SOCK_STREAM) {
				n = picotap->_phy.streamSend(conn->sock, conn->rxbuf, conn->rxsz);	
				if(conn->rxsz-n > 0) // If more remains on buffer
					memcpy(conn->rxbuf, conn->rxbuf+n, conn->rxsz - n);
			  	conn->rxsz -= n;
			}
			// Notify ZT I/O loop that it has new buffer contents
			if(n) {
				if(conn->type==SOCK_STREAM) {
					
					#if DEBUG_LEVEL >= MSG_TRANSFER
						float max = conn->type == SOCK_STREAM ? (float)DEFAULT_TCP_RX_BUF_SZ : (float)DEFAULT_UDP_RX_BUF_SZ;
		            	DEBUG_TRANS("[TCP RX] <---    :: {TX: %.3f%%, RX: %.3f%%, physock=%p} :: %d bytes",
		                	(float)conn->txsz / max, (float)conn->rxsz / max, conn->sock, n);
					#endif
	        	}
	        	if(conn->rxsz == 0) {
					picotap->_phy.setNotifyWritable(sock, false);
				}
				else {
	        		picotap->_phy.setNotifyWritable(sock, true);	
				}
			}
			else {
				picotap->_phy.setNotifyWritable(sock, false);
			}	
		}
		if(!lwip_invoked) {
            picotap->_tcpconns_m.unlock();
            picotap->_rx_buf_m.unlock();
        }
        DEBUG_FLOW(" [ ZTSOCK <- RXBUF] Emitted (%d) from RXBUF(%d) to socket", tot, conn->rxsz);
    }

    // Closes a pico_socket
    void picoTCP::pico_handleClose(PhySocket *sock)
    {
    	/*
    	int ret;
    	if(conn && conn->picosock) {
	    	if((ret = pico_socket_close(conn->picosock)) < 0) {
	    		DEBUG_ERROR("error closing pico_socket(%p)", (void*)(conn->picosock));
	    		// sendReturnValue()
	    	}
	    	return;
    	}
    	DEBUG_ERROR("invalid connection or pico_socket");
    	*/
    }
}
