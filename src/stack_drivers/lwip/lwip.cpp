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

#if defined(__ANDROID__)
    #include "src/debug.h"
#endif

#include "tap.hpp"
#include "sdkutils.hpp"

namespace ZeroTier
{
    void lwip_init_interface(NetconEthernetTap *tap, const InetAddress &ip)
    {
        DEBUG_INFO();
        lwIP_stack *stack = tap->lwipstack;
        Mutex::Lock _l(tap->_ips_m);

        if (std::find(tap->_ips.begin(),tap->_ips.end(),ip) == tap->_ips.end()) {
            tap->_ips.push_back(ip);
            std::sort(tap->_ips.begin(),tap->_ips.end());

        #if defined(SDK_IPV4)
            if (ip.isV4()) {
                // Set IP
                static ip_addr_t ipaddr, netmask, gw;
                IP4_ADDR(&gw,127,0,0,1);
                ipaddr.addr = *((u32_t *)ip.rawIpData());
                netmask.addr = *((u32_t *)ip.netmask().rawIpData());
                stack->__netif_add(&(tap->interface),&ipaddr, &netmask, &gw, NULL, tapif_init, stack->_ethernet_input);
                tap->interface.state = tap;
                tap->interface.output = stack->_etharp_output;
                tap->_mac.copyTo(tap->interface.hwaddr, 6);
                tap->interface.mtu = tap->_mtu;
                tap->interface.name[0] = 'l';
                tap->interface.name[1] = '4';
                tap->interface.linkoutput = low_level_output;
                tap->interface.hwaddr_len = 6;
                tap->interface.flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
                stack->__netif_set_default(&(tap->interface));
                stack->__netif_set_up(&(tap->interface));
                DEBUG_INFO("addr=%s, netmask=%s", ip.toString().c_str(), ip.netmask().toString().c_str());
            }
        #endif
        #if defined(SDK_IPV6)
            if(ip.isV6()) {
                DEBUG_INFO("local_addr=%s", ip.toString().c_str());
                static ip6_addr_t addr6;
                struct sockaddr_in6 in6;
                memcpy(in6.sin6_addr.s6_addr,ip.rawIpData(),16);
                in6_to_ip6((ip6_addr *)&addr6, &in6);
                tap->interface6.mtu = tap->_mtu;
                tap->interface6.name[0] = 'l';
                tap->interface6.name[1] = '6';
                tap->interface6.hwaddr_len = 6;
                tap->interface6.linkoutput = low_level_output;
                tap->interface6.ip6_autoconfig_enabled = 1;
                tap->_mac.copyTo(tap->interface6.hwaddr, tap->interface6.hwaddr_len);
                stack->__netif_create_ip6_linklocal_address(&(tap->interface6), 1);
                stack->__netif_add(&(tap->interface6), NULL, tapif_init, stack->_ethernet_input);
                stack->__netif_set_default(&(tap->interface6));
                stack->__netif_set_up(&(tap->interface6)); 
                netif_ip6_addr_set_state(&(tap->interface6), 1, IP6_ADDR_TENTATIVE); 
                ip6_addr_copy(ip_2_ip6(tap->interface6.ip6_addr[1]), addr6);
                tap->interface6.output_ip6 = stack->_ethip6_output;
                tap->interface6.state = tap;
                tap->interface6.flags = NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
                DEBUG_INFO("addr=%s, netmask=%s", ip.toString().c_str(), ip.netmask().toString().c_str());
            }   
        #endif  
        }
    }

    void lwip_loop(NetconEthernetTap *tap)
    {
        DEBUG_INFO();
        lwIP_stack *stack = tap->lwipstack;
        uint64_t prev_tcp_time = 0, prev_status_time = 0, prev_discovery_time = 0;
        // Main timer loop
        while (tap->_run) {
            uint64_t now = OSUtils::now();
            uint64_t since_tcp = now - prev_tcp_time;
            uint64_t since_discovery = now - prev_discovery_time;
            uint64_t since_status = now - prev_status_time;
            uint64_t tcp_remaining = ZT_LWIP_TCP_TIMER_INTERVAL;
            uint64_t discovery_remaining = 5000;

            #if defined(LWIP_IPV6)
                #define DISCOVERY_INTERVAL 1000
            #elif defined(LWIP_IPV4)
                #define DISCOVERY_INTERVAL ARP_TMR_INTERVAL
            #endif

            // Connection prunning
            if (since_status >= STATUS_TMR_INTERVAL) {
                prev_status_time = now;
                for(size_t i=0;i<tap->_Connections.size();++i) {
                    if(!tap->_Connections[i]->sock || tap->_Connections[i]->type != SOCK_STREAM)
                        continue;
                    int fd = tap->_phy.getDescriptor(tap->_Connections[i]->sock);
                    // DEBUG_INFO(" tap_thread(): tcp\\jobs = {%d, %d}\n", _Connection.size(), jobmap.size());
                    // If there's anything on the RX buf, set to notify in case we stalled
                    if(tap->_Connections[i]->rxsz > 0)
                        tap->_phy.setNotifyWritable(tap->_Connections[i]->sock, true);
                    fcntl(fd, F_SETFL, O_NONBLOCK);
                    unsigned char tmpbuf[BUF_SZ];
                    
                    ssize_t n = read(fd,&tmpbuf,BUF_SZ);
                    if(tap->_Connections[i]->TCP_pcb->state == SYN_SENT) {
                        DEBUG_EXTRA("  should finish or be removed soon, sock=%p, state=SYN_SENT", 
                            (void*)&(tap->_Connections[i]->sock));
                    }
                    if((n < 0 && errno != EAGAIN) || (n == 0 && errno == EAGAIN)) {
                        //DEBUG_INFO(" closing sock (%x)", (void*)_Connections[i]->sock);
                        tap->closeConnection(tap->_Connections[i]->sock);
                    } else if (n > 0) {
                        DEBUG_INFO(" data read during connection check (%ld bytes)", n);
                        tap->phyOnUnixData(tap->_Connections[i]->sock,tap->_phy.getuptr(tap->_Connections[i]->sock),&tmpbuf,n);
                    }       
                }
            }
            // Main TCP/ETHARP timer section
            if (since_tcp >= ZT_LWIP_TCP_TIMER_INTERVAL) {
                prev_tcp_time = now;
                stack->__tcp_tmr();
                // FIXME: could be removed or refactored?
                // Makeshift poll
                for(size_t i=0;i<tap->_Connections.size();++i) {
                    if(tap->_Connections[i]->txsz > 0){
                        lwip_handleWrite(tap, tap->_Connections[i]);
                    }
                }
            } else {
                tcp_remaining = ZT_LWIP_TCP_TIMER_INTERVAL - since_tcp;
            }
            if (since_discovery >= DISCOVERY_INTERVAL) {
                prev_discovery_time = now;
                #if defined(SDK_IPV4)
                    stack->__etharp_tmr();
                #endif
                #if defined(SDK_IPV6)
                    stack->__nd6_tmr();
                #endif
            } else {
                discovery_remaining = DISCOVERY_INTERVAL - since_discovery;
            }
            tap->_phy.poll((unsigned long)std::min(tcp_remaining,discovery_remaining));
        }
        stack->close();
    }

