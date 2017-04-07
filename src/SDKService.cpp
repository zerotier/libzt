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

#if defined(__ANDROID__) || defined(__JNI_LIB__)
    #include <jni.h>
#endif

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

std::string service_path;
std::string localHomeDir; // Local shortened path
std::string givenHomeDir; // What the user/application provides as a suggestion
std::string homeDir;      // The resultant platform-specific dir we *must* use internally
std::string netDir;       // Where network .conf files are to be written


/****************************************************************************/
/* SDK Socket API                                                           */
/****************************************************************************/

void zts_start(const char *path)
{
    DEBUG_INFO("path=%s", path);
    if(path)
        homeDir = path;
    zts_start_core_service(NULL);
}

// Stop the service, proxy server, stack, etc
void zts_stop() {
    DEBUG_INFO();
    zts_stop_service();
}

char *zts_core_version() {
    return (char*)"1.2.2";
}

    // ------------------------------------------------------------------------------
    // --------------------------------- Base zts_* API -----------------------------
    // ------------------------------------------------------------------------------

// Prototypes
void *zts_start_core_service(void *thread_id);
void zts_init_rpc(const char * path, const char * nwid);

// Basic ZT service controls
void zts_join_network(const char * nwid) {
    DEBUG_ERROR();
    std::string confFile = zt1Service->givenHomePath() + "/networks.d/" + nwid + ".conf";
    if(!ZeroTier::OSUtils::mkdir(netDir)) {
        DEBUG_ERROR("unable to create: %s", netDir.c_str());
    }
    if(!ZeroTier::OSUtils::writeFile(confFile.c_str(), "")) {
        DEBUG_ERROR("unable to write network conf file: %s", confFile.c_str());
    }
    zt1Service->join(nwid);
    // Provide the API with the RPC information
    zts_init_rpc(homeDir.c_str(), nwid); 
}
// Just create the dir and conf file required, don't instruct the core to do anything
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
// Prevent service from joining network upon startup
void zts_leave_network_soft(const char * filepath, const char * nwid) {
    std::string net_dir = std::string(filepath) + "/networks.d/";
    ZeroTier::OSUtils::rm((net_dir + nwid + ".conf").c_str()); 
}
// Instruct the service to leave the network
void zts_leave_network(const char * nwid) { 
    if(zt1Service)
        zt1Service->leave(nwid);
}
// Check whether the service is running
int zts_service_is_running() { 
    return !zt1Service ? false : zt1Service->isRunning(); 
}
// Stop the service
void zts_stop_service() {
    if(zt1Service) 
        zt1Service->terminate(); 
}


// FIXME: Re-implemented to make it play nicer with the C-linkage required for Xcode integrations
// Now only returns first assigned address per network. Shouldn't normally be a problem.

