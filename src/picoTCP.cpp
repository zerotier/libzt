/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2016  ZeroTier, Inc.  https://www.zerotier.com/
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
 */

// picoTCP
#include "pico_eth.h"
#include "pico_stack.h"
#include "pico_ipv4.h"
#include "pico_icmp4.h"
#include "pico_dev_tap.h"
#include "pico_protocol.h"
#include "pico_socket.h"
#include "pico_device.h"
#include "pico_ipv6.h"

// SDK
#include "ZeroTierSDK.h"
#include "Utilities.hpp"
#include "SocketTap.hpp"
#include "picoTCP.hpp"

// ZT
#include "Utils.hpp"
#include "OSUtils.hpp"
#include "Mutex.hpp"
#include "Constants.hpp"
#include "Phy.hpp"

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
extern "C" int pico_socket_connect(PICO_SOCKET_CONNECT_SIG);
extern "C" int pico_socket_listen(PICO_SOCKET_LISTEN_SIG);
int pico_socket_read(PICO_SOCKET_READ_SIG);
extern "C" int pico_socket_write(PICO_SOCKET_WRITE_SIG);
extern "C" int pico_socket_close(PICO_SOCKET_CLOSE_SIG);
int pico_socket_shutdown(PICO_SOCKET_SHUTDOWN_SIG);
struct pico_socket * pico_socket_accept(PICO_SOCKET_ACCEPT_SIG);
extern "C" struct pico_ipv6_link * pico_ipv6_link_add(PICO_IPV6_LINK_ADD_SIG);


namespace ZeroTier {

	struct pico_device picodev;

