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

#if defined(__linux__) || defined(__APPLE__)
#include <netdb.h>
#include <unistd.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fcntl.h>
#include <vector>
#include <algorithm>
#include <map>

#include "libzt.h"

#include "RingBuffer.h"
#include "ztproxy.hpp"

#if defined(_WIN32)
#include <time.h>
void sleep(unsigned long ms)
{
	Sleep(ms);
}
#endif

namespace ZeroTier {

	typedef void PhySocket;

	ZTProxy::ZTProxy(int proxy_listen_port, std::string nwid, std::string path, std::string internal_addr, 
		int internal_port, std::string dns_nameserver) 
		:
			_enabled(true),
			_run(true),
			_proxy_listen_port(proxy_listen_port),
			_internal_port(internal_port),
			_nwid(nwid),
			_internal_addr(internal_addr),
			_dns_nameserver(dns_nameserver),
			_phy(this,false,true)
	{
		// Set up TCP listen sockets
		// IPv4
		struct sockaddr_in in4;
		memset(&in4,0,sizeof(in4));
		in4.sin_family = AF_INET;
		in4.sin_addr.s_addr = Utils::hton((uint32_t)(0x7f000001)); // listen for TCP @127.0.0.1
		in4.sin_port = Utils::hton((uint16_t)proxy_listen_port);
		_tcpListenSocket = _phy.tcpListen((const struct sockaddr *)&in4,this);
		if (!_tcpListenSocket) {
			DEBUG_ERROR("Error binding on port %d for IPv4 HTTP listen socket", proxy_listen_port);
		}
		// IPv6
		/*
		struct sockaddr_in6 in6;
		memset((void *)&in6,0,sizeof(in6));
		in6.sin6_family = AF_INET6;
		in6.sin6_port = in4.sin_port;
		in6.sin6_addr.s6_addr[15] = 1; // IPv6 localhost == ::1
		in6.sin6_port = Utils::hton((uint16_t)proxy_listen_port);
		_tcpListenSocket6 = _phy.tcpListen((const struct sockaddr *)&in6,this);
		*/
		/*
		if (!_tcpListenSocket6) {
			DEBUG_ERROR("Error binding on port %d for IPv6 HTTP listen socket", proxy_listen_port);
		}
		*/
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
		// Add DNS nameserver
		if (_dns_nameserver.length() > 0) {
			DEBUG_INFO("setting DNS nameserver (%s)", _dns_nameserver.c_str());
			struct sockaddr_in dns_address;
			dns_address.sin_addr.s_addr = inet_addr(_dns_nameserver.c_str());
			zts_add_dns_nameserver((struct sockaddr*)&dns_address);
		}

		TcpConnection *conn = NULL;
		uint32_t msecs = 1;
		struct timeval tv;
		tv.tv_sec = msecs / 1000;
		tv.tv_usec = (msecs % 1000) * 1000;
		int ret = 0;
  		// Main I/O loop
  		// Moves data between client application socket and libzt VirtualSocket 
		while(_run) {

			_phy.poll(1);

			conn_m.lock();
			// build fd_sets to select upon
			FD_ZERO(&read_set);
  			FD_ZERO(&write_set);
			nfds = 0;
  			for (size_t i=0; i<clist.size(); i++) {
  				FD_SET(clist[i]->zfd, &read_set);
  				FD_SET(clist[i]->zfd, &write_set);
  				nfds = clist[i]->zfd > nfds ? clist[i]->zfd : nfds;
  			}

  			ret = zts_select(nfds + 1, &read_set, &write_set, NULL, &tv);
			if (ret > 0) {
				for (int fd_i=0; fd_i<nfds+1; fd_i++) { // I/O needs to be handled on at least one fd

					// RX, Handle data incoming from libzt
					if (FD_ISSET(fd_i, &read_set)) {
						int wr = 0, rd = 0;
						conn = zmap[fd_i];
						if (conn == NULL) {
							DEBUG_ERROR("invalid conn");
							exit(0);
						}
						// read data from libzt and place it on ring buffer
						conn->rx_m.lock();
						if (conn->RXbuf->count() > 0) {
							//DEBUG_INFO("libzt has incoming data on fd=%d, RXing via conn=%p, sock=%p", 
							//	conn->zfd, conn, conn->client_sock);
						}
						if ((rd = zts_read(conn->zfd, conn->RXbuf->get_buf(),ZT_MAX_MTU)) < 0) {
							DEBUG_ERROR("error while reading data from libzt, err=%d", rd);
						}
						else {
							//DEBUG_INFO("LIBZT -> RXBUFFER = %d bytes", rd);
							conn->RXbuf->produce(rd);
						}
						// attempt to write data to client from buffer
						if ((wr = _phy.streamSend(conn->client_sock, conn->RXbuf->get_buf(), conn->RXbuf->count())) < 0) {
							DEBUG_ERROR("error while writing the data from the RXbuf to the client PhySocket, err=%d", wr);
						}
						else {
							//DEBUG_INFO("RXBUFFER -> CLIENT = %d bytes", wr);
							conn->RXbuf->consume(wr);
						}
						conn->rx_m.unlock();
					}

					// TX, Handle data outgoing from client to libzt
					if (FD_ISSET(fd_i, &write_set)) {
						int wr = 0;
						conn = zmap[fd_i];
						if (conn == NULL) {
							DEBUG_ERROR("invalid conn, possibly closed before transmit was possible");
						}
						// read data from client and place it on ring buffer
						conn->tx_m.lock();
						if (conn->TXbuf->count() > 0) {
							// DEBUG_INFO("client has outgoing data of len=%d on fd=%d, TXing via conn=%p, sock=%p", 
							//	conn->TXbuf->count(), conn->zfd, conn, conn->client_sock);
							if ((wr = zts_write(conn->zfd, conn->TXbuf->get_buf(), conn->TXbuf->count())) < 0) {
								DEBUG_ERROR("error while sending the data over libzt, err=%d", wr);
							}
							else {
								//DEBUG_INFO("TXBUFFER -> LIBZT = %d bytes", wr);
								conn->TXbuf->consume(wr); // data is presumed sent, mark it as such in the ringbuffer
							}
						}
						conn->tx_m.unlock();
					}
				}
			}
			conn_m.unlock();
		}
	}

