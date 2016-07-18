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

#include "SDK_Debug.h"
#include "SDK_EthernetTap.hpp"
#include "Phy.hpp"
#include "Utils.hpp"
#include "OSUtils.hpp"

#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sstream>


#define SOCKS_OPEN          0
#define SOCKS_CONNECT_INIT  1
#define SOCKS_CONNECT_IPV4  2
#define SOCKS_UDP           3 // ?
#define SOCKS_COMPLETE      4

#define CONNECTION_TIMEOUT  8

#define IDX_VERSION         0
#define IDX_COMMAND         1
#define IDX_METHOD          1
#define IDX_FRAG            1
#define IDX_ERROR_CODE      1
#define IDX_NMETHODS        1
#define IDX_METHODS         2 // Supported methods
#define IDX_ATYP            3
#define IDX_DST_ADDR        4 // L:D where L = addrlen, D = addr
#define IDX_

#define THIS_PROXY_VERSION  5
#define MAX_ADDR_LEN        32
#define PORT_LEN            2

namespace ZeroTier
{
	void NetconEthernetTap::StartProxy(const char *sockpath, const char *homepath, uint64_t nwid)
	{	
#if defined (__ANDROID__)
		LOGV("StartProxy()\n");
#else
		printf("StartProxy()\n");
#endif
      
	  	// Look for a port file for this network's proxy server instance
		char portFile[4096];
    	Utils::snprintf(portFile,sizeof(portFile),"%s/networks.d/%.16llx.port",homepath,nwid);
		std::string portStr;
		printf("Proxy(): Reading port from: %s\n", portFile);
		if(ZeroTier::OSUtils::fileExists(portFile,true))
		{
			if(ZeroTier::OSUtils::readFile(portFile, portStr)) {
				proxyListenPort = atoi(portStr.c_str());
			}
		}
		else
		{
			unsigned int randp = 0;
        	Utils::getSecureRandom(&randp,sizeof(randp));
        	proxyListenPort = 1000 + (randp % 1000);
			printf("Proxy(): No port specified in networks.d/%.16llx.port, randomly picking port\n", nwid);
			std::stringstream ss;
			ss << proxyListenPort;
			portStr = ss.str();
			if(!ZeroTier::OSUtils::writeFile(portFile, portStr)) {
				LOGV("unable to write proxy port file: %s\n", portFile);
			}  
		}

        struct sockaddr_in in4;
		memset(&in4,0,sizeof(in4));
		in4.sin_family = AF_INET;
		in4.sin_addr.s_addr = Utils::hton((uint32_t)0x00000000); // right now we just listen for TCP @0.0.0.0
		in4.sin_port = Utils::hton((uint16_t)proxyListenPort);
		proxyListenPhySocket = _phy.tcpListen((const struct sockaddr*)&in4,(void *)this);
		sockstate = SOCKS_OPEN;
		printf("SOCKS5 proxy server address for <%.16llx> is: <0.0.0.0:%d> (sock=%p)\n", nwid, proxyListenPort, (void*)&proxyListenPhySocket);
	}
    
    void ExtractAddress(int addr_type, unsigned char *buf, struct sockaddr_in * addr)
    {
    	// TODO: Generalize extraction logic
        if(addr_type == 3)
        {
            // Extract address from buffer
            int domain_len = buf[IDX_DST_ADDR]; // (L):D
            char addr_[MAX_ADDR_LEN];
            int port_ = 0;
            memset(addr_, 0, MAX_ADDR_LEN);
            memcpy(addr_, &buf[IDX_DST_ADDR+1], domain_len); // L:(D)
            memcpy(&port_, &buf[IDX_DST_ADDR+1]+(domain_len), PORT_LEN);
            port_ = Utils::hton((uint16_t)port_);
            std::string addr_str(addr_);
            // Format address for Netcon/lwIP
            addr->sin_family = AF_INET;
            addr->sin_port = port_;
            addr->sin_addr.s_addr = inet_addr(addr_str.c_str());
        }
    }

