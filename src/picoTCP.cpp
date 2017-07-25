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

#include <ctime>

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
#include "libzt.h"
#include "Utilities.hpp"
#include "SocketTap.hpp"
#include "picoTCP.hpp"
#include "RingBuffer.hpp"

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
extern "C" int pico_string_to_ipv4(PICO_STRING_TO_IPV4_SIG);
extern "C" int pico_string_to_ipv6(PICO_STRING_TO_IPV6_SIG);
extern "C" int pico_socket_recvfrom(PICO_SOCKET_RECVFROM_SIG);
extern "C" struct pico_socket * pico_socket_open(PICO_SOCKET_OPEN_SIG);
extern "C" int pico_socket_connect(PICO_SOCKET_CONNECT_SIG);
extern "C" int pico_socket_listen(PICO_SOCKET_LISTEN_SIG);
extern "C" int pico_socket_write(PICO_SOCKET_WRITE_SIG);
extern "C" int pico_socket_close(PICO_SOCKET_CLOSE_SIG);
extern "C" struct pico_ipv6_link * pico_ipv6_link_add(PICO_IPV6_LINK_ADD_SIG);

/*
int pico_stack_recv(PICO_STACK_RECV_SIG);
int pico_icmp4_ping(PICO_ICMP4_PING_SIG);
int pico_socket_setoption(PICO_SOCKET_SETOPTION_SIG);
uint32_t pico_timer_add(PICO_TIMER_ADD_SIG);
int pico_socket_send(PICO_SOCKET_SEND_SIG);
int pico_socket_sendto(PICO_SOCKET_SENDTO_SIG);
int pico_socket_recv(PICO_SOCKET_RECV_SIG);
int pico_socket_bind(PICO_SOCKET_BIND_SIG);
int pico_socket_read(PICO_SOCKET_READ_SIG);
int pico_socket_shutdown(PICO_SOCKET_SHUTDOWN_SIG);
struct pico_socket * pico_socket_accept(PICO_SOCKET_ACCEPT_SIG);
*/

namespace ZeroTier {

	struct pico_device picodev;

	bool picoTCP::pico_init_interface(SocketTap *tap, const InetAddress &ip)
	{
		if (std::find(tap->_ips.begin(),tap->_ips.end(),ip) == tap->_ips.end()) {
			tap->_ips.push_back(ip);
			std::sort(tap->_ips.begin(),tap->_ips.end());
			
			if(!tap->picodev_initialized)
			{
				picodev.send = pico_eth_send; // tx
			    picodev.poll = pico_eth_poll; // rx
			    picodev.mtu = tap->_mtu;
			    picodev.tap = tap;
			    uint8_t mac[PICO_SIZE_ETH];
			    tap->_mac.copyTo(mac, PICO_SIZE_ETH);
			    if(pico_device_init(&picodev, "pz", mac) != 0) {
			        DEBUG_ERROR("dev init failed");
			        handle_general_failure();
			        return false;
			    }
				tap->picodev_initialized = true;
			}
			if(ip.isV4())
			{
			    struct pico_ip4 ipaddr, netmask;
			    ipaddr.addr = *((uint32_t *)ip.rawIpData());
			    netmask.addr = *((uint32_t *)ip.netmask().rawIpData());
			    pico_ipv4_link_add(&picodev, ipaddr, netmask);
			    DEBUG_INFO("addr  = %s", ip.toString().c_str());
			    return true;
			}
			if(ip.isV6())
			{
				char ipv6_str[INET6_ADDRSTRLEN], nm_str[INET6_ADDRSTRLEN];
				inet_ntop(AF_INET6, ip.rawIpData(), ipv6_str, INET6_ADDRSTRLEN);
				inet_ntop(AF_INET6, ip.netmask().rawIpData(), nm_str, INET6_ADDRSTRLEN);
				struct pico_ip6 ipaddr, netmask;
			    pico_string_to_ipv6(ipv6_str, ipaddr.addr);
		    	pico_string_to_ipv6(nm_str, netmask.addr);
			    pico_ipv6_link_add(&picodev, ipaddr, netmask);
			    DEBUG_INFO("addr6 = %s", ipv6_str);
			   	return true;    
			}
		}
		return false;
	}
	
	void picoTCP::pico_loop(SocketTap *tap)
	{
		while(tap->_run)
		{
			tap->_phy.poll(ZT_PHY_POLL_INTERVAL);
	        pico_stack_tick();
	        //tap->Housekeeping();
		}
	}

