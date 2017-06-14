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

#include "ztproxy.hpp"
#include "ZeroTierSDK.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fcntl.h>
#include <errno.h>

#include <queue>
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <map>

namespace ZeroTier {

	typedef void PhySocket;

	ZTProxy::ZTProxy(int proxy_listen_port, std::string nwid, std::string path, std::string internal_addr, int internal_port) 
		:
			_phy(this,false,true),
			_enabled(true),
			_run(true),
			_proxy_listen_port(proxy_listen_port),
			_internal_port(internal_port),
			_nwid(nwid),
			_internal_addr(internal_addr)
	{
		// Start ZeroTier Node
		// Join Network which contains resources we need to proxy
		printf("waiting for libzt to come online\n");
		zts_simple_start(path.c_str(), nwid.c_str());

		// Set up TCP listen sockets

		// IPv4
		struct sockaddr_in in4;
		memset(&in4,0,sizeof(in4));
		in4.sin_family = AF_INET;
		in4.sin_addr.s_addr = Utils::hton((uint32_t)(0x7f000001)); // right now we just listen for TCP @127.0.0.1
		in4.sin_port = Utils::hton((uint16_t)proxy_listen_port);
		_tcpListenSocket = _phy.tcpListen((const struct sockaddr *)&in4,this);

		// IPv6
		struct sockaddr_in6 in6;
		memset((void *)&in6,0,sizeof(in6));
		in6.sin6_family = AF_INET6;
		in6.sin6_port = in4.sin_port;
		in6.sin6_addr.s6_addr[15] = 1; // IPv6 localhost == ::1
		in6.sin6_port = Utils::hton((uint16_t)proxy_listen_port);
		_tcpListenSocket6 = _phy.tcpListen((const struct sockaddr *)&in6,this);

		if(!_tcpListenSocket)
			printf("Error binding on port %d for IPv4 HTTP listen socket\n", proxy_listen_port);
		if(!_tcpListenSocket6)
			printf("Error binding on port %d for IPv6 HTTP listen socket\n", proxy_listen_port);

		_thread = Thread::start(this);
	} 

	ZTProxy::~ZTProxy()
	{
		_run = false;
		_phy.whack();
		Thread::join(_thread);
		_phy.close(_tcpListenSocket,false);
		_phy.close(_tcpListenSocket6,false);
	}

	void ZTProxy::threadMain()
		throw()
	{
		while(_run) {
			_phy.poll(10); // in ms
		}
	}

