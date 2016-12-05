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

#if defined(SDK_PICOTCP)

#include "tap.hpp"

#include "picotcp.hpp"
#include "pico_stack.h"
#include "pico_ipv4.h"
#include "pico_icmp4.h"
#include "pico_dev_tap.h"
#include "pico_protocol.h"
#include "pico_socket.h"

namespace ZeroTier {

	// This may be removed in production
	void check_buffer_states(Connection *conn)
	{
		#if defined(SDK_DEBUG)
			if(conn->rxsz < 0) {
				DEBUG_ERROR("conn->rxsz < 0");
				exit(0);
			}
			if(conn->txsz < 0) {
				DEBUG_ERROR("conn->txsz < 0");
				exit(0);
			}
		#endif
	}

	// Reference to the tap interface
	// This is needed due to the fact that there's a lot going on in the tap interface
	// that needs to be updated on each of the network stack's callbacks and not every
	// network stack provides a mechanism for storing a reference to the tap.
	//
	// In future releases this will be replaced with a new structure of static pointers that 
	// will make it easier to maintain multiple active tap interfaces
	NetconEthernetTap *picotap;
	struct pico_device picodev;

    int pico_eth_send(struct pico_device *dev, void *buf, int len);
    int pico_eth_poll(struct pico_device *dev, int loop_score);

    // Initialize network stack's interfaces and assign addresses
	void pico_init_interface(NetconEthernetTap *tap, const InetAddress &ip)
	{
		picoTCP_stack *stack = tap->picostack;		
		DEBUG_INFO();
		if (std::find(picotap->_ips.begin(),picotap->_ips.end(),ip) == picotap->_ips.end()) {
			picotap->_ips.push_back(ip);
			std::sort(picotap->_ips.begin(),picotap->_ips.end());
		#if defined(SDK_IPV4)
			if(ip.isV4())
			{
			    struct pico_ip4 ipaddr, netmask;
			    ipaddr.addr = *((u32_t *)ip.rawIpData());
			    netmask.addr = *((u32_t *)ip.netmask().rawIpData());
			    uint8_t mac[PICO_SIZE_ETH];
			    picotap->_mac.copyTo(mac, PICO_SIZE_ETH);
			    DEBUG_ATTN("mac = %s", picotap->_mac.toString().c_str());
			    picodev.send = pico_eth_send; // tx
			    picodev.poll = pico_eth_poll; // rx
			    picodev.mtu = picotap->_mtu;
			    if( 0 != stack->__pico_device_init(&(picodev), "p0", mac)) {
			        DEBUG_ERROR("device init failed");
			        return;
			    }
			    stack->__pico_ipv4_link_add(&(picodev), ipaddr, netmask);
			    // DEBUG_INFO("device initialized as ipv4_addr = %s", ipv4_str);
			   	// picostack->__pico_icmp4_ping("10.8.8.1", 20, 1000, 10000, 64, cb_ping);
			}
		#elif defined(SDK_IPV6)
			if(ip.isV6())
			{
				struct pico_ip6 ipaddr, netmask;
				char ipv6_str[INET6_ADDRSTRLEN], nm_str[INET6_ADDRSTRLEN];
				inet_ntop(AF_INET6, ip.rawIpData(), ipv6_str, INET6_ADDRSTRLEN);
				inet_ntop(AF_INET6, ip.netmask().rawIpData(), nm_str, INET6_ADDRSTRLEN);
		    	stack->__pico_string_to_ipv6(ipv6_str, ipaddr.addr);
		    	stack->__pico_string_to_ipv6(nm_str, netmask.addr);
			    stack->__pico_ipv6_link_add(&(picodev), ipaddr, netmask);
			    picodev.send = pico_eth_send; // tx
			    picodev.poll = pico_eth_poll; // rx
			    uint8_t mac[PICO_SIZE_ETH];
			    picotap->_mac.copyTo(mac, PICO_SIZE_ETH);
			    DEBUG_ATTN("mac = %s", picotap->_mac.toString().c_str());
			    if( 0 != stack->__pico_device_init(&(picodev), "p0", mac)) {
			        DEBUG_ERROR("device init failed");
			        return;
			    }
			    DEBUG_ATTN("device initialized as ipv6_addr = %s", ipv6_str);
			}
		#endif
		}
	}
	
