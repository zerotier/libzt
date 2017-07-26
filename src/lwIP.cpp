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

// lwIP network stack driver 

#include "libzt.h"
#include "SocketTap.hpp"
#include "Utilities.hpp"

#include "lwIP.hpp"
#include "netif/ethernet.h"
#include "lwip/etharp.h"

err_t tapif_init(struct netif *netif)
{
  DEBUG_INFO();
  return ERR_OK;
}

err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    DEBUG_INFO();
    struct pbuf *q;
    char buf[ZT_MAX_MTU+32];
    char *bufptr;
    int totalLength = 0;

    ZeroTier::SocketTap *tap = (ZeroTier::SocketTap*)netif->state;
    bufptr = buf;
    // Copy data from each pbuf, one at a time
    for(q = p; q != NULL; q = q->next) {
        memcpy(bufptr, q->payload, q->len);
        bufptr += q->len;
        totalLength += q->len;
    }
    // Split ethernet header and feed into handler
    struct eth_hdr *ethhdr;
    ethhdr = (struct eth_hdr *)buf;

    ZeroTier::MAC src_mac;
    ZeroTier::MAC dest_mac;
    src_mac.setTo(ethhdr->src.addr, 6);
    dest_mac.setTo(ethhdr->dest.addr, 6);

    tap->_handler(tap->_arg,NULL,tap->_nwid,src_mac,dest_mac,
        ZeroTier::Utils::ntoh((uint16_t)ethhdr->type),0,buf + sizeof(struct eth_hdr),totalLength - sizeof(struct eth_hdr));
    return ERR_OK;
}

namespace ZeroTier
{
    void lwIP::lwip_init_interface(SocketTap *tap, const InetAddress &ip)
    {
        DEBUG_INFO();
        Mutex::Lock _l(tap->_ips_m);

        if (std::find(tap->_ips.begin(),tap->_ips.end(),ip) == tap->_ips.end()) {
            tap->_ips.push_back(ip);
            std::sort(tap->_ips.begin(),tap->_ips.end());
#if defined(LIBZT_IPV4)
            if (ip.isV4()) {
                // Set IP
                static ip_addr_t ipaddr, netmask, gw;
                IP4_ADDR(&gw,127,0,0,1);
                ipaddr.addr = *((u32_t *)ip.rawIpData());
                netmask.addr = *((u32_t *)ip.netmask().rawIpData());
                netif_add(&(tap->lwipdev),&ipaddr, &netmask, &gw, NULL, tapif_init, ethernet_input);
                tap->lwipdev.state = tap;
                tap->lwipdev.output = etharp_output;
                tap->_mac.copyTo(tap->lwipdev.hwaddr, 6);
                tap->lwipdev.mtu = tap->_mtu;
                tap->lwipdev.name[0] = 'l';
                tap->lwipdev.name[1] = '4';
                tap->lwipdev.linkoutput = low_level_output;
                tap->lwipdev.hwaddr_len = 6;
                tap->lwipdev.flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
                netif_set_default(&(tap->lwipdev));
                netif_set_up(&(tap->lwipdev));
                DEBUG_INFO("addr=%s, netmask=%s", ip.toString().c_str(), ip.netmask().toString().c_str());
            }
#endif
#if defined(LIBZT_IPV6)
            if(ip.isV6()) {
                DEBUG_INFO("local_addr=%s", ip.toString().c_str());
                static ip6_addr_t addr6;
                struct sockaddr_in6 in6;
                memcpy(in6.sin6_addr.s6_addr,ip.rawIpData(),16);
                in6_to_ip6((ip6_addr *)&addr6, &in6);
                tap->lwipdev6.mtu = tap->_mtu;
                tap->lwipdev6.name[0] = 'l';
                tap->lwipdev6.name[1] = '6';
                tap->lwipdev6.hwaddr_len = 6;
                tap->lwipdev6.linkoutput = low_level_output;
                tap->lwipdev6.ip6_autoconfig_enabled = 1;
                tap->_mac.copyTo(tap->lwipdev6.hwaddr, tap->lwipdev6.hwaddr_len);
                netif_create_ip6_linklocal_address(&(tap->lwipdev6), 1);
                netif_add(&(tap->lwipdev6), NULL, tapif_init, ethernet_input);
                netif_set_default(&(tap->lwipdev6));
                netif_set_up(&(tap->lwipdev6)); 
                netif_ip6_addr_set_state(&(tap->lwipdev6), 1, IP6_ADDR_TENTATIVE); 
                ip6_addr_copy(ip_2_ip6(tap->lwipdev6.ip6_addr[1]), addr6);
                tap->lwipdev6.output_ip6 = ethip6_output;
                tap->lwipdev6.state = tap;
                tap->lwipdev6.flags = NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
                DEBUG_INFO("addr=%s, netmask=%s", ip.toString().c_str(), ip.netmask().toString().c_str());
            }
#endif 
        }
    }

