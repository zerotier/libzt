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

/**
 * @file
 *
 * lwIP network stack driver.
 *
 * Calls made in this network stack driver may never block since all packet
 * processing (input and output) as well as timer processing (TCP mainly) is done
 * in a single execution context.
 *
 */

#include "libztDefs.h"

#ifdef STACK_LWIP

#include "VirtualSocket.h"

// forward declarations
class VirtualTap;
class VirtualSocket;
bool virt_can_provision_new_socket(int socket_type);

#include "Mutex.hpp"
#include "MAC.hpp"
#include "ZeroTierOne.h"

#include "libzt.h"
#include "SysUtils.h"
#include "Utilities.h"
#include "libztDebug.h"

#include "netif/ethernet.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/tcpip.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/sys.h"
#include "lwip/tcp.h"
#include "lwip/priv/tcp_priv.h" /* for tcp_debug_print_pcbs() */
#include "lwip/timeouts.h"
#include "lwip/stats.h"
#include "lwip/ethip6.h"
#include "lwip/ip_addr.h"
#include "lwip/nd6.h"
#include "lwip/dns.h"
#include "lwip/netifapi.h"

#include "lwIP.h"

struct netif lwipInterfaces[10];
int lwipInterfacesCount = 0;


bool lwip_driver_initialized = false;
ZeroTier::Mutex driver_m;

err_t tapif_init(struct netif *netif)
{
	// we do the actual initialization in elsewhere
	return ERR_OK;
}

/*
static void tcp_timeout(void *data)
{
	DEBUG_EXTRA();
	LWIP_UNUSED_ARG(data);
#if TCP_DEBUG && LWIP_TCP
	// tcp_debug_print_pcbs();
#endif
	sys_timeout(5000, tcp_timeout, NULL);
}
*/

// callback for when the TCPIP thread has been successfully started
static void tcpip_init_done(void *arg)
{
	DEBUG_EXTRA("tcpip-thread");
	sys_sem_t *sem;
	sem = (sys_sem_t *)arg;
	//netif_set_up(&lwipdev);
	lwip_driver_initialized = true;
	driver_m.unlock();
	// sys_timeout(5000, tcp_timeout, NULL);
	sys_sem_signal(sem);
}

// main thread which starts the initialization process
static void main_thread(void *arg)
{
	sys_sem_t sem;
	LWIP_UNUSED_ARG(arg);
	if (sys_sem_new(&sem, 0) != ERR_OK) {
		DEBUG_ERROR("failed to create semaphore", 0);
	}
	tcpip_init(tcpip_init_done, &sem);
	sys_sem_wait(&sem);
	DEBUG_EXTRA("stack thread init complete");
	sys_sem_wait(&sem); // block forever
}