	void picoTCP::pico_init_interface(SocketTap *tap, const InetAddress &ip)
	{
		DEBUG_INFO();
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
			    // DEBUG_ATTN("mac = %s", tap->_mac.toString().c_str());
			    picodev.send = pico_eth_send; // tx
			    picodev.poll = pico_eth_poll; // rx
			    picodev.mtu = tap->_mtu;
			    picodev.tap = tap;
			    if( 0 != pico_device_init(&(picodev), "p0", mac)) {
			        DEBUG_ERROR("dev init failed");
			        return;
			    }
			    pico_ipv4_link_add(&(picodev), ipaddr, netmask);
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
			    picodev.tap = tap;
			    uint8_t mac[PICO_SIZE_ETH];
			    tap->_mac.copyTo(mac, PICO_SIZE_ETH);
			    // DEBUG_ATTN("mac = %s", tap->_mac.toString().c_str());
			    if( 0 != pico_device_init(&(picodev), "p0", mac)) {
			        DEBUG_ERROR("dev init failed");
			        return;
			    }
			}
		#endif
		}
	}
	
	void picoTCP::pico_loop(SocketTap *tap)
	{
		while(tap->_run)
		{
			tap->_phy.poll(ZT_PHY_POLL_INTERVAL); // in ms
	        pico_stack_tick();
		}
	}

	void picoTCP::pico_cb_tcp_read(ZeroTier::SocketTap *tap, struct pico_socket *s)
	{
		DEBUG_INFO();
		Connection *conn = (Connection*)((ConnectionPair*)(s->priv))->conn;
		if(conn) {
			int r;				
			uint16_t port = 0;
			union {
		        struct pico_ip4 ip4;
		        struct pico_ip6 ip6;
		    } peer;
			do {
				int avail = ZT_TCP_RX_BUF_SZ - conn->rxsz;
				if(avail) {
		            r = pico_socket_recvfrom(s, conn->rxbuf + (conn->rxsz), ZT_SDK_MTU, 
		            	(void *)&peer.ip4.addr, &port);
		            //tap->_phy.setNotifyWritable(conn->sock, true);
		            if (r > 0)
		                conn->rxsz += r;
		            picostack->pico_Read(tap, conn->sock, conn, true);
		            DEBUG_INFO("r = %d, conn->rxsz=%d, conn=%p, conn->sock = %p", r, conn->rxsz, conn, conn->sock);
	        	}
	        	else
	        		DEBUG_ERROR("not enough space left on I/O RX buffer for pico_socket(%p)", s);
            }
        	while(r > 0);
        	return;
        }
		DEBUG_ERROR("invalid connection");
	}

	void picoTCP::pico_cb_udp_read(SocketTap *tap, struct pico_socket *s)
	{
		DEBUG_INFO();
		Connection *conn = (Connection*)((ConnectionPair*)(s->priv))->conn;
		if(conn) {

			uint16_t port = 0;
			union {
		        struct pico_ip4 ip4;
		        struct pico_ip6 ip6;
		    } peer;

		    char tmpbuf[ZT_SDK_MTU];
        	unsigned char *addr_pos, *sz_pos, *payload_pos;
			struct sockaddr_in addr_in;
			addr_in.sin_addr.s_addr = peer.ip4.addr;
        	addr_in.sin_port = port;
			
        	// RX
        	int r = pico_socket_recvfrom(s, tmpbuf, ZT_SDK_MTU, (void *)&peer.ip4.addr, &port);
            //DEBUG_FLOW(" [  RXBUF <- STACK] Receiving (%d) from stack, copying to receving buffer", r);

			if(conn->rxsz == ZT_UDP_RX_BUF_SZ) { // if UDP buffer full
                //DEBUG_FLOW(" [  RXBUF <- STACK] UDP RX buffer full. Discarding oldest payload segment");
                memmove(conn->rxbuf, conn->rxbuf + ZT_SDK_MTU, ZT_UDP_RX_BUF_SZ - ZT_SDK_MTU);
                addr_pos = conn->rxbuf + (ZT_UDP_RX_BUF_SZ - ZT_SDK_MTU); // TODO:
                sz_pos = addr_pos + sizeof(struct sockaddr_storage);
                conn->rxsz -= ZT_SDK_MTU;
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
	            conn->rxsz += ZT_SDK_MTU;
	            memcpy(sz_pos, &r, sizeof(r));
	        }
	        if (r < 0) {
            	DEBUG_ERROR("unable to read from picosock=%p", s);
    		} 
	        tap->_rx_buf_m.unlock(); 

	        // TODO: Revisit logic
	        // FIXME: simplify
	        if(r)
	            tap->phyOnUnixWritable(conn->sock, NULL, true);
        	//DEBUG_EXTRA(" Copied onto rxbuf (%d) from stack socket", r);
        	return;
		}
	}

	void picoTCP::pico_cb_tcp_write(SocketTap *tap, struct pico_socket *s)
	{
		DEBUG_INFO();
		Connection *conn = (Connection*)((ConnectionPair*)(s->priv))->conn;

		if(!conn) {
			DEBUG_ERROR("invalid connection");
			return;
		}
		if(!conn->txsz)
			return;
		// Only called from a locked context, no need to lock anything
		if(conn->txsz > 0) {
			int r, max_write_len = conn->txsz < ZT_SDK_MTU ? conn->txsz : ZT_SDK_MTU;
			if((r = pico_socket_write(s, &conn->txbuf, max_write_len)) < 0) {
				DEBUG_ERROR("unable to write to picosock=%p", s);
				return;
			}
			int sz = (conn->txsz)-r;
            if(sz)
                memmove(&conn->txbuf, (conn->txbuf+r), sz);
            conn->txsz -= r;

            #if DEBUG_LEVEL >= MSG_TRANSFER
            	int max = conn->socket_type == SOCK_STREAM ? ZT_TCP_TX_BUF_SZ : ZT_UDP_TX_BUF_SZ;
            	DEBUG_TRANS("[ TCP TX -> STACK] :: {TX: %.3f%%, RX: %.3f%%, physock=%p} :: %d bytes",
                	(float)conn->txsz / (float)max, (float)conn->rxsz / max, conn->sock, r);
        	#endif

            return;
		}
	}

	void picoTCP::pico_cb_socket_activity(uint16_t ev, struct pico_socket *s)
    {
    	// TODO: Test API out of order so this check isn't necessary
    	if(!(SocketTap*)((ConnectionPair*)(s->priv)))
    		return;
    	SocketTap *tap = (SocketTap*)((ConnectionPair*)(s->priv))->tap;
    	Connection *conn = (Connection*)((ConnectionPair*)(s->priv))->conn;
    	
    	int err;
        Mutex::Lock _l(tap->_tcpconns_m);
        if(!conn) {
        	DEBUG_ERROR("invalid connection");
        }
        DEBUG_INFO("conn = %p", conn);
        // accept()
        if (ev & PICO_SOCK_EV_CONN) {
        	if(conn->state == ZT_SOCK_STATE_LISTENING)
        	{
	            uint32_t peer;
				uint16_t port;
	            struct pico_socket *client_psock = pico_socket_accept(s, &peer, &port);
	            if(!client_psock) {
	            	if(pico_err == PICO_ERR_EINVAL)
	            		DEBUG_ERROR("pico_err = PICO_ERR_EINVAL");
	            	if(pico_err == PICO_ERR_EAGAIN)
	            		DEBUG_ERROR("pico_err = PICO_ERR_EAGAIN");
	            	return;
				}			

				// Create a new Connection and add it to the queue,
				//   some time in the future a call to zts_multiplex_accept() will pick up 
				//   this new connection, add it to the connection list and return its
				//   Connection->sock to the application

				Connection *newConn = new Connection();
				newConn->socket_type = SOCK_STREAM;
				newConn->picosock = client_psock;
				newConn->tap = tap;
				newConn->picosock->priv = new ConnectionPair(tap,newConn);
				tap->_Connections.push_back(newConn);
				conn->_AcceptedConnections.push(newConn);
				// For I/O loop participation and referencing the PhySocket's parent Connection in callbacks
				newConn->sock = tap->_phy.wrapSocket(newConn->sdk_fd, newConn);
				DEBUG_INFO("wrapping newConn->sdk_fd = %d", newConn->sdk_fd);
				DEBUG_INFO("         newConn->app_fd = %d", newConn->app_fd);
				DEBUG_INFO("         newConn->sock   = %p", newConn->sock);
				DEBUG_INFO("         conn            = %p", conn);
				DEBUG_INFO("         newConn         = %p", newConn);
				DEBUG_INFO("         oldConn->sock   = %p", conn->sock);
			}
			if(conn->state != ZT_SOCK_STATE_LISTENING) {
				// set state so socket multiplexer logic will pick this up
				conn->state = ZT_SOCK_STATE_UNHANDLED_CONNECTED;
			}
        }
        if (ev & PICO_SOCK_EV_FIN) {
            DEBUG_INFO("socket closed. exit normally. picosock=%p", s);
            //pico_timer_add(2000, compare_results, NULL);
        }
        if (ev & PICO_SOCK_EV_ERR) {
        	if(pico_err == PICO_ERR_ECONNRESET) {
        		DEBUG_ERROR("PICO_ERR_ECONNRESET");
        		conn->state = PICO_ERR_ECONNRESET;
        	}
            DEBUG_INFO("socket error received pico_err=%d, picosock=%p", pico_err, s);
        }
        if (ev & PICO_SOCK_EV_CLOSE) {
            err = pico_socket_close(s);
            DEBUG_INFO("socket closure = %d, picosock=%p", err, s);
            if(err==0)
            	tap->Close(conn);
            return;
        }
        // Read from picoTCP socket
        if (ev & PICO_SOCK_EV_RD) {
        	if(conn->socket_type==SOCK_STREAM)
            	pico_cb_tcp_read(tap, s);
        	if(conn->socket_type==SOCK_DGRAM)
        		pico_cb_udp_read(tap, s);
        }
        // Write to picoTCP socket
        if (ev & PICO_SOCK_EV_WR)
            pico_cb_tcp_write(tap, s);
    }
   
   	int pico_eth_send(struct pico_device *dev, void *buf, int len)
    {
    	SocketTap *tap = (SocketTap*)(dev->tap);
        struct pico_eth_hdr *ethhdr;
        ethhdr = (struct pico_eth_hdr *)buf;
        MAC src_mac;
        MAC dest_mac;
        src_mac.setTo(ethhdr->saddr, 6);
        dest_mac.setTo(ethhdr->daddr, 6);
        tap->_handler(tap->_arg,NULL,tap->_nwid,src_mac,dest_mac,
            Utils::ntoh((uint16_t)ethhdr->proto),0, ((char*)buf) + sizeof(struct pico_eth_hdr),len - sizeof(struct pico_eth_hdr));
        //DEBUG_INFO("len = %d", len);
        return len;
    }

    void picoTCP::pico_rx(SocketTap *tap, const MAC &from,const MAC &to,unsigned int etherType,
    	const void *data,unsigned int len)
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
		// FIXME
		while(newlen > (MAX_PICO_FRAME_RX_BUF_SZ-tap->pico_frame_rxbuf_tot) && ethhdr.proto == 56710)
		{
			mylen = 0;
			memset(tap->pico_frame_rxbuf,0,MAX_PICO_FRAME_RX_BUF_SZ);
			tap->pico_frame_rxbuf_tot=0;
		}
		memcpy(tap->pico_frame_rxbuf + tap->pico_frame_rxbuf_tot, &newlen, sizeof(newlen));                      // size of frame + meta		
		memcpy(tap->pico_frame_rxbuf + tap->pico_frame_rxbuf_tot + sizeof(newlen), &ethhdr, sizeof(ethhdr));     // new eth header
		memcpy(tap->pico_frame_rxbuf + tap->pico_frame_rxbuf_tot + sizeof(newlen) + sizeof(ethhdr), data, len);  // frame data
		tap->pico_frame_rxbuf_tot += newlen;
		DEBUG_FLOW("[ ZWIRE -> FBUF ] Move FRAME(sz=%d) into FBUF(sz=%d), data_len=%d", newlen, tap->pico_frame_rxbuf_tot, len);
	}

    int pico_eth_poll(struct pico_device *dev, int loop_score)
    {
    	SocketTap *tap = (SocketTap*)(dev->tap);
        // FIXME: The copy logic and/or buffer structure should be reworked for better performance after the BETA
        // SocketTap *tap = (SocketTap*)netif->state;
        Mutex::Lock _l(tap->_pico_frame_rxbuf_m);
        unsigned char frame[ZT_SDK_MTU];
        int len;
        int err;
        while (tap->pico_frame_rxbuf_tot > 0 && loop_score > 0) {
        	//DEBUG_FLOW(" [   FBUF -> STACK] Frame buffer SZ=%d", tap->pico_frame_rxbuf_tot);
            memset(frame, 0, sizeof(frame));
            len = 0;
            memcpy(&len, tap->pico_frame_rxbuf, sizeof(len)); // get frame len
            if(len >= 0) {
            	//DEBUG_FLOW(" [   FBUF -> STACK]   Moving FRAME of size (%d) from FBUF(sz=%d) into stack",len, tap->pico_frame_rxbuf_tot-len);
            	memcpy(frame, tap->pico_frame_rxbuf + sizeof(len), len-(sizeof(len)) ); // get frame data
            	memmove(tap->pico_frame_rxbuf, tap->pico_frame_rxbuf + len, MAX_PICO_FRAME_RX_BUF_SZ-len); // shift buffer
            	err = pico_stack_recv(dev, (uint8_t*)frame, (len-sizeof(len))); 
            	//DEBUG_INFO("recv = %d", err);
                tap->pico_frame_rxbuf_tot-=len;
            }
            else {
            	DEBUG_ERROR("Invalid frame size (%d). Exiting.",len);
            	zt_dump_stacktrace(0);
            }
            loop_score--;
        }
        return loop_score;
    }

    int picoTCP::pico_Connect(Connection *conn, int fd, const struct sockaddr *addr, socklen_t addrlen)
    {		
		int err;
		#if defined(SDK_IPV4)
			struct pico_ip4 zaddr;
			struct sockaddr_in *in4 = (struct sockaddr_in*)addr;
			char ipv4_str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, (const void *)&in4->sin_addr.s_addr, ipv4_str, INET_ADDRSTRLEN);
			pico_string_to_ipv4(ipv4_str, &(zaddr.addr));
			DEBUG_ATTN("addr=%s:%d", ipv4_str, Utils::ntoh( in4->sin_port ));
			err = pico_socket_connect(conn->picosock, &zaddr, in4->sin_port);
			DEBUG_INFO("connect_err = %d", err);
		
		#elif defined(SDK_IPV6)
			struct pico_ip6 zaddr;
			struct sockaddr_in6 *in6 = (struct sockaddr_in6*)addr;
			char ipv6_str[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, &(in6->sin6_addr), ipv6_str, INET6_ADDRSTRLEN);
	    	pico_string_to_ipv6(ipv6_str, zaddr.addr);
	    	DEBUG_ATTN("addr=%s:%d", ipv6_str, Utils::ntoh(in6->sin6_port));
			err = pico_socket_connect(conn->picosock, &zaddr, in6->sin6_port);
		#endif
		
		memcpy(&(conn->peer_addr), &addr, sizeof(struct sockaddr_storage));

		if(err == PICO_ERR_EPROTONOSUPPORT)
			DEBUG_ERROR("PICO_ERR_EPROTONOSUPPORT");
		if(err == PICO_ERR_EINVAL)
			DEBUG_ERROR("PICO_ERR_EINVAL");
		if(err == PICO_ERR_EHOSTUNREACH)
			DEBUG_ERROR("PICO_ERR_EHOSTUNREACH");
		return err;
    }

    int picoTCP::pico_Bind(Connection *conn, int fd, const struct sockaddr *addr, socklen_t addrlen)
    {
    	DEBUG_INFO();
    	int err;
		#if defined(SDK_IPV4)
			struct pico_ip4 zaddr;
			struct sockaddr_in *in4 = (struct sockaddr_in*)addr;
			char ipv4_str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, (const void *)&in4->sin_addr.s_addr, ipv4_str, INET_ADDRSTRLEN);
			pico_string_to_ipv4(ipv4_str, &(zaddr.addr));
			
			// DEBUG_ATTN("addr=%s: %d ntoh()=%d", ipv4_str, in4->sin_port, Utils::ntoh(in4->sin_port));
			err = pico_socket_bind(conn->picosock, &zaddr, (uint16_t *)&(in4->sin_port));

		#elif defined(SDK_IPV6)
			struct pico_ip6 zaddr;
			struct sockaddr_in6 *in6 = (struct sockaddr_in6*)addr;
			char ipv6_str[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, &(in6->sin6_addr), ipv6_str, INET6_ADDRSTRLEN);
	    	pico_string_to_ipv6(ipv6_str, zaddr.addr);
	    	//DEBUG_ATTN("addr=%s:%d, physock=%p, picosock=%p", ipv6_str, Utils::ntoh(addr->sin_port), sock, (conn->picosock));
			err = pico_socket_bind(conn->picosock, &zaddr, (uint16_t *)in6->sin6_port);
		#endif
		if(err < 0) {
			DEBUG_ERROR("unable to bind pico_socket(%p), err=%d", (conn->picosock), err);
			if(err == PICO_ERR_EINVAL) {
				DEBUG_ERROR("PICO_ERR_EINVAL - invalid argument");
				errno = EINVAL;
				return -1;
			} 
			if(err == PICO_ERR_ENOMEM) {
				DEBUG_ERROR("PICO_ERR_ENOMEM - not enough space");
				errno = ENOMEM;
				return -1;			
			} 
			if(err == PICO_ERR_ENXIO) {
				DEBUG_ERROR("PICO_ERR_ENXIO - no such device or address");
				errno = ENXIO;
				return -1;
			}
		}
		return err;
    }

    int picoTCP::pico_Listen(Connection *conn, int fd, int backlog)
    {
    	DEBUG_INFO();
    	int err;
    	if((err = pico_socket_listen(conn->picosock, backlog)) < 0)
    	{
    		if(err == PICO_ERR_EINVAL) {
    			DEBUG_ERROR("PICO_ERR_EINVAL");
    			errno = EINVAL;
    			return -1;
    		}
			if(err == PICO_ERR_EISCONN) {
				DEBUG_ERROR("PICO_ERR_EISCONN");
				errno = EISCONN;
				return -1;
			}
    	}
    	conn->state = ZT_SOCK_STATE_LISTENING;
    	return ZT_ERR_OK;
    }

    int picoTCP::pico_Accept(Connection *conn)
    {
    	// Retreive queued Connections from parent connection
    	int err;
        if(!conn->_AcceptedConnections.size()) {
        	err = -1;
        }
        else {
			Connection *new_conn = conn->_AcceptedConnections.front();
			conn->_AcceptedConnections.pop();
			err = new_conn->app_fd;
			DEBUG_INFO("wrapping new_conn->app_fd = %d", new_conn->app_fd);
		}
    	return err;
    }

    void picoTCP::pico_Read(SocketTap *tap, PhySocket *sock, Connection* conn, bool stack_invoked)
    {
    	DEBUG_INFO();
        if(!stack_invoked) {
        	// The stack thread writes to RXBUF as well
            tap->_tcpconns_m.lock();
            tap->_rx_buf_m.lock(); 
        }
        int tot = 0, n = -1, write_attempts = 0;
		
		//Connection *conn = (Connection*)uptr;

		if(conn && conn->rxsz) {	
			DEBUG_INFO("conn = %p", conn);
			//	
			if(conn->socket_type==SOCK_DGRAM) {
				// Try to write ZT_SDK_MTU-sized chunk to app socket
				while(tot < ZT_SDK_MTU) {
					write_attempts++;
					n = tap->_phy.streamSend(conn->sock, (conn->rxbuf)+tot, ZT_SDK_MTU);
					tot += n;
					DEBUG_FLOW("[ ZTSOCK <- RXBUF] wrote = %d, errno=%d", n, errno);
					// If socket is unavailable, attempt to write N times before giving up
					if(errno==35) {
						if(write_attempts == 1024) {
							n = ZT_SDK_MTU; // say we wrote it, even though we didn't (drop packet)
							tot = ZT_SDK_MTU;
						}
					}
				}
				int payload_sz, addr_sz_offset = sizeof(struct sockaddr_storage);
				memcpy(&payload_sz, conn->rxbuf + addr_sz_offset, sizeof(int));
				struct sockaddr_storage addr;
				memcpy(&addr, conn->rxbuf, addr_sz_offset);
				// adjust buffer
				if(conn->rxsz-n > 0) { // If more remains on buffer
					memcpy(conn->rxbuf, conn->rxbuf+ZT_SDK_MTU, conn->rxsz - ZT_SDK_MTU);
				}
			  	conn->rxsz -= ZT_SDK_MTU;
			}
			//
			if(conn->socket_type==SOCK_STREAM) {
				DEBUG_INFO("writing to conn->sock = %p, conn->sdk_fd=%d, conn->app_fd=%d", conn->sock, conn->sdk_fd, conn->app_fd);
				n = tap->_phy.streamSend(conn->sock, conn->rxbuf, conn->rxsz);	
				// FIXME: Revisit the idea of writing directly to the app socketpair instead of using Phy I/O 
				// n = write(conn->sdk_fd, conn->rxbuf, conn->rxsz);
				if(conn->rxsz-n > 0) // If more remains on buffer
					memcpy(conn->rxbuf, conn->rxbuf+n, conn->rxsz - n);
			  	conn->rxsz -= n;
			}
			// Notify ZT I/O loop that it has new buffer contents
			if(n) {
				if(conn->socket_type==SOCK_STREAM) {
					#if DEBUG_LEVEL >= MSG_TRANSFER
						float max = conn->socket_type == SOCK_STREAM ? (float)ZT_TCP_RX_BUF_SZ : (float)ZT_UDP_RX_BUF_SZ;
		            	DEBUG_TRANS("[ TCP RX <- STACK] :: {TX: %.3f%%, RX: %.3f%%, physock=%p} :: %d bytes",
		                	(float)conn->txsz / max, (float)conn->rxsz / max, conn->sock, n);
					#endif
	        	}
	        	if(conn->rxsz == 0) {
					tap->_phy.setNotifyWritable(sock, false);
				}
				else {
	        		tap->_phy.setNotifyWritable(sock, true);	
				}
			}
			else {
				tap->_phy.setNotifyWritable(sock, false);
			}	
		}
		if(!stack_invoked) {
            tap->_tcpconns_m.unlock();
            tap->_rx_buf_m.unlock();
        }
        DEBUG_FLOW("[ ZTSOCK <- RXBUF] Emitted (%d) from RXBUF(%d) to socket", tot, conn->rxsz);
    }

    void picoTCP::pico_Write(Connection *conn)
    {
    	DEBUG_INFO();
		if(!conn || !conn->picosock) {
			DEBUG_ERROR(" invalid connection");
			return;
		}
		int max, r, max_write_len = conn->txsz < ZT_SDK_MTU ? conn->txsz : ZT_SDK_MTU;
	    if((r = pico_socket_write(conn->picosock, &conn->txbuf, max_write_len)) < 0) {
	    	DEBUG_ERROR("unable to write to picosock=%p, r=%d", (conn->picosock), r);
	    	return;
	    }
	    // adjust buffer
	    int sz = (conn->txsz)-r;
	   	if(sz)
	   		memmove(&conn->txbuf, (conn->txbuf+r), sz);
		conn->txsz -= r;
	   	
	   	if(conn->socket_type == SOCK_STREAM) {
	   		max = ZT_TCP_TX_BUF_SZ;
	    	DEBUG_TRANS("[ TCP TX -> STACK] :: {TX: %.3f%%, RX: %.3f%%, physock=%p} :: %d bytes",
	    		(float)conn->txsz / (float)max, (float)conn->rxsz / max, conn->sock, r);
	    }
	   	if(conn->socket_type == SOCK_DGRAM) {
	   		max = ZT_UDP_TX_BUF_SZ;
	    	DEBUG_TRANS("[ UDP TX -> STACK] :: {TX: %.3f%%, RX: %.3f%%, physock=%p} :: %d bytes",
	    		(float)conn->txsz / (float)max, (float)conn->rxsz / max, conn->sock, r);
	    }
    }

    int picoTCP::pico_Close(Connection *conn)
    {
    	DEBUG_INFO();
    	int err;
    	if(conn && conn->picosock) {
	    	if((err = pico_socket_close(conn->picosock)) < 0) {
	    		errno = pico_err;
	    		DEBUG_ERROR("error closing pico_socket(%p)", (void*)(conn->picosock));
	    		return -1;
	    	}
	    	return err;
    	}
    	DEBUG_ERROR("invalid connection or pico_socket");
    	return -1;
    }
}
