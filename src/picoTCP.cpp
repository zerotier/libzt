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
#include <stdint.h>

#include "pico_eth.h"
#include "pico_stack.h"
#include "pico_ipv4.h"
#include "pico_icmp4.h"
#include "pico_dev_tap.h"
#include "pico_protocol.h"
#include "pico_socket.h"
#include "pico_device.h"
#include "pico_ipv6.h"
#include "pico_tcp.h"
#include "pico_dns_client.h"

#include "libzt.h"
#include "Utilities.hpp"
#include "VirtualTap.hpp"
#include "picoTCP.hpp"
#include "RingBuffer.hpp"

#include "Utils.hpp"
#include "OSUtils.hpp"
#include "Mutex.hpp"
#include "Constants.hpp"
#include "Phy.hpp"

int pico_ipv4_to_string(PICO_IPV4_TO_STRING_SIG);
extern "C" int pico_stack_init(void);
extern "C" void pico_stack_tick(void);
extern "C" int pico_ipv4_link_add(PICO_IPV4_LINK_ADD_SIG);
extern "C" int pico_ipv4_route_add(PICO_IPV4_ROUTE_ADD_SIG);
extern "C" int pico_ipv4_route_del(PICO_IPV4_ROUTE_DEL_SIG);
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
extern "C" int pico_dns_client_nameserver(PICO_DNS_CLIENT_NAMESERVER_SIG);

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

extern std::vector<void*> vtaps;

/*
 * Whether our picoTCP device has been initialized
 */
static bool picodev_initialized;

namespace ZeroTier {

	struct pico_device picodev;
	ZeroTier::Mutex _picostack_driver_lock;

	bool picoTCP::pico_init_interface(VirtualTap *tap)
	{
		bool err = false;
		_picostack_driver_lock.lock();
		// give right to vtap to start the stack
		// only one stack loop is permitted 
		if(!picodev_initialized) {
			tap->should_start_stack = true;
			picodev.send = pico_eth_tx; // tx
			picodev.poll = pico_eth_poll; // calls pico_eth_rx
			picodev.mtu = tap->_mtu;
			picodev.tap = tap;
			uint8_t mac[PICO_SIZE_ETH];
			tap->_mac.copyTo(mac, PICO_SIZE_ETH);
			if(pico_device_init(&picodev, tap->vtap_abbr_name, mac) != 0) {
				DEBUG_ERROR("dev init failed");
				handle_general_failure();
				err = false;
			}
			picodev_initialized = true;
			err = true;
		}
		_picostack_driver_lock.unlock();
		return err;
	}

	bool picoTCP::pico_register_address(VirtualTap *tap, const InetAddress &ip)
	{
		_picostack_driver_lock.lock();
		bool err = false;
		char ipbuf[INET6_ADDRSTRLEN];
		uint8_t hwaddr[6];	
		// register addresses
		if(ip.isV4()) {
			struct pico_ip4 ipaddr, netmask;
			ipaddr.addr = *((uint32_t *)ip.rawIpData());
			netmask.addr = *((uint32_t *)ip.netmask().rawIpData());
			pico_ipv4_link_add(&picodev, ipaddr, netmask);
			DEBUG_INFO("addr=%s", ip.toString(ipbuf));
			tap->_mac.copyTo(hwaddr, 6);
			char macbuf[ZT_MAC_ADDRSTRLEN];
			mac2str(macbuf, ZT_MAC_ADDRSTRLEN, hwaddr);
			DEBUG_INFO("mac=%s", macbuf);
			err = true;
		}
		if(ip.isV6()) {
			char ipv6_str[INET6_ADDRSTRLEN], nm_str[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, ip.rawIpData(), ipv6_str, INET6_ADDRSTRLEN);
			inet_ntop(AF_INET6, ip.netmask().rawIpData(), nm_str, INET6_ADDRSTRLEN);
			struct pico_ip6 ipaddr, netmask;
			pico_string_to_ipv6(ipv6_str, ipaddr.addr);
			pico_string_to_ipv6(nm_str, netmask.addr);
			pico_ipv6_link_add(&picodev, ipaddr, netmask);
			DEBUG_INFO("addr=%s", ipv6_str);
			tap->_mac.copyTo(hwaddr, 6);
			char macbuf[ZT_MAC_ADDRSTRLEN];
			mac2str(macbuf, ZT_MAC_ADDRSTRLEN, hwaddr);
			DEBUG_INFO("mac=%s", macbuf);
			err = true;    
		}
		_picostack_driver_lock.unlock();
		return err;
	}

	// TODO:
	// pico_ipv6_route_add
	// pico_ipv6_route_del

	bool picoTCP::pico_route_add(VirtualTap *tap, const InetAddress &addr, const InetAddress &nm, const InetAddress &gw, int metric)
	{
		struct pico_ipv4_link *link = NULL;
		struct pico_ip4 address;
		address.addr = *((uint32_t *)addr.rawIpData());
		struct pico_ip4 netmask;
		netmask.addr = *((uint32_t *)nm.rawIpData());
		struct pico_ip4 gateway;
		gateway.addr = *((uint32_t *)gw.rawIpData());
		int err = pico_ipv4_route_add(address, netmask, gateway, metric, link);
		if(err) {
			DEBUG_ERROR("err=%d, %s", err, beautify_pico_error(pico_err));
		}
		return err;
	}
	
	bool picoTCP::pico_route_del(VirtualTap *tap, const InetAddress &addr, const InetAddress &nm, int metric)
	{
		struct pico_ip4 address;
		address.addr = *((uint32_t *)addr.rawIpData());
		struct pico_ip4 netmask;
		netmask.addr = *((uint32_t *)nm.rawIpData());
		int err = pico_ipv4_route_del(address, netmask, metric);
		if(err) {
			DEBUG_ERROR("err=%d, %s", err, beautify_pico_error(pico_err));
		}
		return err;
	}