// initialize the lwIP stack
void lwip_driver_init()
{
	DEBUG_EXTRA();
	driver_m.lock(); // unlocked from callback indicating completion of driver init
	if (lwip_driver_initialized == true) {
		return;
	}
#if defined(_WIN32)
	sys_init(); // required for win32 initializtion of critical sections
#endif
	sys_thread_new("main_thread", main_thread,
		NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
}



err_t lwip_eth_tx(struct netif *netif, struct pbuf *p)
{
	struct pbuf *q;
	char buf[ZT_MAX_MTU+32];
	char *bufptr;
	int totalLength = 0;

	VirtualTap *tap = (VirtualTap*)netif->state;
	bufptr = buf;
	for (q = p; q != NULL; q = q->next) {
		memcpy(bufptr, q->payload, q->len);
		bufptr += q->len;
		totalLength += q->len;
	}
	struct eth_hdr *ethhdr;
	ethhdr = (struct eth_hdr *)buf;

	ZeroTier::MAC src_mac;
	ZeroTier::MAC dest_mac;
	src_mac.setTo(ethhdr->src.addr, 6);
	dest_mac.setTo(ethhdr->dest.addr, 6);

	char *data = buf + sizeof(struct eth_hdr);
	int len = totalLength - sizeof(struct eth_hdr);
	int proto = ZeroTier::Utils::ntoh((uint16_t)ethhdr->type);
	tap->_handler(tap->_arg, NULL, tap->_nwid, src_mac, dest_mac, proto, 0, data, len);

	if (ZT_MSG_TRANSFER == true) {
		char flagbuf[32];
		memset(&flagbuf, 0, 32);
		char macBuf[ZT_MAC_ADDRSTRLEN], nodeBuf[ZTO_ID_LEN];
		mac2str(macBuf, ZT_MAC_ADDRSTRLEN, ethhdr->dest.addr);
		ZeroTier::MAC mac;
		mac.setTo(ethhdr->dest.addr, 6);
		mac.toAddress(tap->_nwid).toString(nodeBuf);
		DEBUG_TRANS("len=%5d dst=%s [%s TX <-- %s] proto=0x%04x %s %s", totalLength, macBuf, nodeBuf, tap->nodeId().c_str(),
			ZeroTier::Utils::ntoh(ethhdr->type), beautify_eth_proto_nums(ZeroTier::Utils::ntoh(ethhdr->type)), flagbuf);
	}
	return ERR_OK;
}

void lwip_eth_rx(VirtualTap *tap, const ZeroTier::MAC &from, const ZeroTier::MAC &to, unsigned int etherType,
	const void *data, unsigned int len)
{
	struct pbuf *p,*q;
	struct eth_hdr ethhdr;
	from.copyTo(ethhdr.src.addr, 6);
	to.copyTo(ethhdr.dest.addr, 6);
	ethhdr.type = ZeroTier::Utils::hton((uint16_t)etherType);

	if (ZT_MSG_TRANSFER == true) {
		char flagbuf[32];
		memset(&flagbuf, 0, 32);
		char macBuf[ZT_MAC_ADDRSTRLEN], nodeBuf[ZTO_ID_LEN];
		mac2str(macBuf, ZT_MAC_ADDRSTRLEN, ethhdr.dest.addr);
		ZeroTier::MAC mac;
		mac.setTo(ethhdr.src.addr, 6);
		mac.toAddress(tap->_nwid).toString(nodeBuf);
		DEBUG_TRANS("len=%5d dst=%s [%s RX --> %s] proto=0x%04x %s %s", len, macBuf, nodeBuf, tap->nodeId().c_str(),
			ZeroTier::Utils::ntoh(ethhdr.type), beautify_eth_proto_nums(ZeroTier::Utils::ntoh(ethhdr.type)), flagbuf);
	}

	p = pbuf_alloc(PBUF_RAW, len+sizeof(struct eth_hdr), PBUF_POOL);
	if (p != NULL) {
		const char *dataptr = reinterpret_cast<const char *>(data);
		// First pbuf gets ethernet header at start
		q = p;
		if (q->len < sizeof(ethhdr)) {
			DEBUG_ERROR("dropped packet: first pbuf smaller than ethernet header");
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
	}
	else {
		DEBUG_ERROR("dropped packet: no pbufs available");
		return;
	}

	if (lwipInterfacesCount <= 0) {
		DEBUG_ERROR("there are no netifs set up to handle this packet. ignoring.");
		return;
	}

	// Routing
	struct ip_hdr *iphdr;
	//ip_addr_t iphdr_dest;
	switch (((struct eth_hdr *)p->payload)->type)
	{
		case PP_HTONS(ETHTYPE_IPV6): {
			iphdr = (struct ip_hdr *)((char *)p->payload + SIZEOF_ETH_HDR);
			for (int i=0; i<lwipInterfacesCount; i++) {
				if (lwipInterfaces[i].output_ip6 && lwipInterfaces[i].output_ip6 == ethip6_output) {
					if (lwipInterfaces[i].input(p, &lwipInterfaces[i]) != ERR_OK) {
						DEBUG_ERROR("packet input error (ipv6, p=%p, netif=%p)", p, &lwipInterfaces[i]);
						break;
					}

				}
			}	
		} break;
		case PP_HTONS(ETHTYPE_IP): {
			iphdr = (struct ip_hdr *)((char *)p->payload + SIZEOF_ETH_HDR);
			for (int i=0; i<lwipInterfacesCount; i++) {
				if (lwipInterfaces[i].output &&  lwipInterfaces[i].output == etharp_output) {
					if (lwipInterfaces[i].ip_addr.u_addr.ip4.addr == iphdr->dest.addr || ip4_addr_isbroadcast_u32(iphdr->dest.addr, &lwipInterfaces[i])) {
						if (lwipInterfaces[i].input(p, &lwipInterfaces[i]) != ERR_OK) {
							DEBUG_ERROR("packet input error (ipv4, p=%p, netif=%p)", p, &lwipInterfaces[i]);
							break;
						}
					}
				}
			}
		} break;
		case PP_HTONS(ETHTYPE_ARP): {
			for (int i=0; i<lwipInterfacesCount; i++) {
				if (lwipInterfaces[i].state) {
					pbuf_ref(p);
					if (lwipInterfaces[i].input(p, &lwipInterfaces[i]) != ERR_OK) {
						DEBUG_ERROR("packet input error (arp, p=%p, netif=%p)", p, &lwipInterfaces[i]);
					}
					break;
				}
			}
			break;
		} break;
		default:
			break;
	}
}

/*
void lwip_dns_init()
{
	dns_init();
}
*/

void lwip_start_dhcp(void *netif)
{
#if defined(LIBZT_IPV4)
	netifapi_dhcp_start((struct netif *)netif);
#endif
}

void lwip_init_interface(void *tapref, const ZeroTier::MAC &mac, const ZeroTier::InetAddress &ip)
{
	char ipbuf[INET6_ADDRSTRLEN], nmbuf[INET6_ADDRSTRLEN];
	char macbuf[ZT_MAC_ADDRSTRLEN];
	DEBUG_EXTRA("lwipInterfacesCount=%d", lwipInterfacesCount);
	struct netif *lwipdev = &lwipInterfaces[lwipInterfacesCount];
	DEBUG_EXTRA("netif=%p", lwipdev);

	if (ip.isV4()) {
		static ip4_addr_t ipaddr, netmask, gw;
		IP4_ADDR(&gw,127,0,0,1);
		ipaddr.addr = *((u32_t *)ip.rawIpData());
		netmask.addr = *((u32_t *)ip.netmask().rawIpData());
		netif_add(lwipdev, &ipaddr, &netmask, &gw, NULL, tapif_init, tcpip_input);
		lwipdev->state = tapref;
		lwipdev->output = etharp_output;
		lwipdev->mtu = ZT_MAX_MTU;
		lwipdev->name[0] = 'l';
		lwipdev->name[1] = '0'+lwipInterfacesCount;
		lwipdev->linkoutput = lwip_eth_tx;
		lwipdev->hwaddr_len = 6;
		mac.copyTo(lwipdev->hwaddr, lwipdev->hwaddr_len);
		lwipdev->flags = 0;
		lwipdev->flags = NETIF_FLAG_BROADCAST
			| NETIF_FLAG_ETHARP
			| NETIF_FLAG_IGMP
			| NETIF_FLAG_LINK_UP
			| NETIF_FLAG_UP;
		netif_set_default(lwipdev);
		netif_set_link_up(lwipdev);
		netif_set_up(lwipdev);
		mac2str(macbuf, ZT_MAC_ADDRSTRLEN, lwipdev->hwaddr);
		DEBUG_INFO("initialized netif as [mac=%s, addr=%s, nm=%s]", macbuf, ip.toString(ipbuf), ip.netmask().toString(nmbuf));
	}
	if (ip.isV6()) {
		static ip6_addr_t ipaddr;
		memcpy(&(ipaddr.addr), ip.rawIpData(), sizeof(ipaddr.addr));
		lwipdev->mtu = ZT_MAX_MTU;
		lwipdev->name[0] = 'l';
		lwipdev->name[1] = '0'+lwipInterfacesCount;
		lwipdev->hwaddr_len = 6;
		lwipdev->linkoutput = lwip_eth_tx;
		lwipdev->ip6_autoconfig_enabled = 1;
		mac.copyTo(lwipdev->hwaddr, lwipdev->hwaddr_len);
		netif_add(lwipdev, NULL, NULL, NULL, NULL, tapif_init, ethernet_input);
		lwipdev->output_ip6 = ethip6_output;
		lwipdev->state = tapref;
		netif_create_ip6_linklocal_address(lwipdev, 1);
		s8_t idx = 1;
		netif_add_ip6_address(lwipdev, &ipaddr, &idx);
		netif_set_default(lwipdev);
		netif_set_up(lwipdev);
		netif_set_link_up(lwipdev);
		netif_ip6_addr_set_state(lwipdev, 1, IP6_ADDR_TENTATIVE);
		mac2str(macbuf, ZT_MAC_ADDRSTRLEN, lwipdev->hwaddr);
		DEBUG_INFO("initialized netif as [mac=%s, addr=%s]", macbuf, ip.toString(ipbuf));
	}
	lwipInterfacesCount++;
}





/****************************************************************************/
/* Raw API driver                                                           */
/****************************************************************************/

#ifdef ZT_VIRTUAL_SOCKET

void nd6_tmr(void);


int rd_lwip_num_current_tcp_pcbs()
{
	// TODO: These will likely need some sort of locking protection
	int count = 0;
	struct tcp_pcb *pcb_ptr = tcp_active_pcbs; // PCBs that can RX/TX data
	while (pcb_ptr) {
		pcb_ptr = pcb_ptr->next;
		count++;
	}
	pcb_ptr = tcp_tw_pcbs; // PCBs in TIME-WAIT state
	while (pcb_ptr) {
		pcb_ptr = pcb_ptr->next;
		count++;
	}
	/* TODO
	pcb_ptr = tcp_listen_pcbs;
	while (pcb_ptr) {
		pcb_ptr = pcb_ptr->next;
		count++;
		DEBUG_ERROR("FOUND --- tcp_listen_pcbs PCB COUNT = %d", count);
	}*/
	pcb_ptr = tcp_bound_pcbs; // PCBs in a bound state
	while (pcb_ptr) {
		pcb_ptr = pcb_ptr->next;
		count++;
	}
	return count;
}

int rd_lwip_num_current_udp_pcbs()
{
	// TODO: These will likely need some sort of locking protection
	int count = 0;
	struct udp_pcb *pcb_ptr = udp_pcbs;
	while (pcb_ptr) {
		pcb_ptr = pcb_ptr->next;
		count++;
	}
	return count;
}

int rd_lwip_num_current_raw_pcbs()
{
	// TODO: These will likely need some sort of locking protection
	/*
	int count = 0;
	struct raw_pcb *pcb_ptr = raw_pcbs;
	while (pcb_ptr) {
		pcb_ptr = pcb_ptr->next;
		count++;
		DEBUG_ERROR("FOUND --- raw_pcbs PCB COUNT = %d", count);
	}
	return count;
	*/
	return 0;
}

int rd_lwip_num_total_pcbs()
{
	return rd_lwip_num_current_raw_pcbs() + rd_lwip_num_current_udp_pcbs() + rd_lwip_num_current_tcp_pcbs();
}

int rd_lwip_add_dns_nameserver(struct sockaddr *addr)
{
	return -1;
}

int rd_lwip_del_dns_nameserver(struct sockaddr *addr)
{
	return -1;
}

void rd_lwip_loop(VirtualTap *tap)
{
	uint64_t prev_tcp_time = 0, prev_discovery_time = 0;
	while (tap->_run)
	{
		uint64_t now = time_now();
		uint64_t since_tcp = now - prev_tcp_time;
		uint64_t since_discovery = now - prev_discovery_time;
		uint64_t tcp_remaining = LWIP_TCP_TIMER_INTERVAL;
		uint64_t discovery_remaining = 5000;

		// Main TCP/ETHARP timer section
		if (since_tcp >= LWIP_TCP_TIMER_INTERVAL) {
			prev_tcp_time = now;
			tcp_tmr();
		}
		else {
			tcp_remaining = LWIP_TCP_TIMER_INTERVAL - since_tcp;
		}

#if defined(LIBZT_IPV4)
		if (since_discovery >= DISCOVERY_INTERVAL) {
			prev_discovery_time = now;
			etharp_tmr();
		}  else {
			discovery_remaining = DISCOVERY_INTERVAL - since_discovery;
		}
#endif

#if defined(LIBZT_IPV6)
	if (since_discovery >= ND6_DISCOVERY_INTERVAL) {
			prev_discovery_time = now;
			nd6_tmr();
		} else {
			discovery_remaining = ND6_DISCOVERY_INTERVAL - since_discovery;
		}
	}
#endif

		tap->_phy.poll((unsigned long)std::min(tcp_remaining,discovery_remaining));
		tap->Housekeeping();
	}
}

int rd_lwip_socket(void **pcb, int socket_family, int socket_type, int protocol)
{
	if (virt_can_provision_new_socket(socket_type) == false) {
		DEBUG_ERROR("unable to create socket due to limitation of network stack, PCBs=%d", rd_lwip_num_total_pcbs());
		errno = ENOMEM;
		return -1;
	}
	if (socket_type == SOCK_STREAM) {
		struct tcp_pcb *new_tcp_PCB = tcp_new();
		*pcb = new_tcp_PCB;
		tcp_nagle_disable(new_tcp_PCB);
		return ERR_OK;
	}
	if (socket_type == SOCK_DGRAM) {
		struct udp_pcb *new_udp_PCB = udp_new();
		*pcb = new_udp_PCB;
		return ERR_OK;
	}
	errno = ENOMEM;
	return -1;
}

int rd_lwip_connect(VirtualSocket *vs, const struct sockaddr *addr, socklen_t addrlen)
{
	ip_addr_t ba;
	char addrstr[INET6_ADDRSTRLEN];
	int port = 0, err = 0;
/*
#if defined(LIBZT_IPV4)
		struct sockaddr_in *in4 = (struct sockaddr_in *)addr;
		if (addr->sa_family == AF_INET && vs->socket_type == SOCK_STREAM) {
			inet_ntop(AF_INET, &(in4->sin_addr), addrstr, INET_ADDRSTRLEN);
			DEBUG_EXTRA("connecting to %s : %d", addrstr, lwip_ntohs(in4->sin_port));
		}
		convert_ip(in4, &ba);
		port = lwip_ntohs(in4->sin_port);
#endif
#if defined(LIBZT_IPV6)
		struct sockaddr_in6 *in6 = (struct sockaddr_in6*)&addr;
		in6_to_ip6((ip6_addr *)&ba, in6);
		if (addr->sa_family == AF_INET6 && vs->socket_type == SOCK_STREAM) {
			inet_ntop(AF_INET6, &(in6->sin6_addr), addrstr, INET6_ADDRSTRLEN);
			DEBUG_EXTRA("connecting to %s : %d", addrstr, lwip_ntohs(in6->sin6_port));
		}
#endif
*/
	if (vs->socket_type == SOCK_DGRAM) {
		// generates no network traffic
		if ((err = udp_connect((struct udp_pcb*)vs->pcb,(ip_addr_t *)&ba,port)) < 0) {
			DEBUG_ERROR("error while connecting to with UDP");
		}
		udp_recv((struct udp_pcb*)vs->pcb, rd_lwip_cb_udp_recved, vs);
		return ERR_OK;
	}
	if (vs->socket_type == SOCK_STREAM) {
		struct tcp_pcb *tpcb = (struct tcp_pcb*)vs->pcb;
		tcp_sent(tpcb, rd_lwip_cb_sent);
		tcp_recv(tpcb, rd_lwip_cb_tcp_recved);
		tcp_err(tpcb, rd_lwip_cb_err);
		tcp_poll(tpcb, rd_lwip_cb_poll, LWIP_APPLICATION_POLL_FREQ);
		tcp_arg(tpcb, vs);
		if ((err = tcp_connect(tpcb, &ba, port, rd_lwip_cb_connected)) < 0) {
			errno = err_to_errno(err);
			// We should only return a value if failure happens immediately
			// Otherwise, we still need to wait for a callback from lwIP.
			// - This is because an ERR_OK from tcp_connect() only verifies
			//   that the SYN packet was enqueued onto the stack properly,
			//   that's it!
			DEBUG_ERROR("unable to connect");
			err = -1;
		}
	}
	return err;
}

int rd_lwip_bind(VirtualTap *tap, VirtualSocket *vs, const struct sockaddr *addr, socklen_t addrlen)
{
	// TODO: Check case for IP_ADDR_ANY
	ip_addr_t ba;
	char addrstr[INET6_ADDRSTRLEN];
	memset(addrstr, 0, INET6_ADDRSTRLEN);
	int port = 0, err = 0;
	/*
#if defined(LIBZT_IPV4)
		struct sockaddr_in *in4 = (struct sockaddr_in *)addr;
		if (addr->sa_family == AF_INET) {
			inet_ntop(AF_INET, &(in4->sin_addr), addrstr, INET_ADDRSTRLEN);
			DEBUG_EXTRA("binding to %s : %d", addrstr, lwip_ntohs(in4->sin_port));
		}
		convert_ip(in4, &ba);
		port = lwip_ntohs(in4->sin_port);
#endif
#if defined(LIBZT_IPV6)
		struct sockaddr_in6 *in6 = (struct sockaddr_in6*)addr;
		in6_to_ip6((ip6_addr *)&ba, in6);
		if (addr->sa_family == AF_INET6) {
			inet_ntop(AF_INET6, &(in6->sin6_addr), addrstr, INET6_ADDRSTRLEN);
			DEBUG_EXTRA("binding to %s : %d", addrstr, lwip_ntohs(in6->sin6_port));
		}
#endif
	if (vs->socket_type == SOCK_DGRAM) {
		if ((err = udp_bind((struct udp_pcb*)vs->pcb, (const ip_addr_t *)&ba, port)) < 0) {
			errno = err_to_errno(err);
			err = -1;
		}
		else {
			// set callback
			udp_recv((struct udp_pcb*)vs->pcb, rd_lwip_cb_udp_recved, vs);
			err = ERR_OK;
		}
	}
	else if (vs->socket_type == SOCK_STREAM) {
		if ((err = tcp_bind((struct tcp_pcb*)vs->pcb, (const ip_addr_t *)&ba, port)) < 0) {
			errno = err_to_errno(err);
			err = -1;
		}
		else {
			err = ERR_OK;
		}
	}
	*/
	return err;
}

int rd_lwip_listen(VirtualSocket *vs, int backlog)
{
	int err = 0;
	struct tcp_pcb* listeningPCB;
#ifdef TCP_LISTEN_BACKLOG
	listeningPCB = tcp_listen_with_backlog((struct tcp_pcb*)vs->pcb, backlog);
#else
	listeningPCB = tcp_listen((struct tcp_pcb*)vs->pcb);
#endif
	if (listeningPCB) {
		vs->pcb = listeningPCB;
		// set callback
		tcp_accept(listeningPCB, rd_lwip_cb_accept);
		tcp_arg(listeningPCB, vs);
		err = ERR_OK;
	}
	else {
		errno = ENOMEM;
		err = -1;
	}
	return err;
}

VirtualSocket* rd_lwip_accept(VirtualSocket *vs)
{
	if (vs == NULL) {
		DEBUG_ERROR("invalid virtual socket");
		return NULL;
	}
	// Retreive first of queued VirtualSockets from parent VirtualSocket
	// TODO: check multithreaded behaviour
	VirtualSocket *new_vs = NULL;
	if (vs->_AcceptedConnections.size()) {
		new_vs = vs->_AcceptedConnections.front();
		vs->_AcceptedConnections.pop();
	}
	return new_vs;
}

int rd_lwip_read(VirtualSocket *vs, bool lwip_invoked)
{
	DEBUG_EXTRA("vs=%p", vs);
	int err = 0;
	if (vs == NULL) {
		DEBUG_ERROR("no virtual socket");
		return -1;
	}
	if (lwip_invoked == false) {
		DEBUG_INFO("!lwip_invoked");
		vs->tap->_tcpconns_m.lock();
		vs->_rx_m.lock();
	}
	if (vs->socket_type == SOCK_STREAM && vs->RXbuf->count()) {
		int wr = std::min((ssize_t)ZT_STACK_TCP_SOCKET_RX_SZ, (ssize_t)vs->RXbuf->count());
		int n = vs->tap->_phy.streamSend(vs->sock, vs->RXbuf->get_buf(), wr);
		if (n > 0) {
			vs->RXbuf->consume(n);
			tcp_recved((struct tcp_pcb*)vs->pcb, n);
			DEBUG_TRANS("TCP RX %d bytes", n);
		}
	}
	if (vs->RXbuf->count() == 0) {
		vs->tap->_phy.setNotifyWritable(vs->sock, false); // nothing else to send to the app
	}
	if (lwip_invoked == false) {
		vs->tap->_tcpconns_m.unlock();
		vs->_rx_m.unlock();
	}
	return err;
}

int rd_lwip_write(VirtualSocket *vs, void *data, ssize_t len)
{
	int err = 0;
	if (vs == NULL) {
		DEBUG_ERROR("no virtual socket");
		return -1;
	}
	DEBUG_EXTRA("fd=%d, vs=%p, pcb=%p, pcb->state=%d, len=%d",
		vs->app_fd, vs, (struct tcp_pcb*)(vs->pcb), ((struct tcp_pcb*)(vs->pcb))->state, len);
	if (vs->socket_type == SOCK_DGRAM) {
		// TODO: Packet re-assembly hasn't yet been tested with lwIP so UDP packets are limited to MTU-sized chunks
		int udp_trans_len = std::min(len, (ssize_t)ZT_MAX_MTU);
		struct pbuf * pb = pbuf_alloc(PBUF_TRANSPORT, udp_trans_len, PBUF_POOL);
		if (pb == NULL) {
			DEBUG_ERROR("unable to allocate new pbuf of len=%d", udp_trans_len);
			return -1;
		}
		memcpy(pb->payload, data, udp_trans_len);
		int err = udp_send((struct udp_pcb*)vs->pcb, pb);

		if (err == ERR_MEM) {
			DEBUG_ERROR("error sending packet. out of memory");
		} else if (err == ERR_RTE) {
			DEBUG_ERROR("could not find route to destinations address");
		} else if (err != ERR_OK) {
			DEBUG_ERROR("error sending packet - %d", err);
		}
		pbuf_free(pb);
		if (err == ERR_OK) {
			return udp_trans_len;
		}
	}
	if (vs->socket_type == SOCK_STREAM) {
		// How much we are currently allowed to write to the VirtualSocket
		ssize_t sndbuf = ((struct tcp_pcb*)vs->pcb)->snd_buf;
		if (sndbuf == 0) {
			// PCB send buffer is full, turn off readability notifications for the
			// corresponding PhySocket until lwip_cb_sent() is called and confirms that there is
			// now space on the buffer
			DEBUG_ERROR("lwIP stack is full, sndbuf==0");
			//vs->tap->_phy.setNotifyReadable(vs->sock, false);
			err = -1;
		}
		vs->_tx_m.lock();
		int buf_w = vs->TXbuf->write((const char*)data, len);
		if (buf_w != len) {
			DEBUG_ERROR("only wrote len=%d but expected to write len=%d to TX buffer", buf_w, len);
			err = ZT_ERR_GENERAL_FAILURE;
		}
		if (vs->TXbuf->count() <= 0) {
			err = -1; // nothing to write
		}
		if (err == ERR_OK) {
			int r = std::min((ssize_t)vs->TXbuf->count(), sndbuf);
			// Writes data pulled from the client's socket buffer to LWIP. This merely sends the
			// data to LWIP to be enqueued and eventually sent to the network.
			if (r > 0) {
				err = tcp_write((struct tcp_pcb*)vs->pcb, vs->TXbuf->get_buf(), r, vs->copymode);
				tcp_output((struct tcp_pcb*)vs->pcb);
				if (err != ERR_OK) {
					DEBUG_ERROR("error while writing to lwIP tcp_pcb, err=%d", err);
					if (err == ERR_MEM) {
						DEBUG_ERROR("lwIP out of memory");
					}
					err = -1;
				} else {
					if (vs->copymode & TCP_WRITE_FLAG_COPY) {
						// since we copied the data (allocated pbufs), we can consume the buffer
						vs->TXbuf->consume(r); // success
						DEBUG_TRANS("len=%5d tx_buf_len=%10d [VSTXBF        -->     NSLWIP]", err, vs->TXbuf->count());
					}
					else {
						// since we only processed the data by pointer reference we
						// want to preserve it until it has been ACKed by the remote host
						// (DO NOTHING)
					}
					err = ERR_OK;
				}
			}
		}
		vs->_tx_m.unlock();
	}
	return err;
}

int rd_lwip_close(VirtualSocket *vs)
{
	// requests to close non-LISTEN PCBs are handled lwip_cb_poll()
	int err = -1;
	if (vs == NULL) {
		DEBUG_ERROR("invalid vs");
		return -1;
	}
	if (vs->socket_type == SOCK_STREAM) {
		struct tcp_pcb *tpcb = (struct tcp_pcb*)(vs->pcb);
		if (tpcb == NULL) {
			DEBUG_ERROR("invalid pcb");
			return -1;
		}
		// should be safe to tcp_close() from application thread IF PCB is in LISTENING state (I think)
		if (tpcb->state == LISTEN) {
			DEBUG_EXTRA("PCB is in LISTEN, calling tcp_close() from application thread.");
			tcp_accept(tpcb, NULL);
			if ((err = tcp_close(tpcb)) < 0) {
				DEBUG_ERROR("error while calling tcp_close, fd=%d, vs=%p, pcb=%p", vs->app_fd, vs, vs->pcb);
				errno = err_to_errno(err);
				err = -1;
			}
			return ERR_OK;
		}
		// handle junk state values
		if (tpcb->state > TIME_WAIT) {
			DEBUG_EXTRA("invalid TCP pcb state, already closed, report ERR_OK");
			return ERR_OK;
		}
		else {
			// place a request for the stack to close this VirtualSocket's PCB
			vs->set_state(VS_STATE_SHOULD_SHUTDOWN);
			// wait for indication of success, this will block if the PCB can't close
			while (true) {
				sleep(1);
				nanosleep((const struct timespec[]) {{0, (ZT_API_CHECK_INTERVAL * 1000000)}}, NULL);
				DEBUG_EXTRA("checking closure state... pcb->state=%d", tpcb->state);
				if (vs->get_state() == VS_STATE_CLOSED || tpcb->state == CLOSED) {
					return ERR_OK;
				}
			}
		}
	}
	if (vs->socket_type == SOCK_DGRAM) {
		// place a request for the stack to close this VirtualSocket's PCB
		vs->set_state(VS_STATE_SHOULD_SHUTDOWN);
	}
	return err;
}

int rd_lwip_shutdown(VirtualSocket *vs, int how)
{
	int err=0, shut_rx=0, shut_tx=0;
	if (how == SHUT_RD) {
		shut_rx = 1;
	}
	if (how == SHUT_WR) {
		shut_tx = 1;
	}
	if (how == SHUT_RDWR) {
		shut_rx = 1;
		shut_tx = 1;
	}
	if ((err = tcp_shutdown((tcp_pcb*)(vs->pcb), shut_rx, shut_tx) < 0)) {
		DEBUG_ERROR("error while shutting down socket, fd=%d", vs->app_fd);
	}
	return err;
}

/****************************************************************************/
/* Callbacks from lwIP stack                                                */
/****************************************************************************/

// write data from processed packets from the stack to the client app
/*
	With the raw API, tcp_recv() sets up to receive data via a callback function. Your callback
	is delivered chains of pbufs as they become available. You have to manage extracting data
	from the pbuf chain, and don't forget to watch out for multiple pbufs in a single callback:
	the 'tot_len' field indicates the total length of data in the pbuf chain. You must call
	tcp_recved() to tell LWIP when you have processed the received data. As with the netconn API,
	you may receive more or less data than you want, and will have to either wait for further
	callbacks, or hold onto excess data for later processing.

	http://lwip.wikia.com/wiki/Receiving_data_with_LWIP
*/
err_t rd_lwip_cb_tcp_recved(void *arg, struct tcp_pcb *PCB, struct pbuf *p, err_t err)
{
	//DEBUG_INFO();
	VirtualSocket *vs = (VirtualSocket *)arg;
	int tot = 0;
	if (vs == NULL) {
		DEBUG_ERROR("no virtual socket");
		return ERR_OK;
	}
	struct pbuf* q = p;
	if (p == NULL) {
		DEBUG_INFO("p=0x0 for pcb=%p, vs->pcb=%p, this indicates a closure. No need to call tcp_close()", PCB, vs->pcb);
		vs->set_state(VS_STATE_SHOULD_SHUTDOWN);
		return ERR_ABRT;
	}
	vs->tap->_tcpconns_m.lock();
	vs->_rx_m.lock();
	// cycle through pbufs and write them to the RX buffer
	while (p != NULL) {
		if (p->len <= 0) {
			break;
		}
		int avail = ZT_TCP_RX_BUF_SZ - vs->RXbuf->count();
		int len = p->len;
		if (avail < len) {
			DEBUG_ERROR("not enough room (%d bytes) on RX buffer", avail);
		}
		// place new incoming data on ringbuffer before we try to send it to the app
		memcpy(vs->RXbuf->get_buf(), p->payload, len);
		vs->RXbuf->produce(len);
		p = p->next;
		tot += len;
	}
	if (tot) {
		tcp_recved(PCB, tot);
		DEBUG_TRANS("len=%5d rx_buf_len=%10d [NSLWIP        -->     VSRXBF]", tot, vs->RXbuf->count());
		int w, write_attempt_sz = vs->RXbuf->count() < ZT_MAX_MTU ? vs->RXbuf->count() : ZT_MAX_MTU;
		if ((w = write(vs->sdk_fd, vs->RXbuf->get_buf(), write_attempt_sz)) < 0) {
			DEBUG_ERROR("write(fd=%d)=%d, errno=%d", vs->sdk_fd, w, errno);
		}
		if (w > 0) {
			vs->RXbuf->consume(w);
			if (w < write_attempt_sz) {
				DEBUG_TRANS("len=%5d rx_buf_len=%10d [VSRXBF        -->     APPFDS]", w, vs->RXbuf->count());
				DEBUG_EXTRA("intended to write len=%d, only wrote len=%d", write_attempt_sz, w);
			}
			else {
				DEBUG_TRANS("len=%5d rx_buf_len=%10d [VSRXBF        -->     APPFDS]", w, vs->RXbuf->count());
			}
		}
	}
	else {
		DEBUG_EXTRA("warning, wrote 0 bytes");
	}
	vs->tap->_tcpconns_m.unlock();
	vs->_rx_m.unlock();
	pbuf_free(q);
	return ERR_OK;
}

// callback from stack to notify driver of the successful acceptance of a connection
err_t rd_lwip_cb_accept(void *arg, struct tcp_pcb *newPCB, err_t err)
{
	VirtualSocket *vs = (VirtualSocket*)arg;
	struct sockaddr_storage ss;
/*
#if defined(LIBZT_IPV4)
	struct sockaddr_in *in4 = (struct sockaddr_in *)&ss;
	in4->sin_addr.s_addr = newPCB->remote_ip.addr;
	in4->sin_port = newPCB->remote_port;
#endif
#if defined(LIBZT_IPV6)
	struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)&ss;
	// TODO: check this
	memcpy(&(in6->sin6_addr.s6_addr), &(newPCB->remote_ip), sizeof(int32_t)*4);
	in6->sin6_port = newPCB->remote_port;
#endif
	VirtualSocket *new_vs = new VirtualSocket();
	new_vs->socket_type = SOCK_STREAM;
	new_vs->pcb = newPCB;
	new_vs->tap = vs->tap;
	new_vs->sock = vs->tap->_phy.wrapSocket(new_vs->sdk_fd, new_vs);
	memcpy(&(new_vs->peer_addr), &ss, sizeof(new_vs->peer_addr));
	// add new VirtualSocket object to parent VirtualSocket so that we can find it via lwip_Accept()
	vs->_AcceptedConnections.push(new_vs);
	// set callbacks
	tcp_arg(newPCB, new_vs);
	tcp_recv(newPCB, rd_lwip_cb_tcp_recved);
	tcp_err(newPCB, rd_lwip_cb_err);
	tcp_sent(newPCB, rd_lwip_cb_sent);
	tcp_poll(newPCB, rd_lwip_cb_poll, 1);
	// let lwIP know that it can queue additional incoming PCBs
	tcp_accepted((struct tcp_pcb*)vs->pcb);
*/
	return 0;
}

// copy processed datagram to app socket
void rd_lwip_cb_udp_recved(void * arg, struct udp_pcb * upcb, struct pbuf * p, const ip_addr_t * addr, u16_t port)
{
	//DEBUG_EXTRA("arg(vs)=%p, pcb=%p, port=%d)", arg, upcb, port);
	/*
	VirtualSocket *vs = (VirtualSocket *)arg;
	if (vs == NULL) {
		DEBUG_ERROR("invalid virtual socket");
		return;
	}
	if (p == NULL) {
		DEBUG_ERROR("p == NULL");
		return;
	}
	struct pbuf* q = p;
	struct sockaddr_storage ss;

#if defined(LIBZT_IPV4)
	struct sockaddr_in *in4 = (struct sockaddr_in *)&ss;
	in4->sin_addr.s_addr = addr->addr;
	in4->sin_port = port;
#endif
#if defined(LIBZT_IPV6)
	struct sockaddr_in6 *in6 = (struct sockaddr_in6 *)&ss;
	memcpy(&(in6->sin6_addr.s6_addr), &(addr->addr), sizeof(int32_t)*4);
	in6->sin6_port = port;
#endif

	char udp_payload_buf[ZT_SOCKET_MSG_BUF_SZ];
	memset(udp_payload_buf, 0, sizeof(ZT_SOCKET_MSG_BUF_SZ));
	char *msg_ptr = udp_payload_buf;
	int tot_len = 0;
	while (p != NULL) {
		if (p->len <= 0) {
			break;
		}
		memcpy(msg_ptr, p->payload, p->len);
		msg_ptr += p->len;
		tot_len += p->len;
		p = p->next;
	}
	if (tot_len > 0) {
		int w = 0;
		// [sz : addr : payload]
		char udp_msg_buf[ZT_SOCKET_MSG_BUF_SZ];
		memset(udp_msg_buf, 0, sizeof(ZT_SOCKET_MSG_BUF_SZ));
		int32_t len = sizeof(struct sockaddr_storage) + tot_len;
		int32_t msg_tot_len = sizeof(int32_t) + len;
		// len: sockaddr+payload
		memcpy(udp_msg_buf, &len, sizeof(int32_t));
		// sockaddr
		memcpy(udp_msg_buf + sizeof(int32_t), &ss, sizeof(struct sockaddr_storage));
		// payload
		memcpy(udp_msg_buf + sizeof(int32_t) + sizeof(struct sockaddr_storage), &udp_payload_buf, tot_len);
		if ((w = write(vs->sdk_fd, udp_msg_buf, msg_tot_len)) < 0) {
			DEBUG_ERROR("write(fd=%d)=%d, errno=%d", vs->sdk_fd, w, errno);
		}
	}
	pbuf_free(q);
	*/
}

// callback from stack to notify driver that data was sent
err_t rd_lwip_cb_sent(void* arg, struct tcp_pcb *PCB, u16_t len)
{
	//DEBUG_EXTRA("pcb=%p", PCB);
	VirtualSocket *vs = (VirtualSocket *)arg;
	if (vs == NULL) {
		DEBUG_ERROR("invalid vs for PCB=%p, len=%d", PCB, len);
	}
	if ((vs->copymode & TCP_WRITE_FLAG_COPY) == false) {
	/*
		From lwIP docs:

		To achieve zero-copy on transmit, the data passed to the raw API must
		remain unchanged until sent. Because the send- (or write-)functions return
		when the packets have been enqueued for sending, data must be kept stable
		after that, too.

		This implies that PBUF_RAM/PBUF_POOL pbufs passed to raw-API send functions
		must *not* be reused by the application unless their ref-count is 1.

		For no-copy pbufs (PBUF_ROM/PBUF_REF), data must be kept unchanged, too,
		but the stack/driver will/must copy PBUF_REF'ed data when enqueueing, while
		PBUF_ROM-pbufs are just enqueued (as ROM-data is expected to never change).

		Also, data passed to tcp_write without the copy-flag must not be changed!

		Therefore, be careful which type of PBUF you use and if you copy TCP data
		or not!
	*/

		// since we decided in lwip_Write() not to consume the buffere data, as it
		// was not copied and was only used by pointer reference, we can now consume
		// the data on the buffer since we've got an ACK back from the remote host
		vs->_tx_m.lock();
		vs->TXbuf->consume(len);
		vs->_tx_m.unlock();
	}
	return ERR_OK;
}

err_t rd_lwip_cb_connected(void *arg, struct tcp_pcb *PCB, err_t err)
{
	DEBUG_EXTRA("pcb=%p", PCB);
	VirtualSocket *vs = (VirtualSocket *)arg;
	if (vs == NULL) {
		DEBUG_ERROR("invalid virtual socket");
		return -1;
	}
	// add to unhandled connection set for zts_connect to pick up on
	vs->tap->_tcpconns_m.lock();
	vs->set_state(VS_STATE_UNHANDLED_CONNECTED);
	vs->tap->_VirtualSockets.push_back(vs);
	vs->tap->_tcpconns_m.unlock();
	return ERR_OK;
}

err_t rd_lwip_cb_poll(void* arg, struct tcp_pcb *PCB)
{
	VirtualSocket *vs = (VirtualSocket *)arg;
	if (vs == NULL) {
		DEBUG_ERROR("invalid vs");
		return ERR_OK; // TODO: determine appropriate error value, if any
	}
	if (vs->socket_type == SOCK_DGRAM) {
		DEBUG_INFO("fd=%d, vs=%p, pcb=%p", vs->app_fd, vs, PCB, vs->pcb);
	}

	// Handle PCB closure requests (set in lwip_Close())
	if (vs->get_state() == VS_STATE_SHOULD_SHUTDOWN) {
		DEBUG_EXTRA("closing pcb=%p, fd=%d, vs=%p", PCB, vs->app_fd, vs);
		int err = 0;
		errno = 0;
		if (vs->socket_type == SOCK_DGRAM) {
			udp_remove((struct udp_pcb*)vs->pcb);
		}
		if (vs->socket_type == SOCK_STREAM) {
			if (vs->pcb) {
				struct tcp_pcb* tpcb = (struct tcp_pcb*)vs->pcb;
				if (tpcb->state == CLOSED) {
					DEBUG_EXTRA("pcb is in CLOSED state");
					// calling tcp_close() here would be redundant
					return 0;
				}
				//if (tpcb->state == CLOSE_WAIT) {
				//	DEBUG_EXTRA("pcb is in CLOSE_WAIT state");
				//	// calling tcp_close() here would be redundant
				//}
				if (tpcb->state > TIME_WAIT) {
					DEBUG_ERROR("warning, pcb=%p is in an invalid state=%d", vs->pcb, tpcb->state);
					err = -1;
				}
				// unregister callbacks for this PCB
				tcp_arg(tpcb, NULL);
				if (tpcb->state == LISTEN) {
					tcp_accept(tpcb, NULL);
				}
				else {
					tcp_recv(tpcb, NULL);
					tcp_sent(tpcb, NULL);
					tcp_poll(tpcb, NULL, 0);
					tcp_err(tpcb,  NULL);
				}
				if ((err = tcp_close(tpcb)) < 0) {
					DEBUG_ERROR("error while calling tcp_close, fd=%d, vs=%p, pcb=%p", vs->app_fd, vs, vs->pcb);
					errno = err_to_errno(err);
					err = -1;
				}
				else {
					vs->set_state(VS_STATE_CLOSED); // success
				}
			}
		}
	}


	// Handle transmission and reception of data
	if (vs->socket_type == SOCK_STREAM) {
		DEBUG_INFO("fd=%d, vs=%p, PCB=%p, vs->pcb=%p, vs->pcb->state=%d", vs->app_fd, vs, PCB, (struct tcp_pcb*)(vs->pcb), ((struct tcp_pcb*)(vs->pcb))->state);
		if (((struct tcp_pcb*)(vs->pcb))->state == CLOSE_WAIT) {
			DEBUG_EXTRA("pcb->state=CLOSE_WAIT. do nothing");
			return ERR_OK;
		}
		if (((struct tcp_pcb*)(vs->pcb))->state == CLOSED) {
			DEBUG_EXTRA("pcb->state=CLOSED. do nothing");
			return ERR_OK;
		}
		// --- Check buffers to see if we need to finish reading/writing anything ---

		// TODO: Make a more generic form of each of these RX/TX blocks that can be shared
		// between all polling callbacks and read write methods

		// RX
		vs->_rx_m.lock();
		if (vs->RXbuf->count()) {
			// this data has already been acknowledged via tcp_recved(), we merely need to
			// move it off of the ringbuffer and into the client app
			int w, write_attempt_sz = vs->RXbuf->count() < ZT_MAX_MTU ? vs->RXbuf->count() : ZT_MAX_MTU;
			if ((w = write(vs->sdk_fd, vs->RXbuf->get_buf(), write_attempt_sz)) < 0) {
				DEBUG_ERROR("write(fd=%d)=%d, errno=%d", vs->sdk_fd, w, errno);
			}
			if (w > 0) {
				vs->RXbuf->consume(w);
				if (w < write_attempt_sz) {
					DEBUG_TRANS("len=%5d rx_buf_len=%10d [VSRXBF        -->     APPFDS]", w, vs->RXbuf->count());
					DEBUG_EXTRA("intended to write len=%d, only wrote len=%d", write_attempt_sz, w);
				}
				else {
					DEBUG_TRANS("len=%5d rx_buf_len=%10d [VSRXBF        -->     APPFDS]", w, vs->RXbuf->count());
				}
			}
		}
		vs->_rx_m.unlock();
		// No need to lock the TX buffer since lwip_Write() will lock it for us
		// TX
		if (vs->TXbuf->count()) {
			// we previously attempted to tcp_write(), but something went wrong, this
			// is where we retry
			rd_lwip_write(vs, vs->TXbuf->get_buf(), vs->TXbuf->count());
		}
	}
	return ERR_OK;
}

int rd_lwip_setsockopt(VirtualSocket *vs, int level, int optname, const void *optval, socklen_t optlen)
{
	int err = -1;
	errno = 0;
	if (vs == NULL) {
		DEBUG_ERROR("invalid vs");
		return -1;
	} else {
		DEBUG_EXTRA("fd=%d, level=%d, optname=%d", vs->app_fd, level, optname);
	}
	if (level == SOL_SOCKET)
	{
		/* Turns on recording of debugging information. This option enables or disables debugging in the underlying
		protocol modules. This option takes an int value. This is a Boolean option.*/
		if (optname == SO_DEBUG)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Specifies that the rules used in validating addresses supplied to bind() should allow reuse of local
		addresses, if this is supported by the protocol. This option takes an int value. This is a Boolean option.*/
		if (optname == SO_REUSEADDR)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Keeps connections active by enabling the periodic transmission of messages, if this is supported by the
		protocol. This option takes an int value. */
		if (optname == SO_KEEPALIVE)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Requests that outgoing messages bypass the standard routing facilities. */
		if (optname == SO_DONTROUTE)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Lingers on a close() if data is present. */
		if (optname == SO_LINGER)
		{
			// we do this at the VirtualSocket layer since lwIP's raw API doesn't currently have a way to do this
			vs->optflags &= VS_OPT_SO_LINGER;
			return 0;
		}
		/* Permits sending of broadcast messages, if this is supported by the protocol. This option takes an int
		value. This is a Boolean option. */
		if (optname == SO_BROADCAST)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Leaves received out-of-band data (data marked urgent) inline. This option takes an int value. This is a
		Boolean option. */
		if (optname == SO_OOBINLINE)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Sets send buffer size. This option takes an int value. */
		if (optname == SO_SNDBUF)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Sets receive buffer size. This option takes an int value. */
		if (optname == SO_RCVBUF)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* */
		if (optname == SO_STYLE)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* */
		if (optname == SO_TYPE)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Get error status and clear */
		if (optname == SO_ERROR)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
	}
	if (level == IPPROTO_IP)
	{
		if (optname == IP_ADD_MEMBERSHIP) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_ADD_SOURCE_MEMBERSHIP) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_BIND_ADDRESS_NO_PORT) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_BLOCK_SOURCE) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_DROP_MEMBERSHIP) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_DROP_SOURCE_MEMBERSHIP) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_FREEBIND) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_HDRINCL) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_MSFILTER) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_MTU) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_MTU_DISCOVER) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_MULTICAST_ALL) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_MULTICAST_IF) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_MULTICAST_LOOP) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_MULTICAST_TTL) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_NODEFRAG) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_OPTIONS) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_PKTINFO) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_RECVOPTS) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_RECVORIGDSTADDR) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_RECVTOS) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_RECVTTL) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_RETOPTS) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_ROUTER_ALERT) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_TOS) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_TRANSPARENT) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_TTL) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_UNBLOCK_SOURCE) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		// TODO
		return -1;
	}
	if (level == IPPROTO_TCP)
	{
		struct tcp_pcb *pcb = (struct tcp_pcb*)(vs->pcb);
		if (pcb == NULL) {
			return -1;
		}
		/* If set, don't send out partial frames. */
		if (optname == TCP_CORK) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Allow a listener to be awakened only when data arrives on the socket. */
		if (optname == TCP_DEFER_ACCEPT) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Used to collect information about this socket. The kernel returns a struct tcp_info as defined in the
		file /usr/include/linux/tcp.h.*/
		if (optname == TCP_INFO) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* The maximum number of keepalive probes TCP should send before dropping the connection.*/
		if (optname == TCP_KEEPCNT) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* The time (in seconds) the connection needs to remain idle before TCP starts sending keepalive probes,
		if the socket option SO_KEEPALIVE has been set on this socket. */
		if (optname == TCP_KEEPIDLE) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* The time (in seconds) between individual keepalive probes.*/
		if (optname == TCP_KEEPINTVL) {
			// TODO
			return -1;
		}
		/* The lifetime of orphaned FIN_WAIT2 state sockets. */
		if (optname == TCP_LINGER2) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* The maximum segment size for outgoing TCP packets. */
		if (optname == TCP_MAXSEG) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* If set, disable the Nagle algorithm. */
		if (optname == TCP_NODELAY) {
			int enable_nagle = *((const int*)optval);
			if (enable_nagle == true) {
				tcp_nagle_enable(pcb);
			} else {
				tcp_nagle_disable(pcb);
			}
			return 0;
		}
		/* Enable quickack mode if set or disable quickack mode if cleared. */
		if (optname == TCP_QUICKACK) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Set the number of SYN retransmits that TCP should send before aborting the attempt to connect. It
		cannot exceed 255. */
		if (optname == TCP_SYNCNT) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Bound the size of the advertised window to this value. The kernel imposes a minimum size of
		SOCK_MIN_RCVBUF/2. */
		if (optname == TCP_WINDOW_CLAMP) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
	}
	if (level == IPPROTO_UDP)
	{
		/*If this option is enabled, then all data output on this socket is accumulated into a single
		datagram that is transmitted when the option is disabled. */
		if (optname == UDP_CORK) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
	}
	return err;
}

