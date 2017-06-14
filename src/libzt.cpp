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

#include <dlfcn.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pthread.h>

// stack
#include "pico_stack.h"

// ZT
#include "OneService.hpp"
#include "Utils.hpp"
#include "OSUtils.hpp"
#include "InetAddress.hpp"
#include "ZeroTierOne.h"

// SDK
#include "SocketTap.hpp"
#include "libzt.h"

#ifdef __cplusplus
extern "C" {
#endif

static ZeroTier::OneService *zt1Service;

namespace ZeroTier {
    std::string homeDir; // The resultant platform-specific dir we *must* use internally
    std::string netDir;  // Where network .conf files are to be written

    /*
     * Global reference to stack
     */
    picoTCP *picostack = NULL;

    /*
     * "sockets" that have been created but not bound to a SocketTap interface yet
     */
    std::map<int, Connection*> unmap;

    /*
     * For fast lookup of Connections and SocketTaps via given file descriptor
     */
    std::map<int, std::pair<Connection*,SocketTap*>*> fdmap;

    ZeroTier::Mutex _multiplexer_lock;
    ZeroTier::Mutex _accepted_connection_lock;
}

/****************************************************************************/
/* SDK Socket API - Language Bindings are written in terms of these         */
/****************************************************************************/

void zts_start(const char *path)
{
    if(zt1Service)
        return;
    if(ZeroTier::picostack)
        return;
    
    ZeroTier::picostack = new ZeroTier::picoTCP();
    pico_stack_init();

    DEBUG_INFO("path=%s", path);
    if(path)
        ZeroTier::homeDir = path;
    pthread_t service_thread;
    pthread_create(&service_thread, NULL, zts_start_service, (void *)(path));
}

void zts_simple_start(const char *path, const char *nwid)
{
    zts_start(path);
    while(!zts_running())
        usleep(ZT_API_CHECK_INTERVAL * 1000);
    zts_join(nwid);
    while(!zts_has_address(nwid))
        usleep(ZT_API_CHECK_INTERVAL * 1000);
}

void zts_stop() {
    if(zt1Service) { 
        zt1Service->terminate();
        zt1Service->removeNets();
    }
}

void zts_join(const char * nwid) {
    if(zt1Service) {
        std::string confFile = zt1Service->givenHomePath() + "/networks.d/" + nwid + ".conf";
        if(!ZeroTier::OSUtils::mkdir(ZeroTier::netDir))
            DEBUG_ERROR("unable to create: %s", ZeroTier::netDir.c_str());
        if(!ZeroTier::OSUtils::writeFile(confFile.c_str(), ""))
            DEBUG_ERROR("unable to write network conf file: %s", confFile.c_str());
        zt1Service->join(nwid);
    }
}

void zts_join_soft(const char * filepath, const char * nwid) { 
    std::string net_dir = std::string(filepath) + "/networks.d/";
    std::string confFile = net_dir + std::string(nwid) + ".conf";
    if(!ZeroTier::OSUtils::mkdir(net_dir)) {
        DEBUG_ERROR("unable to create: %s", net_dir.c_str());
    }
    if(!ZeroTier::OSUtils::fileExists(confFile.c_str(),false)) {
        if(!ZeroTier::OSUtils::writeFile(confFile.c_str(), "")) {
            DEBUG_ERROR("unable to write network conf file: %s", confFile.c_str());
        }
    }
}

void zts_leave(const char * nwid) { 
    if(zt1Service)
        zt1Service->leave(nwid);
}

void zts_leave_soft(const char * filepath, const char * nwid) {
    std::string net_dir = std::string(filepath) + "/networks.d/";
    ZeroTier::OSUtils::rm((net_dir + nwid + ".conf").c_str()); 
}

void zts_get_homepath(char *homePath, int len) { 
    if(ZeroTier::homeDir.length()) {
        memset(homePath, 0, len);
        memcpy(homePath, ZeroTier::homeDir.c_str(), len < ZeroTier::homeDir.length() ? len : ZeroTier::homeDir.length());
    }
}

void zts_core_version(char *ver) {
    int major, minor, revision;
    ZT_version(&major, &minor, &revision);
    sprintf(ver, "%d.%d.%d", major, minor, revision);
}

void zts_sdk_version(char *ver) {
    sprintf(ver, "%d.%d.%d", ZT_SDK_VERSION_MAJOR, ZT_SDK_VERSION_MINOR, ZT_SDK_VERSION_REVISION);
}

int zts_get_device_id(char *devID) { 
    if(zt1Service) {
        char id[ZT_ID_LEN+1];
        sprintf(id, "%lx",zt1Service->getNode()->address());
        memcpy(devID, id, ZT_ID_LEN+1);
        return 0;
    }
    else // Service isn't online, try to read ID from file
    {
        std::string fname("identity.public");
        std::string fpath(ZeroTier::homeDir);

        if(ZeroTier::OSUtils::fileExists((fpath + ZT_PATH_SEPARATOR_S + fname).c_str(),false)) {
            std::string oldid;
            ZeroTier::OSUtils::readFile((fpath + ZT_PATH_SEPARATOR_S + fname).c_str(),oldid);
            memcpy(devID, oldid.c_str(), ZT_ID_LEN); // first 10 bytes of file
            return 0;
        }
    }
    return -1;
}

int zts_running() { 
    return !zt1Service ? false : zt1Service->isRunning(); 
}

int zts_has_ipv4_address(const char *nwid)
{
    char ipv4_addr[ZT_MAX_IPADDR_LEN];
    memset(ipv4_addr, 0, ZT_MAX_IPADDR_LEN);
    zts_get_ipv4_address(nwid, ipv4_addr, ZT_MAX_IPADDR_LEN);
    return strcmp(ipv4_addr, "\0");
}

int zts_has_ipv6_address(const char *nwid)
{
    char ipv6_addr[ZT_MAX_IPADDR_LEN];
    memset(ipv6_addr, 0, ZT_MAX_IPADDR_LEN);
    zts_get_ipv6_address(nwid, ipv6_addr, ZT_MAX_IPADDR_LEN);
    return strcmp(ipv6_addr, "\0");
}

int zts_has_address(const char *nwid)
{
    return zts_has_ipv4_address(nwid) || zts_has_ipv6_address(nwid);
}

void zts_get_ipv4_address(const char *nwid, char *addrstr, const int addrlen)
{
    if(zt1Service) {
        uint64_t nwid_int = strtoull(nwid, NULL, 16);
        ZeroTier::SocketTap *tap = zt1Service->getTap(nwid_int);
        if(tap && tap->_ips.size()){ 
            for(int i=0; i<tap->_ips.size(); i++) {
                if(tap->_ips[i].isV4()) {
                    std::string addr = tap->_ips[i].toString();
                    int len = addrlen < addr.length() ? addrlen : addr.length();
                    memset(addrstr, 0, len);
                    memcpy(addrstr, addr.c_str(), len); 
                    return;
                }
            }
        }
    }
    else
        memcpy(addrstr, "\0", 1);
}

void zts_get_ipv6_address(const char *nwid, char *addrstr, const int addrlen)
{
    if(zt1Service) {
        uint64_t nwid_int = strtoull(nwid, NULL, 16);
        ZeroTier::SocketTap *tap = zt1Service->getTap(nwid_int);
        if(tap && tap->_ips.size()){ 
            for(int i=0; i<tap->_ips.size(); i++) {
                if(tap->_ips[i].isV6()) {
                    std::string addr = tap->_ips[i].toString();
                    int len = addrlen < addr.length() ? addrlen : addr.length();
                    memset(addrstr, 0, len);
                    memcpy(addrstr, addr.c_str(), len); 
                    return;
                }
            }
        }
    }
    else
        memcpy(addrstr, "\0", 1);
}

void zts_get_6plane_addr(char *addr, const char *nwid, const char *devID)
{
    ZeroTier::InetAddress _6planeAddr = ZeroTier::InetAddress::makeIpv66plane(
        ZeroTier::Utils::hexStrToU64(nwid),ZeroTier::Utils::hexStrToU64(devID));
    memcpy(addr, _6planeAddr.toIpString().c_str(), 40);
}

void zts_get_rfc4193_addr(char *addr, const char *nwid, const char *devID)
{
    ZeroTier::InetAddress _6planeAddr = ZeroTier::InetAddress::makeIpv6rfc4193(
        ZeroTier::Utils::hexStrToU64(nwid),ZeroTier::Utils::hexStrToU64(devID));
    memcpy(addr, _6planeAddr.toIpString().c_str(), 40);
}

unsigned long zts_get_peer_count() {
    if(zt1Service)
        return zt1Service->getNode()->peers()->peerCount;
    else
        return 0;
}

int zts_get_peer_address(char *peer, const char *devID) {
    if(zt1Service) {
        ZT_PeerList *pl = zt1Service->getNode()->peers();
        // uint64_t addr;
        for(int i=0; i<pl->peerCount; i++) {
            // ZT_Peer *p = &(pl->peers[i]);
            // DEBUG_INFO("peer[%d] = %lx", i, p->address);
        }
        return pl->peerCount;
    }
    else
        return -1;
}

void zts_enable_http_control_plane()
{
    // TODO
}

void zts_disable_http_control_plane()
{
    // TODO
}

/****************************************************************************/
/* SocketTap Multiplexer Functionality                                      */
/* - This section of the API is used to implement the general socket        */
/*   controls. Basically this is designed to handle socket provisioning     */
/*   requests when no SocketTap is yet initialized, and as a way to         */
/*   determine which SocketTap is to be used for a particular connect() or  */ 
/*   bind() call. This enables multi-network support                        */
/****************************************************************************/

/*

    socket fd = 0 (nwid=X)---\                            /--- SocketTap=A // e.x. 172.27.0.0/?
                              \                          /
    socket fd = 1 (nwid=Y)--------Multiplexed z* calls-------- SocketTap=B // e.x. 192.168.0.1/16
                              /                          \
    socket fd = 2 (nwid=Z)---/                            \--- SocketTap=C // e.x. 10.9.9.0/24

*/

/*

Darwin:

    [  ] [EACCES]           Permission to create a socket of the specified type and/or protocol is denied.
    [  ] [EAFNOSUPPORT]     The specified address family is not supported.
    [--] [EMFILE]           The per-process descriptor table is full.
    [NA] [ENFILE]           The system file table is full.
    [  ] [ENOBUFS]          Insufficient buffer space is available.  The socket cannot be created until sufficient resources are freed.
    [  ] [ENOMEM]           Insufficient memory was available to fulfill the request.
    [  ] [EPROTONOSUPPORT]  The protocol type or the specified protocol is not supported within this domain.
    [  ] [EPROTOTYPE]       The socket type is not supported by the protocol.
*/

// int socket_family, int socket_type, int protocol
int zts_socket(ZT_SOCKET_SIG) {
    int err = 0;
    if(!zt1Service) {
        DEBUG_ERROR("cannot create socket, no service running. call zts_start() first.");
        errno = EMFILE; // could also be ENFILE
        err = -1;
    }
    else {
        ZeroTier::_multiplexer_lock.lock();
        //DEBUG_INFO("unmap=%d, fdmap=%d", ZeroTier::unmap.size(), ZeroTier::fdmap.size());
        //DEBUG_INFO("timers = %d, max = %d", pico_ntimers(), PICO_MAX_TIMERS);
        if(pico_ntimers() >= PICO_MAX_TIMERS) {
            DEBUG_ERROR("cannot provision additional socket due to limitation of PICO_MAX_TIMERS. current = %d", pico_ntimers());
            errno = EMFILE;
            err = -1;
        }
        else
        {
            ZeroTier::Connection *conn = new ZeroTier::Connection();
            int protocol_version = 0;
            struct pico_socket *psock;
            
            // TODO: check ifdef logic here
            //#if defined(SDK_IPV4)
            if(socket_family == AF_INET){
                // DEBUG_ERROR("AF_INET");
                protocol_version = PICO_PROTO_IPV4;
            }
            //#endif
            //#if defined(SDK_IPV6)
            if(socket_family == AF_INET6) {
                // DEBUG_ERROR("AF_INET6");
                protocol_version = PICO_PROTO_IPV6;
            }
            //#endif
            
            if(socket_type == SOCK_DGRAM) {
                psock = pico_socket_open(
                    protocol_version, PICO_PROTO_UDP, &ZeroTier::picoTCP::pico_cb_socket_activity);
            }
            if(socket_type == SOCK_STREAM) {
                psock = pico_socket_open(
                    protocol_version, PICO_PROTO_TCP, &ZeroTier::picoTCP::pico_cb_socket_activity);
            }
            // set up Unix Domain socketpair (used for data later on)
            if(psock) {     
                conn->socket_family = socket_family;
                conn->socket_type = socket_type;
                conn->picosock = psock;
                memset(conn->rxbuf, 0, ZT_UDP_RX_BUF_SZ);
                ZeroTier::unmap[conn->app_fd] = conn;
                err = conn->app_fd; // return one end of the socketpair
            }
            else {
                DEBUG_ERROR("failed to create pico_socket");
                err = -1;
            }
        }
        //DEBUG_INFO(" unmap=%d, fdmap=%d", ZeroTier::unmap.size(), ZeroTier::fdmap.size());
        ZeroTier::_multiplexer_lock.unlock();
    }
    return err;
}


/*

Darwin:

    [  ] [EACCES]           The destination address is a broadcast address and the socket option SO_BROADCAST is not set.
    [  ] [EADDRINUSE]       The address is already in use.
    [  ] [EADDRNOTAVAIL]    The specified address is not available on this machine.
    [  ] [EAFNOSUPPORT]     Addresses in the specified address family cannot be used with this socket.
    [  ] [EALREADY]         The socket is non-blocking and a previous connection attempt has not yet been completed.
    [--] [EBADF]            socket is not a valid descriptor.
    [  ] [ECONNREFUSED]     The attempt to connect was ignored (because the target is not listening for connections) or explicitly rejected.
    [  ] [EFAULT]           The address parameter specifies an area outside the process address space.
    [  ] [EHOSTUNREACH]     The target host cannot be reached (e.g., down, disconnected).
    [--] [EINPROGRESS]      The socket is non-blocking and the connection cannot be completed immediately.  It is possible to select(2) for completion by selecting the socket for writing.
    [NA] [EINTR]            Its execution was interrupted by a signal.
    [  ] [EINVAL]           An invalid argument was detected (e.g., address_len is not valid for the address family, the specified address family is invalid).
    [  ] [EISCONN]          The socket is already connected.
    [  ] [ENETDOWN]         The local network interface is not functioning.
    [--] [ENETUNREACH]      The network isn't reachable from this host.
    [  ] [ENOBUFS]          The system call was unable to allocate a needed memory buffer.
    [  ] [ENOTSOCK]         socket is not a file descriptor for a socket.
    [  ] [EOPNOTSUPP]       Because socket is listening, no connection is allowed.
    [  ] [EPROTOTYPE]       address has a different type than the socket that is bound to the specified peer address.
    [  ] [ETIMEDOUT]        Connection establishment timed out without establishing a connection.
    [  ] [ECONNRESET]       Remote host reset the connection request.
*/
int zts_connect(ZT_CONNECT_SIG) {
    // DEBUG_INFO("fd = %d", fd);
    int err = 0;
    if(fd < 0) {
        errno = EBADF;
        DEBUG_ERROR("EBADF");
        return -1;
    }
    if(!zt1Service) {
        DEBUG_ERROR("Service not started. Call zts_start(path) first");
        errno = EBADF;
        return -1;
    }
    ZeroTier::_multiplexer_lock.lock();
    ZeroTier::Connection *conn = ZeroTier::unmap[fd];
    ZeroTier::SocketTap *tap;

    if(conn) {      
        char ipstr[INET6_ADDRSTRLEN];
        memset(ipstr, 0, INET6_ADDRSTRLEN);
        ZeroTier::InetAddress iaddr;

        if(conn->socket_family == AF_INET) {
            inet_ntop(AF_INET, 
                (const void *)&((struct sockaddr_in *)addr)->sin_addr.s_addr, ipstr, INET_ADDRSTRLEN);
            iaddr.fromString(ipstr);
        }
        if(conn->socket_family == AF_INET6) {
            inet_ntop(AF_INET6, 
                (const void *)&((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr, ipstr, INET6_ADDRSTRLEN);
            // TODO: This is a hack, determine a proper way to do this
            iaddr.fromString(ipstr + std::string("/88"));
        }
        //DEBUG_INFO("ipstr= %s", ipstr);
        //DEBUG_INFO("iaddr= %s", iaddr.toString().c_str());
        tap = zt1Service->getTap(iaddr);
        if(!tap) {
            DEBUG_ERROR("no route to host");
            errno = ENETUNREACH;
            err = -1;
        }
        else {
            // pointer to tap we use in callbacks from the stack
            conn->picosock->priv = new ZeroTier::ConnectionPair(tap, conn); 
            //DEBUG_INFO("found appropriate SocketTap");
            // Semantically: tap->stack->connect
            err = tap->Connect(conn, fd, addr, addrlen); 
            if(err == 0) {
                tap->_Connections.push_back(conn); // Give this Connection to the tap we decided on
                conn->tap = tap;
            }
            // Wrap the socketpair we created earlier
            // For I/O loop participation and referencing the PhySocket's parent Connection in callbacks
            conn->sock = tap->_phy.wrapSocket(conn->sdk_fd, conn);     
            //DEBUG_INFO("wrapping conn->sdk_fd = %d", conn->sdk_fd);   
            //DEBUG_INFO("         conn->app_fd = %d", conn->app_fd);     
        }
    }
    else {
        DEBUG_ERROR("unable to locate connection");
        errno = EBADF;
        err = -1;
    }
    ZeroTier::unmap.erase(fd);
    ZeroTier::fdmap[fd] = new std::pair<ZeroTier::Connection*,ZeroTier::SocketTap*>(conn, tap);
    ZeroTier::_multiplexer_lock.unlock();

    // NOTE: pico_socket_connect() will return 0 if no error happens immediately, but that doesn't indicate
    // the connection was completed, for that we must wait for a callback from the stack. During that
    // callback we will place the Connection in a ZT_SOCK_STATE_UNHANDLED_CONNECTED state to signal 
    // to the multiplexer logic that this connection is complete and a success value can be sent to the
    // user application

    int f_err, blocking = 1;
    if ((f_err = fcntl(fd, F_GETFL, 0)) < 0) {
        DEBUG_ERROR("fcntl error, err = %s, errno = %d", f_err, errno);
        err = -1;
    } 
    else {
        blocking = !(f_err & O_NONBLOCK);
    }

    // non-blocking
    if(err == 0 && !blocking) {
        DEBUG_EXTRA("EINPROGRESS, not a real error, assuming non-blocking mode");
        errno = EINPROGRESS;
        err = -1;
    }
    else // blocking
    {
        // FIXME: Double check that accept/connect queues in multithreaded apps don't get mixed up
        if(err == 0 && blocking) {
            bool complete = false;
            while(true)
            {
                // FIXME: locking and unlocking so often might cause a performance bottleneck while outgoing connections
                // are being established (also applies to accept())
                usleep(ZT_CONNECT_RECHECK_DELAY * 1000);
                tap->_tcpconns_m.lock();
                for(int i=0; i<tap->_Connections.size(); i++)
                {
                    if(tap->_Connections[i]->state == PICO_ERR_ECONNRESET) {   
                        errno = ECONNRESET;
                        DEBUG_ERROR("ECONNRESET");
                        err = -1;
                    }
                    if(tap->_Connections[i]->state == ZT_SOCK_STATE_UNHANDLED_CONNECTED) {
                        tap->_Connections[i]->state = ZT_SOCK_STATE_CONNECTED;
                        errno = 0;
                        err = 0; // complete
                        complete = true;
                    }
                }
                tap->_tcpconns_m.unlock();
                if(complete)
                    break;
            }
        }
    }
    return err;
}

/*

Darwin:

    [--] [EBADF]            S is not a valid descriptor.
    [  ] [ENOTSOCK]         S is not a socket.
    [--] [EADDRNOTAVAIL]    The specified address is not available from the local
                            machine.
    [  ] [EADDRINUSE]       The specified address is already in use.
    [  ] [EINVAL]           The socket is already bound to an address.
    [  ] [EACCES]           The requested address is protected, and the current
                            user has inadequate permission to access it.
    [  ] [EFAULT]           The name parameter is not in a valid part of the user
                            address space.
*/
int zts_bind(ZT_BIND_SIG) {
    //DEBUG_EXTRA("fd = %d", fd);
    int err = 0;
    if(fd < 0) {
        errno = EBADF;
        err = -1;
    }
    if(!zt1Service) {
        DEBUG_ERROR("Service not started. Call zts_start(path) first");        
        errno = EBADF;
        err = -1;
    }
    ZeroTier::_multiplexer_lock.lock();
    ZeroTier::Connection *conn = ZeroTier::unmap[fd];
    ZeroTier::SocketTap *tap;
    
    if(conn) {      
        char ipstr[INET6_ADDRSTRLEN]; //, nm_str[INET6_ADDRSTRLEN];
        memset(ipstr, 0, INET6_ADDRSTRLEN);
        ZeroTier::InetAddress iaddr;

        if(conn->socket_family == AF_INET) {
            inet_ntop(AF_INET, 
                (const void *)&((struct sockaddr_in *)addr)->sin_addr.s_addr, ipstr, INET_ADDRSTRLEN);
        }
        if(conn->socket_family == AF_INET6) {
            inet_ntop(AF_INET6, 
                (const void *)&((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr, ipstr, INET6_ADDRSTRLEN);
        }
        iaddr.fromString(ipstr);
        tap = zt1Service->getTap(iaddr);

        if(!tap) {
            DEBUG_ERROR("no matching interface to bind to");            
            errno = EADDRNOTAVAIL;
            err = -1;
        }
        else {
            conn->picosock->priv = new ZeroTier::ConnectionPair(tap, conn);
            tap->_Connections.push_back(conn); // Give this Connection to the tap we decided on
            err = tap->Bind(conn, fd, addr, addrlen); // Semantically: tap->stack->connect
            conn->tap = tap;
            if(err == 0) { // success
                ZeroTier::unmap.erase(fd);
                ZeroTier::fdmap[fd] = new std::pair<ZeroTier::Connection*,ZeroTier::SocketTap*>(conn, tap);
            }
        }
    }
    else {
        DEBUG_ERROR("unable to locate connection");
        errno = EBADF;
        err = -1;
    }
    
    ZeroTier::_multiplexer_lock.unlock();
    return err;
}

/*

Darwin:

    [--] [EACCES]           The current process has insufficient privileges.
    [--] [EBADF]            The argument socket is not a valid file descriptor.
    [  ] [EDESTADDRREQ]     The socket is not bound to a local address and the protocol does not support listening on an unbound socket.
    [  ] [EINVAL]           socket is already connected.
    [  ] [ENOTSOCK]         The argument socket does not reference a socket.
    [  ] [EOPNOTSUPP]       The socket is not of a type that supports the operation listen().
*/
int zts_listen(ZT_LISTEN_SIG) {
    DEBUG_EXTRA("fd = %d", fd);
    int err = 0;
    if(fd < 0) {
        errno = EBADF;
        err = -1;
    }
    if(!zt1Service) {
        DEBUG_ERROR("service not started. call zts_start(path) first");
        errno = EACCES;
        return -1;
    }
    ZeroTier::_multiplexer_lock.lock();
    std::pair<ZeroTier::Connection*, ZeroTier::SocketTap*> *p = ZeroTier::fdmap[fd];
    if(!p) {
        DEBUG_ERROR("unable to locate connection pair. did you bind?");
        return -1;
    }
    ZeroTier::Connection *conn = p->first;
    ZeroTier::SocketTap *tap = p->second;
    
    if(!tap || !conn) {
        DEBUG_ERROR("unable to locate tap interface for file descriptor");
        return -1;
    }
    err = tap->Listen(conn, fd, backlog);
    //DEBUG_INFO("put conn=%p into LISTENING state (err=%d)", conn, err);
    ZeroTier::_multiplexer_lock.unlock();
    return err;
}

/*

Darwin:

    [--] [EBADF]            The descriptor is invalid.
    [  ] [ENOTSOCK]         The descriptor references a file, not a socket.
    [  ] [EOPNOTSUPP]       The referenced socket is not of type SOCK_STREAM.
    [  ] [EFAULT]           The addr parameter is not in a writable part of the
                            user address space.
    [--] [EWOULDBLOCK]      The socket is marked non-blocking and no connections
                            are present to be accepted.
    [--] [EMFILE]           The per-process descriptor table is full.
    [  ] [ENFILE]           The system file table is full.
*/
int zts_accept(ZT_ACCEPT_SIG) {
    DEBUG_EXTRA("fd = %d", fd);
    int err = 0;
    if(fd < 0) {
        errno = EBADF;
        err = -1;
    }
    else
    {
        // +1 since we'll be creating a new pico_socket when we accept the connection
        if(pico_ntimers()+1 >= PICO_MAX_TIMERS) {
            DEBUG_ERROR("cannot provision additional socket due to limitation of PICO_MAX_TIMERS.");
            errno = EMFILE;
            err = -1;
        }
        ZeroTier::_multiplexer_lock.lock();
        std::pair<ZeroTier::Connection*, ZeroTier::SocketTap*> *p = ZeroTier::fdmap[fd];
        if(!p) {
            DEBUG_ERROR("unable to locate connection pair (did you zts_bind())?");
            errno = EBADF;
            err = -1;
        }
        else {
            ZeroTier::Connection *conn = p->first;
            ZeroTier::SocketTap *tap = p->second;
            ZeroTier::Connection *accepted_conn;

            // BLOCKING: loop and keep checking until we find a newly accepted connection
            int f_err, blocking = 1;
            if ((f_err = fcntl(fd, F_GETFL, 0)) < 0) {
                DEBUG_ERROR("fcntl error, err = %s, errno = %d", f_err, errno);
                err = -1;
            } 
            else {
                blocking = !(f_err & O_NONBLOCK);
            }
            if(!err && !blocking) { // non-blocking
                DEBUG_EXTRA("EWOULDBLOCK, not a real error, assuming non-blocking mode");
                errno = EWOULDBLOCK;
                err = -1;
                accepted_conn = tap->Accept(conn);
            }
            else if (!err && blocking) { // blocking
                while(true) {
                    DEBUG_EXTRA("checking...");
                    usleep(ZT_ACCEPT_RECHECK_DELAY * 1000);
                    accepted_conn = tap->Accept(conn);
                    if(accepted_conn)
                        break; // accepted fd = err
                }
            }
            if(accepted_conn) {
                ZeroTier::fdmap[accepted_conn->app_fd] = new std::pair<ZeroTier::Connection*,ZeroTier::SocketTap*>(accepted_conn, tap);
                err = accepted_conn->app_fd;
            }
        }
        ZeroTier::_multiplexer_lock.unlock();
    }
    return err;
}


/*
Linux accept() (and accept4()) passes already-pending network errors on the new socket as an error code from accept(). This behavior differs from other BSD socket implementations. For reliable operation the application should detect the network errors defined for the protocol after accept() and treat them like EAGAIN by retrying. In the case of TCP/IP, these are ENETDOWN, EPROTO, ENOPROTOOPT, EHOSTDOWN, ENONET, EHOSTUNREACH, EOPNOTSUPP, and ENETUNREACH.
Errors

    [  ] EAGAIN or EWOULDBLOCK The socket is marked nonblocking and no connections are present to be accepted. POSIX.1-2001 allows either error to be returned for this case, and does not require these constants to have the same value, so a portable application should check for both possibilities.
    [--] EBADF The descriptor is invalid.
    [  ] ECONNABORTED A connection has been aborted.
    [  ] EFAULT The addr argument is not in a writable part of the user address space.
    [NA] EINTR The system call was interrupted by a signal that was caught before a valid connection arrived; see signal(7).
    [  ] EINVAL Socket is not listening for connections, or addrlen is invalid (e.g., is negative).
    [  ] EINVAL (accept4()) invalid value in flags.
    [  ] EMFILE The per-process limit of open file descriptors has been reached.
    [  ] ENFILE The system limit on the total number of open files has been reached.
    [  ] ENOBUFS, ENOMEM Not enough free memory. This often means that the memory allocation is limited by the socket buffer limits, not by the system memory.
    [  ] ENOTSOCK The descriptor references a file, not a socket.
    [  ] EOPNOTSUPP The referenced socket is not of type SOCK_STREAM.
    [  ] EPROTO Protocol error.

In addition, Linux accept() may fail if:

EPERM Firewall rules forbid connection.
*/
#if defined(__linux__)
    int zts_accept4(ZT_ACCEPT4_SIG)
    {
        DEBUG_INFO("fd = %d", fd);
        int err = 0;
        if(fd < 0) {
            errno = EBADF;
            err = -1;
        }
        return 0;
    }
#endif


/*
    [--] [EBADF]            The argument s is not a valid descriptor.
    [  ] [ENOTSOCK]         The argument s is a file, not a socket.
    [  ] [ENOPROTOOPT]      The option is unknown at the level indicated.
    [  ] [EFAULT]           The address pointed to by optval is not in a valid
                            part of the process address space.  For getsockopt(),
                            this error may also be returned if optlen is not in a
                            valid part of the process address space.
    [  ] [EDOM]             The argument value is out of bounds.
*/
int zts_setsockopt(ZT_SETSOCKOPT_SIG)
{
    //DEBUG_INFO("fd = %d", fd);
    int err = 0;
    if(fd < 0) {
        errno = EBADF;
        err = -1;
    }
    err = setsockopt(fd, level, optname, optval, optlen);
    //DEBUG_INFO("err = %d", err);
    return err;
}

/*
    [--] [EBADF]            The argument s is not a valid descriptor.
    [  ] [ENOTSOCK]         The argument s is a file, not a socket.
    [  ] [ENOPROTOOPT]      The option is unknown at the level indicated.
    [  ] [EFAULT]           The address pointed to by optval is not in a valid
                            part of the process address space.  For getsockopt(),
                            this error may also be returned if optlen is not in a
                            valid part of the process address space.
    [  ] [EDOM]             The argument value is out of bounds.

*/
int zts_getsockopt(ZT_GETSOCKOPT_SIG)
{
    //DEBUG_INFO("fd = %d", fd);    
    int err = 0;
    if(fd < 0) {
        errno = EBADF;
        err = -1;
    }
    err = getsockopt(fd, level, optname, optval, optlen);
    return err;
}

/*
    [--] [EBADF]            The argument s is not a valid descriptor.
    [  ] [ENOTSOCK]         The argument s is a file, not a socket.
    [  ] [ENOBUFS]          Insufficient resources were available in the system to
                            perform the operation.
    [  ] [EFAULT]           The name parameter points to memory not in a valid
                            part of the process address space.
*/
int zts_getsockname(ZT_GETSOCKNAME_SIG)
{
    DEBUG_INFO("fd = %d", fd);
    int err = 0;
    if(fd < 0) {
        errno = EBADF;
        err = -1;
    }
    // TODO
    return err;
}

/*
    [--] [EBADF]            The argument s is not a valid descriptor.
    [  ] [ENOTSOCK]         The argument s is a file, not a socket.
    [  ] [ENOTCONN]         The socket is not connected.
    [  ] [ENOBUFS]          Insufficient resources were available in the system to
                            perform the operation.
    [  ] [EFAULT]           The name parameter points to memory not in a valid
                            part of the process address space.
*/
int zts_getpeername(ZT_GETPEERNAME_SIG)
{
    DEBUG_INFO("fd = %d", fd);
    int err = 0;
    if(fd < 0) {
        errno = EBADF;
        err = -1;
    }
    // TODO
    return err;
}

/*

Linux:

    See: http://yarchive.net/comp/linux/close_return_value.html

Darwin:

    [--] [EBADF]            fildes is not a valid, active file descriptor.
    [NA] [EINTR]            Its execution was interrupted by a signal.
    [  ] [EIO]              A previously-uncommitted write(2) encountered an input/output error.
*/

// FIXME: See pico_socket_setoption's LINGER functionality

int zts_close(ZT_CLOSE_SIG)
{
    //DEBUG_EXTRA("fd = %d", fd);
    int err = 0;
    if(fd < 0) {
        errno = EBADF;
        err = -1;
    }
    else
    {
        if(!zt1Service) {
            DEBUG_ERROR("cannot close socket. service not started. call zts_start(path) first");
            errno = EBADF;
            err = -1;
        }
        else
        {
            ZeroTier::_multiplexer_lock.lock();
            //DEBUG_INFO("unmap=%d, fdmap=%d", ZeroTier::unmap.size(), ZeroTier::fdmap.size());

            // First, look for for unassigned connections
            ZeroTier::Connection *conn = ZeroTier::unmap[fd];

            // Since we found an unassigned connection, we don't need to consult the stack or tap
            // during closure - it isn't yet stitched into the clockwork
            if(conn) // unassigned
            {
                DEBUG_ERROR("unassigned closure");
                if((err = pico_socket_close(conn->picosock)) < 0)
                    DEBUG_ERROR("error calling pico_socket_close()");
                if((err = close(conn->app_fd)) < 0)
                    DEBUG_ERROR("error closing app_fd");
                if((err = close(conn->sdk_fd)) < 0)
                    DEBUG_ERROR("error closing sdk_fd");            
                delete conn;
                ZeroTier::unmap.erase(fd);
            }
            else // assigned
            {
                std::pair<ZeroTier::Connection*, ZeroTier::SocketTap*> *p = ZeroTier::fdmap[fd];
                if(!p) 
                {
                    DEBUG_ERROR("unable to locate connection pair.");
                    errno = EBADF;
                    err = -1;
                }
                else
                {

                    conn = p->first;
                    ZeroTier::SocketTap *tap = p->second;

                    //DEBUG_ERROR("close...., conn = %p, fd = %d", conn, fd);

                    // For cases where data might still need to pass through the library
                    // before socket closure
                    if(ZT_SOCK_BEHAVIOR_LINGER) {
                        socklen_t optlen;
                        struct linger so_linger;
                        zts_getsockopt(fd, SOL_SOCKET, SO_LINGER, &so_linger, &optlen);
                        if (so_linger.l_linger != 0) {
                            sleep(so_linger.l_linger); // do the linger!
                        }    
                    }
                    // Tell the tap to stop monitoring this PhySocket
                    //if((err = pico_socket_close(conn->picosock)) < 0)
                    //   DEBUG_ERROR("error calling pico_socket_close()");
                    tap->Close(conn);
                    // delete objects
                    // FIXME: double check this
                    //delete p;
                    ZeroTier::fdmap.erase(fd);
                    err = 0;
                }
            }
            //DEBUG_INFO(" unmap=%d, fdmap=%d", ZeroTier::unmap.size(), ZeroTier::fdmap.size());
            ZeroTier::_multiplexer_lock.unlock();
        }
    }
    return err;
}

int zts_fcntl(ZT_FCNTL_SIG)
{
    //DEBUG_INFO("fd = %d", fd);
    int err;
    if(fd < 0) {
        errno = EBADF;
        err = -1;
    }
    else {
        err = fcntl(fd, cmd, flags);
    }
    return err;
}

// TODO
ssize_t zts_sendto(ZT_SENDTO_SIG)
{
    DEBUG_INFO("fd = %d", fd);
    int err;
    if(fd < 0) {
        errno = EBADF;
        err = -1;
    }
    else {
        err = sendto(fd, buf, len, flags, addr, addrlen);
    }
    return err;
}

// TODO
ssize_t zts_sendmsg(ZT_SENDMSG_SIG)
{
    DEBUG_INFO("fd = %d", fd);
    int err;
    if(fd < 0) {
        errno = EBADF;
        err = -1;
    }
    else {
        err = sendmsg(fd, msg, flags);
    }
    return err;
}

// TODO
ssize_t zts_recvfrom(ZT_RECVFROM_SIG)
{
    DEBUG_INFO("fd = %d", fd);
    int err;
    if(fd < 0) {
        errno = EBADF;
        err = -1;
    }
    else {
        err = recvfrom(fd, buf, len, flags, addr, addrlen);
    }
    return err;
}

// TODO
ssize_t zts_recvmsg(ZT_RECVMSG_SIG)
{
    DEBUG_INFO("fd = %d", fd);
    int err;
    if(fd < 0) {
        errno = EBADF;
        err = -1;
    }
    else {
        err = recvmsg(fd, msg, flags);
    }
    return err;
}

int zts_read(ZT_READ_SIG) {
    //DEBUG_EXTRA("fd = %d", fd);
    return read(fd, buf, len);
}

int zts_write(ZT_WRITE_SIG) {
    //DEBUG_EXTRA("fd = %d", fd);
    return write(fd, buf, len);
}

/****************************************************************************/
/* SDK Socket API (Java Native Interface JNI)                               */
/* JNI naming convention: Java_PACKAGENAME_CLASSNAME_METHODNAME             */
/****************************************************************************/


#if defined(SDK_JNI)

namespace ZeroTier {

    #include <jni.h>

    JNIEXPORT int JNICALL Java_zerotier_ZeroTier_ztjni_1start(JNIEnv *env, jobject thisObj, jstring path) {
        if(path) {
            homeDir = env->GetStringUTFChars(path, NULL);
            zts_start(homeDir.c_str());
        }
    }
    // Shuts down ZeroTier service and SOCKS5 Proxy server
    JNIEXPORT void JNICALL Java_zerotier_ZeroTier_ztjni_1stop(JNIEnv *env, jobject thisObj) {
        if(zt1Service)
            zts_stop();
    }

    // Returns whether the ZeroTier service is running
    JNIEXPORT jboolean JNICALL Java_zerotier_ZeroTier_ztjni_1running(
        JNIEnv *env, jobject thisObj) 
    {
        return  zts_running();
    }
    // Returns path for ZT config/data files    
    JNIEXPORT jstring JNICALL Java_zerotier_ZeroTier_ztjni_1homepath(
        JNIEnv *env, jobject thisObj) 
    {
        // TODO: fix, should copy into given arg
        // return (*env).NewStringUTF(zts_get_homepath());
        return (*env).NewStringUTF("");
    }
    // Join a network
    JNIEXPORT void JNICALL Java_zerotier_ZeroTier_ztjni_1join(
        JNIEnv *env, jobject thisObj, jstring nwid) 
    {
        const char *nwidstr;
        if(nwid) {
            nwidstr = env->GetStringUTFChars(nwid, NULL);
            zts_join(nwidstr);
        }
    }
    // Leave a network
    JNIEXPORT void JNICALL Java_zerotier_ZeroTier_ztjni_1leave(
        JNIEnv *env, jobject thisObj, jstring nwid) 
    {
        const char *nwidstr;
        if(nwid) {
            nwidstr = env->GetStringUTFChars(nwid, NULL);
            zts_leave(nwidstr);
        }
    }
    // FIXME: Re-implemented to make it play nicer with the C-linkage required for Xcode integrations
    // Now only returns first assigned address per network. Shouldn't normally be a problem
    JNIEXPORT jobject JNICALL Java_zerotier_ZeroTier_ztjni_1get_1ipv4_1address(
        JNIEnv *env, jobject thisObj, jstring nwid) 
    {
        const char *nwid_str = env->GetStringUTFChars(nwid, NULL);
        char address_string[32];
        memset(address_string, 0, 32);
        zts_get_ipv4_address(nwid_str, address_string, ZT_MAX_IPADDR_LEN);
        jclass clazz = (*env).FindClass("java/util/ArrayList");
        jobject addresses = (*env).NewObject(clazz, (*env).GetMethodID(clazz, "<init>", "()V"));        
        jstring _str = (*env).NewStringUTF(address_string);
        env->CallBooleanMethod(addresses, env->GetMethodID(clazz, "add", "(Ljava/lang/Object;)Z"), _str);
        return addresses;
	}

    JNIEXPORT jobject JNICALL Java_zerotier_ZeroTier_ztjni_1get_1ipv6_1address(
        JNIEnv *env, jobject thisObj, jstring nwid) 
    {
        const char *nwid_str = env->GetStringUTFChars(nwid, NULL);
        char address_string[32];
        memset(address_string, 0, 32);
        zts_get_ipv6_address(nwid_str, address_string, ZT_MAX_IPADDR_LEN);
        jclass clazz = (*env).FindClass("java/util/ArrayList");
        jobject addresses = (*env).NewObject(clazz, (*env).GetMethodID(clazz, "<init>", "()V"));        
        jstring _str = (*env).NewStringUTF(address_string);
        env->CallBooleanMethod(addresses, env->GetMethodID(clazz, "add", "(Ljava/lang/Object;)Z"), _str);
        return addresses;
	}

    // Returns the device is in integer form
    JNIEXPORT jint Java_zerotier_ZeroTier_ztjni_1get_1device_1id() 
    {
        return zts_get_device_id(NULL); // TODO
    }

    JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1send(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len, int flags)
    {
        jbyte *body = (*env).GetByteArrayElements((_jbyteArray *)buf, 0);
        char * bufp = (char *)malloc(sizeof(char)*len);
        memcpy(bufp, body, len);
        (*env).ReleaseByteArrayElements((_jbyteArray *)buf, body, 0);
        int written_bytes = zts_write(fd, body, len);
        return written_bytes;
    }

    JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1sendto(
        JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len, jint flags, jobject ztaddr)
    {
        struct sockaddr_in addr;
        jclass cls = (*env).GetObjectClass( ztaddr);
        jfieldID f = (*env).GetFieldID( cls, "port", "I");
        addr.sin_port = htons((*env).GetIntField( ztaddr, f));
        f = (*env).GetFieldID( cls, "_rawAddr", "J");
        addr.sin_addr.s_addr = (*env).GetLongField( ztaddr, f);
        addr.sin_family = AF_INET;
        //LOGV("zt_sendto(): fd = %d\naddr = %s\nport=%d", fd, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        // TODO: Optimize this
        jbyte *body = (*env).GetByteArrayElements((_jbyteArray *)buf, 0);
        char * bufp = (char *)malloc(sizeof(char)*len);
        memcpy(bufp, body, len);
        (*env).ReleaseByteArrayElements((_jbyteArray *)buf, body, 0);
        // "connect" and send buffer contents
        int sent_bytes = zts_sendto(fd, body, len, flags, (struct sockaddr *)&addr, sizeof(addr));
        return sent_bytes;
    }

    JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1recvfrom(
        JNIEnv *env, jobject thisObj, jint fd, jbyteArray buf, jint len, jint flags, jobject ztaddr)
    {
        struct sockaddr_in addr;
        jbyte *body = (*env).GetByteArrayElements( buf, 0);
        unsigned char buffer[ZT_SDK_MTU];
        int payload_offset = sizeof(int) + sizeof(struct sockaddr_storage);
        int rxbytes = zts_recvfrom(fd, &buffer, len, flags, (struct sockaddr *)&addr, (socklen_t *)sizeof(struct sockaddr_storage));
        if(rxbytes > 0)
            memcpy(body, (jbyte*)buffer + payload_offset, rxbytes);
        (*env).ReleaseByteArrayElements( buf, body, 0);
        // Update fields of Java ZTAddress object
        jfieldID fid;
        jclass cls = (*env).GetObjectClass( ztaddr);
        fid = (*env).GetFieldID( cls, "port", "I");
        (*env).SetIntField( ztaddr, fid, addr.sin_port);
        fid = (*env).GetFieldID( cls,"_rawAddr", "J");
        (*env).SetLongField( ztaddr, fid,addr.sin_addr.s_addr);        
        return rxbytes;
    }

    JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1write(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len)
    {
        jbyte *body = (*env).GetByteArrayElements((_jbyteArray *)buf, 0);
        char * bufp = (char *)malloc(sizeof(char)*len);
        memcpy(bufp, body, len);
        (*env).ReleaseByteArrayElements((_jbyteArray *)buf, body, 0);
        int written_bytes = zts_write(fd, body, len);
        return written_bytes;
    }

    JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1read(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len)
    {
        jbyte *body = (*env).GetByteArrayElements((_jbyteArray *)buf, 0);
        int read_bytes = read(fd, body, len);
        (*env).ReleaseByteArrayElements((_jbyteArray *)buf, body, 0);
        return read_bytes;
    }    

    JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1setsockopt(
        JNIEnv *env, jobject thisObj, jint fd, jint level, jint optname, jint optval, jint optlen) {
        return zts_setsockopt(fd, level, optname, (const void*)optval, optlen);
    }

    JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1getsockopt(JNIEnv *env, jobject thisObj, jint fd, jint level, jint optname, jint optval, jint optlen) {
        return zts_getsockopt(fd, level, optname, (void*)optval, (socklen_t *)optlen);
    }

    JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1socket(JNIEnv *env, jobject thisObj, jint family, jint type, jint protocol) {
        return zts_socket(family, type, protocol);
    }
    
    JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1connect(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port) {
        struct sockaddr_in addr;
        const char *str = (*env).GetStringUTFChars( addrstr, 0);
        addr.sin_addr.s_addr = inet_addr(str);
        addr.sin_family = AF_INET;
        addr.sin_port = htons( port );
        (*env).ReleaseStringUTFChars( addrstr, str);
        return zts_connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    }

    JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1bind(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port) {
        struct sockaddr_in addr;
        const char *str = (*env).GetStringUTFChars( addrstr, 0);
        DEBUG_INFO("fd=%d, addr=%s, port=%d", fd, str, port);
        addr.sin_addr.s_addr = inet_addr(str);
        addr.sin_family = AF_INET;
        addr.sin_port = htons( port );
        (*env).ReleaseStringUTFChars( addrstr, str);
        return zts_bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    }

#if defined(__linux__)
     JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1accept4(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port, jint flags) {
        struct sockaddr_in addr;
        char *str;
        // = env->GetStringUTFChars(addrstr, NULL);
        (*env).ReleaseStringUTFChars( addrstr, str);
        addr.sin_addr.s_addr = inet_addr(str);
        addr.sin_family = AF_INET;
        addr.sin_port = htons( port );
        return zts_accept4(fd, (struct sockaddr *)&addr, sizeof(addr), flags);
    }
#endif

    JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1accept(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port) {
        struct sockaddr_in addr;
        // TODO: Send addr info back to Javaland
        addr.sin_addr.s_addr = inet_addr("");
        addr.sin_family = AF_INET;
        addr.sin_port = htons( port );
        return zts_accept(fd, (struct sockaddr *)&addr, (socklen_t *)sizeof(addr));    
    }

    JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1listen(JNIEnv *env, jobject thisObj, jint fd, int backlog) {
        return zts_listen(fd, backlog);
    }

    JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1close(JNIEnv *env, jobject thisObj, jint fd) {
        return zts_close(fd);
    }

    JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1getsockname(JNIEnv *env, jobject thisObj, jint fd, jobject ztaddr) {
        struct sockaddr_in addr;
        int err = zts_getsockname(fd, (struct sockaddr *)&addr, (socklen_t *)sizeof(struct sockaddr));
        jfieldID fid;
        jclass cls = (*env).GetObjectClass(ztaddr);
        fid = (*env).GetFieldID( cls, "port", "I");
        (*env).SetIntField( ztaddr, fid, addr.sin_port);
        fid = (*env).GetFieldID( cls,"_rawAddr", "J");
        (*env).SetLongField( ztaddr, fid,addr.sin_addr.s_addr);        
        return err;    
    }

    JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1getpeername(JNIEnv *env, jobject thisObj, jint fd, jobject ztaddr) {
        struct sockaddr_in addr;
        int err = zts_getpeername(fd, (struct sockaddr *)&addr, (socklen_t *)sizeof(struct sockaddr));
        jfieldID fid;
        jclass cls = (*env).GetObjectClass( ztaddr);
        fid = (*env).GetFieldID( cls, "port", "I");
        (*env).SetIntField( ztaddr, fid, addr.sin_port);
        fid = (*env).GetFieldID( cls,"_rawAddr", "J");
        (*env).SetLongField( ztaddr, fid,addr.sin_addr.s_addr);        
        return err;
    }

    JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_ztjni_1fcntl(JNIEnv *env, jobject thisObj, jint fd, jint cmd, jint flags) {
        return zts_fcntl(fd,cmd,flags);
    }
}
#endif

/****************************************************************************/
/* SDK Socket API Helper functions --- DON'T CALL THESE DIRECTLY            */
/****************************************************************************/

int zts_nsockets()
{
    ZeroTier::_multiplexer_lock.unlock();
    int num = ZeroTier::unmap.size() + ZeroTier::fdmap.size();
    ZeroTier::_multiplexer_lock.unlock(); 
    return num;
}

int zts_maxsockets()
{
    // TODO: This is only an approximation
    return PICO_MAX_TIMERS - 10;
}

// Starts a ZeroTier service in the background
void *zts_start_service(void *thread_id) {

    DEBUG_INFO("homeDir=%s", ZeroTier::homeDir.c_str());
    // Where network .conf files will be stored
    ZeroTier::netDir = ZeroTier::homeDir + "/networks.d";
    zt1Service = (ZeroTier::OneService *)0;
    
    // Construct path for network config and supporting service files
    if (ZeroTier::homeDir.length()) {
        std::vector<std::string> hpsp(ZeroTier::OSUtils::split(ZeroTier::homeDir.c_str(),
            ZT_PATH_SEPARATOR_S,"",""));
        std::string ptmp;
        if (ZeroTier::homeDir[0] == ZT_PATH_SEPARATOR)
            ptmp.push_back(ZT_PATH_SEPARATOR);
        for(std::vector<std::string>::iterator pi(hpsp.begin());pi!=hpsp.end();++pi) {
            if (ptmp.length() > 0)
                ptmp.push_back(ZT_PATH_SEPARATOR);
            ptmp.append(*pi);
            if ((*pi != ".")&&(*pi != "..")) {
                if (!ZeroTier::OSUtils::mkdir(ptmp)) {
                    DEBUG_ERROR("home path does not exist, and could not create");
                    perror("error\n");
                }
            }
        }
    }
    else {
        DEBUG_ERROR("homeDir is empty, could not construct path");
        return NULL;
    }
    // rpc dir
    if(!ZeroTier::OSUtils::mkdir(ZeroTier::homeDir + "/" + ZT_SDK_RPC_DIR_PREFIX)) {
        DEBUG_ERROR("unable to create dir: " ZT_SDK_RPC_DIR_PREFIX);
        return NULL;
    }

    // Generate random port for new service instance
    unsigned int randp = 0;
    ZeroTier::Utils::getSecureRandom(&randp,sizeof(randp));
    // TODO: Better port random range selection
    int servicePort = 9000 + (randp % 1000);

    for(;;) {
        zt1Service = ZeroTier::OneService::newInstance(ZeroTier::homeDir.c_str(),servicePort);
        switch(zt1Service->run()) {
            case ZeroTier::OneService::ONE_STILL_RUNNING: 
            case ZeroTier::OneService::ONE_NORMAL_TERMINATION:
                break;
            case ZeroTier::OneService::ONE_UNRECOVERABLE_ERROR:
                DEBUG_ERROR("fatal error: %s",zt1Service->fatalErrorMessage().c_str());
                break;
            case ZeroTier::OneService::ONE_IDENTITY_COLLISION: {
                delete zt1Service;
                zt1Service = (ZeroTier::OneService *)0;
                std::string oldid;
                ZeroTier::OSUtils::readFile((ZeroTier::homeDir + ZT_PATH_SEPARATOR_S 
                    + "identity.secret").c_str(),oldid);
                if (oldid.length()) {
                    ZeroTier::OSUtils::writeFile((ZeroTier::homeDir + ZT_PATH_SEPARATOR_S 
                        + "identity.secret.saved_after_collision").c_str(),oldid);
                    ZeroTier::OSUtils::rm((ZeroTier::homeDir + ZT_PATH_SEPARATOR_S 
                        + "identity.secret").c_str());
                    ZeroTier::OSUtils::rm((ZeroTier::homeDir + ZT_PATH_SEPARATOR_S 
                        + "identity.public").c_str());
                }
            }	
            continue; // restart!
        }
        break; // terminate loop -- normally we don't keep restarting
    }
    delete zt1Service;
    zt1Service = (ZeroTier::OneService *)0;
    return NULL;
}

#ifdef __cplusplus
}
#endif