// Get IPV4 Address for this device on given network
int zts_has_address(const char *nwid)
{
    char ipv4_addr[64], ipv6_addr[64];
    memset(ipv4_addr, 0, 64);
    memset(ipv6_addr, 0, 64);
    zts_get_ipv4_address(nwid, ipv4_addr);
    zts_get_ipv6_address(nwid, ipv6_addr);
    if(!strcmp(ipv4_addr, "-1.-1.-1.-1/-1") && !strcmp(ipv4_addr, "-1.-1.-1.-1/-1")) {
        return false;
    }
    return true;
}
void zts_get_ipv4_address(const char *nwid, char *addrstr)
{
    uint64_t nwid_int = strtoull(nwid, NULL, 16);
    ZeroTier::SocketTap *tap = zt1Service->getTap(nwid_int);
    if(tap && tap->_ips.size()){ 
        for(int i=0; i<tap->_ips.size(); i++) {
            if(tap->_ips[i].isV4()) {
                std::string addr = tap->_ips[i].toString();
                // DEBUG_EXTRA("addr=%s, addrlen=%d", addr.c_str(), addr.length());
                memcpy(addrstr, addr.c_str(), addr.length()); // first address found that matches protocol version 4
                return;
            }
        }
    }
    else {
        memcpy(addrstr, "-1.-1.-1.-1/-1", 14);
    }
}
// Get IPV6 Address for this device on given network
void zts_get_ipv6_address(const char *nwid, char *addrstr)
{
    uint64_t nwid_int = strtoull(nwid, NULL, 16);
    ZeroTier::SocketTap *tap = zt1Service->getTap(nwid_int);
    if(tap && tap->_ips.size()){ 
        for(int i=0; i<tap->_ips.size(); i++) {
            if(tap->_ips[i].isV6()) {
                std::string addr = tap->_ips[i].toString();
                // DEBUG_EXTRA("addr=%s, addrlen=%d", addr.c_str(), addr.length());
                memcpy(addrstr, addr.c_str(), addr.length()); // first address found that matches protocol version 4
                return;
            }
        }
    }
    else {
        memcpy(addrstr, "-1.-1.-1.-1/-1", 14);
    }
}
// Get device ID (from running service)
int zts_get_device_id(char *devID) { 
    if(zt1Service) {
        char id[10];
        sprintf(id, "%lx",zt1Service->getNode()->address());
        memcpy(devID, id, 10);
        return 0;
    }
    else
        return -1;
}
// Get device ID (from file)
int zts_get_device_id_from_file(const char *filepath, char *devID) {
    std::string fname("identity.public");
    std::string fpath(filepath);

    if(ZeroTier::OSUtils::fileExists((fpath + ZT_PATH_SEPARATOR_S + fname).c_str(),false)) {
        std::string oldid;
        ZeroTier::OSUtils::readFile((fpath + ZT_PATH_SEPARATOR_S + fname).c_str(),oldid);
        memcpy(devID, oldid.c_str(), 10); // first 10 bytes of file
        return 0;
    }
    return -1;
}
// Get the IP address of a peer if a direct path is available
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
// Return the number of peers on this network
unsigned long zts_get_peer_count() {
    if(zt1Service)
        return zt1Service->getNode()->peers()->peerCount;
    else
        return 0;
}
// Return the home path for this instance of ZeroTier
char *zts_get_homepath() {
    return (char*)givenHomeDir.c_str();
}
// Returns a 6PLANE IPv6 address given a network ID and zerotier ID
void zts_get_6plane_addr(char *addr, const char *nwid, const char *devID)
{
    ZeroTier::InetAddress _6planeAddr = ZeroTier::InetAddress::makeIpv66plane(ZeroTier::Utils::hexStrToU64(nwid),ZeroTier::Utils::hexStrToU64(devID));
    memcpy(addr, _6planeAddr.toIpString().c_str(), 40);
}
// Returns a RFC 4193 IPv6 address given a network ID and zerotier ID
void zts_get_rfc4193_addr(char *addr, const char *nwid, const char *devID)
{
    ZeroTier::InetAddress _6planeAddr = ZeroTier::InetAddress::makeIpv6rfc4193(ZeroTier::Utils::hexStrToU64(nwid),ZeroTier::Utils::hexStrToU64(devID));
    memcpy(addr, _6planeAddr.toIpString().c_str(), 40);
}

    // ------------------------------------------------------------------------------
    // ------------------------------ EXPORTED JNI METHODS --------------------------
    // ------------------------------------------------------------------------------
    // JNI naming convention: Java_PACKAGENAME_CLASSNAME_METHODNAME