int rd_lwip_getsockopt(VirtualSocket *vs, int level, int optname, void *optval, socklen_t *optlen)
{
	int err = -1, optval_tmp = 0;
	errno = 0;
	if (vs == NULL) {
		DEBUG_ERROR("invalid vs");
		return -1;
	} else {
		DEBUG_EXTRA("fd=%d, level=%d, optname=%d", vs->app_fd, level, optname);
	}
	if (level == SOL_SOCKET)
	{
		/* Turns on recording of debugging information. This option enables or disables debugging in the underlying
		protocol modules. This option takes an int value. This is a Boolean option.*/
		if (optname == SO_DEBUG)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Specifies that the rules used in validating addresses supplied to bind() should allow reuse of local
		addresses, if this is supported by the protocol. This option takes an int value. This is a Boolean option.*/
		if (optname == SO_REUSEADDR)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Keeps connections active by enabling the periodic transmission of messages, if this is supported by the
		protocol. This option takes an int value. */
		if (optname == SO_KEEPALIVE)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Requests that outgoing messages bypass the standard routing facilities. */
		if (optname == SO_DONTROUTE)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Get SO_LINGER flag value */
		if (optname == SO_LINGER)
		{
			// we do this at the VirtualSocket layer since lwIP's raw API doesn't currently have a way to do this
			optval_tmp = (vs->optflags & VS_OPT_SO_LINGER);
			memcpy(optval, &optval_tmp, *optlen);
			return 0;
		}
		/* Permits sending of broadcast messages, if this is supported by the protocol. This option takes an int
		value. This is a Boolean option. */
		if (optname == SO_BROADCAST)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Leaves received out-of-band data (data marked urgent) inline. This option takes an int value. This is a
		Boolean option. */
		if (optname == SO_OOBINLINE)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Sets send buffer size. This option takes an int value. */
		if (optname == SO_SNDBUF)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Sets receive buffer size. This option takes an int value. */
		if (optname == SO_RCVBUF)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* */
		if (optname == SO_STYLE)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* */
		if (optname == SO_TYPE)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		/* Get error status and clear */
		if (optname == SO_ERROR)
		{
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
	}
	if (level == IPPROTO_IP)
	{
		if (optname == IP_ADD_MEMBERSHIP) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_ADD_SOURCE_MEMBERSHIP) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_BIND_ADDRESS_NO_PORT) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_BLOCK_SOURCE) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_DROP_MEMBERSHIP) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_DROP_SOURCE_MEMBERSHIP) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_FREEBIND) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_HDRINCL) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_MSFILTER) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_MTU) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_MTU_DISCOVER) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_MULTICAST_ALL) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_MULTICAST_IF) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_MULTICAST_LOOP) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_MULTICAST_TTL) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_NODEFRAG) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_OPTIONS) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_PKTINFO) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_RECVOPTS) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_RECVORIGDSTADDR) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_RECVTOS) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_RECVTTL) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_RETOPTS) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_ROUTER_ALERT) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_TOS) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_TRANSPARENT) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_TTL) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		if (optname == IP_UNBLOCK_SOURCE) {
			// TODO
			errno = ENOPROTOOPT;
			return -1;
		}
		// TODO
		return -1;
	}
	if (level == IPPROTO_TCP)
	{
		struct tcp_pcb *pcb = (struct tcp_pcb*)(vs->pcb);
		if (pcb == NULL) {
			return -1;
		}
		/* If set, don't send out partial frames. */
		if (optname == TCP_CORK) {
			// TODO
			errno = ENOPROTOOPT;
			err = -1;
		}
		/* Allow a listener to be awakened only when data arrives on the socket. */
		if (optname == TCP_DEFER_ACCEPT) {
			// TODO
			errno = ENOPROTOOPT;
			err = -1;
		}
		/* Used to collect information about this socket. */
		if (optname == TCP_INFO) {
			// TODO
			errno = ENOPROTOOPT;
			err = -1;
		}
		/* The maximum number of keepalive probes TCP should send before dropping the connection.*/
		if (optname == TCP_KEEPCNT) {
			// TODO
			errno = ENOPROTOOPT;
			err = -1;
		}
		/* The time (in seconds) the connection needs to remain idle before TCP starts sending keepalive probes,
		if the socket option SO_KEEPALIVE has been set on this socket. */
		if (optname == TCP_KEEPIDLE) {
			// TODO
			errno = ENOPROTOOPT;
			err = -1;
		}
		/* The time (in seconds) between individual keepalive probes.*/
		if (optname == TCP_KEEPINTVL) {
			// TODO
			err = -1;
		}
		/* The lifetime of orphaned FIN_WAIT2 state sockets. */
		if (optname == TCP_LINGER2) {
			// TODO
			errno = ENOPROTOOPT;
			err = -1;
		}
		/* The maximum segment size for outgoing TCP packets. */
		if (optname == TCP_MAXSEG) {
			// TODO
			errno = ENOPROTOOPT;
			err = -1;
		}
		/* Get value of Nagle algorithm flag */
		if (optname == TCP_NODELAY) {
			optval_tmp = tcp_nagle_disabled(pcb);
			memcpy(optval, &optval_tmp, *optlen);
			err = 0;
		}
		/* Enable quickack mode if set or disable quickack mode if cleared. */
		if (optname == TCP_QUICKACK) {
			// TODO
			errno = ENOPROTOOPT;
			err = -1;
		}
		/* Set the number of SYN retransmits that TCP should send before aborting the attempt to connect. It
		cannot exceed 255. */
		if (optname == TCP_SYNCNT) {
			// TODO
			errno = ENOPROTOOPT;
			err = -1;
		}
		/* Bound the size of the advertised window to this value. The kernel imposes a minimum size of
		SOCK_MIN_RCVBUF/2. */
		if (optname == TCP_WINDOW_CLAMP) {
			// TODO
			errno = ENOPROTOOPT;
			err = -1;
		}
	}
	if (level == IPPROTO_UDP)
	{
		/* If this option is enabled, then all data output on this socket is accumulated into a single
		datagram that is transmitted when the option is disabled. */
		if (optname == UDP_CORK) {
			// TODO
			errno = ENOPROTOOPT;
			err = -1;
		}
	}
	return err;
}