	int picoTCP::pico_add_dns_nameserver(struct sockaddr *addr)
	{
		int err = errno = 0; 
		// TODO: De-complexify this
		struct pico_ip4 ns;
		memset(&ns, 0, sizeof (struct pico_ip4));
		struct sockaddr_in *in4 = (struct sockaddr_in*)addr;
		char ipv4_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, (const void *)&in4->sin_addr.s_addr, ipv4_str, INET_ADDRSTRLEN);
		uint32_t ipval = 0;
		pico_string_to_ipv4(ipv4_str, &ipval);
		ns.addr = ipval;
		if((err = pico_dns_client_nameserver(&ns, PICO_DNS_NS_ADD)) < 0) {
			DEBUG_ERROR("error while adding DNS nameserver, err=%d, pico_err=%d, %s", 
				err, pico_err, beautify_pico_error(pico_err));
			map_pico_err_to_errno(pico_err);
		}
		return err;
	}
	
	int picoTCP::pico_del_dns_nameserver(struct sockaddr *addr)
	{
		int err = errno = 0; 
		// TODO: De-complexify this
		struct pico_ip4 ns;
		memset(&ns, 0, sizeof (struct pico_ip4));
		struct sockaddr_in *in4 = (struct sockaddr_in*)addr;
		char ipv4_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, (const void *)&in4->sin_addr.s_addr, ipv4_str, INET_ADDRSTRLEN);
		uint32_t ipval = 0;
		pico_string_to_ipv4(ipv4_str, &ipval);
		ns.addr = ipval;
		if((err = pico_dns_client_nameserver(&ns, PICO_DNS_NS_DEL)) < 0) {
			DEBUG_ERROR("error while removing DNS nameserver, err=%d, pico_err=%d, %s", 
				err, pico_err, beautify_pico_error(pico_err));
		}
		return err;
	}

	void picoTCP::pico_loop(VirtualTap *tap)
	{
		while(tap->_run)
		{
			tap->_phy.poll(ZT_PHY_POLL_INTERVAL);
			//_picostack_driver_lock.lock();
			pico_stack_tick();
			//_picostack_driver_lock.unlock();
			tap->Housekeeping();
		}
	}

	// from stack socket to app socket
	void picoTCP::pico_cb_tcp_read(ZeroTier::VirtualTap *tap, struct pico_socket *s)
	{
		VirtualSocket *vs = (VirtualSocket*)(((VirtualBindingPair*)s->priv)->vs);
		if(!vs) {
			DEBUG_ERROR("s->priv yielded no valid vs");
			handle_general_failure();
			return;
		}
		Mutex::Lock _l(vs->_rx_m);
		
		if(!tap) {
			DEBUG_ERROR("invalid tap");
			handle_general_failure();
			return;
		}
		if(!vs) {
			DEBUG_ERROR("invalid vs");
			handle_general_failure();
			return;
		}
		int r;		
		uint16_t port = 0;
		union {
			struct pico_ip4 ip4;
			struct pico_ip6 ip6;
		} peer;
		
		do {
			int n = 0;
			int avail = ZT_TCP_RX_BUF_SZ - vs->RXbuf->count();
			if(avail) {
				r = pico_socket_recvfrom(s, vs->RXbuf->get_buf(), ZT_STACK_SOCKET_RD_MAX, 
					(void *)&peer.ip4.addr, &port);
				if (r > 0)
				{
					vs->RXbuf->produce(r);
					n = tap->_phy.streamSend(vs->sock, vs->RXbuf->get_buf(), r);

					if(n>0)
						vs->RXbuf->consume(n);
				}
				if(vs->RXbuf->count() == 0) {
					tap->_phy.setNotifyWritable(vs->sock, false);
				}
				else {
					tap->_phy.setNotifyWritable(vs->sock, true);	
				}				
			}
			else {
				//tap->_phy.setNotifyWritable(vs->sock, false);
				DEBUG_ERROR("not enough space left on I/O RX buffer for pico_socket(%p)", s);
				handle_general_failure();
			}
		}
		while(r > 0);
	}

	// from stack socket to app socket
	void picoTCP::pico_cb_udp_read(VirtualTap *tap, struct pico_socket *s)
	{
		// DEBUG_INFO();
		VirtualSocket *vs = (VirtualSocket*)(((VirtualBindingPair*)s->priv)->vs);
		if(!vs) {
			DEBUG_ERROR("s->priv yielded no valid vs");
			handle_general_failure();
			return;
		}
		Mutex::Lock _l(vs->_rx_m);

		if(!tap) {
			DEBUG_ERROR("invalid tap");
			handle_general_failure();
			return;
		}
		if(!vs) {
			DEBUG_ERROR("invalid vs");
			handle_general_failure();
			return;
		}

		uint16_t port = 0;
		union {
			struct pico_ip4 ip4;
			struct pico_ip6 ip6;
		} peer;
		int r = 0, w = 0;
		// TODO: Consolidate this
		if(vs->socket_family == AF_INET) {
			struct sockaddr_in in4;
			char udp_payload_buf[ZT_MAX_MTU];
			if((r = pico_socket_recvfrom(s, udp_payload_buf, ZT_SDK_MTU, (void *)&peer.ip4.addr, &port)) < 0) {
				DEBUG_ERROR("err=%d, %s", r, beautify_pico_error(pico_err));
			}
			in4.sin_addr.s_addr = peer.ip4.addr;
			in4.sin_port = port;
			// immediately attempt to write addr and payload to app socket. The idea is that the zts_recvfrom() has
			// been called and will pick this up and correctly handle it
			char udp_msg_buf[ZT_SOCKET_MSG_BUF_SZ]; // [sz : addr : payload]
			int32_t len = sizeof(struct sockaddr_storage) + r;
			int32_t tot_len = sizeof(int32_t) + len;
			memcpy(udp_msg_buf, &len, sizeof(int32_t)); // len: sockaddr+payload
			memcpy(udp_msg_buf + sizeof(int32_t), &in4, sizeof(struct sockaddr_storage)); // sockaddr
			memcpy(udp_msg_buf + sizeof(int32_t) + sizeof(struct sockaddr_storage), &udp_payload_buf, r); // payload
			if((w = write(vs->sdk_fd, udp_msg_buf, tot_len)) < 0) {
				DEBUG_ERROR("write()=%d, errno=%d", w, errno);
			}
		}
		if(vs->socket_family == AF_INET6) {
			struct sockaddr_in6 in6;
			char udp_payload_buf[ZT_MAX_MTU];
			if((r = pico_socket_recvfrom(s, udp_payload_buf, ZT_SDK_MTU, (void *)&peer.ip6.addr, &port)) < 0) {
				DEBUG_ERROR("err=%d, %s", r, beautify_pico_error(pico_err));
			}
			memcpy(&(in6.sin6_addr.s6_addr), &(peer.ip6.addr), sizeof(peer.ip6.addr));
			in6.sin6_port = port;
			// immediately attempt to write addr and payload to app socket. The idea is that the zts_recvfrom() has
			// been called and will pick this up and correctly handle it
			char udp_msg_buf[ZT_SOCKET_MSG_BUF_SZ]; // [sz : addr : payload]
			int32_t len = sizeof(struct sockaddr_storage) + r;
			int32_t tot_len = sizeof(int32_t) + len;
			memcpy(udp_msg_buf, &len, sizeof(int32_t)); // len: sockaddr+payload
			memcpy(udp_msg_buf + sizeof(int32_t), &in6, sizeof(struct sockaddr_storage)); // sockaddr
			memcpy(udp_msg_buf + sizeof(int32_t) + sizeof(struct sockaddr_storage), &udp_payload_buf, r); // payload
			if((w = write(vs->sdk_fd, udp_msg_buf, tot_len)) < 0) {
				DEBUG_ERROR("write()=%d, errno=%d", w, errno);
			}
		}
	}

	void picoTCP::pico_cb_tcp_write(VirtualTap *tap, struct pico_socket *s)
	{
		VirtualSocket *vs = (VirtualSocket*)(((VirtualBindingPair*)s->priv)->vs);
		if(!vs) {
			DEBUG_ERROR("s->priv yielded no valid vs");
			handle_general_failure();
			return;
		}
		Mutex::Lock _l(vs->_tx_m);
		if(!vs) {
			DEBUG_ERROR("invalid VirtualSocket");
			handle_general_failure();
			return;
		}
		int txsz = vs->TXbuf->count();
		if(txsz <= 0)
			return;
		//DEBUG_INFO("TXbuf->count()=%d", vs->TXbuf->count());

		int r, max_write_len = std::min(std::min(txsz, ZT_SDK_MTU),ZT_STACK_SOCKET_WR_MAX);
		if((r = pico_socket_write(vs->picosock, vs->TXbuf->get_buf(), max_write_len)) < 0) {
			DEBUG_ERROR("unable to write to pico_socket=%p, err=%d, pico_err=%d, %s", 
				vs->picosock, r, pico_err, beautify_pico_error(pico_err));
			handle_general_failure();
			return;
		}
		if(vs->socket_type == SOCK_STREAM) {
			DEBUG_TRANS("len=%5d buf_len=%13d [VSTXBF        -->     NSPICO] proto=0x%04x (TCP)", r, vs->TXbuf->count(), PICO_PROTO_TCP);
		}
		if(r == 0) {
			// DEBUG_ERROR("err=%d, pico_err=%d, %s", r, pico_err, beautify_pico_error(pico_err));
			// This is a peciliarity of the picoTCP network stack, if we receive no error code, and the size of 
			// the byte stream written is 0, this is an indication that the buffer for this pico_socket is too small
			// DEBUG_ERROR("pico_socket buffer is too small (adjust ZT_STACK_SOCKET_TX_SZ, ZT_STACK_SOCKET_RX_SZ)");
			// handle_general_failure(); 
		}
		if(r>0)
			vs->TXbuf->consume(r);
	}

	void picoTCP::pico_cb_socket_ev(uint16_t ev, struct pico_socket *s)
	{
		int err = 0;
		//DEBUG_EXTRA("s=%p, s->state=%d %s", s, s->state, beautify_pico_state(s->state));

		// --- handle error events ---

		// PICO_SOCK_EV_FIN - triggered when the socket is closed. No further communication is
		// possible from this point on the socket.
		if (ev & PICO_SOCK_EV_FIN) {
			DEBUG_EXTRA("PICO_SOCK_EV_FIN (socket closed), picosock=%p", s);
			//DEBUG_EXTRA("PICO_SOCK_EV_FIN (socket closed), picosock=%p, vs=%p, app_fd=%d, sdk_fd=%d", s, vs, vs->app_fd, vs->sdk_fd);
			//vs->closure_ts = std::time(nullptr);	
		}

		// PICO_SOCK_EV_ERR - triggered when an error occurs.
		if (ev & PICO_SOCK_EV_ERR) {
			if(pico_err == PICO_ERR_ECONNRESET) {
				DEBUG_ERROR("PICO_ERR_ECONNRESET");
			}
			//DEBUG_ERROR("PICO_SOCK_EV_ERR, err=%s, picosock=%p, app_fd=%d, sdk_fd=%d", 
			//	beautify_pico_error(pico_err), s, vs->app_fd, vs->sdk_fd); 
		}
		// PICO_SOCK_EV_CLOSE - triggered when a FIN segment is received (TCP only). This event
		// indicates that the oher endpont has closed the VirtualSocket, so the local TCP layer is only
		// allowed to send new data until a local shutdown or close is initiated. PicoTCP is able to
		// keep the VirtualSocket half-open (only for sending) after the FIN packet has been received,
		// allowing new data to be sent in the TCP CLOSE WAIT state.
		if (ev & PICO_SOCK_EV_CLOSE) {
			if((err = pico_socket_close(s)) < 0) {
				DEBUG_ERROR("pico_socket_close()=%d, pico_err=%d, %s", err, pico_err, beautify_pico_error(pico_err));
			}
			DEBUG_EXTRA("PICO_SOCK_EV_CLOSE (socket closure) err=%d (%s), picosock=%p", pico_err, beautify_pico_error(pico_err), s);
			//DEBUG_EXTRA("PICO_SOCK_EV_CLOSE (socket closure) err=%d, picosock=%p, vs=%p, app_fd=%d, sdk_fd=%d", err, s, vs, vs->app_fd, vs->sdk_fd);            
			//vs->closure_ts = std::time(nullptr);	
			return;
		}

		// --- handle non-error events ---

		VirtualBindingPair *vbp = (VirtualBindingPair*)(s->priv);
		if(!vbp) {
			DEBUG_ERROR("s->priv yielded no valid VirtualBindingPair");
			handle_general_failure();
			return;
		}
		VirtualTap *tap = static_cast<VirtualTap*>(vbp->tap);
		VirtualSocket *vs = static_cast<VirtualSocket*>(vbp->vs);
		if(!vs) {
			DEBUG_ERROR("invalid VirtualSocket");
			handle_general_failure();
			return;
		}
		// PICO_SOCK_EV - triggered when VirtualSocket is established (TCP only). This event is
		// received either after a successful call to pico socket vsect to indicate that the VirtualSocket
		// has been established, or on a listening socket, indicating that a call to pico socket accept
		// may now be issued in order to accept the incoming VirtualSocket from a remote host.
		if (ev & PICO_SOCK_EV_CONN) {
			DEBUG_EXTRA("PICO_SOCK_EV_CONN");
			if(vs->state == ZT_SOCK_STATE_LISTENING)
			{
				uint16_t port;
				struct pico_socket *client_psock = nullptr;
				struct pico_ip4 orig4;
				struct pico_ip6 orig6;

				if(vs->socket_family == AF_INET) { // NOTE: p->net->proto_number == PICO_PROTO_IPV4
					client_psock = pico_socket_accept(s, &orig4, &port);
				}
				if(vs->socket_family == AF_INET6) { // NOTE: p->net->proto_number == PICO_PROTO_IPV4
					client_psock = pico_socket_accept(s, &orig6, &port);
				}
				if(!client_psock) {
					DEBUG_ERROR("pico_socket_accept(): pico_socket=%p, pico_err=%d, %s", s, pico_err, beautify_pico_error(pico_err));
					return;
				}	

				// Create a new VirtualSocket and add it to the queue,
				//   some time in the future a call to zts_multiplex_accept() will pick up 
				//   this new VirtualSocket, add it to the VirtualSocket list and return its
				//   VirtualSocket->sock to the application
				VirtualSocket *new_vs = new VirtualSocket();
				new_vs->socket_type = SOCK_STREAM;
				new_vs->picosock = client_psock;
				
				// TODO: Condense this
				if(vs->socket_family == AF_INET) {
					char addrstr[INET_ADDRSTRLEN];
					struct sockaddr_storage ss4;
					struct sockaddr_in *in4 = (struct sockaddr_in *)&ss4;
					in4->sin_addr.s_addr = orig4.addr;
					in4->sin_port = Utils::hton(port);
					memcpy(&(new_vs->peer_addr), in4, sizeof(new_vs->peer_addr));
					inet_ntop(AF_INET, &(in4->sin_addr), addrstr, INET_ADDRSTRLEN);
					DEBUG_EXTRA("accepted connection from: %s : %d", addrstr, port);
					ZeroTier::InetAddress inet;
					inet.fromString(addrstr);
					new_vs->tap = getTapByAddr(&inet); // assign to tap based on incoming address
				}
				if(vs->socket_family == AF_INET6) {
					char addrstr[INET6_ADDRSTRLEN];
					struct sockaddr_in6 in6;
					memcpy(&(in6.sin6_addr.s6_addr), &orig6, sizeof(in6.sin6_addr.s6_addr));
					in6.sin6_port = Utils::hton(port);
					memcpy(&(new_vs->peer_addr), &in6, sizeof(new_vs->peer_addr));
					inet_ntop(AF_INET6, &(in6.sin6_addr), addrstr, INET6_ADDRSTRLEN);
					DEBUG_EXTRA("accepted connection from: %s : %d", addrstr, port);
					ZeroTier::InetAddress inet;
					inet.fromString(addrstr);
					new_vs->tap = getTapByAddr(&inet); // assign to tap based on incoming address
				}
				if(!new_vs->tap) {
					DEBUG_ERROR("no valid VirtualTap could be found");
					handle_general_failure();
					return;
				}
				// Assign this VirtualSocket to the appropriate VirtualTap
				new_vs->picosock->priv = new VirtualBindingPair(new_vs->tap,new_vs);
				new_vs->tap->addVirtualSocket(new_vs);
				vs->_AcceptedConnections.push(new_vs);
				new_vs->sock = new_vs->tap->_phy.wrapSocket(new_vs->sdk_fd, new_vs);
			}
			if(vs->state != ZT_SOCK_STATE_LISTENING) {
				// set state so socket multiplexer logic will pick this up
				vs->state = ZT_SOCK_STATE_UNHANDLED_CONNECTED;
			}
		}
		// PICO_SOCK_EV_RD - triggered when new data arrives on the socket. A new receive action
		// can be taken by the socket owner because this event indicates there is new data to receive.
		if (ev & PICO_SOCK_EV_RD) {
			if(vs->socket_type==SOCK_STREAM)
				pico_cb_tcp_read(tap, s);
			if(vs->socket_type==SOCK_DGRAM)
				pico_cb_udp_read(tap, s);
		}
		// PICO_SOCK_EV_WR - triggered when ready to write to the socket. Issuing a write/send call
		// will now succeed if the buffer has enough space to allocate new outstanding data
		if (ev & PICO_SOCK_EV_WR) {
			pico_cb_tcp_write(tap, s);
		}
	}
   
	int pico_eth_tx(struct pico_device *dev, void *buf, int len)
	{
		//_picostack_driver_lock.lock();
		VirtualTap *tap = static_cast<VirtualTap*>(dev->tap);
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
		if(ZT_DEBUG_LEVEL >= ZT_MSG_TRANSFER) {
			char macBuf[ZT_MAC_ADDRSTRLEN], nodeBuf[ZT_ID_LEN];
			mac2str(macBuf, ZT_MAC_ADDRSTRLEN, ethhdr->daddr);
			ZeroTier::MAC mac;
			mac.setTo(ethhdr->daddr, 6);
			mac.toAddress(tap->_nwid).toString(nodeBuf);

			char flagbuf[32];
			memset(&flagbuf, 0, 32);
			struct pico_tcp_hdr *hdr;
			void * tcp_hdr_ptr;
			
			if(Utils::ntoh(ethhdr->proto) == 0x86dd) { // tcp, ipv6
				tcp_hdr_ptr = &ethhdr + PICO_SIZE_ETHHDR + PICO_SIZE_IP4HDR;
			}

			if(Utils::ntoh(ethhdr->proto) == 0x0800) // tcp
			{
				tcp_hdr_ptr = &buf + PICO_SIZE_ETHHDR + PICO_SIZE_IP4HDR;
				hdr = (struct pico_tcp_hdr *)tcp_hdr_ptr;
				
				if(hdr) {
					char *flag_ptr = flagbuf;

					if (hdr->flags & PICO_TCP_PSH) {
						sprintf(flag_ptr, "PSH ");
						flag_ptr+=4;
					}
					if (hdr->flags & PICO_TCP_SYN) {
						sprintf(flag_ptr, "SYN ");
						flag_ptr+=4;
					}
					if (hdr->flags & PICO_TCP_ACK) {
						sprintf(flag_ptr, "ACK ");
						flag_ptr+=4;				
					}
					if (hdr->flags & PICO_TCP_FIN) {
						sprintf(flag_ptr, "FIN ");
						flag_ptr+=4;
					}
					if (hdr->flags & PICO_TCP_RST) {
						sprintf(flag_ptr, "RST ");
						flag_ptr+=4;
					}
				}
			}

			DEBUG_TRANS("len=%5d dst=%s [%s TX <-- %s] proto=0x%04x %s %s", len, macBuf, nodeBuf, tap->nodeId().c_str(), 
				Utils::ntoh(ethhdr->proto), beautify_eth_proto_nums(Utils::ntoh(ethhdr->proto)), flagbuf);
		}
		tap->_handler(tap->_arg,NULL,tap->_nwid,src_mac,dest_mac,
			Utils::ntoh((uint16_t)ethhdr->proto), 0, ((char*)buf) 
			+ sizeof(struct pico_eth_hdr),len - sizeof(struct pico_eth_hdr));
		//_picostack_driver_lock.unlock();
		return len;
	}

	// receive frames from zerotier virtual wire and copy them to a guarded buffer awaiting placement into network stack
	void picoTCP::pico_eth_rx(VirtualTap *tap, const MAC &from,const MAC &to,unsigned int etherType,
		const void *data,unsigned int len)
	{
		//_picostack_driver_lock.lock();
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
		int32_t msg_len = len + sizeof(int32_t) + sizeof(struct pico_eth_hdr);
		
		if(ZT_DEBUG_LEVEL >= ZT_MSG_TRANSFER) {
			char macBuf[ZT_MAC_ADDRSTRLEN], nodeBuf[ZT_ID_LEN];
			mac2str(macBuf, sizeof(macBuf), ethhdr.saddr);
			ZeroTier::MAC mac;
			mac.setTo(ethhdr.saddr, 6);
			mac.toAddress(tap->_nwid).toString(nodeBuf);

			char flagbuf[32];
			memset(&flagbuf, 0, 32);
			struct pico_tcp_hdr *hdr;
			void * tcp_hdr_ptr;
			
			if(etherType == 0x86dd) { // tcp, ipv6
				tcp_hdr_ptr = &ethhdr + PICO_SIZE_ETHHDR + PICO_SIZE_IP4HDR;
			}

			if(etherType == 0x0800) // tcp, ipv4
			{				
				tcp_hdr_ptr = &ethhdr + PICO_SIZE_ETHHDR + PICO_SIZE_IP4HDR;
				hdr = (struct pico_tcp_hdr *)tcp_hdr_ptr;
				if(hdr) {
					char *flag_ptr = flagbuf;
					
					if (hdr->flags & PICO_TCP_PSH) {
						sprintf(flag_ptr, "PSH ");
						flag_ptr+=4;
					}
					if (hdr->flags & PICO_TCP_SYN) {
						sprintf(flag_ptr, "SYN ");
						flag_ptr+=4;
					}
					if (hdr->flags & PICO_TCP_ACK) {
						sprintf(flag_ptr, "ACK ");
						flag_ptr+=4;				
					}
					if (hdr->flags & PICO_TCP_FIN) {
						sprintf(flag_ptr, "FIN ");
						flag_ptr+=4;
					}
					if (hdr->flags & PICO_TCP_RST) {
						sprintf(flag_ptr, "RST ");
						flag_ptr+=4;
					}
				}
			}
			DEBUG_TRANS("len=%5d src=%s [%s RX --> %s] proto=0x%04x %s %s", len, macBuf, nodeBuf, tap->nodeId().c_str(), 
				etherType, beautify_eth_proto_nums(etherType), flagbuf);
		}
		// write virtual ethernet frame to guarded buffer (emptied by pico_eth_poll())
		memcpy(tap->pico_frame_rxbuf + tap->pico_frame_rxbuf_tot, &msg_len, sizeof(int32_t));                     // size of frame + meta		
		memcpy(tap->pico_frame_rxbuf + tap->pico_frame_rxbuf_tot + sizeof(int32_t), &ethhdr, sizeof(ethhdr));     // new eth header
		memcpy(tap->pico_frame_rxbuf + tap->pico_frame_rxbuf_tot + sizeof(int32_t) + sizeof(ethhdr), data, len);  // frame data
		tap->pico_frame_rxbuf_tot += msg_len;
		//_picostack_driver_lock.unlock();
	}

	// feed frames on the guarded RX buffer (from zerotier virtual wire) into the network stack
	int pico_eth_poll(struct pico_device *dev, int loop_score)
	{
		VirtualTap *tap = static_cast<VirtualTap*>(dev->tap);
		if(!tap) {
			DEBUG_ERROR("invalid dev->tap");
			handle_general_failure();
			return ZT_ERR_GENERAL_FAILURE;
		}
		// TODO: Optimize (use Ringbuffer)
		Mutex::Lock _l(tap->_pico_frame_rxbuf_m);
		unsigned char frame[ZT_SDK_MTU];
		int32_t len, err = 0;
		while (tap->pico_frame_rxbuf_tot > 0 && loop_score > 0) {
			memset(frame, 0, sizeof(frame));
			len = 0;
			// get frame len
			memcpy(&len, tap->pico_frame_rxbuf, sizeof(int32_t));
			if(len > sizeof(int32_t)) { // meaning, since we package the len in the msg, we don't want to recv a 0-(sizeof(int32_t)) sized frame
				memcpy(frame, tap->pico_frame_rxbuf + sizeof(int32_t), len-(sizeof(int32_t)) ); // get frame data
				memmove(tap->pico_frame_rxbuf, tap->pico_frame_rxbuf + len, MAX_PICO_FRAME_RX_BUF_SZ-len); // shift buffer
				if((err = pico_stack_recv(dev, (uint8_t*)frame, (len-sizeof(int32_t)))) < 0) {
					if(picostack) {
						DEBUG_ERROR("pico_stack_recv(), err=%d, pico_err=%d, %s", err, pico_err, picostack->beautify_pico_error(pico_err));
					}
				}
				tap->pico_frame_rxbuf_tot-=len;
			}
			else {
				DEBUG_ERROR("invalid frame size (%d)",len);
				handle_general_failure();
			}
			loop_score--;
		}
		return loop_score;
	}
	
	int picoTCP::pico_Socket(struct pico_socket **p, int socket_family, int socket_type, int protocol)
	{
		int err = 0;
		if(!can_provision_new_socket(socket_type)) {
			DEBUG_ERROR("cannot create additional socket, see PICO_MAX_TIMERS. current=%d", pico_ntimers());
			errno = EMFILE;
			err = -1;
		}
		else
		{
			int protocol_version = 0;
			struct pico_socket *psock;
			if(socket_family == AF_INET)
				protocol_version = PICO_PROTO_IPV4;
			if(socket_family == AF_INET6)
				protocol_version = PICO_PROTO_IPV6;
			
			if(socket_type == SOCK_DGRAM) {
				psock = pico_socket_open(
					protocol_version, PICO_PROTO_UDP, &ZeroTier::picoTCP::pico_cb_socket_ev);
				if(psock) { 
					// configure size of UDP SND/RCV buffers
					// TODO
				}
			}
			if(socket_type == SOCK_STREAM) {	
				psock = pico_socket_open(
					protocol_version, PICO_PROTO_TCP, &ZeroTier::picoTCP::pico_cb_socket_ev);
				if(psock) { 
					// configure size of TCP SND/RCV buffers
					int tx_buf_sz = ZT_STACK_TCP_SOCKET_TX_SZ;
					int rx_buf_sz = ZT_STACK_TCP_SOCKET_RX_SZ;
					int t_err = 0;

					// int value = 1;
					// pico_socket_setoption(psock, PICO_TCP_NODELAY, &value);

					if((t_err = pico_socket_setoption(psock, PICO_SOCKET_OPT_SNDBUF, &tx_buf_sz)) < 0)
						DEBUG_ERROR("unable to set SNDBUF size, err=%d, pico_err=%d, %s", 
							t_err, pico_err, beautify_pico_error(pico_err));
					if((t_err = pico_socket_setoption(psock, PICO_SOCKET_OPT_RCVBUF, &rx_buf_sz)) < 0)
						DEBUG_ERROR("unable to set RCVBUF size, err=%d, pico_err=%d, %s", 
							t_err, pico_err, beautify_pico_error(pico_err));

					if(ZT_SOCK_BEHAVIOR_LINGER) {
						int linger_time_ms = ZT_SOCK_BEHAVIOR_LINGER_TIME;
						if((t_err = pico_socket_setoption(psock, PICO_SOCKET_OPT_LINGER, &linger_time_ms)) < 0)
							DEBUG_ERROR("unable to set LINGER, err=%d, pico_err=%d, %s", 
								t_err, pico_err, beautify_pico_error(pico_err));
					}
				}
			}
			*p = psock;
		}
		return err;
	}

	int picoTCP::pico_Connect(VirtualSocket *vs, const struct sockaddr *addr, socklen_t addrlen)
	{		
		if(!vs || !vs->picosock) {
			DEBUG_ERROR("invalid vs or vs->picosock");
			handle_general_failure();
			return ZT_ERR_GENERAL_FAILURE;
		}
		int err = 0;
		if(vs->socket_family == AF_INET) {
			struct pico_ip4 zaddr;
			memset(&zaddr, 0, sizeof (struct pico_ip4));
			struct sockaddr_in *in4 = (struct sockaddr_in*)addr;
			char ipv4_str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, (const void *)&in4->sin_addr.s_addr, ipv4_str, INET_ADDRSTRLEN);
			uint32_t ipval = 0;
			pico_string_to_ipv4(ipv4_str, &ipval);
			zaddr.addr = ipval;
			if(vs->socket_type == SOCK_STREAM) { // connect is an implicit call for non-connection-based VirtualSockets
				DEBUG_EXTRA("connecting to addr=%s port=%d", ipv4_str, Utils::ntoh(in4->sin_port));
			}
			err = pico_socket_connect(vs->picosock, &zaddr, in4->sin_port);
		}
		if(vs->socket_family == AF_INET6) {
			struct pico_ip6 zaddr;
			struct sockaddr_in6 *in6 = (struct sockaddr_in6*)addr;
			char ipv6_str[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, &(in6->sin6_addr), ipv6_str, INET6_ADDRSTRLEN);
			pico_string_to_ipv6(ipv6_str, zaddr.addr);
			if(vs->socket_type == SOCK_STREAM) {
				DEBUG_EXTRA("connecting to addr=%s port=%d", ipv6_str, Utils::ntoh(in6->sin6_port));
			}
			err = pico_socket_connect(vs->picosock, &zaddr, in6->sin6_port);
		}
		if(err) {
			DEBUG_ERROR("error connecting pico_socket=%p, err=%d, pico_err=%d, %s", 
				vs->picosock, err, pico_err, beautify_pico_error(pico_err));
			return map_pico_err_to_errno(pico_err);
		}
		memcpy(&(vs->peer_addr), &addr, sizeof(struct sockaddr_storage));
		return err;
	}

	int picoTCP::pico_Bind(VirtualSocket *vs, const struct sockaddr *addr, socklen_t addrlen)
	{
		if(!vs || !vs->picosock) {
			DEBUG_ERROR("invalid vs or vs->picosock");
			handle_general_failure();
			return ZT_ERR_GENERAL_FAILURE;
		}
		int err = 0;
		if(vs->socket_family == AF_INET) { 
			struct pico_ip4 zaddr;
			uint32_t tempaddr;
			memset(&zaddr, 0, sizeof (struct pico_ip4));
			struct sockaddr_in *in4 = (struct sockaddr_in*)addr;
			char ipv4_str[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, (const void *)&in4->sin_addr.s_addr, ipv4_str, INET_ADDRSTRLEN);
			pico_string_to_ipv4(ipv4_str, &tempaddr);
			zaddr.addr = tempaddr;
			DEBUG_EXTRA("binding to addr=%s port=%d", ipv4_str, Utils::ntoh(in4->sin_port));
			err = pico_socket_bind(vs->picosock, &zaddr, (uint16_t *)&(in4->sin_port));
		}
		if(vs->socket_family == AF_INET6) { 
			struct pico_ip6 pip6;
			struct sockaddr_in6 *in6 = (struct sockaddr_in6*)addr;
			char ipv6_str[INET6_ADDRSTRLEN];
			inet_ntop(AF_INET6, &(in6->sin6_addr), ipv6_str, INET6_ADDRSTRLEN);
			// TODO: This isn't proper
			pico_string_to_ipv6("::", pip6.addr);
			DEBUG_EXTRA("binding to addr=%s port=%d", ipv6_str, Utils::ntoh(in6->sin6_port));
			err = pico_socket_bind(vs->picosock, &pip6, (uint16_t *)&(in6->sin6_port));
		}
		if(err < 0) {
			DEBUG_ERROR("unable to bind pico_socket=%p, err=%d, pico_err=%d, %s", 
				(vs->picosock), err, pico_err, beautify_pico_error(pico_err));
			return map_pico_err_to_errno(pico_err);
		}
		return err;
	}

	int picoTCP::pico_Listen(VirtualSocket *vs, int backlog)
	{
		if(!vs || !vs->picosock) {
			DEBUG_ERROR("invalid vs or vs->picosock");
			handle_general_failure();
			return ZT_ERR_GENERAL_FAILURE;
		}
		int err = 0;
		if((err = pico_socket_listen(vs->picosock, backlog)) < 0) {
			DEBUG_ERROR("error putting pico_socket=%p into listening state. err=%d, pico_err=%d, %s", 
				vs->picosock, err, pico_err, beautify_pico_error(pico_err));
			return map_pico_err_to_errno(pico_err);
		}
		vs->state = ZT_SOCK_STATE_LISTENING;
		return ZT_ERR_OK;
	}

	VirtualSocket* picoTCP::pico_Accept(VirtualSocket *vs)
	{
		if(!vs) {
			DEBUG_ERROR("invalid vs");
			handle_general_failure();
			return NULL;
		}
		// Retreive first of queued VirtualSockets from parent VirtualSocket
		VirtualSocket *new_vs = NULL;
		if(vs->_AcceptedConnections.size()) {
			new_vs = vs->_AcceptedConnections.front();
			vs->_AcceptedConnections.pop();
		}
		return new_vs;
	}

	int picoTCP::pico_Read(VirtualTap *tap, PhySocket *sock, VirtualSocket* vs, bool stack_invoked)
	{
		// DEBUG_INFO();
		// Vestigial
		return 0;
	}

	int picoTCP::pico_Write(VirtualSocket *vs, void *data, ssize_t len)
	{
		int err = 0;
		// TODO: Add RingBuffer overflow checks
		// DEBUG_INFO("vs=%p, len=%d", vs, len);
		if(!vs) {
			DEBUG_ERROR("invalid vs");
			handle_general_failure();
			return ZT_ERR_GENERAL_FAILURE;
		}
		Mutex::Lock _l(vs->_tx_m);
		if(len <= 0) {
			DEBUG_ERROR("invalid write length (len=%d)", len);
			handle_general_failure();
			return -1;
		}
		if(vs->picosock->state & PICO_SOCKET_STATE_CLOSED){
			DEBUG_ERROR("socket is CLOSED, this wrpico_cb_tcp_writeite() will fail");
			return -1;
		}
		if(!vs) {
			DEBUG_ERROR("invalid VirtualSocket (len=%d)", len);
			handle_general_failure();
			return -1;
		}
		if(vs->socket_type == SOCK_DGRAM) {
			int r;
			if((r = pico_socket_write(vs->picosock, data, len)) < 0) {
				DEBUG_ERROR("unable to write to picosock=%p, err=%d, pico_err=%d, %s", 
					vs->picosock, r, pico_err, beautify_pico_error(pico_err));
				err = -1;
			}
			else {
				err = r; // successful write
			}
		}
		if(vs->socket_type == SOCK_STREAM) {
			int original_txsz = vs->TXbuf->count();
			if(original_txsz + len >= ZT_TCP_TX_BUF_SZ) {
				DEBUG_ERROR("txsz=%d, len=%d", original_txsz, len);
				DEBUG_ERROR("TX buffer is too small, try increasing ZT_TCP_TX_BUF_SZ in libzt.h");
				handle_general_failure();
				return ZT_ERR_GENERAL_FAILURE;
			}
			int buf_w = vs->TXbuf->write((const unsigned char*)data, len);
				if (buf_w != len) {
				// because we checked ZT_TCP_TX_BUF_SZ above, this should not happen
				DEBUG_ERROR("TX wrote only %d but expected to write %d", buf_w, len);
				handle_general_failure();
				return ZT_ERR_GENERAL_FAILURE;
			}
			int txsz = vs->TXbuf->count();
			int r, max_write_len = std::min(std::min(txsz, ZT_SDK_MTU),ZT_STACK_SOCKET_WR_MAX);			
			if((r = pico_socket_write(vs->picosock, vs->TXbuf->get_buf(), max_write_len)) < 0) {
				DEBUG_ERROR("unable to write to picosock=%p, err=%d, pico_err=%d, %s", 
					vs->picosock, r, pico_err, beautify_pico_error(pico_err));
				err = -1;
			}
			else {
				err = r; // successful write
			}
			if(r>0){
				vs->TXbuf->consume(r);
			}
			if(vs->socket_type == SOCK_STREAM) {
				DEBUG_TRANS("len=%5d buf_len=%13d [VSTXBF        -->     NSPICO] proto=0x%04x (TCP)", r, vs->TXbuf->count(), PICO_PROTO_TCP);
			}
			if(vs->socket_type == SOCK_DGRAM) {
				DEBUG_TRANS("len=%5d buf_len=               [APPFDS        -->     NSPICO] proto=0x%04x (UDP)", r, PICO_PROTO_TCP);
			}
		}
		return err;
	}

	int picoTCP::pico_Close(VirtualSocket *vs)
	{
		if(!vs) {
			DEBUG_ERROR("invalid vs");
			handle_general_failure();
			return ZT_ERR_GENERAL_FAILURE;		
		}
		DEBUG_EXTRA("vs=%p, picosock=%p, fd=%d", vs, vs->picosock, vs->app_fd);
		if(!vs || !vs->picosock)
			return ZT_ERR_GENERAL_FAILURE;
		int err = 0;
		Mutex::Lock _l(vs->tap->_tcpconns_m);
		if(vs->closure_ts != -1) // it was closed at some point in the past, it'll work itself out 
			return ZT_ERR_OK;
		if((err = pico_socket_close(vs->picosock)) < 0) {
			errno = pico_err;
			DEBUG_ERROR("error closing pico_socket, err=%d, pico_err=%s, %s", 
				err, pico_err, beautify_pico_error(pico_err));
		}
		return err;
	}

	int picoTCP::pico_Shutdown(VirtualSocket *vs, int how)
	{
		int err = 0, mode = 0;
		if(how == SHUT_RD) {
			mode = PICO_SHUT_RD;
		}
		if(how == SHUT_WR) {
			mode = PICO_SHUT_WR;
		}
		if(how == SHUT_RDWR) { 
			mode = PICO_SHUT_RDWR;
		}
		if((err = pico_socket_shutdown(vs->picosock, mode)) < 0) {
			DEBUG_ERROR("error while shutting down socket, fd=%d, pico_err=%d, %s", vs->app_fd, pico_err, beautify_pico_error(pico_err));
		}
		return err;
	}

	int picoTCP::map_pico_err_to_errno(int err)
	{
		if(err == PICO_ERR_NOERR) { errno = 0; return 0; } //
		if(err == PICO_ERR_EPERM) { errno = ENXIO; }
		if(err == PICO_ERR_ENOENT) { errno = ENXIO; }
		if(err == PICO_ERR_EINTR) { errno = ENXIO; }
		if(err == PICO_ERR_EIO) { errno = ENXIO; }
		if(err == PICO_ERR_ENXIO) { errno = ENXIO; } //
		if(err == PICO_ERR_EAGAIN) { errno = ENXIO; }
		if(err == PICO_ERR_ENOMEM) { errno = ENOMEM; } //
		if(err == PICO_ERR_EACCESS) { errno = ENXIO; }
		if(err == PICO_ERR_EFAULT) { errno = ENXIO; }
		if(err == PICO_ERR_EBUSY) { errno = ENXIO; }
		if(err == PICO_ERR_EEXIST) { errno = ENXIO; }
		if(err == PICO_ERR_EINVAL) { errno = EINVAL; } //
		if(err == PICO_ERR_ENONET) { errno = ENXIO; }
		if(err == PICO_ERR_EPROTO) { errno = ENXIO; }
		if(err == PICO_ERR_ENOPROTOOPT) { errno = ENXIO; }
		if(err == PICO_ERR_EPROTONOSUPPORT) { errno = ENXIO; }
		if(err == PICO_ERR_EOPNOTSUPP) { errno = ENXIO; }
		if(err == PICO_ERR_EADDRINUSE) { errno = ENXIO; }
		if(err == PICO_ERR_EADDRNOTAVAIL) { errno = ENXIO; }
		if(err == PICO_ERR_ENETDOWN) { errno = ENXIO; }
		if(err == PICO_ERR_ENETUNREACH) { errno = ENXIO; }
		if(err == PICO_ERR_ECONNRESET) { errno = ENXIO; }
		if(err == PICO_ERR_EISCONN) { errno = ENXIO; }
		if(err == PICO_ERR_ENOTCONN) { errno = ENXIO; }
		if(err == PICO_ERR_ESHUTDOWN) { errno = ENXIO; }
		if(err == PICO_ERR_ETIMEDOUT) { errno = ENXIO; }
		if(err == PICO_ERR_ECONNREFUSED) { errno = ENXIO; }
		if(err == PICO_ERR_EHOSTDOWN) { errno = ENXIO; }
		if(err == PICO_ERR_EHOSTUNREACH) { errno = ENXIO; }
		return -1;
	}

	char *picoTCP::beautify_pico_error(int err)
	{
		if(err==  0) return (char*)"PICO_ERR_NOERR";
		if(err==  1) return (char*)"PICO_ERR_EPERM";
		if(err==  2) return (char*)"PICO_ERR_ENOENT";
		// ...
		if(err==  4) return (char*)"PICO_ERR_EINTR";
		if(err==  5) return (char*)"PICO_ERR_EIO";
		if(err==  6) return (char*)"PICO_ERR_ENXIO (no such device or address)";
		// ...
		if(err== 11) return (char*)"PICO_ERR_EAGAIN";
		if(err== 12) return (char*)"PICO_ERR_ENOMEM (not enough space)";
		if(err== 13) return (char*)"PICO_ERR_EACCESS";
		if(err== 14) return (char*)"PICO_ERR_EFAULT";
		// ...
		if(err== 16) return (char*)"PICO_ERR_EBUSY";
		if(err== 17) return (char*)"PICO_ERR_EEXIST";
		// ...
		if(err== 22) return (char*)"PICO_ERR_EINVAL (invalid argument)";
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
