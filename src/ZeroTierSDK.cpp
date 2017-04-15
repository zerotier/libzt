/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2016  ZeroTier, Inc.  https://www.zerotier.com/
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

#include "OneService.hpp"
#include "Utils.hpp"
#include "OSUtils.hpp"
#include "InetAddress.hpp"
#include "ZeroTierOne.h"

#include "SocketTap.hpp"
#include "ZeroTierSDK.h"

#ifdef __cplusplus
extern "C" {
#endif

static ZeroTier::OneService *zt1Service;

namespace ZeroTier {
    std::string homeDir;      // The resultant platform-specific dir we *must* use internally
    std::string netDir;       // Where network .conf files are to be written

    picoTCP *picostack = NULL;
    std::map<int, Connection*> UnassignedConnections;
    std::map<int, std::pair<Connection*,SocketTap*>*> AssignedFileDescriptors;
    Mutex _multiplexer_lock; 
    Mutex _accepted_connection_lock; 
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
    pthread_create(&service_thread, NULL, _start_service, (void *)(path));
}

void zts_stop() {
    if(zt1Service) { 
        zt1Service->terminate();
        zt1Service->removeNets();
    }
}

void zts_join_network(const char * nwid) {
    if(zt1Service) {
        std::string confFile = zt1Service->givenHomePath() + "/networks.d/" + nwid + ".conf";
        if(!ZeroTier::OSUtils::mkdir(ZeroTier::netDir))
            DEBUG_ERROR("unable to create: %s", ZeroTier::netDir.c_str());
        if(!ZeroTier::OSUtils::writeFile(confFile.c_str(), ""))
            DEBUG_ERROR("unable to write network conf file: %s", confFile.c_str());
        zt1Service->join(nwid);
    }
}

void zts_join_network_soft(const char * filepath, const char * nwid) { 
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

void zts_leave_network(const char * nwid) { 
    if(zt1Service)
        zt1Service->leave(nwid);
}

void zts_leave_network_soft(const char * filepath, const char * nwid) {
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

int zts_service_running() { 
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

}

void zts_disable_http_control_plane()
{

}

/****************************************************************************/
/* SocketTap Multiplexer Functionality                                      */
/* - This section of the API is used to implement the general socket        */
/*   controls. Basically this is designed to handle socket provisioning     */
/*   requests when no SocketTap is yet initialized, and as a way to         */
/*   determine which SocketTap is to be used for a particular connect() or  */ 
/*   bind() call. This enables multi-network support                        */
/****************************************************************************/

int zts_socket(ZT_SOCKET_SIG) {
    DEBUG_INFO();
    ZeroTier::_multiplexer_lock.lock();
    ZeroTier::Connection *conn = new ZeroTier::Connection();
    int err, protocol_version;
    // set up pico_socket
    struct pico_socket * psock;
    #if defined(SDK_IPV4)
        protocol_version = PICO_PROTO_IPV4;
    #elif defined(SDK_IPV6)
        protocol_version = PICO_PROTO_IPV6;
    #endif
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
        ZeroTier::UnassignedConnections[conn->app_fd] = conn;
        err = conn->app_fd; // return one end of the socketpair
    }
    else {
        DEBUG_ERROR("failed to create pico_socket");
        err = -1;
    }
    ZeroTier::_multiplexer_lock.unlock();
    return err;
}

int zts_connect(ZT_CONNECT_SIG) {
    if(!zt1Service) {
        DEBUG_ERROR("zt1Service = NULL");
        // errno = ?
        return -1;
    }
    ZeroTier::_multiplexer_lock.lock();
    int err;
    ZeroTier::Connection *conn = ZeroTier::UnassignedConnections[fd];
    ZeroTier::SocketTap *tap;

    if(conn) {      
        char ipstr[INET6_ADDRSTRLEN];//, nm_str[INET6_ADDRSTRLEN];
        memset(ipstr, 0, INET6_ADDRSTRLEN);
        ZeroTier::InetAddress iaddr;

        if(conn->socket_family == AF_INET) {
            // FIXME: Fix this typecast mess
            inet_ntop(AF_INET, 
                (const void *)&((struct sockaddr_in *)addr)->sin_addr.s_addr, ipstr, INET_ADDRSTRLEN);
        }
        if(conn->socket_family == AF_INET6) {
            // FIXME: Fix this typecast mess
            inet_ntop(AF_INET6, 
                (const void *)&((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr, ipstr, INET6_ADDRSTRLEN);
        }
        iaddr.fromString(ipstr);

        DEBUG_INFO("ipstr= %s", ipstr);
        DEBUG_INFO("iaddr= %s", iaddr.toString().c_str());

        tap = zt1Service->getTap(iaddr);

        if(!tap) {
            // TODO: More canonical error?
            DEBUG_ERROR("no route to host");
            // errno = ?
            err = -1;
        }
        else {
            conn->sock = tap->_phy.wrapSocket(conn->sdk_fd, conn); // wrap the socketpair we created earlier
            conn->picosock->priv = new ZeroTier::Larg(tap, conn); // pointer to tap we use in callbacks from the stack
            DEBUG_INFO("found appropriate SocketTap");
            // TODO: Perhaps move this connect call outside of the lock
            tap->_Connections.push_back(conn); // Give this Connection to the tap we decided on
            err = tap->Connect(conn, fd, addr, addrlen); // Semantically: tap->stack->connect
            conn->tap = tap;
        }
    }
    else {
        DEBUG_ERROR("unable to locate connection");
        // errno = ?
        err = -1;
    }
    ZeroTier::AssignedFileDescriptors[fd] = new std::pair<ZeroTier::Connection*,ZeroTier::SocketTap*>(conn, tap);
    ZeroTier::_multiplexer_lock.unlock();
    return err;
}

int zts_bind(ZT_BIND_SIG) {
    if(!zt1Service) {
        DEBUG_ERROR("zt1Service = NULL");
        // errno = ?
        return -1;
    }
    ZeroTier::_multiplexer_lock.lock();
    int err;
    ZeroTier::Connection *conn = ZeroTier::UnassignedConnections[fd];
    ZeroTier::SocketTap *tap;
    
    if(conn) {      
        char ipstr[INET6_ADDRSTRLEN];//, nm_str[INET6_ADDRSTRLEN];
        memset(ipstr, 0, INET6_ADDRSTRLEN);
        ZeroTier::InetAddress iaddr;

        if(conn->socket_family == AF_INET) {
            // FIXME: Fix this typecast mess
            inet_ntop(AF_INET, 
                (const void *)&((struct sockaddr_in *)addr)->sin_addr.s_addr, ipstr, INET_ADDRSTRLEN);
        }
        if(conn->socket_family == AF_INET6) {
            // FIXME: Fix this typecast mess
            inet_ntop(AF_INET6, 
                (const void *)&((struct sockaddr_in6 *)addr)->sin6_addr.s6_addr, ipstr, INET6_ADDRSTRLEN);
        }
        iaddr.fromString(ipstr);

        DEBUG_INFO("ipstr= %s", ipstr);
        DEBUG_INFO("iaddr= %s", iaddr.toString().c_str());

        tap = zt1Service->getTap(iaddr);

        if(!tap) {
            // TODO: More canonical error?
            DEBUG_ERROR("no appropriate interface to bind to");            
            // errno = ?
            err = -1;
        }
        else {
            DEBUG_INFO("found appropriate SocketTap");
            DEBUG_INFO("conn->picosock = %p", conn->picosock);
            conn->picosock->priv = new ZeroTier::Larg(tap, conn);
            // TODO: Perhaps move this connect call outside of the lock
            tap->_Connections.push_back(conn); // Give this Connection to the tap we decided on
            err = tap->Bind(conn, fd, addr, addrlen); // Semantically: tap->stack->connect
            conn->tap = tap;
        }
    }
    else {
        DEBUG_ERROR("unable to locate connection");
        // errno = ?
        err = -1;
    }
    ZeroTier::AssignedFileDescriptors[fd] = new std::pair<ZeroTier::Connection*,ZeroTier::SocketTap*>(conn, tap);
    ZeroTier::_multiplexer_lock.unlock();
    return err;
}

int zts_listen(ZT_LISTEN_SIG) {
    if(!zt1Service) {
        DEBUG_ERROR("zt1Service = NULL");
        // errno = ?
        return -1;
    }
    int err;
    ZeroTier::_multiplexer_lock.lock();
    ZeroTier::Connection *conn = ZeroTier::AssignedFileDescriptors[fd]->first;
    ZeroTier::SocketTap *tap = ZeroTier::AssignedFileDescriptors[fd]->second;
    
    if(!tap || !conn) {
        DEBUG_ERROR("unable to locate tap interface for file descriptor");
        err = -1;
    }
    tap->Listen(conn, fd, backlog);
    err = 0;
    DEBUG_INFO("put %p into LISTENING state", conn);
    ZeroTier::_multiplexer_lock.unlock();
    return err;
}

int zts_accept(ZT_ACCEPT_SIG) {
    int err;    
    ZeroTier::Connection *conn = ZeroTier::AssignedFileDescriptors[fd]->first;
    ZeroTier::SocketTap *tap = ZeroTier::AssignedFileDescriptors[fd]->second;
    // BLOCKING: loop and keep checking until we find a newly accepted connection
    if(true) {
        while(true) {
            usleep(ZT_ACCEPT_RECHECK_DELAY * 1000);
            err = tap->Accept(conn);
            if(err >= 0)
                return err;
        }
    }
    // NON-BLOCKING: only check for a new connection once
    else
        err = tap->Accept(conn);
    return err;
}

#if defined(__linux__)
    int zts_accept4(ZT_ACCEPT4_SIG)
    {
        return 0;
    }
#endif

int zts_setsockopt(ZT_SETSOCKOPT_SIG)
{
    return 0;
}

int zts_getsockopt(ZT_GETSOCKOPT_SIG)
{
    return 0;
}

int zts_getsockname(ZT_GETSOCKNAME_SIG)
{
    return 0;
}

int zts_getpeername(ZT_GETPEERNAME_SIG)
{
    return 0;
}

int zts_close(ZT_CLOSE_SIG)
{
    return 0;
}

int zts_fcntl(ZT_FCNTL_SIG)
{
    return 0;
}

ssize_t zts_sendto(ZT_SENDTO_SIG)
{
    return 0;
}

ssize_t zts_sendmsg(ZT_SENDMSG_SIG)
{
    return 0;
}

ssize_t zts_recvfrom(ZT_RECVFROM_SIG)
{
    return 0;
}

ssize_t zts_recvmsg(ZT_RECVMSG_SIG)
{
    return 0;
}

int zts_read(ZT_READ_SIG) {
    return read(fd, buf, len);
}

int zts_write(ZT_WRITE_SIG) {
    return write(fd, buf, len);
}



/****************************************************************************/
/* SDK Socket API (Java Native Interface JNI)                               */
/* JNI naming convention: Java_PACKAGENAME_CLASSNAME_METHODNAME             */
/****************************************************************************/

#if defined(__ANDROID__) || defined(__JNI_LIB__)
    // Returns whether the ZeroTier service is running
    JNIEXPORT jboolean JNICALL Java_zerotier_ZeroTier_zt_1service_1is_1running(
        JNIEnv *env, jobject thisObj) 
    {
        return  zts_service_is_running();
    }
    // Returns path for ZT config/data files    
    JNIEXPORT jstring JNICALL Java_zerotier_ZeroTier_zt_1get_1homepath(
        JNIEnv *env, jobject thisObj) 
    {
        return (*env).NewStringUTF(zts_get_homepath());
    }
    // Join a network
    JNIEXPORT void JNICALL Java_zerotier_ZeroTier_zt_1join_1network(
        JNIEnv *env, jobject thisObj, jstring nwid) 
    {
        const char *nwidstr;
        if(nwid) {
            nwidstr = env->GetStringUTFChars(nwid, NULL);
            zts_join_network(nwidstr);
        }
    }
    // Leave a network
    JNIEXPORT void JNICALL Java_zerotier_ZeroTier_zt_1leave_1network(
        JNIEnv *env, jobject thisObj, jstring nwid) 
    {
        const char *nwidstr;
        if(nwid) {
            nwidstr = env->GetStringUTFChars(nwid, NULL);
            zts_leave_network(nwidstr);
        }
    }
    // FIXME: Re-implemented to make it play nicer with the C-linkage required for Xcode integrations
    // Now only returns first assigned address per network. Shouldn't normally be a problem
    JNIEXPORT jobject JNICALL Java_zerotier_ZeroTier_zt_1get_1ipv4_1address(
        JNIEnv *env, jobject thisObj, jstring nwid) 
    {
        const char *nwid_str = env->GetStringUTFChars(nwid, NULL);
        char address_string[32];
        memset(address_string, 0, 32);
        zts_get_ipv4_address(nwid_str, address_string);
        jclass clazz = (*env).FindClass("java/util/ArrayList");
        jobject addresses = (*env).NewObject(clazz, (*env).GetMethodID(clazz, "<init>", "()V"));        
        jstring _str = (*env).NewStringUTF(address_string);
        env->CallBooleanMethod(addresses, env->GetMethodID(clazz, "add", "(Ljava/lang/Object;)Z"), _str);
        return addresses;
	}

    JNIEXPORT jobject JNICALL Java_zerotier_ZeroTier_zt_1get_1ipv6_1address(
        JNIEnv *env, jobject thisObj, jstring nwid) 
    {
        const char *nwid_str = env->GetStringUTFChars(nwid, NULL);
        char address_string[32];
        memset(address_string, 0, 32);
        zts_get_ipv6_address(nwid_str, address_string);
        jclass clazz = (*env).FindClass("java/util/ArrayList");
        jobject addresses = (*env).NewObject(clazz, (*env).GetMethodID(clazz, "<init>", "()V"));        
        jstring _str = (*env).NewStringUTF(address_string);
        env->CallBooleanMethod(addresses, env->GetMethodID(clazz, "add", "(Ljava/lang/Object;)Z"), _str);
        return addresses;
	}

    // Returns the device is in integer form
    JNIEXPORT jint Java_zerotier_ZeroTier_zt_1get_1device_1id() 
    {
        return zts_get_device_id(NULL); // TODO
    }
    // Returns whether the path to an endpoint is currently relayed by a root server
    JNIEXPORT jboolean JNICALL Java_zerotier_ZeroTier_zt_1is_1relayed()
    {
        return 0;
        // TODO
        // zts_is_relayed();
    }
#endif

/****************************************************************************/
/* SDK Socket API Helper functions --- DONT CALL THESE DIRECTLY             */
/****************************************************************************/

// Starts a ZeroTier service in the background
void *_start_service(void *thread_id) {

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
    int servicePort = 9000 + (randp % 10000);
    DEBUG_ERROR("servicePort = %d", servicePort);

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