#if defined(__ANDROID__) || defined(__JNI_LIB__)
    // Returns whether the ZeroTier service is running
    JNIEXPORT jboolean JNICALL Java_zerotier_ZeroTier_zt_1service_1is_1running(JNIEnv *env, jobject thisObj) {
        if(zt1Service)
            return  zts_service_is_running();
        return false;
    }
    // Returns path for ZT config/data files    
    JNIEXPORT jstring JNICALL Java_zerotier_ZeroTier_zt_1get_1homepath(JNIEnv *env, jobject thisObj) {
        return (*env).NewStringUTF(zts_get_homepath());
    }
    // Join a network
    JNIEXPORT void JNICALL Java_zerotier_ZeroTier_zt_1join_1network(JNIEnv *env, jobject thisObj, jstring nwid) {
        const char *nwidstr;
        if(nwid) {
            nwidstr = env->GetStringUTFChars(nwid, NULL);
            zts_join_network(nwidstr);
        }
    }
    // Leave a network
    JNIEXPORT void JNICALL Java_zerotier_ZeroTier_zt_1leave_1network(JNIEnv *env, jobject thisObj, jstring nwid) {
        const char *nwidstr;
        if(nwid) {
            nwidstr = env->GetStringUTFChars(nwid, NULL);
            zts_leave_network(nwidstr);
        }
    }
    // FIXME: Re-implemented to make it play nicer with the C-linkage required for Xcode integrations
    // Now only returns first assigned address per network. Shouldn't normally be a problem
    JNIEXPORT jobject JNICALL Java_zerotier_ZeroTier_zt_1get_1ipv4_1address(JNIEnv *env, jobject thisObj, jstring nwid) {
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

    JNIEXPORT jobject JNICALL Java_zerotier_ZeroTier_zt_1get_1ipv6_1address(JNIEnv *env, jobject thisObj, jstring nwid) {
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
    JNIEXPORT jint Java_zerotier_ZeroTier_zt_1get_1device_1id() {
        return zts_get_device_id(NULL); // TODO
    }
    // Returns whether the path to an endpoint is currently relayed by a root server
    JNIEXPORT jboolean JNICALL Java_zerotier_ZeroTier_zt_1is_1relayed() {
        return 0;
        // TODO
        // zts_is_relayed();
    }
#endif


    // ------------------------------------------------------------------------------
    // --------------------------- zts_start_core_service ---------------------------
    // ------------------------------------------------------------------------------


// Starts a ZeroTier service in the background
void *zts_start_core_service(void *thread_id) {

    #if defined(SDK_BUNDLED)
        if(thread_id)
            homeDir = std::string((char*)thread_id);
    #endif

    #if defined(__IOS__)
        char current_dir[MAX_DIR_SZ];
        // Go to the app's data directory so we can shorten the sun_path we bind to
        getcwd(current_dir, MAX_DIR_SZ);
        std::string targetDir = homeDir; // + "/../../";
        chdir(targetDir.c_str());
        homeDir = localHomeDir;
    #endif

    #if defined(__APPLE__)
        #include "TargetConditionals.h"
        #if TARGET_IPHONE_SIMULATOR
            // homeDir = "dont/run/this/in/the/simulator/it/wont/work";
        #elif TARGET_OS_IPHONE
            localHomeDir = "ZeroTier/One";
            std::string del = givenHomeDir.length() && givenHomeDir[givenHomeDir.length()-1]!='/' ? "/" : "";
            homeDir = givenHomeDir + del + localHomeDir;
        #endif
    #endif

    #if defined(__APPLE__) && !defined(__IOS__)
        localHomeDir = homeDir; // Used for RPC and *can* differ from homeDir on some platforms
    #endif

    DEBUG_INFO("homeDir=%s", homeDir.c_str());
    // Where network .conf files will be stored
    netDir = homeDir + "/networks.d";
    zt1Service = (ZeroTier::OneService *)0;
    
    // Construct path for network config and supporting service files
    if (homeDir.length()) {
        std::vector<std::string> hpsp(ZeroTier::OSUtils::split(homeDir.c_str(),ZT_PATH_SEPARATOR_S,"",""));
        std::string ptmp;
        if (homeDir[0] == ZT_PATH_SEPARATOR)
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

    DEBUG_INFO("starting service...");

    // Generate random port for new service instance
    unsigned int randp = 0;
    ZeroTier::Utils::getSecureRandom(&randp,sizeof(randp));
    int servicePort = 9000 + (randp % 1000);

    for(;;) {
        zt1Service = ZeroTier::OneService::newInstance(homeDir.c_str(),servicePort);
        switch(zt1Service->run()) {
            case ZeroTier::OneService::ONE_STILL_RUNNING: // shouldn't happen, run() won't return until done
            case ZeroTier::OneService::ONE_NORMAL_TERMINATION:
                break;
            case ZeroTier::OneService::ONE_UNRECOVERABLE_ERROR:
                DEBUG_ERROR("fatal error: %s",zt1Service->fatalErrorMessage().c_str());
                break;
            case ZeroTier::OneService::ONE_IDENTITY_COLLISION: {
                delete zt1Service;
                zt1Service = (ZeroTier::OneService *)0;
                std::string oldid;
                ZeroTier::OSUtils::readFile((homeDir + ZT_PATH_SEPARATOR_S + "identity.secret").c_str(),oldid);
                if (oldid.length()) {
                    ZeroTier::OSUtils::writeFile((homeDir + ZT_PATH_SEPARATOR_S + "identity.secret.saved_after_collision").c_str(),oldid);
                    ZeroTier::OSUtils::rm((homeDir + ZT_PATH_SEPARATOR_S + "identity.secret").c_str());
                    ZeroTier::OSUtils::rm((homeDir + ZT_PATH_SEPARATOR_S + "identity.public").c_str());
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