void rd_lwip_cb_err(void *arg, err_t err)
{
	VirtualSocket *vs = (VirtualSocket *)arg;
	if (vs == NULL) {
		DEBUG_ERROR("err=%d, invalid virtual socket", err);
		errno = -1;
	}
	if (vs->socket_type == SOCK_STREAM) {
		DEBUG_ERROR("vs=%p, pcb=%p, pcb->state=%d, fd=%d, err=%d", vs, vs->pcb, ((struct tcp_pcb*)(vs->pcb))->state, vs->app_fd, err);
	}
	if (vs->socket_type == SOCK_DGRAM) {
		DEBUG_ERROR("vs=%p, pcb=%p, fd=%d, err=%d", vs, vs->pcb, vs->app_fd, err);
	}
	switch(err)
	{
		case ERR_MEM: // -1
			DEBUG_ERROR("ERR_MEM->ENOMEM, Out of memory error.");
			break;
		case ERR_BUF: // -2
			DEBUG_ERROR("ERR_BUF->ENOBUFS, Buffer error.");
			break;
		case ERR_TIMEOUT: // -3
			DEBUG_ERROR("ERR_TIMEOUT->ETIMEDOUT, Timeout.");
			break;
		case ERR_RTE: // -4
			DEBUG_ERROR("ERR_RTE->ENETUNREACH, Routing problem.");
			break;
		case ERR_INPROGRESS: // -5
			DEBUG_ERROR("ERR_INPROGRESS->EINPROGRESS, Operation in progress.");
			break;
		case ERR_VAL: // -6
			DEBUG_ERROR("ERR_VAL->EINVAL, Illegal value.");
			break;
		case ERR_WOULDBLOCK: // -7
			DEBUG_ERROR("ERR_WOULDBLOCK->EWOULDBLOCK, Operation would block.");
			break;
		case ERR_USE: // -8
			DEBUG_ERROR("ERR_USE->EADDRINUSE, Address in use.");
			break;
		case ERR_ALREADY: // -9 ?
			DEBUG_ERROR("ERR_ALREADY->EISCONN, Already connecting.");
			break;
		case ERR_ISCONN: // -10
			DEBUG_ERROR("ERR_ISCONN->EISCONN, Already connected");
			break;
		case ERR_CONN: // -11 ?
			DEBUG_ERROR("ERR_CONN->EISCONN, Not connected");
			break;
		case ERR_IF: // -12
			DEBUG_ERROR("ERR_IF, Low-level netif error.");
			break;
		case ERR_ABRT: // -13
			DEBUG_ERROR("ERR_ABRT, Connection aborted.");
			break;
		case ERR_RST: // -14
			DEBUG_ERROR("ERR_RST, Connection reset.");
			break;
		case ERR_CLSD: // -15
			DEBUG_ERROR("ERR_CLSD, Connection closed.");
			break;
		case ERR_ARG: // -16
			DEBUG_ERROR("ERR_ARG, Illegal argument.");
			break;
		default:
			break;
	}
	errno = err_to_errno(err);
}

