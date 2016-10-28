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

#include "debug.h"
#include "tap.hpp"
 
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

void dwr(int level, const char *fmt, ... );

namespace ZeroTier
{
	int NetconEthernetTap::getProxyServerAddress(struct sockaddr_storage *addr) {
		if(sockstate >= 0) {
			addr = &proxyServerAddress;
			return 0;
		}
		return -1;
	}

	int NetconEthernetTap::getProxyServerPort() {
		struct sockaddr_in *in4;
		in4 = (struct sockaddr_in *)&proxyServerAddress;
		return in4->sin_port;
	}

	int NetconEthernetTap::stopProxyServer()
	{
		DEBUG_INFO();
		if(proxyListenPhySocket) {
			_phy.close(proxyListenPhySocket);
			return 0;
		}
		DEBUG_ERROR("invalid proxyListenPhySocket");
		return -1;
	}

	int NetconEthernetTap::startProxyServer(const char *homepath, uint64_t nwid, struct sockaddr_storage *addr)
	{	
		// Address of proxy server is determined in the following order:
		// - Provided address in param: addr
		// - If no address, assume 127.0.0.1:<networks.d/nwid.port>
		// - If no port assignment file, 127.0.0.1:RANDOM_PORT

		DEBUG_INFO();      
		int portno = -1;		
		if(addr) {
			DEBUG_INFO("using provided address");
			// This address pointer may come from a different memory space and might be de-allocated, so we keep a copy
			memcpy(&proxyServerAddress, addr, sizeof(struct sockaddr_storage)); 
			struct sockaddr_in *in4 = (struct sockaddr_in *)&addr;
			proxyListenPhySocket = _phy.tcpListen((const struct sockaddr*)&in4,(void *)this);
			sockstate = SOCKS_OPEN;
			DEBUG_INFO("SOCKS5 proxy server address for <%.16lx> is: <%s> (sock=%p)", nwid, inet_ntoa(in4->sin_addr), /*ntohs(in4->sin_port), */(void*)&proxyListenPhySocket);
			return 0;
		}
		else {
			DEBUG_INFO("no address provided. Checking port file.");
			// Look for a port file for this network's proxy server instance
			char portFile[4096];
			Utils::snprintf(portFile,sizeof(portFile),"%s/networks.d/%.16llx.port",homepath,nwid);
			std::string portStr;
			DEBUG_INFO("reading port from: %s\n", portFile);
			if(ZeroTier::OSUtils::fileExists(portFile,true))
			{
				if(ZeroTier::OSUtils::readFile(portFile, portStr)) {
					portno = atoi(portStr.c_str());
				}
			}
			else {
				unsigned int randp = 0;
				Utils::getSecureRandom(&randp,sizeof(randp));
				portno = 1000 + (randp % 1000);
				DEBUG_INFO("no port specified in networks.d/%.16lx.port, randomly picking port", nwid);
				std::stringstream ss;
				ss << portno;
				portStr = ss.str();
				if(!ZeroTier::OSUtils::writeFile(portFile, portStr)) {
					DEBUG_ERROR("unable to write proxy port file: %s", portFile);
				}  
			}
			struct sockaddr_in in4;
			memset(&in4,0,sizeof(in4));
			in4.sin_family = AF_INET;
			in4.sin_addr.s_addr = Utils::hton((uint32_t)0x00000000); // right now we just listen for TCP @0.0.0.0
			in4.sin_port = Utils::hton((uint16_t)portno);
			proxyListenPhySocket = _phy.tcpListen((const struct sockaddr*)&in4,(void *)this);
			sockstate = SOCKS_OPEN;
			//DEBUG_INFO("SOCKS5 proxy server address for <%.16llx> is: <%s:%d> (sock=%p)\n", nwid, , portno, (void*)&proxyListenPhySocket);
		}
		return 0;
	}
    