    void lwip_rx(NetconEthernetTap *tap, const MAC &from,const MAC &to,unsigned int etherType,const void *data,unsigned int len)
    {
        // DEBUG_EXTRA();
        lwIP_stack *stack = tap->lwipstack;
        struct pbuf *p,*q;
        if (!tap->_enabled)
            return;
        struct eth_hdr ethhdr;
        from.copyTo(ethhdr.src.addr, 6);
        to.copyTo(ethhdr.dest.addr, 6);
        ethhdr.type = Utils::hton((uint16_t)etherType);
        
        p = stack->__pbuf_alloc(PBUF_RAW, len+sizeof(struct eth_hdr), PBUF_POOL);
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
            #if defined(SDK_IPV6)
                if(tap->interface6.input(p, &(tap->interface6)) != ERR_OK) {
                    DEBUG_ERROR("error while feeding frame into stack interface6");
                }
            #endif
            #if defined(SDK_IPV4)
                if(tap->interface.input(p, &(tap->interface)) != ERR_OK) {
                    DEBUG_ERROR("error while feeding frame into stack interface");
                }
            #endif
        }
    }

    // Create and set up a lwIP socket PCB and Connection object
    Connection *lwip_handleSocket(NetconEthernetTap *tap, PhySocket *sock, void **uptr, struct socket_st* socket_rpc)
    {
        lwIP_stack *stack = tap->lwipstack;
        struct udp_pcb *new_udp_PCB = NULL;
        struct tcp_pcb *new_tcp_PCB = NULL;

        if(socket_rpc->socket_type == SOCK_DGRAM) {
            DEBUG_EXTRA("SOCK_DGRAM");
            Mutex::Lock _l(tap->_tcpconns_m);
            new_udp_PCB = stack->__udp_new();
        }
        else if(socket_rpc->socket_type == SOCK_STREAM) {
            DEBUG_EXTRA("SOCK_STREAM");
            Mutex::Lock _l(tap->_tcpconns_m);
            new_tcp_PCB = stack->__tcp_new();
        }
        else if(socket_rpc->socket_type == SOCK_RAW) {
            DEBUG_ERROR("SOCK_RAW, not currently supported.");
        }
        if(new_udp_PCB || new_tcp_PCB) {
            Connection * newConn = new Connection();
            *uptr = newConn;
            newConn->type = socket_rpc->socket_type;
            newConn->sock = sock;
            newConn->local_addr = NULL;
            newConn->peer_addr = NULL;
            if(newConn->type == SOCK_DGRAM) newConn->UDP_pcb = new_udp_PCB;
            if(newConn->type == SOCK_STREAM) newConn->TCP_pcb = new_tcp_PCB;
            tap->_Connections.push_back(newConn);
            return newConn;
        }
        DEBUG_ERROR(" memory not available for new PCB");
        tap->sendReturnValue(tap->_phy.getDescriptor(sock), -1, ENOMEM);
        return NULL;
    }

    // 
    Connection * lwip_handleSocketProxy(NetconEthernetTap *tap, PhySocket *sock, int socket_type)
    {
        /*
        Connection *conn = getConnection(sock);
        if(!conn){
            DEBUG_ERROR("unable to locate Connection object for this PhySocket sock=%p", (void*)&sock);
            return NULL;
        }
        DEBUG_ATTN("sock=%p", (void*)&sock);
        struct udp_pcb *new_udp_PCB = NULL;
        struct tcp_pcb *new_tcp_PCB = NULL;
        if(socket_type == SOCK_DGRAM) {
            DEBUG_EXTRA("SOCK_DGRAM");
            Mutex::Lock _l(_tcpconns_m);
            new_udp_PCB = lwipstack->__udp_new();
        }
        else if(socket_type == SOCK_STREAM) {
            DEBUG_EXTRA("SOCK_STREAM");
            Mutex::Lock _l(_tcpconns_m);
            new_tcp_PCB = lwipstack->__tcp_new();
        }
        if(new_udp_PCB || new_tcp_PCB) {
            conn->sock = sock;
            conn->type = socket_type;
            conn->local_addr = NULL;
            conn->peer_addr = NULL;
            if(conn->type == SOCK_DGRAM) conn->UDP_pcb = new_udp_PCB;
            if(conn->type == SOCK_STREAM) conn->TCP_pcb = new_tcp_PCB;
            DEBUG_INFO(" updated sock=%p", (void*)&sock);
            return conn;
        }
        DEBUG_ERROR(" memory not available for new PCB");
        */
        return NULL;
    }

    // Connects an lwIP socket PCB to a remote host 
    void lwip_handleConnect(NetconEthernetTap *tap, PhySocket *sock, PhySocket *rpcSock, Connection *conn, struct connect_st* connect_rpc)
    {
        lwIP_stack *stack = tap->lwipstack;
        ip_addr_t ba;
        char addrstr[INET6_ADDRSTRLEN];
        struct sockaddr_in6 *rawAddr = (struct sockaddr_in6 *) &connect_rpc->addr;
        struct sockaddr *addr = (struct sockaddr*)rawAddr;
        int err, port = stack->__lwip_ntohs(rawAddr->sin6_port);

        // ipv4
        #if defined(SDK_IPV4)
            if(addr->sa_family == AF_INET) {
                struct sockaddr_in *connaddr = (struct sockaddr_in *)addr;
                inet_ntop(AF_INET, &(connaddr->sin_addr), addrstr, INET_ADDRSTRLEN);    
                sprintf(addrstr, "%s:%d", addrstr, stack->__lwip_ntohs(connaddr->sin_port));
            }
            struct sockaddr_in *rawAddr4 = (struct sockaddr_in *) &connect_rpc->addr;
            ba = convert_ip(rawAddr4); 
            port = stack->__lwip_ntohs(rawAddr4->sin_port);
        #endif

        // ipv6
        #if defined(SDK_IPV6)
            struct sockaddr_in6 *in6 = (struct sockaddr_in6*)&connect_rpc->addr;
            in6_to_ip6((ip6_addr *)&ba, in6);

            if(addr->sa_family == AF_INET6) {        
                struct sockaddr_in6 *connaddr6 = (struct sockaddr_in6 *)addr;
                inet_ntop(AF_INET6, &(connaddr6->sin6_addr), addrstr, INET6_ADDRSTRLEN);
                sprintf(addrstr, "%s:%d", addrstr, stack->__lwip_ntohs(connaddr6->sin6_port));
            }
        #endif
        DEBUG_INFO("addr=%s", addrstr);

        if(conn->type == SOCK_DGRAM) {
            // Generates no network traffic
            if((err = stack->__udp_connect(conn->UDP_pcb,(ip_addr_t *)&ba,port)) < 0)
                DEBUG_ERROR("error while connecting to with UDP");
            stack->__udp_recv(conn->UDP_pcb, nc_udp_recved, new Larg(tap, conn));
            tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), 0, ERR_OK);
            return;
        }
        if(conn != NULL) {
            stack->__tcp_sent(conn->TCP_pcb, nc_sent);
            stack->__tcp_recv(conn->TCP_pcb, nc_recved);
            stack->__tcp_err(conn->TCP_pcb, nc_err);
            stack->__tcp_poll(conn->TCP_pcb, nc_poll, APPLICATION_POLL_FREQ);
            stack->__tcp_arg(conn->TCP_pcb, new Larg(tap, conn));
                
            DEBUG_EXTRA(" pcb->state=%x", conn->TCP_pcb->state);
            if(conn->TCP_pcb->state != CLOSED) {
                DEBUG_INFO(" cannot connect using this PCB, PCB!=CLOSED");
                tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), -1, EAGAIN);
                return;
            }
            if((err = stack->__tcp_connect(conn->TCP_pcb,&ba,port,nc_connected)) < 0)
            {
                if(err == ERR_ISCONN) {
                    tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), -1, EISCONN); // Already in connected state
                    return;
                } if(err == ERR_USE) {
                    tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), -1, EADDRINUSE); // Already in use
                    return;
                } if(err == ERR_VAL) {
                    tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), -1, EINVAL); // Invalid ipaddress parameter
                    return;
                } if(err == ERR_RTE) {
                    tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), -1, ENETUNREACH); // No route to host
                    return;
                } if(err == ERR_BUF) {
                    tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), -1, EAGAIN); // No more ports available
                    return;
                }
                if(err == ERR_MEM) {
                    tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), -1, EAGAIN); // TODO: Doesn't describe the problem well, but closest match
                    return;
                }
                // We should only return a value if failure happens immediately
                // Otherwise, we still need to wait for a callback from lwIP.
                // - This is because an ERR_OK from tcp_connect() only verifies
                //   that the SYN packet was enqueued onto the stack properly,
                //   that's it!
                // - Most instances of a retval for a connect() should happen
                //   in the nc_connect() and nc_err() callbacks!
                DEBUG_ERROR(" unable to connect");
                tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), -1, EAGAIN);
            }
            // Everything seems to be ok, but we don't have enough info to retval
            conn->listening=true;
            conn->rpcSock=rpcSock; // used for return value from lwip CB
        } 
        else {
            DEBUG_ERROR(" could not locate PCB based on application-provided fd");
            tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), -1, EBADF);
        }
    }

    // Connect to a remote hose via the built-in SOCKS5 proxy server
    int lwip_handleConnectProxy(NetconEthernetTap *tap, PhySocket *sock, struct sockaddr_in *rawAddr)
    {
        /*
        DEBUG_ATTN("sock=%p", (void*)&sock);
        Mutex::Lock _l(_tcpconns_m);
        int port = rawAddr->sin_port;
        ip_addr_t connAddr = convert_ip(rawAddr);
        int err = 0;

        Connection *conn = getConnection(sock);
        if(!conn) {
            DEBUG_INFO(" unable to locate Connection object for sock=%p", (void*)&sock);
            return -1;
        }
        if(conn->type == SOCK_DGRAM) {
            // Generates no network traffic
            if((err = lwipstack->__udp_connect(conn->UDP_pcb,&connAddr,port)) < 0)
                DEBUG_INFO("error while connecting to with UDP (sock=%p)", (void*)&sock);
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
            DEBUG_INFO(" addr=%d.%d.%d.%d:%d", d[0],d[1],d[2],d[3], port);  
            DEBUG_INFO(" pcb->state=%x", conn->TCP_pcb->state);
            if(conn->TCP_pcb->state != CLOSED) {
                DEBUG_INFO(" cannot connect using this PCB, PCB!=CLOSED");
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
                    // Can occur for the following reasons: tcp_enqueue_flags()

                    // 1) tcp_enqueue_flags is always called with either SYN or FIN in flags.
                    // We need one available snd_buf byte to do that.
                    // This means we can't send FIN while snd_buf==0. A better fix would be to
                    // not include SYN and FIN sequence numbers in the snd_buf count.

                    // 2) Cannot allocate new pbuf
                    // 3) Cannot allocate new TCP segment

                    errno = EAGAIN; // TODO: Doesn't describe the problem well, but closest match
                    return -1;
                }
                // We should only return a value if failure happens immediately
                // Otherwise, we still need to wait for a callback from lwIP.
                // - This is because an ERR_OK from tcp_connect() only verifies
                //   that the SYN packet was enqueued onto the stack properly,
                //   that's it!
                // - Most instances of a retval for a connect() should happen
                //   in the nc_connect() and nc_err() callbacks!
                DEBUG_ERROR(" unable to connect");
                errno = EAGAIN;
                return -1;
            }
            // Everything seems to be ok, but we don't have enough info to retval
            conn->listening=true;
            return 0;
        } else {
            DEBUG_ERROR(" could not locate PCB based on application-provided fd");
            errno = EBADF;
            return -1;
        }
        */
        return -1;
    }

    void lwip_handleBind(NetconEthernetTap *tap, PhySocket *sock, PhySocket *rpcSock, void **uptr, struct bind_st *bind_rpc)
    {
        lwIP_stack *stack = tap->lwipstack;
        ip_addr_t ba;
        char addrstr[INET6_ADDRSTRLEN];
        struct sockaddr_in6 *rawAddr = (struct sockaddr_in6 *) &bind_rpc->addr;
        struct sockaddr *addr = (struct sockaddr*)rawAddr;
        int err, port = stack->__lwip_ntohs(rawAddr->sin6_port);

        // ipv4
        #if defined(SDK_IPV4)
            if(addr->sa_family == AF_INET) {
                struct sockaddr_in *connaddr = (struct sockaddr_in *)addr;
                inet_ntop(AF_INET, &(connaddr->sin_addr), addrstr, INET_ADDRSTRLEN);    
                sprintf(addrstr, "%s:%d", addrstr, stack->__lwip_ntohs(connaddr->sin_port));
            }
            struct sockaddr_in *rawAddr4 = (struct sockaddr_in *) &bind_rpc->addr;
            ba = convert_ip(rawAddr4); 
            port = stack->__lwip_ntohs(rawAddr4->sin_port);
        #endif

        // ipv6
        #if defined(SDK_IPV6)
            struct sockaddr_in6 *in6 = (struct sockaddr_in6*)&bind_rpc->addr;
            in6_to_ip6((ip6_addr *)&ba, in6);
            if(addr->sa_family == AF_INET6) {        
                struct sockaddr_in6 *connaddr6 = (struct sockaddr_in6 *)addr;
                inet_ntop(AF_INET6, &(connaddr6->sin6_addr), addrstr, INET6_ADDRSTRLEN);
                sprintf(addrstr, "%s:%d", addrstr, stack->__lwip_ntohs(connaddr6->sin6_port));
            }
        #endif
        
        Connection *conn = tap->getConnection(sock);
        DEBUG_ATTN(" addr=%s, sock=%p, fd=%d", addrstr, (void*)&sock, bind_rpc->fd);
        if(conn) {
            if(conn->type == SOCK_DGRAM) {
                #if defined(__ANDROID__)
                    err = stack->__udp_bind(conn->UDP_pcb, NULL, port);
                #else
                    err = stack->__udp_bind(conn->UDP_pcb, (const ip_addr_t *)&ba, port);
                #endif
                if(err == ERR_USE) // port in use
                    tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), -1, EADDRINUSE);
                else {
                    stack->__udp_recv(conn->UDP_pcb, nc_udp_recved, new Larg(tap, conn));
                    struct sockaddr_in addr_in;
                    memcpy(&addr_in, &bind_rpc->addr, sizeof(addr_in));
                    addr_in.sin_port = Utils::ntoh(conn->UDP_pcb->local_port); // Newly assigned port
                    memcpy(&conn->local_addr, &addr_in, sizeof(addr_in));
                    tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), ERR_OK, ERR_OK); // Success
                }
                return;
            }
            else if (conn->type == SOCK_STREAM) {
                if(conn->TCP_pcb->state == CLOSED){
                    err = stack->__tcp_bind(conn->TCP_pcb, (const ip_addr_t *)&ba, port);
                    if(err != ERR_OK) {
                        DEBUG_ERROR("err=%d", err);
                        if(err == ERR_USE)
                            tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), -1, EADDRINUSE);
                        if(err == ERR_MEM)
                            tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), -1, ENOMEM);
                        if(err == ERR_BUF)
                            tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), -1, ENOMEM);
                    } else {
                        conn->local_addr = (struct sockaddr_storage *) &bind_rpc->addr;
                        tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), ERR_OK, ERR_OK); // Success
                    }
                } else {
                    DEBUG_ERROR(" ignoring BIND request, PCB (conn=%p, pcb=%p) not in CLOSED state. ", 
                        (void*)&conn, (void*)&conn->TCP_pcb);
                    tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), -1, EINVAL);
                }
            }
        } 
        else {
            DEBUG_ERROR(" unable to locate Connection");
            tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), -1, EBADF);
        }
    }

    void lwip_handleListen(NetconEthernetTap *tap, PhySocket *sock, PhySocket *rpcSock, void **uptr, struct listen_st *listen_rpc)
    {
        DEBUG_ATTN("sock=%p", (void*)&sock);
        lwIP_stack *stack = tap->lwipstack;

        Connection *conn = tap->getConnection(sock);
        if(conn->type==SOCK_DGRAM) {
            // FIX: Added sendReturnValue() call to fix listen() return bug on Android
            tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), ERR_OK, ERR_OK);
            return;
        }
        if(!conn) {
            DEBUG_ERROR(" unable to locate Connection");
            tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), -1, EBADF);
            return;
        }
        if(conn->TCP_pcb->state == LISTEN) {
            DEBUG_ERROR(" PCB is already in listening state");
            tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), ERR_OK, ERR_OK);
            return;
        }
        struct tcp_pcb* listeningPCB;

        #ifdef TCP_LISTEN_BACKLOG
            listeningPCB = stack->__tcp_listen_with_backlog(conn->TCP_pcb, listen_rpc->backlog);
        #else
            listeningPCB = stack->__tcp_listen(conn->pcb);
        #endif
        if(listeningPCB != NULL) {
            conn->TCP_pcb = listeningPCB;
            stack->__tcp_accept(listeningPCB, nc_accept);
            stack->__tcp_arg(listeningPCB, new Larg(tap, conn));
            fcntl(tap->_phy.getDescriptor(conn->sock), F_SETFL, O_NONBLOCK);
            conn->listening = true;
            tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), ERR_OK, ERR_OK);
            return;
        }
      tap->sendReturnValue(tap->_phy.getDescriptor(rpcSock), -1, -1);
    }

    // (RX packet) Read data from the RX buffer that lwIP just wrote to
    void lwip_handleRead(NetconEthernetTap *tap, PhySocket *sock, void **uptr, bool lwip_invoked)
    {
        DEBUG_EXTRA();
        lwIP_stack *stack = tap->lwipstack;
        if(!lwip_invoked) {
            tap->_tcpconns_m.lock();
            tap->_rx_buf_m.lock(); 
        }
        Connection *conn = tap->getConnection(sock); 
        if(conn && conn->rxsz) {
            float max = conn->type == SOCK_STREAM ? (float)DEFAULT_TCP_RX_BUF_SZ : (float)DEFAULT_UDP_RX_BUF_SZ;
            long n = tap->_phy.streamSend(conn->sock, conn->rxbuf, ZT_MAX_MTU);
            int payload_sz, addr_sz_offset = sizeof(struct sockaddr_storage);
            memcpy(&payload_sz, conn->rxbuf + addr_sz_offset, sizeof(int)); // OPT:
            // extract address
            struct sockaddr_storage addr;
            memcpy(&addr, conn->rxbuf, addr_sz_offset);
            
            if(n == ZT_MAX_MTU) {
                if(conn->rxsz-n > 0) // If more remains on buffer
                    memcpy(conn->rxbuf, conn->rxbuf+ZT_MAX_MTU, conn->rxsz - ZT_MAX_MTU);
                conn->rxsz -= ZT_MAX_MTU;
                // DGRAM
                if(conn->type==SOCK_DGRAM){
                    tap->_phy.setNotifyWritable(conn->sock, false);

                #if DEBUG_LEVEL >= MSG_TRANSFER
                    struct sockaddr_in * addr_in2 = (struct sockaddr_in *)&addr;
                    int port = stack->__lwip_ntohs(addr_in2->sin_port);
                    int ip = addr_in2->sin_addr.s_addr;
                    unsigned char d[4];
                    d[0] = ip & 0xFF;
                    d[1] = (ip >>  8) & 0xFF;
                    d[2] = (ip >> 16) & 0xFF;
                    d[3] = (ip >> 24) & 0xFF;
                    DEBUG_TRANS("UDP RX <---    :: {TX: %.3f%%, RX: %d, sock=%p} :: payload = %d bytes (src_addr=%d.%d.%d.%d:%d)", 
                        (float)conn->txsz / max, conn->rxsz/* / max*/, (void*)conn->sock, payload_sz, d[0],d[1],d[2],d[3], port);
                #endif
                }
                // STREAM
                //DEBUG_INFO("phyOnUnixWritable(): tid = %d\n", pthread_mach_thread_np(pthread_self()));
                if(conn->type==SOCK_STREAM) { // Only acknolwedge receipt of TCP packets
                    stack->__tcp_recved(conn->TCP_pcb, n);
                    DEBUG_TRANS("TCP RX <---    :: {TX: %.3f%%, RX: %.3f%%, sock=%p} :: %ld bytes",
                        (float)conn->txsz / max, (float)conn->rxsz / max, (void*)conn->sock, n);
                }
            } else {
                DEBUG_EXTRA(" errno = %d, rxsz = %d", errno, conn->rxsz);
                tap->_phy.setNotifyWritable(conn->sock, false);
            }
        }
        // If everything on the buffer has been written
        if(conn->rxsz == 0) {
            tap->_phy.setNotifyWritable(conn->sock, false);
        }
        if(!lwip_invoked) {
            tap->_tcpconns_m.unlock();
            tap->_rx_buf_m.unlock();
        }
    }

    // (TX packet) Write data from user app to network stack
    void lwip_handleWrite(NetconEthernetTap *tap, Connection *conn)
    {
        DEBUG_EXTRA("conn=%p", (void*)&conn);
        lwIP_stack *stack = tap->lwipstack;
        if(!conn || (!conn->TCP_pcb && !conn->UDP_pcb)) {
            DEBUG_ERROR(" invalid connection");
            return;
        }
        if(conn->type == SOCK_DGRAM) {
            if(!conn->UDP_pcb) {
                DEBUG_ERROR(" invalid UDP_pcb, type=SOCK_DGRAM");
                return;
            }
            // TODO: Packet re-assembly hasn't yet been tested with lwIP so UDP packets are limited to MTU-sized chunks
            int udp_trans_len = conn->txsz < ZT_UDP_DEFAULT_PAYLOAD_MTU ? conn->txsz : ZT_UDP_DEFAULT_PAYLOAD_MTU;
            
            DEBUG_EXTRA(" allocating pbuf chain of size=%d for UDP packet, txsz=%d", udp_trans_len, conn->txsz);
            struct pbuf * pb = stack->__pbuf_alloc(PBUF_TRANSPORT, udp_trans_len, PBUF_POOL);
            if(!pb){
                DEBUG_ERROR(" unable to allocate new pbuf of size=%d", conn->txsz);
                return;
            }
            memcpy(pb->payload, conn->txbuf, udp_trans_len);
            int err = stack->__udp_send(conn->UDP_pcb, pb);
            
            if(err == ERR_MEM) {
                DEBUG_ERROR(" error sending packet. out of memory");
            } else if(err == ERR_RTE) {
                DEBUG_ERROR(" could not find route to destinations address");
            } else if(err != ERR_OK) {
                DEBUG_ERROR(" error sending packet - %d", err);
            } else {
                // Success
                int buf_remaining = (conn->txsz)-udp_trans_len;
                if(buf_remaining)
                    memmove(&conn->txbuf, (conn->txbuf+udp_trans_len), buf_remaining);
                conn->txsz -= udp_trans_len;

                #if DEBUG_LEVEL >= MSG_TRANSFER
                    struct sockaddr_in * addr_in2 = (struct sockaddr_in *)conn->peer_addr;
                    int port = stack->__lwip_ntohs(addr_in2->sin_port);
                    int ip = addr_in2->sin_addr.s_addr;
                    unsigned char d[4];
                    d[0] = ip & 0xFF;
                    d[1] = (ip >>  8) & 0xFF;
                    d[2] = (ip >> 16) & 0xFF;
                    d[3] = (ip >> 24) & 0xFF;
                    DEBUG_TRANS("[UDP TX] --->    :: {TX: ------, RX: ------, sock=%p} :: %d bytes (dest_addr=%d.%d.%d.%d:%d)", 
                        (void*)conn->sock, udp_trans_len, d[0], d[1], d[2], d[3], port);
                #endif
            }
            stack->__pbuf_free(pb);
            return;
        }
        else if(conn->type == SOCK_STREAM) {
            if(!conn->TCP_pcb) {
                DEBUG_ERROR(" invalid TCP_pcb, type=SOCK_STREAM");
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
                    DEBUG_ERROR(" LWIP stack is full, sndbuf == 0");
                    tap->_phy.setNotifyReadable(conn->sock, false);
                    conn->probation = true;
                }
                return;
            }
            if(conn->txsz <= 0)
                return; // Nothing to write
            if(!conn->listening)
                stack->__tcp_output(conn->TCP_pcb);

            if(conn->sock) {
                r = conn->txsz < sndbuf ? conn->txsz : sndbuf;
                // Writes data pulled from the client's socket buffer to LWIP. This merely sends the
                // data to LWIP to be enqueued and eventually sent to the network.
                if(r > 0) {
                    err = stack->__tcp_write(conn->TCP_pcb, &conn->txbuf, r, TCP_WRITE_FLAG_COPY);
                    stack->__tcp_output(conn->TCP_pcb);
                    if(err != ERR_OK) {
                        DEBUG_ERROR(" error while writing to PCB, err=%d", err);
                        if(err == -1)
                            DEBUG_ERROR("out of memory");
                        return;
                    } else {
                        // adjust buffer
                        sz = (conn->txsz)-r;
                        if(sz)
                            memmove(&conn->txbuf, (conn->txbuf+r), sz);
                        conn->txsz -= r;
                        int max = conn->type == SOCK_STREAM ? DEFAULT_TCP_TX_BUF_SZ : DEFAULT_UDP_TX_BUF_SZ;
                        DEBUG_TRANS("[TCP TX] --->    :: {TX: %.3f%%, RX: %.3f%%, sock=%p} :: %d bytes",
                            (float)conn->txsz / (float)max, (float)conn->rxsz / max, (void*)&conn->sock, r);
                        return;
                    }
                }
            }
        }
    }

    void lwip_handleClose(NetconEthernetTap *tap, PhySocket *sock, Connection *conn)
    {
        DEBUG_ATTN();
        lwIP_stack *stack = tap->lwipstack;

        if(conn->type==SOCK_DGRAM) {
            stack->__udp_remove(conn->UDP_pcb);
        }
        if(conn->TCP_pcb && conn->TCP_pcb->state != CLOSED) {
            DEBUG_EXTRA("conn=%p, sock=%p, PCB->state = %d", 
                (void*)&conn, (void*)&sock, conn->TCP_pcb->state);
            if(conn->TCP_pcb->state == SYN_SENT /*|| conn->TCP_pcb->state == CLOSE_WAIT*/) {
                DEBUG_EXTRA("ignoring close request. invalid PCB state for this operation. sock=%p", (void*)&sock);
                return;
            }   
            // DEBUG_BLANK("__tcp_close(...)");
            if(stack->__tcp_close(conn->TCP_pcb) == ERR_OK) {
                // Unregister callbacks for this PCB
                stack->__tcp_arg(conn->TCP_pcb, NULL);
                stack->__tcp_recv(conn->TCP_pcb, NULL);
                stack->__tcp_err(conn->TCP_pcb, NULL);
                stack->__tcp_sent(conn->TCP_pcb, NULL);
                stack->__tcp_poll(conn->TCP_pcb, NULL, 1);
            }
            else {
                DEBUG_EXTRA("error while calling tcp_close() sock=%p", (void*)&sock);
            }
        }
    }

    /*------------------------------------------------------------------------------
    -------------------------------- lwIP Callbacks --------------------------------
    ------------------------------------------------------------------------------*/

    err_t tapif_init(struct netif *netif)
    {
      // Actual init functionality is in addIp() of tap
      return ERR_OK;
    }

    /*
     * Outputs data from the pbuf queue to the interface
     */
    err_t low_level_output(struct netif *netif, struct pbuf *p)
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

    //
    err_t nc_recved(void *arg, struct tcp_pcb *PCB, struct pbuf *p, err_t err)
    {
        Larg *l = (Larg*)arg;
        DEBUG_EXTRA("conn=%p, pcb=%p", (void*)&(l->conn), (void*)&PCB);
        int tot = 0;
        struct pbuf* q = p;
        Mutex::Lock _l(l->tap->_tcpconns_m);

        if(!l->conn) {
            DEBUG_ERROR("no connection");
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
                DEBUG_ERROR("not enough room (%d bytes) on RX buffer", avail);
            memcpy(l->conn->rxbuf + (l->conn->rxsz), p->payload, len);
            l->conn->rxsz += len;
            p = p->next;
            tot += len;
        }
        if(tot) {
            //#if defined(USE_SOCKS_PROXY)
            //  l->tap->phyOnTcpWritable(l->conn->sock, NULL, true);
            //#else
                l->tap->phyOnUnixWritable(l->conn->sock, NULL, true);
            //#endif
        }
        l->tap->lwipstack->__pbuf_free(q);
        return ERR_OK;
    }

    //
    err_t nc_accept(void *arg, struct tcp_pcb *newPCB, err_t err)
    {
        DEBUG_ATTN("pcb=%p", (void*)&newPCB);
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
                    // sendReturnValue(conn, -1, errno);
                    DEBUG_ERROR("unable to create socketpair");
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
            if(conn->TCP_pcb->state == LISTEN)
                return ERR_OK;
            tcp_accepted(conn->TCP_pcb); // Let lwIP know that it can queue additional incoming connections
            return ERR_OK;
        } else
            DEBUG_ERROR("can't locate Connection object for PCB");
        return -1;
    }
        
    //
    void nc_udp_recved(void * arg, struct udp_pcb * upcb, struct pbuf * p, ip_addr_t * addr, u16_t port)
    {
        Larg *l = (Larg*)arg;
        DEBUG_EXTRA("nc_udp_recved(conn=%p,pcb=%p,port=%d)\n", (void*)&(l->conn), (void*)&upcb, port);
        /*
        int tot = 0;
        unsigned char *addr_pos, *sz_pos, *payload_pos;
        struct pbuf* q = p;
        struct sockaddr_storage sockaddr_big;

    #if defined(LWIP_IPV6)
        struct sockaddr_in6 addr_in;
        addr_in.sin6_addr.s6_addr = addr->u_addr.ip6.addr;
        addr_in.sin6_port = port;
    #else // ipv4
        struct sockaddr_in *addr_in = (struct sockaddr_in *)&sockaddr_big;
        addr_in->sin_addr.s_addr = addr->addr;
        addr_in->sin_port = port;
    #endif

        // TODO: Finish address treatment

        Mutex::Lock _l2(l->tap->_rx_buf_m);
        // Cycle through pbufs and write them to the RX buffer
        // The RX "buffer" will be emptied via phyOnUnixWritable()
        if(p) {
            // Intra-API "packetization" scheme: [addr_len|addr|payload_len|payload]
            if(l->conn->rxsz == DEFAULT_UDP_RX_BUF_SZ) { // if UDP buffer full
                DEBUG_INFO("UDP RX buffer full. Discarding oldest payload segment");
                memmove(l->conn->rxbuf, l->conn->rxbuf + ZT_MAX_MTU, DEFAULT_UDP_RX_BUF_SZ - ZT_MAX_MTU);
                addr_pos = l->conn->rxbuf + (DEFAULT_UDP_RX_BUF_SZ - ZT_MAX_MTU); // TODO:
                sz_pos = addr_pos + sizeof(struct sockaddr_storage);
                l->conn->rxsz -= ZT_MAX_MTU;
            }
            else {
                addr_pos = l->conn->rxbuf + l->conn->rxsz; // where we'll prepend the size of the address
                sz_pos = addr_pos + sizeof(struct sockaddr_storage);
            }
            payload_pos = addr_pos + sizeof(struct sockaddr_storage) + sizeof(tot); // where we'll write the payload
            // write remote host address
            memcpy(addr_pos, &addr_in, sizeof(struct sockaddr_storage));
        }
        while(p != NULL) {
            if(p->len <= 0)
                break;
            int len = p->len;
            memcpy(payload_pos, p->payload, len);
            payload_pos = payload_pos + len;
            p = p->next;
            tot += len;
        }
        if(tot) {
            l->conn->rxsz += ZT_MAX_MTU;
            memcpy(sz_pos, &tot, sizeof(tot));
            //DEBUG_EXTRA(" nc_udp_recved(): data_len = %d, rxsz = %d, addr_info_len = %d\n", 
            //  tot, l->conn->rxsz, sizeof(u32_t) + sizeof(u16_t));
            l->tap->phyOnUnixWritable(l->conn->sock, NULL, true);
            l->tap->_phy.setNotifyWritable(l->conn->sock, true);
        }
        l->tap->lwipstack->__pbuf_free(q);
        */
    }
       
    //
    err_t nc_sent(void* arg, struct tcp_pcb *PCB, u16_t len)
    {
        DEBUG_EXTRA("pcb=%p", (void*)&PCB);
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

    //
    err_t nc_connected_proxy(void *arg, struct tcp_pcb *PCB, err_t err)
    {
        DEBUG_INFO("pcb=%p", (void*)&PCB);
        return ERR_OK;
    }
       
    // 
    err_t nc_connected(void *arg, struct tcp_pcb *PCB, err_t err)
    {
        DEBUG_ATTN("pcb=%p", (void*)&PCB);
        Larg *l = (Larg*)arg;
        if(l && l->conn)
            l->tap->sendReturnValue(l->tap->_phy.getDescriptor(l->conn->rpcSock), ERR_OK, 0);
        return ERR_OK;
    }

    //
    err_t nc_poll(void* arg, struct tcp_pcb *PCB)
    {
        return ERR_OK;
    }

    //
    void nc_err(void *arg, err_t err)
    {
        DEBUG_ERROR("err=%d", err);
        Larg *l = (Larg*)arg;
        Mutex::Lock _l(l->tap->_tcpconns_m);

        if(!l->conn)
            DEBUG_ERROR("conn==NULL");
        int fd = l->tap->_phy.getDescriptor(l->conn->sock);
        switch(err)
        {
            case ERR_MEM:
              DEBUG_ERROR("ERR_MEM->ENOMEM");
                l->tap->sendReturnValue(fd, -1, ENOMEM);
                break;
            case ERR_BUF:
                DEBUG_ERROR("ERR_BUF->ENOBUFS");
                l->tap->sendReturnValue(fd, -1, ENOBUFS);
                break;
            case ERR_TIMEOUT:
                DEBUG_ERROR("ERR_TIMEOUT->ETIMEDOUT");
                l->tap->sendReturnValue(fd, -1, ETIMEDOUT);
                break;
            case ERR_RTE:
                DEBUG_ERROR("ERR_RTE->ENETUNREACH");
                l->tap->sendReturnValue(fd, -1, ENETUNREACH);
                break;
            case ERR_INPROGRESS:
                DEBUG_ERROR("ERR_INPROGRESS->EINPROGRESS");
                l->tap->sendReturnValue(fd, -1, EINPROGRESS);
                break;
            case ERR_VAL:
                DEBUG_ERROR("ERR_VAL->EINVAL");
                l->tap->sendReturnValue(fd, -1, EINVAL);
                break;
            case ERR_WOULDBLOCK:
                DEBUG_ERROR("ERR_WOULDBLOCK->EWOULDBLOCK");
                l->tap->sendReturnValue(fd, -1, EWOULDBLOCK);
                break;
            case ERR_USE:
                DEBUG_ERROR("ERR_USE->EADDRINUSE");
                l->tap->sendReturnValue(fd, -1, EADDRINUSE);
                break;
            case ERR_ISCONN:
                DEBUG_ERROR("ERR_ISCONN->EISCONN");
                l->tap->sendReturnValue(fd, -1, EISCONN);
                break;
            case ERR_ABRT:
                DEBUG_ERROR("ERR_ABRT->ECONNREFUSED");
                l->tap->sendReturnValue(fd, -1, ECONNREFUSED);
                break;

                // TODO: Below are errors which don't have a standard errno correlate

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
        DEBUG_ERROR(" closing connection");
        l->tap->closeConnection(l->conn->sock);
    }
}