#include "lwip/ip_addr.h"
#include <netinet/in.h>

#include "lwip/ip_addr.h"
#define ip4_addr1b(ipaddr) (((u8_t*)(ipaddr))[0])
#define ip4_addr2b(ipaddr) (((u8_t*)(ipaddr))[1])
#define ip4_addr3b(ipaddr) (((u8_t*)(ipaddr))[2])
#define ip4_addr4b(ipaddr) (((u8_t*)(ipaddr))[3])
inline void convert_ip(struct sockaddr_in * addr, struct ip4_addr *addr4);

/**
 * Convert from standard sockaddr address to ip4_addr
 */
inline void convert_ip(struct sockaddr_in *addr, struct ip4_addr *addr4)
{
    struct sockaddr_in *ipv4 = addr;
    short a = ip4_addr1b(&(ipv4->sin_addr));
    short b = ip4_addr2b(&(ipv4->sin_addr));
    short c = ip4_addr3b(&(ipv4->sin_addr));
    short d = ip4_addr4b(&(ipv4->sin_addr));
    IP4_ADDR(addr4, a,b,c,d);
}

#define IP6_ADDR2(ipaddr, a,b,c,d,e,f,g,h) do { (ipaddr)->addr[0] = ZeroTier::Utils::hton((u32_t)((a & 0xffff) << 16) | (b & 0xffff)); \
                                               (ipaddr)->addr[1] = ZeroTier::Utils::hton(((c & 0xffff) << 16) | (d & 0xffff)); \
                                               (ipaddr)->addr[2] = ZeroTier::Utils::hton(((e & 0xffff) << 16) | (f & 0xffff)); \
                                               (ipaddr)->addr[3] = ZeroTier::Utils::hton(((g & 0xffff) << 16) | (h & 0xffff)); } while(0)

/**
 * Convert from standard IPV6 address structure to an lwIP native structure
 */
inline void in6_to_ip6(ip6_addr *ba, struct sockaddr_in6 *in6)
{
    uint8_t *ip = &(in6->sin6_addr).s6_addr[0];
    IP6_ADDR2(ba,
        (((ip[ 0] & 0xffff) << 8) | ((ip[ 1]) & 0xffff)),
        (((ip[ 2] & 0xffff) << 8) | ((ip[ 3]) & 0xffff)),
        (((ip[ 4] & 0xffff) << 8) | ((ip[ 5]) & 0xffff)),
        (((ip[ 6] & 0xffff) << 8) | ((ip[ 7]) & 0xffff)),
        (((ip[ 8] & 0xffff) << 8) | ((ip[ 9]) & 0xffff)),
        (((ip[10] & 0xffff) << 8) | ((ip[11]) & 0xffff)),
        (((ip[12] & 0xffff) << 8) | ((ip[13]) & 0xffff)),
        (((ip[14] & 0xffff) << 8) | ((ip[15]) & 0xffff))
    );
}

#endif // ZT_VIRTUAL_SOCKET
#endif // STACK_LWIP