    void lwIP::lwip_loop(SocketTap *tap)
    {
       DEBUG_INFO();
       uint64_t prev_tcp_time = 0, prev_status_time = 0, prev_discovery_time = 0;
       while(tap->_run)
       {
            uint64_t now = OSUtils::now();
            uint64_t since_tcp = now - prev_tcp_time;
            uint64_t since_discovery = now - prev_discovery_time;
            uint64_t since_status = now - prev_status_time;
            uint64_t tcp_remaining = LWIP_TCP_TIMER_INTERVAL;
            uint64_t discovery_remaining = 5000;

#if defined(LIBZT_IPV6)
                #define DISCOVERY_INTERVAL 1000
#elif defined(LIBZT_IPV4)
                #define DISCOVERY_INTERVAL ARP_TMR_INTERVAL
#endif
            // Main TCP/ETHARP timer section
            if (since_tcp >= LWIP_TCP_TIMER_INTERVAL) {
                prev_tcp_time = now;
                tcp_tmr();
            } 
            else {
                tcp_remaining = LWIP_TCP_TIMER_INTERVAL - since_tcp;
            }
            if (since_discovery >= DISCOVERY_INTERVAL) {
                prev_discovery_time = now;
#if defined(LIBZT_IPV4)
                    etharp_tmr();
#endif
#if defined(LIBZT_IPV6)
                    nd6_tmr();
#endif
            } else {
                discovery_remaining = DISCOVERY_INTERVAL - since_discovery;
            }
            tap->_phy.poll((unsigned long)std::min(tcp_remaining,discovery_remaining));
        }
    }

    void lwIP::lwip_rx(SocketTap *tap, const MAC &from,const MAC &to,unsigned int etherType,const void *data,unsigned int len)
    {
        DEBUG_INFO();
        struct pbuf *p,*q;
        if (!tap->_enabled)
            return;
        struct eth_hdr ethhdr;
        from.copyTo(ethhdr.src.addr, 6);
        to.copyTo(ethhdr.dest.addr, 6);
        ethhdr.type = ZeroTier::Utils::hton((uint16_t)etherType);

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
        {
#if defined(LIBZT_IPV6)
                if(tap->lwipdev6.input(p, &(tap->lwipdev6)) != ERR_OK) {
                    DEBUG_ERROR("error while feeding frame into stack lwipdev6");
                }
#endif
#if defined(LIBZT_IPV4)
                if(tap->lwipdev.input(p, &(tap->lwipdev)) != ERR_OK) {
                    DEBUG_ERROR("error while feeding frame into stack lwipdev");
                }
#endif
        }
    }

    int lwIP::lwip_Socket(void **pcb, int socket_family, int socket_type, int protocol)
    {
        // TODO: check lwIP timers, and max sockets
        DEBUG_INFO();
        if(socket_type == SOCK_STREAM) {
            struct tcp_pcb *new_tcp_PCB = tcp_new();
            *pcb = new_tcp_PCB;
            return ERR_OK;
        }
        if(socket_type == SOCK_DGRAM) {
            struct udp_pcb *new_udp_PCB = udp_new();
            *pcb = new_udp_PCB;
            return ERR_OK;
        }
        if(socket_type == SOCK_RAW) {
            DEBUG_ERROR("SOCK_RAW, not currently supported.");
            return -1;
        }
        return -1;
    }

    int lwIP::lwip_Connect(Connection *conn, int fd, const struct sockaddr *addr, socklen_t addrlen)
    {
        DEBUG_INFO();
    }