	bool isValidIPAddress(const char *ip)
	{
		struct sockaddr_in sa;
		return inet_pton(AF_INET, ip, &(sa.sin_addr)) == 1;
	}

	void ZTProxy::phyOnTcpData(PhySocket *sock, void **uptr, void *data, unsigned long len)
	{
		int wr = 0, zfd = -1, err = 0;
		DEBUG_EXTRA("sock=%p, len=%lu", sock, len);
		std::string host = _internal_addr;
		TcpConnection *conn = cmap[sock];
		if (conn == NULL) {
			DEBUG_ERROR("invalid conn");
			exit(0);
		}
		if (conn->zfd < 0) { // no connection yet
			if (host == "") {
				DEBUG_ERROR("invalid hostname or address (empty)");
				return;
			}
			DEBUG_INFO("establishing proxy connection...");
		
			uint16_t dest_port, ipv;
			dest_port = _internal_port;
			host = _internal_addr;

			if (isValidIPAddress(host.c_str()) == false) {
				// try to resolve this if it isn't an IP address
				struct hostent *h = zts_gethostbyname(host.c_str());
				if (h == NULL) {
					DEBUG_ERROR("unable to resolve hostname (%s) (errno=%d)", host.c_str(), errno);
					return;
				}
				// TODO
				// host = h->h_addr_list[0];
			}

			// determine address type
			ipv = host.find(":") != std::string::npos ? 6 : 4;

			// connect to remote host
			if (ipv == 4) {
				DEBUG_INFO("attempting to proxy [0.0.0.0:%d -> %s:%d]", _proxy_listen_port, host.c_str(), dest_port);
				struct sockaddr_in in4;
				memset(&in4,0,sizeof(in4));
				in4.sin_family = AF_INET;
				in4.sin_addr.s_addr = inet_addr(host.c_str());
				in4.sin_port = Utils::hton(dest_port);				
				if ((zfd = zts_socket(AF_INET, SOCK_STREAM, 0)) < 0) {
					DEBUG_ERROR("unable to create socket (errno=%d)", errno);
					return;
				}
				if ((err = zts_connect(zfd, (const struct sockaddr *)&in4, sizeof(in4))) < 0) {
					DEBUG_ERROR("unable to connect to remote host (errno=%d)", errno);
					return;
				}		
			}
			if (ipv == 6) {
				//DEBUG_INFO("attempting to proxy [0.0.0.0:%d -> %s:%d]", _proxy_listen_port, host.c_str(), dest_port);
				/*
				struct sockaddr_in6 in6;
				memset(&in6,0,sizeof(in6));
				in6.sin6_family = AF_INET;
				struct hostent *server;
				server = gethostbyname2((char*)host.c_str(),AF_INET6);
				memmove((char *) &in6.sin6_addr.s6_addr, (char *) server->h_addr, server->h_length);
				in6.sin6_port = Utils::hton(dest_port);			
				zfd = zts_socket(AF_INET, SOCK_STREAM, 0);
				err = zts_connect(zfd, (const struct sockaddr *)&in6, sizeof(in6));
				*/
			}
			if (zfd < 0 || err < 0) {
				// now release TX buffer contents we previously saved, since we can't connect
				DEBUG_ERROR("error while connecting to remote host (zfd=%d, err=%d)", zfd, err);
				conn->tx_m.lock();
				conn->TXbuf->reset();
				conn->tx_m.unlock();
				return;
			}		
			else {
				DEBUG_INFO("successfully connected to remote host");
			}	
			
			conn_m.lock();
			// on success, add connection entry to map, set physock for later
			clist.push_back(conn);
			conn->zfd = zfd;
			conn->client_sock = sock;
			cmap[conn->client_sock] = conn;	
			zmap[zfd] = conn;
			conn_m.unlock();			
		}
		// Write data coming from client TCP connection to its TX buffer, later emptied into libzt by threadMain I/O loop
		conn->tx_m.lock();
		if ((wr = conn->TXbuf->write((const char *)data, len)) < 0) {
			DEBUG_ERROR("there was an error while writing data from client to tx buffer, err=%d", wr);
		}
		else {
			// DEBUG_INFO("CLIENT -> TXBUFFER = %d bytes", wr);
		}
		conn->tx_m.unlock();
	}