	// I/O thread loop
	void pico_loop(NetconEthernetTap *tap)
	{
		DEBUG_INFO();
		while(tap->_run)
		{
			tap->_phy.poll(25); // in ms
			usleep(50);
	        tap->picostack->__pico_stack_tick();
		}
	}

	// RX packets from network onto internal buffer
	// Also notifies the tap service that data can be read
   	// -----------------------------------------
	// | TAP <-> MEM BUFFER <-> STACK <-> APP  |
    // |                                       | 
    // | APP <-> I/O BUFFER <-> STACK <-> TAP  |
    // |         |<-----------------|          | RX
    // ----------------------------------------- 
    // After this step, buffer will be emptied periodically by pico_handleRead()
	void pico_cb_tcp_read(NetconEthernetTap *tap, struct pico_socket *s)
	{
		// TODO: Verify 
		DEBUG_INFO();
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
		            // r = tap->picostack->__pico_socket_read(s, conn->rxbuf + (conn->rxsz), ZT_MAX_MTU);
		            r = tap->picostack->__pico_socket_recvfrom(s, conn->rxbuf + (conn->rxsz), ZT_MAX_MTU, (void *)&peer.ip4.addr, &port);
		            // DEBUG_ATTN("received packet (%d byte) from %08X:%u", r, long_be2(peer.ip4.addr), short_be(port));
		            tap->_phy.setNotifyWritable(conn->sock, true);
		            DEBUG_EXTRA("read=%d", r);
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

	// RX packets from network onto internal buffer
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
	void pico_cb_udp_read(NetconEthernetTap *tap, struct pico_socket *s)
	{
		Connection *conn = tap->getConnection(s);
		if(conn) {
			
			uint16_t port = 0;
			union {
		        struct pico_ip4 ip4;
		        struct pico_ip6 ip6;
		    } peer;

		    char tmpbuf[ZT_MAX_MTU];
            int tot = 0;
        	unsigned char *addr_pos, *sz_pos, *payload_pos;
			struct sockaddr_in addr_in;
			addr_in.sin_addr.s_addr = peer.ip4.addr;
        	addr_in.sin_port = port;
			
        	// RX
        	int r = tap->picostack->__pico_socket_recvfrom(s, tmpbuf, ZT_MAX_MTU, (void *)&peer.ip4.addr, &port);
            DEBUG_EXTRA("read=%d", r);

			// Mutex::Lock _l2(tap->_rx_buf_m);
			// struct sockaddr_in6 addr_in6;
        	// addr_in6.sin6_addr.s6_addr;
        	// addr_in6.sin6_port = Utils::ntoh(s->remote_port);
			// DEBUG_ATTN("remote_port=%d, local_port=%d", s->remote_port, Utils::ntoh(s->local_port));
			
			if(conn->rxsz == DEFAULT_UDP_RX_BUF_SZ) { // if UDP buffer full
                DEBUG_INFO("UDP RX buffer full. Discarding oldest payload segment");
                memmove(conn->rxbuf, conn->rxbuf + ZT_MAX_MTU, DEFAULT_UDP_RX_BUF_SZ - ZT_MAX_MTU);
                addr_pos = conn->rxbuf + (DEFAULT_UDP_RX_BUF_SZ - ZT_MAX_MTU); // TODO:
                sz_pos = addr_pos + sizeof(struct sockaddr_storage);
                conn->rxsz -= ZT_MAX_MTU;
            }
            else {
                addr_pos = conn->rxbuf + conn->rxsz; // where we'll prepend the size of the address
                sz_pos = addr_pos + sizeof(struct sockaddr_storage);
            }
            payload_pos = addr_pos + sizeof(struct sockaddr_storage) + sizeof(tot);
           	memcpy(addr_pos, &addr_in, sizeof(struct sockaddr_storage));

            // Adjust buffer size
			if(r) {
	            conn->rxsz += ZT_MAX_MTU;
	            memcpy(sz_pos, &r, sizeof(r));
	            tap->phyOnUnixWritable(conn->sock, NULL, false);
	            tap->_phy.setNotifyWritable(conn->sock, false);
	        }
        	if (r < 0) {
            	DEBUG_ERROR("unable to read from picosock=%p", s);
    		} 
        	memcpy(payload_pos, tmpbuf, r); // write payload to app's socket
        	return;
		}
	}

	// TX packets from internal buffer to network
	void pico_cb_tcp_write(NetconEthernetTap *tap, struct pico_socket *s)
	{
		Connection *conn = tap->getConnection(s);
		if(!conn)
			DEBUG_ERROR("invalid connection");
		if(!conn->txsz)
			return;
		DEBUG_INFO("txsz=%d bytes ready to be written", conn->txsz);

		// Only called from a locked context, no need to lock anything
		if(conn->txsz > 0) {
			int r = conn->txsz < ZT_MAX_MTU ? conn->txsz : ZT_MAX_MTU;
			if((r = tap->picostack->__pico_socket_write(s, &conn->txbuf, r)) < 0) {
				DEBUG_ERROR("unable to write to picosock=%p", (void*)s);
				return;
			}
			int sz = (conn->txsz)-r;
            if(sz)
                memmove(&conn->txbuf, (conn->txbuf+r), sz);
            conn->txsz -= r;
            int max = conn->type == SOCK_STREAM ? DEFAULT_TCP_TX_BUF_SZ : DEFAULT_UDP_TX_BUF_SZ;
            DEBUG_TRANS("[TCP TX] --->    :: {TX: %.3f%%, RX: %.3f%%, physock=%p} :: %d bytes",
                (float)conn->txsz / (float)max, (float)conn->rxsz / max, conn->sock, r);
            return;
		}
	}

	// Main callback for TCP connections
	void pico_cb_socket_activity(uint16_t ev, struct pico_socket *s)
    {
    	int err;
        Mutex::Lock _l(picotap->_tcpconns_m);
        Connection *conn = picotap->getConnection(s);
        if(!conn) {
        	DEBUG_ERROR("invalid connection");
        }
        // Accept connection (analogous to lwip_nc_accept)
        if (ev & PICO_SOCK_EV_CONN) {
            DEBUG_INFO("connection established with server, picosock=%p", (void*)(conn->picosock));
            uint32_t peer;
			uint16_t port;
            struct pico_socket *client = picotap->picostack->__pico_socket_accept(s, &peer, &port);
            if(!client) {
				DEBUG_EXTRA("unable to accept conn. (event might not be incoming, not necessarily an error), picosock=%p", (void*)(conn->picosock));
			}
			ZT_PHY_SOCKFD_TYPE fds[2];
			if(socketpair(PF_LOCAL, SOCK_STREAM, 0, fds) < 0) {
				if(errno < 0) {
					// FIXME: Return a value to the client
					//picotap->sendReturnValue(conn, -1, errno);
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
        }
        if (ev & PICO_SOCK_EV_FIN) {
            DEBUG_INFO("socket closed. exit normally.");
            //picotap->__pico_timer_add(2000, compare_results, NULL);
        }
        if (ev & PICO_SOCK_EV_ERR) {
            DEBUG_INFO("socket error received" /*, strerror(pico_err)*/);
        }
        if (ev & PICO_SOCK_EV_CLOSE) {
            err = picotap->picostack->__pico_socket_close(s);
            DEBUG_INFO("socket closure = %d", err);
            picotap->closeConnection(conn);
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
        picotap->picostack->__pico_ipv4_to_string(host, s->dst.addr);
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
        DEBUG_INFO("len=%d", len);
        struct eth_hdr *ethhdr;
        ethhdr = (struct eth_hdr *)buf;

        MAC src_mac;
        MAC dest_mac;
        src_mac.setTo(ethhdr->src.addr, 6);
        dest_mac.setTo(ethhdr->dest.addr, 6);

        picotap->_handler(picotap->_arg,picotap->_nwid,src_mac,dest_mac,
            Utils::ntoh((uint16_t)ethhdr->type),0, ((char*)buf) + sizeof(struct eth_hdr),len - sizeof(struct eth_hdr));
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
    void pico_rx(NetconEthernetTap *tap, const MAC &from,const MAC &to,unsigned int etherType,const void *data,unsigned int len)
	{
		// DEBUG_INFO();
		// Since picoTCP only allows the reception of frames from within the polling function, we
		// must enqueue each frame into a memory structure shared by both threads. This structure will
		Mutex::Lock _l(tap->_pico_frame_rxbuf_m);
		if(len > ((1024 * 1024) - tap->pico_frame_rxbuf_tot)) {
			DEBUG_ERROR("dropping packet (len = %d) - not enough space left on RX frame buffer", len);
			return;
		}
		//if(len != memcpy(pico_frame_rxbuf, data, len)) {
		//	DEBUG_ERROR("dropping packet (len = %d) - unable to copy contents of frame to RX frame buffer", len);
		//	return;
		//}

		// assemble new eth header
		struct eth_hdr ethhdr;
		from.copyTo(ethhdr.src.addr, 6);
		to.copyTo(ethhdr.dest.addr, 6);
		ethhdr.type = Utils::hton((uint16_t)etherType);
		int newlen = len+sizeof(struct eth_hdr);
		// 
		memcpy(tap->pico_frame_rxbuf + tap->pico_frame_rxbuf_tot, &newlen, sizeof(newlen));                      // size of frame
		memcpy(tap->pico_frame_rxbuf + tap->pico_frame_rxbuf_tot + sizeof(newlen), &ethhdr, sizeof(ethhdr));     // new eth header
		memcpy(tap->pico_frame_rxbuf + tap->pico_frame_rxbuf_tot + sizeof(newlen) + sizeof(ethhdr), data, len);  // frame data
		tap->pico_frame_rxbuf_tot += len + sizeof(len) + sizeof(ethhdr);
		// DEBUG_INFO("RX frame buffer %3f full", (float)pico_frame_rxbuf_tot / (float)(1024 * 1024));
		DEBUG_INFO("len=%d", len);
	}

	// Called periodically by the stack, this removes data from the locked memory buffer and feeds it into the stack.
	// A maximum of 'loop_score' frames can be processed in each call
   	// -----------------------------------------
	// | TAP <-> MEM BUFFER <-> STACK <-> APP  |
    // |         |----------------->|          | RX 
    // | APP <-> I/O BUFFER <-> STACK <-> TAP  |
    // |                                       |
    // ----------------------------------------- 
    int pico_eth_poll(struct pico_device *dev, int loop_score)
    {
    	// DEBUG_EXTRA();
        // OPTIMIZATION: The copy logic and/or buffer structure should be reworked for better performance after the BETA
        // NetconEthernetTap *tap = (NetconEthernetTap*)netif->state;
        Mutex::Lock _l(picotap->_pico_frame_rxbuf_m);
        unsigned char frame[ZT_MAX_MTU];
        uint32_t len;

        while (picotap->pico_frame_rxbuf_tot > 0) {
            memset(frame, 0, sizeof(frame));
            len = 0;
            memcpy(&len, picotap->pico_frame_rxbuf, sizeof(len)); // get frame len
            memcpy(frame, picotap->pico_frame_rxbuf + sizeof(len), len); // get frame data
            memmove(picotap->pico_frame_rxbuf, picotap->pico_frame_rxbuf + sizeof(len) + len, ZT_MAX_MTU-(sizeof(len) + len));
            picotap->picostack->__pico_stack_recv(dev, (uint8_t*)frame, len); 
            picotap->pico_frame_rxbuf_tot-=(sizeof(len) + len);
            // DEBUG_EXTRA("RX frame buffer %3f full", (float)(picotap->pico_frame_rxbuf_tot) / (float)(MAX_PICO_FRAME_RX_BUF_SZ));
            loop_score--;
        }
        return loop_score;
    }

    // Creates a new pico_socket and Connection object to represent a new connection to be.
    Connection *pico_handleSocket(PhySocket *sock, void **uptr, struct socket_st* socket_rpc)
    {
    	DEBUG_INFO();
    	struct pico_socket * psock;
    	int protocol, protocol_version;

		#if defined(SDK_IPV4)
			protocol_version = PICO_PROTO_IPV4;
		#elif defined(SDK_IPV6)
			protocol_version = PICO_PROTO_IPV6;
		#endif
		if(socket_rpc->socket_type == SOCK_DGRAM) {
			protocol = PICO_PROTO_UDP;
			psock = picotap->picostack->__pico_socket_open(protocol_version, protocol, &pico_cb_socket_activity);
		}
		if(socket_rpc->socket_type == SOCK_STREAM) {
			protocol = PICO_PROTO_TCP;
			psock = picotap->picostack->__pico_socket_open(protocol_version, protocol, &pico_cb_socket_activity);
		}

		if(psock) {
			DEBUG_ATTN("physock=%p, picosock=%p", sock, (void*)psock);
			Connection * newConn = new Connection();
	        *uptr = newConn;
	        newConn->type = socket_rpc->socket_type;
	        newConn->sock = sock;
			newConn->local_addr = NULL;
			// newConn->peer_addr = NULL;
			newConn->picosock = psock;
	        picotap->_Connections.push_back(newConn);
	        return newConn;
		}
		else {
			DEBUG_ERROR("failed to create pico_socket");
		}
		return NULL;
    }

    // Writes data from the I/O buffer to the network stack
   	// -----------------------------------------
	// | TAP <-> MEM BUFFER <-> STACK <-> APP  |
    // |                                       | 
    // | APP <-> I/O BUFFER <-> STACK <-> TAP  |
    // |         |----------------->|          | TX
    // ----------------------------------------- 
    void pico_handleWrite(Connection *conn)
    {
    	DEBUG_INFO();
		if(!conn || !conn->picosock) {
			DEBUG_ERROR(" invalid connection");
			return;
		}

		int max, r, max_write_len = conn->txsz < ZT_MAX_MTU ? conn->txsz : ZT_MAX_MTU;
	    if((r = picotap->picostack->__pico_socket_write(conn->picosock, &conn->txbuf, max_write_len)) < 0) {
	    	DEBUG_ERROR("unable to write to pico_socket(%p), r=%d", (conn->picosock), r);
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
    void pico_handleConnect(PhySocket *sock, PhySocket *rpcSock, Connection *conn, struct connect_st* connect_rpc)
    {
    	DEBUG_INFO();
		if(conn->picosock) {
			struct sockaddr_in *addr = (struct sockaddr_in *) &connect_rpc->addr;
			int ret;
			// TODO: Rewrite this
			#if defined(SDK_IPV4)
				struct pico_ip4 zaddr;
    			struct sockaddr_in *in4 = (struct sockaddr_in*)&connect_rpc->addr;
    			char ipv4_str[INET_ADDRSTRLEN];
    			inet_ntop(AF_INET, &(in4->sin_addr), ipv4_str, INET_ADDRSTRLEN);
				picotap->picostack->__pico_string_to_ipv4(ipv4_str, &(zaddr.addr));
				DEBUG_ATTN("addr=%s:%d", ipv4_str, Utils::ntoh(addr->sin_port));
				ret = picotap->picostack->__pico_socket_connect(conn->picosock, &zaddr, addr->sin_port);
			#elif defined(SDK_IPV6) // "fd56:5799:d8f6:1238:8c99:9322:30ce:418a"
				struct pico_ip6 zaddr;
				struct sockaddr_in6 *in6 = (struct sockaddr_in6*)&connect_rpc->addr;
				char ipv6_str[INET6_ADDRSTRLEN];
				inet_ntop(AF_INET6, &(in6->sin6_addr), ipv6_str, INET6_ADDRSTRLEN);
		    	picotap->picostack->__pico_string_to_ipv6(ipv6_str, zaddr.addr);
		    	DEBUG_ATTN("addr=%s:%d", ipv6_str, Utils::ntoh(addr->sin_port));
				ret = picotap->picostack->__pico_socket_connect(conn->picosock, &zaddr, addr->sin_port);
			#endif
			
			memcpy(&(conn->peer_addr), &connect_rpc->addr, sizeof(struct sockaddr_storage));

			if(ret == PICO_ERR_EPROTONOSUPPORT) {
				DEBUG_ERROR("PICO_ERR_EPROTONOSUPPORT");
			}
			if(ret == PICO_ERR_EINVAL) {
				DEBUG_ERROR("PICO_ERR_EINVAL");
			}
			if(ret == PICO_ERR_EHOSTUNREACH) {
				DEBUG_ERROR("PICO_ERR_EHOSTUNREACH");
			}
	        picotap->sendReturnValue(picotap->_phy.getDescriptor(rpcSock), 0, ERR_OK);
		}
    }

    // Instructs the stack to bind to a given address
    void pico_handleBind(PhySocket *sock, PhySocket *rpcSock, void **uptr, struct bind_st *bind_rpc)
    {
    	DEBUG_INFO();
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
			picotap->picostack->__pico_string_to_ipv4(ipv4_str, &(zaddr.addr));
			DEBUG_ATTN("addr=%s:%d, physock=%p, picosock=%p", ipv4_str, Utils::ntoh(addr->sin_port), sock, (conn->picosock));
			ret = picotap->picostack->__pico_socket_bind(conn->picosock, &zaddr, (uint16_t*)&(addr->sin_port));
		#elif defined(SDK_IPV6)
			struct pico_ip6 zaddr;
			struct sockaddr_in6 *in6 = (struct sockaddr_in6*)&bind_rpc->addr;
			char ipv6_str[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, &(in6->sin6_addr), ipv6_str, INET6_ADDRSTRLEN);
	    	picotap->picostack->__pico_string_to_ipv6(ipv6_str, zaddr.addr);
	    	DEBUG_ATTN("addr=%s:%d, physock=%p, picosock=%p", ipv6_str, Utils::ntoh(addr->sin_port), sock, (conn->picosock));
			ret = picotap->picostack->__pico_socket_bind(conn->picosock, &zaddr, (uint16_t*)&(addr->sin_port));
		#endif
		if(ret < 0) {
			DEBUG_ERROR("unable to bind pico_socket(%p)", (void*)(conn->picosock));
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
    void pico_handleListen(PhySocket *sock, PhySocket *rpcSock, void **uptr, struct listen_st *listen_rpc)
    {
    	Connection *conn = picotap->getConnection(sock);
    	DEBUG_ATTN("conn = %p", (void*)conn);
    	if(!sock || !conn) {
    		DEBUG_ERROR("invalid connection");
    		return;
    	}
    	int ret, backlog = 1;
    	if((ret = picotap->picostack->__pico_socket_listen(conn->picosock, backlog)) < 0)
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
   	// -----------------------------------------
	// | TAP <-> MEM BUFFER <-> STACK <-> APP  |
    // |                                       | 
    // | APP <-> I/O BUFFER <-> STACK <-> TAP  |
    // | |<---------------|                    | RX
    // ----------------------------------------- 
    void pico_handleRead(PhySocket *sock,void **uptr,bool lwip_invoked)
    {
    	/*
        if(!lwip_invoked) {
            picotap->_tcpconns_m.lock();
            //picotap->_rx_buf_m.lock(); 
        }
		*/
		
    	DEBUG_ATTN();
		Connection *conn = picotap->getConnection(sock);
		if(conn && conn->rxsz) {
			float max = conn->type == SOCK_STREAM ? (float)DEFAULT_TCP_RX_BUF_SZ : (float)DEFAULT_UDP_RX_BUF_SZ;
			long n = -1;
			// extract address and payload size info
			
			if(conn->type==SOCK_DGRAM) {
				n = picotap->_phy.streamSend(conn->sock, conn->rxbuf, ZT_MAX_MTU);
				DEBUG_EXTRA("SOCK_DGRAM, physock=%p", sock);
				int payload_sz, addr_sz_offset = sizeof(struct sockaddr_storage);
				memcpy(&payload_sz, conn->rxbuf + addr_sz_offset, sizeof(int));
				struct sockaddr_storage addr;
				memcpy(&addr, conn->rxbuf, addr_sz_offset);
				// adjust buffer
				if(conn->rxsz-n > 0) // If more remains on buffer
					memcpy(conn->rxbuf, conn->rxbuf+ZT_MAX_MTU, conn->rxsz - ZT_MAX_MTU);
			  	conn->rxsz -= ZT_MAX_MTU;
			}
			
			if(conn->type==SOCK_STREAM) {
				n = picotap->_phy.streamSend(conn->sock, conn->rxbuf, conn->rxsz);	
				DEBUG_EXTRA("SOCK_STREAM, physock=%p", sock);
				if(conn->rxsz-n > 0) // If more remains on buffer
					memcpy(conn->rxbuf, conn->rxbuf+n, conn->rxsz - n);
			  	conn->rxsz -= n;
			}
			if(n) {
				if(conn->type==SOCK_STREAM) {
	            	DEBUG_TRANS("[TCP RX] <---    :: {TX: %.3f%%, RX: %.3f%%, physock=%p} :: %ld bytes",
	                	(float)conn->txsz / max, (float)conn->rxsz / max, (void*)conn->sock, n);
	        	}
	        	picotap->_phy.setNotifyWritable(conn->sock, true);
			}
			if(conn->rxsz == 0) {
				picotap->_phy.setNotifyWritable(conn->sock, false);
			}
		}
		check_buffer_states(conn);
    }

    // Closes a pico_socket
    /*
    static void pico_handleClose(Connection *conn)
    {
    	DEBUG_INFO();
    	int ret;
    	if(conn && conn->picosock) {
	    	if((ret = picotap->picostack->__pico_socket_close(conn->picosock)) < 0) {
	    		DEBUG_ERROR("error closing pico_socket(%p)", (void*)(conn->picosock));
	    		// sendReturnValue()
	    	}
	    	return;
    	}
    	DEBUG_ERROR("invalid connection or pico_socket");
    }
    */

}

#endif // SDK_PICOTCP