    int lwIP::lwip_Bind(SocketTap *tap, Connection *conn, int fd, const struct sockaddr *addr, socklen_t addrlen)
    {
        DEBUG_INFO();
        ip_addr_t ba;
        char addrstr[INET6_ADDRSTRLEN];
        int port = 0, err = 0;

#if defined(LIBZT_IPV4)
        DEBUG_ERROR("A");
            struct sockaddr_in *in4;
            if(addr->sa_family == AF_INET) {
                DEBUG_ERROR("A");
                in4 = (struct sockaddr_in *)addr;
                DEBUG_ERROR("A");
                inet_ntop(AF_INET, &(in4->sin_addr), addrstr, INET_ADDRSTRLEN); 
                DEBUG_ERROR("A");   
                DEBUG_INFO("%s:%d", addrstr, lwip_ntohs(in4->sin_port));
            }
            ba = convert_ip(in4); 
            port = lwip_ntohs(in4->sin_port);
            DEBUG_INFO("port=%d", port);
            DEBUG_INFO("port=%d", lwip_ntohs(port));
#endif
#if defined(LIBZT_IPV6)
            struct sockaddr_in6 *in6 = (struct sockaddr_in6*)&addr;
            in6_to_ip6((ip6_addr *)&ba, in6);
            if(addr->sa_family == AF_INET6) {        
                struct sockaddr_in6 *connaddr6 = (struct sockaddr_in6 *)addr;
                inet_ntop(AF_INET6, &(connaddr6->sin6_addr), addrstr, INET6_ADDRSTRLEN);
                DEBUG_INFO("%s:%d", addrstr, lwip_ntohs(connaddr6->sin6_port));
            }
#endif
        if(conn->socket_type == SOCK_DGRAM) {
            err = udp_bind((struct udp_pcb*)conn->pcb, (const ip_addr_t *)&ba, port);
            if(err == ERR_USE) {
                err = -1;
                errno = EADDRINUSE; // port in use
            }
            else {
                // set the recv callback
                udp_recv((struct udp_pcb*)conn->pcb, nc_udp_recved, new ConnectionPair(tap, conn));
                err = ERR_OK; 
                errno = ERR_OK; // success
            }
        }
        else if (conn->socket_type == SOCK_STREAM) {
            err = tcp_bind((struct tcp_pcb*)conn->pcb, (const ip_addr_t *)&ba, port);
            if(err != ERR_OK) {
                DEBUG_ERROR("err=%d", err);
                if(err == ERR_USE){
                    err = -1; 
                    errno = EADDRINUSE;
                }
                if(err == ERR_MEM){
                    err = -1; 
                    errno = ENOMEM;
                }
                if(err == ERR_BUF){
                    err = -1; 
                    errno = ENOMEM;
                }
            } 
            else {
                err = ERR_OK; 
                errno = ERR_OK; // success
            }
        }
        return err;
    }

    int lwIP::lwip_Listen(SocketTap *tap, PhySocket *sock, PhySocket *rpcSock, void **uptr, struct listen_st *listen_rpc)
    {
        DEBUG_INFO();
        // to be implemented
    }

    int lwIP::lwip_Read(SocketTap *tap, PhySocket *sock, void **uptr, bool lwip_invoked)
    {
        DEBUG_EXTRA();
        // to be implemented
    }

    int lwIP::lwip_Write(SocketTap *tap, Connection *conn)
    {
        DEBUG_EXTRA("conn=%p", (void*)&conn);
        // to be implemented
    }

    int lwIP::lwip_Close(SocketTap *tap, PhySocket *sock, Connection *conn)
    {
        DEBUG_INFO();
        // to be implemented
    }

    /****************************************************************************/
    /* Callbacks from lwIP stack                                                */
    /****************************************************************************/

    err_t lwIP::nc_recved(void *arg, struct tcp_pcb *PCB, struct pbuf *p, err_t err)
    {
        DEBUG_INFO();
        // to be implemented
        return ERR_OK;
    }

    err_t lwIP::nc_accept(void *arg, struct tcp_pcb *newPCB, err_t err)
    {
        DEBUG_INFO();
        // to be implemented
        return -1;
    }
        
    void lwIP::nc_udp_recved(void * arg, struct udp_pcb * upcb, struct pbuf * p, const ip_addr_t * addr, u16_t port)
    {
        DEBUG_INFO();
        // to be implemented
    }
       
    err_t lwIP::nc_sent(void* arg, struct tcp_pcb *PCB, u16_t len)
    {
        DEBUG_EXTRA("pcb=%p", (void*)&PCB);
        // to be implemented
        return ERR_OK;
    }
       
    err_t lwIP::nc_connected(void *arg, struct tcp_pcb *PCB, err_t err)
    {
        DEBUG_ATTN("pcb=%p", (void*)&PCB);
        // to be implemented
        return ERR_OK;
    }

    err_t lwIP::nc_poll(void* arg, struct tcp_pcb *PCB)
    {
        DEBUG_INFO();
        // to be implemented
        return ERR_OK;
    }

    void lwIP::nc_err(void *arg, err_t err)
    {
        DEBUG_INFO();
        // to be implemented
    }
}
