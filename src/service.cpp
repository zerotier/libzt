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

#include "tap.hpp"
#include "sdk.h"
#include "debug.h"

#ifdef __cplusplus
extern "C" {
#endif

static ZeroTier::OneService *zt1Service;

std::string service_path;
std::string localHomeDir; // Local shortened path
std::string givenHomeDir; // What the user/application provides as a suggestion
std::string homeDir;      // The resultant platform-specific dir we *must* use internally
std::string netDir;       // Where network .conf files are to be written

pthread_t intercept_thread;
pthread_key_t thr_id_key;

int * intercept_thread_id;

    // ------------------------------------------------------------------------------
    // --------------------------------- Base zts_* API -----------------------------
    // ------------------------------------------------------------------------------

// Prototypes
void *zts_start_core_service(void *thread_id);
void zts_init_rpc(const char * path, const char * nwid);

//
int zts_start_proxy_server(const char *homepath, const char * nwid, struct sockaddr_storage * addr) {
    DEBUG_INFO();
    uint64_t nwid_int = strtoull(nwid, NULL, 16);
    ZeroTier::NetconEthernetTap * tap = zt1Service->getTap(nwid_int);
    if(tap) {
        if(tap->startProxyServer(homepath, nwid_int, addr) < 0) {
            DEBUG_ERROR("zts_start_proxy_server(%s): Problem while starting server.", nwid);
            return -1;
        }
    }
    DEBUG_ERROR("zts_start_proxy_server(%s): Invalid tap. Possibly incorrect NWID", nwid);
    return 0;
}
//
int zts_stop_proxy_server(const char *nwid) {
    DEBUG_INFO();
    uint64_t nwid_int = strtoull(nwid, NULL, 16);
    ZeroTier::NetconEthernetTap * tap = zt1Service->getTap(nwid_int);
    if(tap) {
        if(tap->stopProxyServer() < 0) {
            DEBUG_ERROR("zts_stop_proxy_server(%s): Problem while stopping server.", nwid);
            return -1;
        }
    }
    DEBUG_ERROR("zts_stop_proxy_server(%s): Invalid tap. Possibly incorrect NWID", nwid);
    return 0;
}
//
bool zts_proxy_is_running(const char *nwid)
{
    return false; // TODO
}
//
int zts_get_proxy_server_address(const char * nwid, struct sockaddr_storage * addr) {
    uint64_t nwid_int = strtoull(nwid, NULL, 16);
    ZeroTier::NetconEthernetTap * tap = zt1Service->getTap(nwid_int);
    if(tap) {
        tap->getProxyServerAddress(addr);
        return 0;
    }
    return -1;
}

// Basic ZT service controls
// Will also spin up a SOCKS5 proxy server if USE_SOCKS_PROXY is set
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
    // SOCKS5 Proxy server
    // Default is 127.0.0.1:RANDOM_PORT
    #if defined(USE_SOCKS_PROXY)
        zts_start_proxy_server(homeDir.c_str(), nwid, NULL); // NULL addr for default
    #endif
}
// Just create the dir and conf file required, don't instruct the core to do anything
void zts_join_network_soft(const char * filepath, const char * nwid) { 
    std::string net_dir = std::string(filepath) + "/networks.d/";
    std::string confFile = net_dir + std::string(nwid) + ".conf";
    if(!ZeroTier::OSUtils::mkdir(net_dir)) {
        DEBUG_ERROR("unable to create: %s", net_dir.c_str());
    }
    if(!ZeroTier::OSUtils::writeFile(confFile.c_str(), "")) {
        DEBUG_ERROR("unable to write network conf file: %s", confFile.c_str());
    }
}
//
void zts_leave_network(const char * nwid) { 
    if(zt1Service)
        zt1Service->leave(nwid); 
}
//
bool zts_service_is_running() { 
    return !zt1Service ? false : zt1Service->isRunning(); 
}
//
void zts_stop_service() {
    if(zt1Service) 
        zt1Service->terminate(); 
}
void zts_stop() {
    DEBUG_INFO("Stopping STSDK");
    zts_stop_service();
    /* TODO: kill each proxy server as well
    zts_stop_proxy_server(...); */
}

// FIXME: Re-implemented to make it play nicer with the C-linkage required for Xcode integrations
// Now only returns first assigned address per network. Shouldn't normally be a problem.

// Get IPV4 Address for this device on given network

bool zts_has_address(const char *nwid)
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
    ZeroTier::NetconEthernetTap *tap = zt1Service->getTap(nwid_int);
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
    ZeroTier::NetconEthernetTap *tap = zt1Service->getTap(nwid_int);
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
// Get device ID
int zts_get_device_id() 
{ 
    // zt->node->status
    /* TODO */ return 0; 
}
// 
bool zts_is_relayed() {
    // TODO
    // zt1Service->getNode()->peers()
    return false;
}
// Return the home path for this instance of ZeroTier
char *zts_get_homepath() {
    return (char*)givenHomeDir.c_str();
}


    // ------------------------------------------------------------------------------
    // ----------------------------- .NET Interop functions -------------------------
    // ------------------------------------------------------------------------------