	void NetconEthernetTap::phyOnTcpData(PhySocket *sock,void **uptr,void *data,unsigned long len)
	{
		printf("phyOnTcpData(): sock=%p, len=%lu\n", (void*)&sock, len);
		unsigned char *buf;
		buf = (unsigned char *)data;
        
		// Get connection for this PhySocket
		Connection *conn = getConnection(sock);
		if(!conn) {
			printf("phyOnTcpData(): Unable to locate Connection for sock=%p\n", (void*)&sock);
			return;
		}

		// Write data to lwIP PCB (outgoing)
        if(conn->proxy_conn_state == SOCKS_COMPLETE)
        {
        	if(len) {
	            printf("len=%lu\n", len);
	            memcpy((&conn->txbuf)+(conn->txsz), buf, len);
	   			conn->txsz += len;
	   			handleWrite(conn);
   			}
        }

		if(conn->proxy_conn_state==SOCKS_UDP)
		{
			printf("SOCKS_UDP from client\n");
            // +----+------+------+----------+----------+----------+
            // |RSV | FRAG | ATYP | DST.ADDR | DST.PORT |   DATA   |
            // +----+------+------+----------+----------+----------+
            // | 2  |  1   |  1   | Variable |    2     | Variable |
            // +----+------+------+----------+----------+----------+

			//int fragment_num = buf[2];
			//int addr_type = buf[3];
		}

        // SOCKS_OPEN
        // +----+----------+----------+
        // |VER | NMETHODS | METHODS  |
        // +----+----------+----------+
        // | 1  |    1     | 1 to 255 |
        // +----+----------+----------+
		if(conn->proxy_conn_state==SOCKS_OPEN)
		{
			if(len >= 3)
			{
				int version = buf[IDX_VERSION];
				int methodsLength = buf[IDX_NMETHODS];
				int firstSupportedMethod = buf[IDX_METHODS];
				int supportedMethod = 0;

				// Password auth
				if(firstSupportedMethod == 2) {
					supportedMethod = firstSupportedMethod;
				}
				printf(" INFO <ver=%d, meth_len=%d, supp_meth=%d>\n", version, methodsLength, supportedMethod);

                // Send METHOD selection msg
                // +----+--------+
                // |VER | METHOD |
                // +----+--------+
                // | 1  |   1    |
                // +----+--------+
				char reply[2];
				reply[IDX_VERSION] = THIS_PROXY_VERSION; // version
				reply[IDX_METHOD] = supportedMethod;
				_phy.streamSend(sock, reply, sizeof(reply));

				// Set state for next message
				conn->proxy_conn_state = SOCKS_CONNECT_INIT;
			}
		}

        // SOCKS_CONNECT
        // +----+-----+-------+------+----------+----------+
        // |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
        // +----+-----+-------+------+----------+----------+
        // | 1  |  1  | X'00' |  1   | Variable |    2     |
        // +----+-----+-------+------+----------+----------+
		if(conn->proxy_conn_state==SOCKS_CONNECT_INIT)
		{
			// Ex. 4(meta) + 4(ipv4) + 2(port) = 10
			if(len >= 10)
			{
				int version = buf[IDX_VERSION];
				int cmd = buf[IDX_COMMAND];
				int addr_type = buf[IDX_ATYP];

				printf("SOCKS REQUEST = <ver=%d, cmd=%d, typ=%d>\n", version, cmd, addr_type);

				// CONNECT request
				if(cmd == 1) {
					// Ipv4
					if(addr_type == 1)
					{
						//printf("IPv4\n");
						int raw_addr;
						memcpy(&raw_addr, &buf[4], 4);
						char newaddr[16];
						inet_ntop(AF_INET, &raw_addr, (char*)newaddr, INET_ADDRSTRLEN);
						printf("new addr = %s\n", newaddr);

						int rawport, port;
						memcpy(&rawport, &buf[5], 2); 
						port = Utils::ntoh(rawport);
						printf("new port = %d\n", port);

						// Assemble new address
						struct sockaddr_in addr;
						addr.sin_addr.s_addr = IPADDR_ANY;
						addr.sin_family = AF_INET;
						addr.sin_port = Utils::hton(proxyListenPort);

						int fd = socket(AF_INET, SOCK_STREAM, 0);

						if(fd < 0)
							perror("socket");

						int err = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
						if(err < 0)
							perror("connect");
					}

					// Fully-qualified domain name
					if(addr_type == 3)
					{
                        int domain_len = buf[IDX_DST_ADDR]; // (L):D
                        struct sockaddr_in addr;
                        ExtractAddress(addr_type,buf,&addr);
						PhySocket * new_sock = handleSocketProxy(sock, SOCK_STREAM);
                        if(!new_sock)
							printf("Error while creating proxied-socket\n");
                        handleConnectProxy(sock, &addr);

						// Convert connection err code into SOCKS-err-code
						// X'00' succeeded
						// X'01' general SOCKS server failure
						// X'02' connection not allowed by ruleset
						// X'03' Network unreachable
						// X'04' Host unreachable
						// X'05' Connection refused
						// X'06' TTL expired
						// X'07' Command not supported
						// X'08' Address type not supported
						// X'09' to X'FF' unassigned

						// SOCKS_CONNECT_REPLY
                        // +----+-----+-------+------+----------+----------+
                        // |VER | REP |  RSV  | ATYP | BND.ADDR | BND.PORT |
                        // +----+-----+-------+------+----------+----------+
                        // | 1  |  1  | X'00' |  1   | Variable |    2     |
                        // +----+-----+-------+------+----------+----------+

                        printf("REPLY = %d\n", addr.sin_port);
						char reply[len]; // TODO: determine proper length
						int addr_len = domain_len;
						memset(reply, 0, len); // Create reply buffer at least as big as incoming SOCKS request data
						memcpy(&reply[IDX_DST_ADDR],&buf[IDX_DST_ADDR],domain_len);
						reply[IDX_VERSION] = THIS_PROXY_VERSION; // version
						reply[IDX_ERROR_CODE] = 0; // success/err code
						reply[2] = 0; // RSV
						reply[IDX_ATYP] = addr_type; // ATYP (1, 3, 4)
						reply[IDX_DST_ADDR] = addr_len;
						memcpy(&reply[IDX_DST_ADDR+domain_len], &addr.sin_port, PORT_LEN); // PORT
						_phy.streamSend(sock, reply, sizeof(reply));
                        
                        // Any further data activity on this PhySocket will be considered data to send
                        conn->proxy_conn_state = SOCKS_COMPLETE;
					}
					// END CONNECT
				}

				// BIND Request
				if(cmd == 2)
				{
					printf("BIND request\n");
					//char raw_addr[15];
					//int bind_port;
				}

				// UDP ASSOCIATION Request
				if(cmd == 3)
				{
					// PORT supplied should be port assigned by server in previous msg
					printf("UDP association request\n");

                    // SOCKS_CONNECT (Cont.)
                    // +----+-----+-------+------+----------+----------+
                    // |VER | CMD |  RSV  | ATYP | DST.ADDR | DST.PORT |
                    // +----+-----+-------+------+----------+----------+
                    // | 1  |  1  | X'00' |  1   | Variable |    2     |
                    // +----+-----+-------+------+----------+----------+

                    // NOTE: Similar to cmd==1, should consolidate logic

                    // NOTE: Can't separate out port with method used in IPv4 block
                    int domain_len = buf[4];
                    // Grab Addr:Port
                    char raw_addr[domain_len];
                    memset(raw_addr, 0, domain_len);
                    memcpy(raw_addr, &buf[5], domain_len);
                    
                    std::string ip, port, addrstr(raw_addr);
                    int del = addrstr.find(":");
                    ip = addrstr.substr(0, del);
                    port = addrstr.substr(del+1, domain_len);
                    
                    // Create new lwIP PCB
                    PhySocket * new_sock = handleSocketProxy(sock, SOCK_DGRAM);
                    
                    printf("new_sock = %p\n", (void*)&sock);
                    printf("new_sock = %p\n", (void*)&new_sock);
                    if(!new_sock)
                        printf("Error while creating proxied-socket\n");
                    
                    // Form address
                    struct sockaddr_in addr;
                    memset(&addr, '0', sizeof(addr));
                    addr.sin_family = AF_INET;
                    addr.sin_port = Utils::hton((uint16_t)atoi(port.c_str()));
                    addr.sin_addr.s_addr = inet_addr(ip.c_str());
                    //addr.sin_addr.s_addr = inet_addr("10.5.5.2");
                    handleConnectProxy(sock, &addr);
					conn->proxy_conn_state = SOCKS_UDP;
				}

				//if(addr_type == 1337)
				//{
				//	// IPv6
				//}
			}
		}
	}