	void ZTProxy::phyOnTcpData(PhySocket *sock,void **uptr,void *data,unsigned long len)
	{
		printf("phyOnTcpData(sock=%p, len=%lu)\n", sock, len);
		//printf("cq=%d, cmap=%d, dmap=%d\n", cqueue.size(), cmap.size(), dmap.size());
		unsigned char *buf = (unsigned char*)data;
		std::string host = _internal_addr;

		// Get the TcpConnection object 
		TcpConnection *conn = cmap[sock];
		if(conn == NULL) {
			conn = cmap[dmap[sock]];
			if(conn == NULL) {	
			printf("no connection object\n");	
				return; // Nothing
			}
		}

		if(!conn->destination_sock) { // no connection yet
			printf("!conn->destination_sock\n");	
			// If HOST was parsed correctly, establish remote connection
			if(host != "")
			{
				uint16_t dest_port, ipv;x
				dest_port = _internal_port;

				// Save buffer to TcpConnection's write buffer, we'll forward 
				// this data along only after the phyOnTcpConnect callback is called successfully
				// Got data for connection but it hasn't been fully established, save to buffer for later writing
				conn->tcp_client_m.lock();
				memcpy(conn->client_buf, buf, len);
				conn->client_buf_len = len;
				conn->tcp_client_m.unlock();

				host = _internal_addr;

				// check for address type
				if(host.find(":") != std::string::npos)
					ipv = 6;
				else
					ipv = 4;

				bool connected;
				if(ipv == 4)
				{
					printf("ipv4, %s -> %s:%d\n", host.c_str(), host.c_str(), dest_port);
					struct sockaddr_in in4;
					memset(&in4,0,sizeof(in4));
					in4.sin_family = AF_INET;
					in4.sin_addr.s_addr = inet_addr(host.c_str());
					in4.sin_port = Utils::hton(dest_port);
					
					// conn->destination_sock = _phy.tcpConnect((const struct sockaddr *)&in4, connected, this);
					// 
				
					int sockfd = zts_socket(AF_INET, SOCK_STREAM, 0);
					if(zts_connect(sockfd, (const struct sockaddr *)&in4, sizeof(in4)) < 0) {
						printf("error while connecting to remote host\n");
					}
					else {
						conn->destination_sock = _phy.wrapSocket(sockfd);

						conn->tcp_client_m.lock();
						int n = 0, tot = conn->client_buf_len;
						while(tot > 0) {
							if((n = _phy.streamSend(conn->destination_sock, conn->client_buf, conn->client_buf_len)) > 0) {
								tot -= n;
								if(n < conn->client_buf_len) { // If we couldn't write the entire buffer
									memmove(conn->client_buf, conn->client_buf+n, BUF_SZ-n);
									// printf("n = %d, memmove(%d)\n", n, BUF_SZ-n);
									conn->client_buf_len-=n;
								}
								else {
									conn->client_buf_len = 0;
								}
							}
							else
								printf(" an error occured while writing to the destination_sock\n");
						}
						conn->tcp_client_m.unlock();

					}
				}
				if(ipv == 6)
				{
					printf("ipv6, %s -> [%s]:%d\n", host.c_str(), host.c_str(), dest_port);
					struct sockaddr_in6 in6;
					memset(&in6,0,sizeof(in6));
					in6.sin6_family = AF_INET;
    				struct hostent *server;
    				server = gethostbyname2((char*)host.c_str(),AF_INET6);
    				memmove((char *) &in6.sin6_addr.s6_addr, (char *) server->h_addr, server->h_length);
					in6.sin6_port = Utils::hton(dest_port);
					conn->destination_sock = _phy.tcpConnect((const struct sockaddr *)&in6, connected, this);
				}
				dmap[conn->destination_sock] = conn->origin_sock; // for reverse lookup from callbacks
				if(!conn->destination_sock) {
					printf(" there was an error connecting to the remote host\n");
					return;
				}
			}
		}
	}

	void ZTProxy::phyOnDatagram(PhySocket *sock,void **uptr,const struct sockaddr *localAddr,const struct sockaddr *from,void *data,unsigned long len)
	{
		printf("phyOnDatagram\n");

	}

	void ZTProxy::phyOnTcpWritable(PhySocket *sock,void **uptr)
	{
		printf("phyOnTcpWritable\n");

		TcpConnection *conn = cmap[sock];
	 	bool to_client = true;
		if(conn == NULL) {
			conn = cmap[dmap[sock]];
			if(conn == NULL) {		
				printf("phyOnTcpWritable(%p): no connection found\n", sock);
				return; // Nothing
			}
			to_client = false;
		}
		// To Client
		if(to_client) { 
			conn->tcp_client_m.lock();
			int n = 0, tot = conn->client_buf_len;
			while(tot > 0) {
				if((n = _phy.streamSend(sock, conn->client_buf, conn->client_buf_len)) > 0) {
					tot -= n;
					if(n < conn->client_buf_len) { // If we couldn't write the entire buffer
						memmove(conn->client_buf, conn->client_buf+n, BUF_SZ-n);
						// printf("n = %d, memmove(%d)\n", n, BUF_SZ-n);
						conn->client_buf_len-=n;
					}
					else {
						conn->client_buf_len = 0;
					}
				}		
			}
			if(!tot)
				_phy.setNotifyWritable(sock, false);
			conn->tcp_client_m.unlock();
		}
		// To Server
		else {
			conn->tcp_server_m.lock();
			int n = 0, tot = conn->server_buf_len;
			while(tot > 0) {
				if((n = _phy.streamSend(sock, conn->server_buf, conn->server_buf_len)) > 0) {
					tot -= n;
					if(n < conn->server_buf_len) { // If we couldn't write the entire buffer
						memmove(conn->server_buf, conn->server_buf+n, BUF_SZ-n);
						// printf("n = %d, memmove(%d)\n", n, BUF_SZ-n);
						conn->server_buf_len-=n;
					}
					else {
						conn->server_buf_len = 0;
					}
				}		
			}
			if(!tot)
				_phy.setNotifyWritable(sock, false);
			conn->tcp_server_m.unlock();		
		}
	}
	void ZTProxy::phyOnFileDescriptorActivity(PhySocket *sock,void **uptr,bool readable,bool writable)
	{
		printf("phyOnFileDescriptorActivity\n");
	}