#if defined(__UNITY_3D__)
    // .NET Interop-friendly debug mechanism
    typedef void (*FuncPtr)( const char * );
    FuncPtr Debug;
    void SetDebugFunction( FuncPtr fp ) { Debug = fp; }
    
    /*
    // Starts a ZeroTier service at the given path
    void unity_start_service(char * path, int len) {
        zts_start_service(path);

        //std::string dstr = std::string(path);
        //dstr = "unity_start_service(): path = " + dstr; 
        //Debug(dstr.c_str());
        //init_service(INTERCEPT_DISABLED, path);
    }
    // Starts a ZeroTier service and RPC
    void unity_start_service_and_rpc(char * path, char *nwid, int len) {
        std::string dstr = std::string(path);
        dstr = "unity_start_service_and_rpc(): path = " + dstr; 
        Debug(dstr.c_str());
        init_service_and_rpc(INTERCEPT_DISABLED, path, nwid);
    }
    */
#endif


    // ------------------------------------------------------------------------------
    // ----------------------------------- Other ------------------------------------
    // ------------------------------------------------------------------------------
    // For use when symbols are used in Swift


void zts_start_service(const char *path)
{
    DEBUG_INFO("path=%s", path);
    if(path)
        homeDir = path;
    zts_start_core_service(NULL);
}

// Typically used on iOS/OSX 
#if !defined(__ANDROID__)
/*
    // Starts a service thread and performs basic setup tasks
    void init_service(int key, const char * path) {
        givenHomeDir = path;
        pthread_key_create(&thr_id_key, NULL);
        intercept_thread_id = (int*)malloc(sizeof(int));
        *intercept_thread_id = key;
        pthread_create(&intercept_thread, NULL, zts_start_core_service, (void *)(intercept_thread_id));
    }
    
    //void init_service_and_rpc(int key, const char * path, const char * nwid) {
    //    rpcNWID = nwid;
    //    init_service(key, path);
    //}

    // Enables or disables intercept for current thread using key in thread-local storage
    void set_intercept_status(int mode) {
        #if defined(__APPLE__)
            DEBUG_INFO("mode=%d, tid=%d", mode, pthread_mach_thread_np(pthread_self()));
        #else
            // fprintf(stderr, "set_intercept_status(mode=%d): tid = %d\n", mode, gettid());
        #endif
        pthread_key_create(&thr_id_key, NULL);
        intercept_thread_id = (int*)malloc(sizeof(int));
        *intercept_thread_id = mode;
        pthread_setspecific(thr_id_key, intercept_thread_id);
    }
    */
#endif


    // ------------------------------------------------------------------------------
    // ------------------------------ EXPORTED JNI METHODS --------------------------
    // ------------------------------------------------------------------------------
    // JNI naming convention: Java_PACKAGENAME_CLASSNAME_METHODNAME