	void NetconEthernetTap::phyOnTcpAccept(PhySocket *sockL,PhySocket *sockN,void **uptrL,void **uptrN,const struct sockaddr *from)
	{
		printf("phyOnTcpAccept(): sock=%p\n", (void*)&sockN);
		Connection *newConn = new Connection();
		newConn->sock = sockN;
		_phy.setNotifyWritable(sockN, false);
		_Connections.push_back(newConn);
	}

	void NetconEthernetTap::phyOnTcpConnect(PhySocket *sock,void **uptr,bool success)
	{
		printf("phyOnTcpConnect(): sock=%p\n", (void*)&sock);
	}

	// Unused -- no UDP or TCP from this thread/Phy<>
	void NetconEthernetTap::phyOnDatagram(PhySocket *sock,void **uptr,const struct sockaddr *local_address, const struct sockaddr *from,void *data,unsigned long len)
	{
		printf("phyOnDatagram(): len = %lu\n", len);
		if(len) {
            Connection *conn = getConnection(sock);
            if(!conn){
                printf("unable to locate Connection: sock=%p\n", (void*)sock);
                return;
            }
            unsigned char *buf = (unsigned char*)data;
			memcpy((&conn->txbuf)+(conn->txsz), buf, len);
			conn->txsz += len;
			handleWrite(conn);
		}
	}

	void NetconEthernetTap::phyOnTcpClose(PhySocket *sock,void **uptr) 
	{
		printf("phyOnTcpClose(): sock=%p\n", (void*)&sock);
		Mutex::Lock _l(_tcpconns_m);
		closeConnection(sock);
	}

	void NetconEthernetTap::phyOnTcpWritable(PhySocket *sock,void **uptr, bool lwip_invoked) 
	{
		printf(" phyOnTcpWritable(): sock=%p\n", (void*)&sock);
		processReceivedData(sock,uptr,lwip_invoked);
	}

	// RX data on stream socks and send back over client sock's underlying fd
	void NetconEthernetTap::phyOnFileDescriptorActivity(PhySocket *sock,void **uptr,bool readable,bool writable)
	{
		printf("phyOnFileDescriptorActivity(): sock=%p\n", (void*&)sock);
	}
}