	// from stack socket to app socket
	void picoTCP::pico_cb_tcp_read(ZeroTier::SocketTap *tap, struct pico_socket *s)
	{
		Connection *conn = (Connection*)((ConnectionPair*)(s->priv))->conn;
		Mutex::Lock _l(conn->_rx_m);
		
		if(!conn || !tap) {
    		DEBUG_ERROR("invalid tap or conn");
    		handle_general_failure();
    		return;
    	}

		int r, n;				
		uint16_t port = 0;
		union {
	        struct pico_ip4 ip4;
	        struct pico_ip6 ip6;
	    } peer;

		do {
			n = 0;
			//DEBUG_INFO("RXbuf->count() = %d", conn->RXbuf->count());
			int avail = ZT_TCP_RX_BUF_SZ - conn->RXbuf->count();
			if(avail) {

	            r = pico_socket_recvfrom(s, conn->RXbuf->get_buf(), ZT_STACK_SOCKET_RD_MAX, 
	            	(void *)&peer.ip4.addr, &port);
	            
	            conn->tot += r;

	            if (r > 0)
	            {
	            	conn->RXbuf->produce(r);
	            	//DEBUG_INFO("RXbuf->count() = %d", conn->RXbuf->count());
	            	n = tap->_phy.streamSend(conn->sock, conn->RXbuf->get_buf(), r);

	            	if(n>0)
	            		conn->RXbuf->consume(n);
	            	//DEBUG_INFO("pico_recv = %d, streamSend = %d, rxsz = %d, tot = %d", r, n, conn->RXbuf->count(), conn->tot);

	            	//DEBUG_TRANS("[ TCP RX <- STACK] :: conn = %p, len = %d", conn, n);
	            }

	        	if(conn->RXbuf->count() == 0) {
					tap->_phy.setNotifyWritable(conn->sock, false);
				}
				else {
	        		tap->_phy.setNotifyWritable(conn->sock, true);	
				}				
        	}
        	else {
        		//tap->_phy.setNotifyWritable(conn->sock, false);
        		DEBUG_ERROR("not enough space left on I/O RX buffer for pico_socket(%p)", s);
        		handle_general_failure();
        	}
        }
    	while(r > 0);
	}

	// from stack socket to app socket
	void picoTCP::pico_cb_udp_read(SocketTap *tap, struct pico_socket *s)
	{
		/*
		DEBUG_INFO();
		Connection *conn = (Connection*)((ConnectionPair*)(s->priv))->conn;
		Mutex::Lock _l(conn->_rx_m);
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
            	handle_general_failure();
    		} 
	        tap->_rx_buf_m.unlock(); 

	        if(r)
	            tap->phyOnUnixWritable(conn->sock, NULL, true);
        	//DEBUG_EXTRA(" Copied onto rxbuf (%d) from stack socket", r);
        	return;
		}
		*/
	}

	void picoTCP::pico_cb_tcp_write(SocketTap *tap, struct pico_socket *s)
	{
		Connection *conn = (Connection*)((ConnectionPair*)(s->priv))->conn;
	    Mutex::Lock _l(conn->_tx_m);
	   	if(!conn) {
			DEBUG_ERROR("invalid connection");
			handle_general_failure();
			return;
		}
	   	int txsz = conn->TXbuf->count();
	   	if(txsz <= 0)
	   		return;
	    //DEBUG_INFO("TXbuf->count() = %d", conn->TXbuf->count());

		int r, max_write_len = std::min(std::min(txsz, ZT_SDK_MTU),ZT_STACK_SOCKET_WR_MAX);
	    if((r = pico_socket_write(conn->picosock, conn->TXbuf->get_buf(), max_write_len)) < 0) {
	    	DEBUG_ERROR("unable to write to picosock=%p, r=%d", conn->picosock, r);
	    	handle_general_failure();
	    	return;
	    }
	   	if(conn->socket_type == SOCK_STREAM) {
	    	//DEBUG_TRANS("[ TCP TX -> STACK] :: conn = %p, len = %d", conn, r);
	    }
	   	if(conn->socket_type == SOCK_DGRAM) {
	    	//DEBUG_TRANS("[ UDP TX -> STACK] :: conn = %p, len = %d", conn, r);
	    }
	    if(r == 0) {
	    	// This is a peciliarity of the picoTCP network stack, if we receive no error code, and the size of 
	    	// the byte stream written is 0, this is an indication that the buffer for this pico_socket is too small
	    	// DEBUG_ERROR("pico_socket buffer is too small (adjust ZT_STACK_SOCKET_TX_SZ, ZT_STACK_SOCKET_RX_SZ)");
	    	// handle_general_failure(); 
	    }
	    if(r>0)
	    	conn->TXbuf->consume(r);
	}