	void ZTProxy::phyOnTcpConnect(PhySocket *sock,void **uptr,bool success)
	{
		// Not used, connections are handled via user space network stack and SocketTap subsystem
	}

	void ZTProxy::phyOnTcpAccept(PhySocket *sockL,PhySocket *sockN,void **uptrL,void **uptrN,const struct sockaddr *from)
	{
		printf("phyOnTcpAccept\n");
		TcpConnection *conn;
		// try to recycle TcpConnection objects instead of allocating new ones
		if(cqueue.size()) {
			conn = cqueue.front();
			cqueue.pop();
		}
		else {
			conn = new TcpConnection();
		}
		conn->origin_sock = sockN;
		cmap[sockN]=conn; // add new connection
	}

	void ZTProxy::phyOnUnixClose(PhySocket *sock,void **uptr) 
	{
		printf("phyOnUnixClose\n");

	}
	void ZTProxy::phyOnUnixData(PhySocket *sock,void **uptr,void *data,ssize_t len)
	{
		printf("phyOnUnixData(sock=%p, len=%lu)\n", sock, len);
		unsigned char *buf = (unsigned char*)data;
		
		// Get the TcpConnection object 
		TcpConnection *conn = cmap[sock];
		if(conn == NULL) {
			conn = cmap[dmap[sock]];
			if(conn == NULL) {	
				printf("no connection object\n");	
				return; // Nothing
			}
		}

		if(!conn->destination_sock) { // no connection yet
			printf("!conn->destination_sock\n");	
		}
		else // If connection to host already established, just forward the data in the correct direction
		{
			int n = 0;
			if(sock == conn->destination_sock) { // RX
				conn->tcp_client_m.lock();
				if(!conn->client_buf_len) // If nothing is buffered, attempt to write, otherwise copy to buffer to preserver order
					n = _phy.streamSend(conn->origin_sock, buf, len);				
				if(n < len) {
					memcpy(conn->client_buf+conn->client_buf_len, buf+n, len-n);
					conn->client_buf_len += len-n;
					_phy.setNotifyWritable(conn->origin_sock, true);
				}
				conn->tcp_client_m.unlock();
			}
			if(sock == conn->origin_sock) { // TX
				conn->tcp_server_m.lock();
				if(!conn->server_buf_len)
					n = _phy.streamSend(conn->destination_sock, buf, len);
				if(n < len) {
					memcpy(conn->server_buf+conn->server_buf_len, buf+n, len-n);
					conn->server_buf_len += len-n;
					_phy.setNotifyWritable(conn->destination_sock, true);
				}
				conn->tcp_server_m.unlock();
			}
		}

	}
	void ZTProxy::phyOnUnixWritable(PhySocket *sock,void **uptr,bool lwip_invoked)
	{
		printf("phyOnUnixWritable\n");
	}

	void ZTProxy::phyOnTcpClose(PhySocket *sock,void **uptr) 
	{		
		printf("phyOnTcpClose\n");

		TcpConnection *conn = cmap[sock];
		if(conn)
		{
			conn->origin_sock=NULL;
			conn->destination_sock=NULL;
			conn->client_buf_len=0;
			cqueue.push(conn);
		}
		cmap.erase(sock);
		dmap.erase(sock);
		close(_phy.getDescriptor(sock));
	}
}

int main(int argc, char **argv)
{
	if(argc != 6) {
		printf("\nZeroTier TCP Proxy Service\n");
		printf("ztproxy [local port] [network ID]/[ZT internal IP]/port\n");
		exit(0);
	}

	std::string path          = argv[1];
	int proxy_listen_port     = atoi(argv[2]);
	std::string nwid          = argv[3];
	std::string internal_addr = argv[4];
	int internal_port         = atoi(argv[5]);

	ZeroTier::ZTProxy *proxy = new ZeroTier::ZTProxy(proxy_listen_port, nwid, path, internal_addr, internal_port);
	
	if(proxy) {
		printf("ZTProxy started. Listening on %d\n", proxy_listen_port);
		printf("Traffic will be proxied to and from %s:%d on network %s\n", internal_addr.c_str(), internal_port, nwid.c_str());
		printf("Proxy Node config files and key stored in: %s/\n", path.c_str());
		while(1) {
			sleep(1);
		}
	}
	else {
		printf("unable to create proxy\n");
	}
	return 0;
}
//#endif