    void ExtractAddress(int addr_type, unsigned char *buf, struct sockaddr_in * addr)
    {
    	// TODO: Generalize extraction logic
        if(addr_type == 144)
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
		DEBUG_INFO("sock=%p, len=%lu", (void*)&sock, len);
		unsigned char *buf;
		buf = (unsigned char *)data;
        
		// Get connection for this PhySocket
		Connection *conn = getConnection(sock);
		if(!conn) {
			DEBUG_INFO("unable to locate Connection for sock=%p", (void*)&sock);
			return;
		}

		// Write data to lwIP PCB (outgoing)
        if(conn->proxy_conn_state == SOCKS_COMPLETE)
        {
        	if(len) {
	            DEBUG_INFO("len=%lu\n", len);
	            memcpy((&conn->txbuf)+(conn->txsz), buf, len);
	   			conn->txsz += len;
	   			handleWrite(conn);
   			}
        }

		if(conn->proxy_conn_state==SOCKS_UDP)
		{
			DEBUG_INFO("SOCKS_UDP from client\n");
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
				DEBUG_INFO(" INFO <ver=%d, meth_len=%d, supp_meth=%d>", version, methodsLength, supportedMethod);

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

				DEBUG_INFO("SOCKS REQUEST = <ver=%d, cmd=%d, typ=%d>", version, cmd, addr_type);

				// CONNECT request
				if(cmd == 1) {
					DEBUG_INFO("CONNECT request");
					// Ipv4
					/*
					if(addr_type == 144)
					{
						//DEBUG_INFO("IPv4\n");
						int raw_addr;
						memcpy(&raw_addr, &buf[4], 4);
						char newaddr[16];
						inet_ntop(AF_INET, &raw_addr, (char*)newaddr, INET_ADDRSTRLEN);
						DEBUG_INFO("new addr = %s\n", newaddr);

						int rawport, port;
						memcpy(&rawport, &buf[5], 2); 
						port = Utils::ntoh(rawport);
						DEBUG_INFO("new port = %d\n", port);

						// Assemble new address
						struct sockaddr_in addr;
						addr.sin_addr.s_addr = IPADDR_ANY;
						addr.sin_family = AF_INET;
						addr.sin_port = Utils::hton(8080);

						int fd = socket(AF_INET, SOCK_STREAM, 0);
						DEBUG_INFO("fd = %d\n", fd);

						if(fd < 0)
							perror("socket");

						int err = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
						DEBUG_INFO("connect_err = %d\n", err);
						if(err < 0)
							perror("connect");
					}
					*/
					// Fully-qualified domain name
					if(addr_type == 144)
					{
                        int domain_len = buf[IDX_DST_ADDR]; // (L):D
                        struct sockaddr_in addr;
                        ExtractAddress(addr_type,buf,&addr);
						PhySocket * new_sock = handleSocketProxy(sock, SOCK_STREAM);
                        if(!new_sock)
							DEBUG_ERROR("error while creating proxied-socket");
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

                        DEBUG_INFO("REPLY = %d", addr.sin_port);
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
					DEBUG_INFO("BIND request");
					//char raw_addr[15];
					//int bind_port;
				}

				// UDP ASSOCIATION Request
				if(cmd == 3)
				{
					// PORT supplied should be port assigned by server in previous msg
					DEBUG_INFO("UDP association request");

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
                    ssize_t del = addrstr.find(":");
                    ip = addrstr.substr(0, del);
                    port = addrstr.substr(del+1, domain_len);
                    
                    // Create new lwIP PCB
                    PhySocket * new_sock = handleSocketProxy(sock, SOCK_DGRAM);
                    
                    DEBUG_INFO("sock = %p", (void*)&sock);
                    DEBUG_INFO("new_sock = %p", (void*)&new_sock);
                    if(!new_sock)
                        DEBUG_ERROR("error while creating proxied-socket");
                    
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
		DEBUG_INFO("sock=%p", (void*)&sockN);
		Connection *newConn = new Connection();
		newConn->sock = sockN;
		_phy.setNotifyWritable(sockN, false);
		_Connections.push_back(newConn);
	}

	void NetconEthernetTap::phyOnTcpConnect(PhySocket *sock,void **uptr,bool success)
	{
		DEBUG_INFO("sock=%p", (void*)&sock);
	}

	// Unused -- no UDP or TCP from this thread/Phy<>
	void NetconEthernetTap::phyOnDatagram(PhySocket *sock,void **uptr,const struct sockaddr *local_address, const struct sockaddr *from,void *data,unsigned long len)
	{
		DEBUG_INFO("len = %lu", len);
		if(len) {
            Connection *conn = getConnection(sock);
            if(!conn){
                DEBUG_ERROR("unable to locate Connection: sock=%p", (void*)sock);
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
		DEBUG_INFO("sock=%p", (void*)&sock);
		Mutex::Lock _l(_tcpconns_m);
		closeConnection(sock);
	}

	void NetconEthernetTap::phyOnTcpWritable(PhySocket *sock,void **uptr, bool lwip_invoked) 
	{
		DEBUG_INFO("sock=%p", (void*)&sock);
		handleRead(sock,uptr,true);
	}

	// RX data on stream socks and send back over client sock's underlying fd
	void NetconEthernetTap::phyOnFileDescriptorActivity(PhySocket *sock,void **uptr,bool readable,bool writable)
	{
		DEBUG_INFO("sock=%p", (void*&)sock);
	}
}