	void picoTCP::pico_cb_socket_activity(uint16_t ev, struct pico_socket *s)
    {
    	if(!(SocketTap*)((ConnectionPair*)(s->priv)))
    		return;
    	SocketTap *tap = (SocketTap*)((ConnectionPair*)(s->priv))->tap;
    	Connection *conn = (Connection*)((ConnectionPair*)(s->priv))->conn;
    	if(!tap || !conn) {
    		DEBUG_ERROR("invalid tap or conn");
    		handle_general_failure();
    		return;
    	}
    	int err = 0;
        if(!conn) {
        	DEBUG_ERROR("invalid connection");
        	handle_general_failure();
        	return;
        }
        // PICO_SOCK_EV_CONN - triggered when connection is established (TCP only). This event is
		// received either after a successful call to pico socket connect to indicate that the connection
		// has been established, or on a listening socket, indicating that a call to pico socket accept
		// may now be issued in order to accept the incoming connection from a remote host.
        if (ev & PICO_SOCK_EV_CONN) {
        	if(conn->state == ZT_SOCK_STATE_LISTENING)
        	{
        		Mutex::Lock _l(tap->_tcpconns_m);
	            uint32_t peer;
				uint16_t port;
	            struct pico_socket *client_psock = pico_socket_accept(s, &peer, &port);
	            if(!client_psock) {
	            	DEBUG_ERROR("pico_err=%s, picosock=%p", beautify_pico_error(pico_err), s);
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


                int value = 1;
                pico_socket_setoption(newConn->picosock, PICO_TCP_NODELAY, &value);
                
				if(ZT_SOCK_BEHAVIOR_LINGER) {
                    int linger_time_ms = ZT_SOCK_BEHAVIOR_LINGER_TIME;
                    int t_err = 0;
                	if((t_err = pico_socket_setoption(newConn->picosock, PICO_SOCKET_OPT_LINGER, &linger_time_ms)) < 0)
                   		DEBUG_ERROR("unable to set LINGER size, err = %d, pico_err = %d, app_fd=%d, sdk_fd=%d", t_err, pico_err, conn->app_fd, conn->sdk_fd);
           		}
/*
               	linger_time_ms = 0;
                if((t_err = pico_socket_getoption(newConn->picosock, PICO_SOCKET_OPT_LINGER, &linger_time_ms)) < 0)
                   DEBUG_ERROR("unable to set LINGER size, err = %d, pico_err = %d", t_err, pico_err);
				DEBUG_TEST("getting linger = %d", linger_time_ms);
*/
				// For I/O loop participation and referencing the PhySocket's parent Connection in callbacks
				newConn->sock = tap->_phy.wrapSocket(newConn->sdk_fd, newConn);
				//DEBUG_ERROR("sock->fd = %d", tap->_phy.getDescriptor(newConn->sock));      

			}
			if(conn->state != ZT_SOCK_STATE_LISTENING) {
				// set state so socket multiplexer logic will pick this up
				conn->state = ZT_SOCK_STATE_UNHANDLED_CONNECTED;
			}
        }

        // PICO_SOCK_EV_FIN - triggered when the socket is closed. No further communication is
		// possible from this point on the socket.
        if (ev & PICO_SOCK_EV_FIN) {
            //DEBUG_EXTRA("PICO_SOCK_EV_FIN (socket closed), picosock=%p, conn=%p, app_fd=%d, sdk_fd=%d", s, conn, conn->app_fd, conn->sdk_fd);
            conn->closure_ts = std::time(nullptr);	
        }

        // PICO_SOCK_EV_ERR - triggered when an error occurs.
        if (ev & PICO_SOCK_EV_ERR) {
        	if(pico_err == PICO_ERR_ECONNRESET) {
        		DEBUG_ERROR("PICO_ERR_ECONNRESET");
        		conn->state = PICO_ERR_ECONNRESET;
        	}
            DEBUG_ERROR("PICO_SOCK_EV_ERR, err=%s, picosock=%p, app_fd=%d, sdk_fd=%d", beautify_pico_error(pico_err), s, conn->app_fd, conn->sdk_fd); 
        }
        // PICO_SOCK_EV_CLOSE - triggered when a FIN segment is received (TCP only). This event
		// indicates that the oher endpont has closed the connection, so the local TCP layer is only
		// allowed to send new data until a local shutdown or close is initiated. PicoTCP is able to
		// keep the connection half-open (only for sending) after the FIN packet has been received,
		// allowing new data to be sent in the TCP CLOSE WAIT state.
        if (ev & PICO_SOCK_EV_CLOSE) {
            err = pico_socket_close(s);
            //DEBUG_INFO("PICO_SOCK_EV_CLOSE (socket closure) err = %d, picosock=%p, conn=%p, app_fd=%d, sdk_fd=%d", err, s, conn, conn->app_fd, conn->sdk_fd);            
            conn->closure_ts = std::time(nullptr);	
            return;
        }
        // PICO_SOCK_EV_RD - triggered when new data arrives on the socket. A new receive action
		// can be taken by the socket owner because this event indicates there is new data to receive.
        if (ev & PICO_SOCK_EV_RD) {
        	if(conn->socket_type==SOCK_STREAM)
            	pico_cb_tcp_read(tap, s);
        	if(conn->socket_type==SOCK_DGRAM)
        		pico_cb_udp_read(tap, s);
        }
        // PICO_SOCK_EV_WR - triggered when ready to write to the socket. Issuing a write/send call
		// will now succeed if the buffer has enough space to allocate new outstanding data
        if (ev & PICO_SOCK_EV_WR) {
            pico_cb_tcp_write(tap, s);
        }
    }
   
   	int pico_eth_send(struct pico_device *dev, void *buf, int len)
    {
    	//DEBUG_INFO("len = %d", len);
    	SocketTap *tap = (SocketTap*)(dev->tap);
    	if(!tap) {
    		DEBUG_ERROR("invalid dev->tap");
    		handle_general_failure();
    		return ZT_ERR_GENERAL_FAILURE;
    	}
        struct pico_eth_hdr *ethhdr;
        ethhdr = (struct pico_eth_hdr *)buf;
        MAC src_mac;
        MAC dest_mac;
        src_mac.setTo(ethhdr->saddr, 6);
        dest_mac.setTo(ethhdr->daddr, 6);
        tap->_handler(tap->_arg,NULL,tap->_nwid,src_mac,dest_mac,
            Utils::ntoh((uint16_t)ethhdr->proto),0, ((char*)buf) + sizeof(struct pico_eth_hdr),len - sizeof(struct pico_eth_hdr));
        return len;
    }

    // receive frames from zerotier virtual wire and copy them to a guarded buffer awaiting placement into network stack
    void picoTCP::pico_rx(SocketTap *tap, const MAC &from,const MAC &to,unsigned int etherType,
    	const void *data,unsigned int len)
	{
		DEBUG_INFO("len = %d", len);
		if(!tap) {
    		DEBUG_ERROR("invalid tap");
    		handle_general_failure();
    		return;
    	}
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
		//DEBUG_FLOW("[ ZWIRE -> FBUF ] Move FRAME(sz=%d) into FBUF(sz=%d), data_len=%d", newlen, tap->pico_frame_rxbuf_tot, len);
	}

	// feed frames on the guarded RX buffer (from zerotier virtual wire) into the network stack
    int pico_eth_poll(struct pico_device *dev, int loop_score)
    {
    	SocketTap *tap = (SocketTap*)(dev->tap);
    	if(!tap) {
    		DEBUG_ERROR("invalid dev->tap");
    		handle_general_failure();
    		return ZT_ERR_GENERAL_FAILURE;
    	}
        // FIXME: The copy logic and/or buffer structure should be reworked for better performance after the BETA
        // SocketTap *tap = (SocketTap*)netif->state;
        Mutex::Lock _l(tap->_pico_frame_rxbuf_m);
        unsigned char frame[ZT_SDK_MTU];
        int len;
    	int err = 0;
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
            	handle_general_failure();
            }
            loop_score--;
        }
        return loop_score;
    }

    int picoTCP::pico_Connect(Connection *conn, int fd, const struct sockaddr *addr, socklen_t addrlen)
    {		
    	if(!conn || !conn->picosock) {
    		DEBUG_ERROR("invalid conn or conn->picosock");
    		handle_general_failure();
    		return ZT_ERR_GENERAL_FAILURE;
    	}
		int err = 0;
		if(conn->socket_family == AF_INET) {
			struct pico_ip4 zaddr;
			memset(&zaddr, 0, sizeof (struct pico_ip4));
			struct sockaddr_in *in4 = (struct sockaddr_in*)addr;
			char ipv4_str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, (const void *)&in4->sin_addr.s_addr, ipv4_str, INET_ADDRSTRLEN);
			uint32_t ipval = 0;
			pico_string_to_ipv4(ipv4_str, &ipval);
			zaddr.addr = ipval;
			err = pico_socket_connect(conn->picosock, &zaddr, in4->sin_port);
		}
		if(conn->socket_family == AF_INET6) {
			struct pico_ip6 zaddr;
			struct sockaddr_in6 *in6 = (struct sockaddr_in6*)addr;
			char ipv6_str[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, &(in6->sin6_addr), ipv6_str, INET6_ADDRSTRLEN);
	    	pico_string_to_ipv6(ipv6_str, zaddr.addr);
			err = pico_socket_connect(conn->picosock, &zaddr, in6->sin6_port);
		}
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
    	//DEBUG_INFO();
    	if(!conn || !conn->picosock) {
    		DEBUG_ERROR("invalid conn or conn->picosock");
    		handle_general_failure();
    		return ZT_ERR_GENERAL_FAILURE;
    	}
    	int err = 0;
		if(conn->socket_family == AF_INET) { 
			struct pico_ip4 zaddr;
			memset(&zaddr, 0, sizeof (struct pico_ip4));
			struct sockaddr_in *in4 = (struct sockaddr_in*)addr;
			char ipv4_str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, (const void *)&in4->sin_addr.s_addr, ipv4_str, INET_ADDRSTRLEN);
			pico_string_to_ipv4(ipv4_str, &(zaddr.addr));
			//DEBUG_EXTRA("addr=%s:%d", ipv4_str, Utils::ntoh(in4->sin_port));
			err = pico_socket_bind(conn->picosock, &zaddr, (uint16_t *)&(in4->sin_port));
		}
		if(conn->socket_family == AF_INET6) { 
			struct pico_ip6 pip6;
			struct sockaddr_in6 *in6 = (struct sockaddr_in6*)addr;
			char ipv6_str[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, &(in6->sin6_addr), ipv6_str, INET6_ADDRSTRLEN);
			// TODO: This isn't proper
	    	pico_string_to_ipv6("::", pip6.addr);
       		//DEBUG_EXTRA("addr=%s:%d", ipv6_str, Utils::ntoh(in6->sin6_port));
			err = pico_socket_bind(conn->picosock, &pip6, (uint16_t *)&(in6->sin6_port));
		}
		if(err < 0) {
			if(pico_err < 0)
				DEBUG_ERROR("pico_err = %d", pico_err);
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
    	//DEBUG_INFO();
    	if(!conn || !conn->picosock) {
    		DEBUG_ERROR("invalid conn or conn->picosock");
    		handle_general_failure();
    		return ZT_ERR_GENERAL_FAILURE;
    	}
    	int err = 0;
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

    Connection* picoTCP::pico_Accept(Connection *conn)
    {
    	if(!conn) {
    		DEBUG_ERROR("invalid conn");
    		handle_general_failure();
    		return NULL;
    	}
    	// Retreive first of queued Connections from parent connection
    	Connection *new_conn = NULL;
        if(conn->_AcceptedConnections.size()) {
			new_conn = conn->_AcceptedConnections.front();
			conn->_AcceptedConnections.pop();
		}
    	return new_conn;
    }

    void picoTCP::pico_Read(SocketTap *tap, PhySocket *sock, Connection* conn, bool stack_invoked)
    {
    	DEBUG_INFO();
    	//exit(0);
    	/*
    	if(!conn || !tap || !conn) {
    		DEBUG_ERROR("invalid tap, sock, or conn");
    		handle_general_failure();
    		return;
    	}
    	//DEBUG_INFO();
        if(!stack_invoked) {
        	// The stack thread writes to RXBUF as well
            tap->_tcpconns_m.lock();
            tap->_rx_buf_m.lock(); 
        }
        int tot = 0, n = -1, write_attempts = 0;
		
		if(conn && conn->rxsz) {	
			//DEBUG_INFO("conn = %p", conn);
			//	
			if(conn->socket_type==SOCK_DGRAM) {
				// Try to write ZT_SDK_MTU-sized chunk to app socket
				while(tot < ZT_SDK_MTU) {
					write_attempts++;
					n = tap->_phy.streamSend(conn->sock, (conn->rxbuf)+tot, ZT_SDK_MTU);
					tot += n;
					//DEBUG_FLOW("[ ZTSOCK <- RXBUF] wrote = %d, errno=%d", n, errno);
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
				//DEBUG_INFO("writing to conn->sock = %p, conn->sdk_fd=%d, conn->app_fd=%d", conn->sock, conn->sdk_fd, conn->app_fd);
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
					//#if DEBUG_LEVEL >= MSG_TRANSFER
		            //	DEBUG_TRANS("[ TCP RX <- STACK] :: conn = %p, len = %d", conn, n);
					//#endif
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
        // DEBUG_FLOW("[ ZTSOCK <- RXBUF] Emitted (%d) from RXBUF(%d) to socket", tot, conn->rxsz);
        */
    }

    void picoTCP::pico_Write(Connection *conn, void *data, ssize_t len)
    {
	    // TODO: Add RingBuffer overflow checks
	    //DEBUG_INFO("conn=%p, len = %d", conn, len);
	    Mutex::Lock _l(conn->_tx_m);
		if(len <= 0) {
    		DEBUG_ERROR("invalid write length (len=%d)", len);
    		handle_general_failure();
    		return;
    	}
    	if(conn->picosock->state & PICO_SOCKET_STATE_CLOSED){
    		DEBUG_ERROR("socket is CLOSED, this write() will fail");
    		return;
    	}
    	if(!conn) {
    		DEBUG_ERROR("invalid connection (len=%d)", len);
    		handle_general_failure();
    		return;
    	}

    	int original_txsz = conn->TXbuf->count();

    	if(original_txsz + len >= ZT_TCP_TX_BUF_SZ) {
    		DEBUG_ERROR("txsz = %d, len = %d", original_txsz, len);
    		DEBUG_ERROR("TX buffer is too small, try increasing ZT_TCP_TX_BUF_SZ in libzt.h");
    		exit(0);
    	}

	    int buf_w = conn->TXbuf->write((const unsigned char*)data, len);
            if (buf_w != len) {
		// because we checked ZT_TCP_TX_BUF_SZ above, this should not happen
    		DEBUG_ERROR("TX wrote only %d but expected to write %d", buf_w, len);
    		exit(0);
	    }
	    //DEBUG_INFO("TXbuf->count() = %d", conn->TXbuf->count());
	   	int txsz = conn->TXbuf->count();

	   	//if(original_txsz > 0)
	   	//	return; // don't write here, we already have stuff in the queue, a callback will handle it
	   	
		int r, max_write_len = std::min(std::min(txsz, ZT_SDK_MTU),ZT_STACK_SOCKET_WR_MAX);
		//int buf_r = conn->TXbuf->read(conn->tmptxbuf, max_write_len);
	    
	    if((r = pico_socket_write(conn->picosock, conn->TXbuf->get_buf(), max_write_len)) < 0) {
	    	DEBUG_ERROR("unable to write to picosock=%p, r=%d", conn->picosock, r);
	    	return;
	    }
	   	if(conn->socket_type == SOCK_STREAM) {
	    	//DEBUG_TRANS("[ TCP TX -> STACK] :: conn = %p, len = %d", conn, r);
	    }
	   	if(conn->socket_type == SOCK_DGRAM) {
	    	//DEBUG_TRANS("[ UDP TX -> STACK] :: conn = %p, len = %d", conn, r);
	    }
	    if(r>0)
	    	conn->TXbuf->consume(r);
    }

    int picoTCP::pico_Close(Connection *conn)
    {
    	DEBUG_INFO("conn = %p, picosock=%p, fd = %d", conn, conn->picosock, conn->app_fd);
    	if(!conn || !conn->picosock)
    		return ZT_ERR_GENERAL_FAILURE;
    	int err = 0;
    	Mutex::Lock _l(conn->tap->_tcpconns_m);
    	if(conn->closure_ts != -1) // it was closed at some point in the past, it'll work itself out 
    		return ZT_ERR_OK;
    	if((err = pico_socket_close(conn->picosock)) < 0) {
    		errno = pico_err;
    		DEBUG_ERROR("error closing pico_socket(%p)", (void*)(conn->picosock));
    	}
    	return err;
    }

	char *picoTCP::beautify_pico_error(int err)
	{
		if(err==  0) return (char*)"PICO_ERR_NOERR";
		if(err==  1) return (char*)"PICO_ERR_EPERM";
		if(err==  2) return (char*)"PICO_ERR_ENOENT";
		// ...
		if(err==  4) return (char*)"PICO_ERR_EINTR";
		if(err==  5) return (char*)"PICO_ERR_EIO";
		if(err==  6) return (char*)"PICO_ERR_ENXIO";
		// ...
		if(err== 11) return (char*)"PICO_ERR_EAGAIN";
		if(err== 12) return (char*)"PICO_ERR_ENOMEM";
		if(err== 13) return (char*)"PICO_ERR_EACCESS";
		if(err== 14) return (char*)"PICO_ERR_EFAULT";
		// ...
		if(err== 16) return (char*)"PICO_ERR_EBUSY";
		if(err== 17) return (char*)"PICO_ERR_EEXIST";
		// ...
		if(err== 22) return (char*)"PICO_ERR_EINVAL";
		// ...
		if(err== 64) return (char*)"PICO_ERR_ENONET";
		// ...
		if(err== 71) return (char*)"PICO_ERR_EPROTO";
		// ...
		if(err== 92) return (char*)"PICO_ERR_ENOPROTOOPT";
		if(err== 93) return (char*)"PICO_ERR_EPROTONOSUPPORT";
		// ...
		if(err== 95) return (char*)"PICO_ERR_EOPNOTSUPP";
		if(err== 98) return (char*)"PICO_ERR_EADDRINUSE";
		if(err== 99) return (char*)"PICO_ERR_EADDRNOTAVAIL";
		if(err==100) return (char*)"PICO_ERR_ENETDOWN";
		if(err==101) return (char*)"PICO_ERR_ENETUNREACH";
		// ...
		if(err==104) return (char*)"PICO_ERR_ECONNRESET";
		// ...
		if(err==106) return (char*)"PICO_ERR_EISCONN";
		if(err==107) return (char*)"PICO_ERR_ENOTCONN";
		if(err==108) return (char*)"PICO_ERR_ESHUTDOWN";
		// ...
		if(err==110) return (char*)"PICO_ERR_ETIMEDOUT";
		if(err==111) return (char*)"PICO_ERR_ECONNREFUSED";
		if(err==112) return (char*)"PICO_ERR_EHOSTDOWN";
		if(err==113) return (char*)"PICO_ERR_EHOSTUNREACH";
		return (char*)"UNKNOWN_ERROR";
	}

/*

#define PICO_SOCKET_STATE_UNDEFINED       0x0000u
#define PICO_SOCKET_STATE_SHUT_LOCAL      0x0001u
#define PICO_SOCKET_STATE_SHUT_REMOTE     0x0002u
#define PICO_SOCKET_STATE_BOUND           0x0004u
#define PICO_SOCKET_STATE_CONNECTED       0x0008u
#define PICO_SOCKET_STATE_CLOSING         0x0010u
#define PICO_SOCKET_STATE_CLOSED          0x0020u

# define PICO_SOCKET_STATE_TCP                0xFF00u
# define PICO_SOCKET_STATE_TCP_UNDEF          0x00FFu
# define PICO_SOCKET_STATE_TCP_CLOSED         0x0100u
# define PICO_SOCKET_STATE_TCP_LISTEN         0x0200u
# define PICO_SOCKET_STATE_TCP_SYN_SENT       0x0300u
# define PICO_SOCKET_STATE_TCP_SYN_RECV       0x0400u
# define PICO_SOCKET_STATE_TCP_ESTABLISHED    0x0500u
# define PICO_SOCKET_STATE_TCP_CLOSE_WAIT     0x0600u
# define PICO_SOCKET_STATE_TCP_LAST_ACK       0x0700u
# define PICO_SOCKET_STATE_TCP_FIN_WAIT1      0x0800u
# define PICO_SOCKET_STATE_TCP_FIN_WAIT2      0x0900u
# define PICO_SOCKET_STATE_TCP_CLOSING        0x0a00u
# define PICO_SOCKET_STATE_TCP_TIME_WAIT      0x0b00u
# define PICO_SOCKET_STATE_TCP_ARRAYSIZ       0x0cu

*/
	char *picoTCP::beautify_pico_state(int state)
	{
		static char state_str[512];
		char *str_ptr = state_str;

		if(state & PICO_SOCKET_STATE_UNDEFINED) {
			sprintf(str_ptr, "UNDEFINED ");
			str_ptr += strlen("UNDEFINED ");
		}
		if(state & PICO_SOCKET_STATE_SHUT_LOCAL) {
			sprintf(str_ptr, "SHUT_LOCAL ");
			str_ptr += strlen("SHUT_LOCAL ");
		}
		if(state & PICO_SOCKET_STATE_SHUT_REMOTE) {
			sprintf(str_ptr, "SHUT_REMOTE ");
			str_ptr += strlen("SHUT_REMOTE ");
		}
		if(state & PICO_SOCKET_STATE_BOUND) {
			sprintf(str_ptr, "BOUND ");
			str_ptr += strlen("BOUND ");
		}
		if(state & PICO_SOCKET_STATE_CONNECTED) {
			sprintf(str_ptr, "CONNECTED ");
			str_ptr += strlen("CONNECTED ");
		}
		if(state & PICO_SOCKET_STATE_CLOSING) {
			sprintf(str_ptr, "CLOSING ");
			str_ptr += strlen("CLOSING ");
		}
		if(state & PICO_SOCKET_STATE_CLOSED) {
			sprintf(str_ptr, "CLOSED ");
			str_ptr += strlen("CLOSED ");
		}


		if(state & PICO_SOCKET_STATE_TCP) {
			sprintf(str_ptr, "TCP ");
			str_ptr += strlen("TCP ");
		}
		if(state & PICO_SOCKET_STATE_TCP_UNDEF) {
			sprintf(str_ptr, "TCP_UNDEF ");
			str_ptr += strlen("TCP_UNDEF ");
		}
		if(state & PICO_SOCKET_STATE_TCP_CLOSED) {
			sprintf(str_ptr, "TCP_CLOSED ");
			str_ptr += strlen("TCP_CLOSED ");
		}
		if(state & PICO_SOCKET_STATE_TCP_LISTEN) {
			sprintf(str_ptr, "TCP_LISTEN ");
			str_ptr += strlen("TCP_LISTEN ");
		}
		if(state & PICO_SOCKET_STATE_TCP_SYN_SENT) {
			sprintf(str_ptr, "TCP_SYN_SENT ");
			str_ptr += strlen("TCP_SYN_SENT ");
		}
		if(state & PICO_SOCKET_STATE_TCP_SYN_RECV) {
			sprintf(str_ptr, "TCP_SYN_RECV ");
			str_ptr += strlen("TCP_SYN_RECV ");
		}
		if(state & PICO_SOCKET_STATE_TCP_ESTABLISHED) {
			sprintf(str_ptr, "TCP_ESTABLISHED ");
			str_ptr += strlen("TCP_ESTABLISHED ");
		}
		if(state & PICO_SOCKET_STATE_TCP_CLOSE_WAIT) {
			sprintf(str_ptr, "TCP_CLOSE_WAIT ");
			str_ptr += strlen("TCP_CLOSE_WAIT ");
		}
		if(state & PICO_SOCKET_STATE_TCP_LAST_ACK) {
			sprintf(str_ptr, "TCP_LAST_ACK ");
			str_ptr += strlen("TCP_LAST_ACK ");
		}
		if(state & PICO_SOCKET_STATE_TCP_FIN_WAIT1) {
			sprintf(str_ptr, "TCP_FIN_WAIT1 ");
			str_ptr += strlen("TCP_FIN_WAIT1 ");
		}
		if(state & PICO_SOCKET_STATE_TCP_FIN_WAIT2) {
			sprintf(str_ptr, "TCP_FIN_WAIT2 ");
			str_ptr += strlen("TCP_FIN_WAIT2 ");
		}
		if(state & PICO_SOCKET_STATE_TCP_CLOSING) {
			sprintf(str_ptr, "TCP_CLOSING ");
			str_ptr += strlen("TCP_CLOSING ");
		}
		if(state & PICO_SOCKET_STATE_TCP_TIME_WAIT) {
			sprintf(str_ptr, "TCP_TIME_WAIT ");
			str_ptr += strlen("TCP_TIME_WAIT ");
		}
		if(state & PICO_SOCKET_STATE_TCP_ARRAYSIZ) {
			sprintf(str_ptr, "TCP_ARRAYSIZ ");
			str_ptr += strlen("TCP_ARRAYSIZ ");
		}
		return (char*)state_str;
	}
}