	void ZTProxy::phyOnTcpAccept(PhySocket *sockL, PhySocket *sockN, void **uptrL, void **uptrN, 
		const struct sockaddr *from)
	{
		DEBUG_INFO("sockL=%p, sockN=%p", sockL, sockN);
		TcpConnection *conn = new TcpConnection();
		conn->client_sock = sockN;
		cmap[sockN]=conn;
	}

	void ZTProxy::phyOnTcpClose(PhySocket *sock, void **uptr) 
	{		
		DEBUG_INFO("sock=%p", sock);
		conn_m.lock();
		TcpConnection *conn = cmap[sock];
		if (conn) {
			conn->client_sock=NULL;
			if (conn->zfd >= 0) {
				zts_close(conn->zfd);
			}
			cmap.erase(sock);
			for (size_t i=0; i<clist.size(); i++) {
				if (conn == clist[i]) {
					clist.erase(clist.begin()+i);
					break;
				}
			}
			zmap[conn->zfd] = NULL;
			delete conn;
			conn = NULL;
		}
#if defined(_WIN32)
		closesocket(_phy.getDescriptor(sock));
#else
		close(_phy.getDescriptor(sock));
#endif
		conn_m.unlock();
	}

	void ZTProxy::phyOnDatagram(PhySocket *sock, void **uptr, const struct sockaddr *localAddr, 
		const struct sockaddr *from, void *data, unsigned long len) {
		DEBUG_INFO();
	}
	void ZTProxy::phyOnTcpWritable(PhySocket *sock, void **uptr) {
		DEBUG_INFO();
	}
	void ZTProxy::phyOnFileDescriptorActivity(PhySocket *sock, void **uptr, bool readable, bool writable) {
		DEBUG_INFO("sock=%p", sock);
	}
	void ZTProxy::phyOnTcpConnect(PhySocket *sock, void **uptr, bool success) {
		DEBUG_INFO("sock=%p", sock);
	}
	void ZTProxy::phyOnUnixClose(PhySocket *sock, void **uptr)  {
		DEBUG_INFO("sock=%p", sock);
	}
	void ZTProxy::phyOnUnixData(PhySocket *sock,void **uptr,void *data,ssize_t len) {
		DEBUG_INFO("sock=%p, len=%lu", sock, len);
	}
	void ZTProxy::phyOnUnixWritable(PhySocket *sock, void **uptr, bool lwip_invoked) {
		DEBUG_INFO("sock=%p", sock);
	}
}

int main(int argc, char **argv)
{
	if (argc < 6 || argc > 7) {
		printf("\nZeroTier TCP Proxy Service\n");
		printf("ztproxy [config_file_path] [local_listen_port] [nwid] [zt_host_addr] [zt_resource_port] [optional_dns_nameserver]\n");
		exit(0);
	}
	std::string path          = argv[1];
	int proxy_listen_port     = atoi(argv[2]);
	std::string nwidstr       = argv[3];
	std::string internal_addr = argv[4];
	int internal_port         = atoi(argv[5]);
	std::string dns_nameserver= "";//argv[6];

	// Start ZeroTier Node
	// Join Network which contains resources we need to proxy
	DEBUG_INFO("waiting for libzt to come online");
	uint64_t nwid = strtoull(nwidstr.c_str(),NULL,16);
	zts_startjoin(path.c_str(), nwid);

	ZeroTier::ZTProxy *proxy = new ZeroTier::ZTProxy(proxy_listen_port, nwidstr, path, internal_addr, internal_port, dns_nameserver);
	
	if (proxy) {
		printf("\nZTProxy started. Listening on %d\n", proxy_listen_port);
		printf("Traffic will be proxied to and from %s:%d on network %s\n", internal_addr.c_str(), internal_port, nwidstr.c_str());
		printf("Proxy Node config files and key stored in: %s/\n\n", path.c_str());
		while(1) {
			sleep(1);
		}
	}
	else {
		printf("unable to create proxy\n");
	}
	return 0;
}