#if defined(__ANDROID__)
    // Starts a new service instance
    /* NOTE: Since on Android devices the sdcard is formatted as fat32, we can't use just any 
    location to set up the RPC unix domain socket. Rather we must use the application's specific 
    data directory given by getApplicationContext().getFilesDir() */
    JNIEXPORT int JNICALL Java_ZeroTier_ZTSDK_zt_1start_1service(JNIEnv *env, jobject thisObj, jstring path) {
        if(path)
            homeDir = env->GetStringUTFChars(path, NULL);
        zts_start_core_service(NULL);
    }
    // Shuts down ZeroTier service and SOCKS5 Proxy server
    JNIEXPORT void JNICALL Java_ZeroTier_ZTSDK_zt_1stop_1service(JNIEnv *env, jobject thisObj) {
        if(zt1Service)
            zts_stop_service();
        // TODO: Also terminate SOCKS5 Proxy
        // zts_stop_proxy_server();
    }
    // Returns whether the ZeroTier service is running
    JNIEXPORT jboolean JNICALL Java_ZeroTier_ZTSDK_zt_1service_1is_1running(JNIEnv *env, jobject thisObj) {
        if(zt1Service)
            return  zts_service_is_running();
        return false;
    }
    // Returns path for ZT config/data files    
    JNIEXPORT jstring JNICALL Java_ZeroTier_ZTSDK_zt_1get_1homepath(JNIEnv *env, jobject thisObj) {
        return (*env).NewStringUTF(zts_get_homepath());
    }
    // Join a network
    JNIEXPORT void JNICALL Java_ZeroTier_ZTSDK_zt_1join_1network(JNIEnv *env, jobject thisObj, jstring nwid) {
        const char *nwidstr;
        if(nwid) {
            nwidstr = env->GetStringUTFChars(nwid, NULL);
            zts_join_network(nwidstr);
        }
    }
    // Leave a network
    JNIEXPORT void JNICALL Java_ZeroTier_ZTSDK_zt_1leave_1network(JNIEnv *env, jobject thisObj, jstring nwid) {
        const char *nwidstr;
        if(nwid) {
            nwidstr = env->GetStringUTFChars(nwid, NULL);
            zts_leave_network(nwidstr);
        }
    }
    // FIXME: Re-implemented to make it play nicer with the C-linkage required for Xcode integrations
    // Now only returns first assigned address per network. Shouldn't normally be a problem
    JNIEXPORT jobject JNICALL Java_ZeroTier_ZTSDK_zt_1get_1ipv4_1address(JNIEnv *env, jobject thisObj, jstring nwid) {
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

    JNIEXPORT jobject JNICALL Java_ZeroTier_ZTSDK_zt_1get_1ipv6_1address(JNIEnv *env, jobject thisObj, jstring nwid) {
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
    JNIEXPORT jint Java_ZeroTier_ZTSDK_zt_1get_1device_1id() {
        return zts_get_device_id();
    }
    // Returns whether the path to an endpoint is currently relayed by a root server
    JNIEXPORT jboolean JNICALL Java_ZeroTier_ZTSDK_zt_1is_1relayed() {
        return zts_is_relayed();
    }
    // Starts a SOCKS5 proxy server for a given ZeroTier network
    JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1start_1proxy_1server(JNIEnv *env, jobject thisObj, jstring nwid, jobject ztaddr) {
        const char *nwidstr = env->GetStringUTFChars(nwid, NULL);
        struct sockaddr_in addr;
        // GET ZTAddress fields
        jclass cls = env->GetObjectClass(ztaddr);
        jfieldID fid = env->GetFieldID(cls, "port", "I");
        addr.sin_port = htons(env->GetIntField(ztaddr, fid));
        fid = env->GetFieldID(cls, "_rawAddr", "J");
        addr.sin_addr.s_addr = env->GetLongField(ztaddr, fid);
        return zts_start_proxy_server((char *)zts_get_homepath, nwidstr, (struct sockaddr_storage *)&addr);
    }
    //
    JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1stop_1proxy_1server(JNIEnv *env, jobject thisObj, jstring nwid) {
        return zts_stop_proxy_server((char*)env->GetStringUTFChars(nwid, NULL));
    }
    // Returns the local address of the SOCKS5 Proxy server
    JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1get_1proxy_1server_1address(JNIEnv *env, jobject thisObj, jstring nwid, jobject ztaddr) {
        struct sockaddr_in addr;
        int err = zts_get_proxy_server_address(env->GetStringUTFChars(nwid, NULL), (struct sockaddr_storage*)&addr);
        // SET ZTAddress fields
        jfieldID fid;
        jclass cls = env->GetObjectClass(ztaddr);
        fid = env->GetFieldID(cls, "port", "I");
        env->SetIntField(ztaddr, fid, addr.sin_port);
        fid = env->GetFieldID(cls,"_rawAddr", "J");
        env->SetLongField(ztaddr, fid,addr.sin_addr.s_addr);
        return err;    
    }
    //
    JNIEXPORT jboolean JNICALL Java_ZeroTier_ZTSDK_zt_1proxy_1is_1running(JNIEnv *env, jobject thisObj, jstring nwid) {
        // TODO: implement
    }
#endif


    // ------------------------------------------------------------------------------
    // ------------------------------- zts_start_service ----------------------------
    // ------------------------------------------------------------------------------


// Starts a ZeroTier service in the background
void *zts_start_core_service(void *thread_id) {

    #if defined(SDK_BUNDLED)
        if(thread_id)
            homeDir = std::string((char*)thread_id);
    #endif

    char current_dir[MAX_DIR_SZ];

    //#if defined(SDK_BUNDLED) && !defined(__ANDROID__)
    //    set_intercept_status(INTERCEPT_DISABLED); // Ignore network calls from ZT service
    //#endif

    #if defined(__IOS__)
        // Go to the app's data directory so we can shorten the sun_path we bind to
        getcwd(current_dir, MAX_DIR_SZ);
        std::string targetDir = homeDir; // + "/../../";
        chdir(targetDir.c_str());
        homeDir = localHomeDir;
    #endif

    #if defined(__UNITY_3D__)
        getcwd(current_dir, MAX_DIR_SZ);
        chdir(service_path.c_str());
        homeDir = current_dir; // homeDir shall be current_dir
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
