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

#include "SDK_EthernetTap.hpp"
#include "SDK.h"
#include "SDK_Debug.h"

std::string service_path;
pthread_t intercept_thread;
int * intercept_thread_id;
pthread_key_t thr_id_key;
static ZeroTier::OneService *volatile zt1Service;

std::string localHomeDir; // Local shortened path
std::string givenHomeDir; // What the user/application provides as a suggestion
std::string homeDir; // The resultant platform-specific dir we *must* use internally
std::string netDir;
std::string rpcNWID;

bool rpcEnabled;

#ifdef __cplusplus
extern "C" {
#endif

void zt_init_rpc(const char * path, const char * nwid);
void dwr(int level, const char *fmt, ... );

#if defined(__UNITY_3D__)
    // .NET Interop-friendly debug mechanism
    typedef void (*FuncPtr)( const char * );
    FuncPtr Debug;
    void SetDebugFunction( FuncPtr fp ) { Debug = fp; }
    
    // Starts a ZeroTier service at the given path
    void unity_start_service(char * path, int len) {
        std::string dstr = std::string(path);
        dstr = "unity_start_service(): path = " + dstr; 
        Debug(dstr.c_str());
        init_service(INTERCEPT_DISABLED, path);
    }
    // Starts a ZeroTier service and RPC
    void unity_start_service_and_rpc(char * path, char *nwid, int len) {
        std::string dstr = std::string(path);
        dstr = "unity_start_service_and_rpc(): path = " + dstr; 
        Debug(dstr.c_str());
        init_service_and_rpc(INTERCEPT_DISABLED, path, nwid);
    }
#endif
    

int zts_start_proxy_server(const char *homepath, const char * nwid, struct sockaddr_storage * addr) {
        LOGV("zts_start_proxy_server\n");

    dwr(MSG_DEBUG, "zts_start_proxy_server()");
    uint64_t nwid_int = strtoull(nwid, NULL, 16);
    ZeroTier::NetconEthernetTap * tap = zt1Service->getTaps()[nwid_int];
    if(tap) {
        if(tap->startProxyServer(homepath, nwid_int, addr) < 0) {
            dwr(MSG_ERROR, "zts_start_proxy_server(%s): Problem while starting server.", nwid);
            return -1;
        }
    }
    dwr(MSG_ERROR, "zts_start_proxy_server(%s): Invalid tap. Possibly incorrect NWID", nwid);
    return 0;
}
int zts_stop_proxy_server(const char *nwid) {
    dwr(MSG_DEBUG, "zts_stop_proxy_server()");
    uint64_t nwid_int = strtoull(nwid, NULL, 16);
    ZeroTier::NetconEthernetTap * tap = zt1Service->getTaps()[nwid_int];
    if(tap) {
        if(tap->stopProxyServer() < 0) {
            dwr(MSG_ERROR, "zts_stop_proxy_server(%s): Problem while stopping server.", nwid);
            return -1;
        }
    }
    dwr(MSG_ERROR, "zts_stop_proxy_server(%s): Invalid tap. Possibly incorrect NWID", nwid);
    return 0;
}
int zts_get_proxy_server_address(const char * nwid, struct sockaddr_storage * addr) {
    uint64_t nwid_int = strtoull(nwid, NULL, 16);
    ZeroTier::NetconEthernetTap * tap = zt1Service->getTaps()[nwid_int];
    if(tap) {
        tap->getProxyServerAddress(addr);
        return 0;
    }
    return -1;
}

// Basic ZT service controls
// Will also spin up a SOCKS5 proxy server if USE_SOCKS_PROXY is set
void zts_join_network(const char * nwid) { 
    LOGV("zts_join_network\n");
    std::string confFile = zt1Service->givenHomePath() + "/networks.d/" + nwid + ".conf";
    if(!ZeroTier::OSUtils::mkdir(netDir)) {
        dwr(MSG_ERROR, "unable to create %s\n", netDir.c_str());
    }
    if(!ZeroTier::OSUtils::writeFile(confFile.c_str(), "")) {
        dwr(MSG_ERROR, "unable to write network conf file: %s\n", confFile.c_str());
    }
    // Provide the API with the RPC information
    zt_init_rpc(homeDir.c_str(), nwid); 

    // SOCKS5 Proxy server
    // Default is 127.0.0.1:RANDOM_PORT
    #if defined(USE_SOCKS_PROXY)
        zts_start_proxy_server(homeDir.c_str(), nwid, NULL); // NULL addr for default
    #endif
}
void zts_leave_network(const char * nwid) { zt1Service->leave(nwid); }
bool zts_is_running() { return zt1Service->isRunning(); }
void zts_stop_service() { zt1Service->terminate(); }

// FIXME: Re-implemented to make it play nicer with the C-linkage required for Xcode integrations
// Now only returns first assigned address per network. Shouldn't normally be a problem
void zts_get_addresses(const char * nwid, char *addrstr)
{
    uint64_t nwid_int = strtoull(nwid, NULL, 16);
    ZeroTier::NetconEthernetTap * tap = zt1Service->getTaps()[nwid_int];
    if(tap && tap->_ips.size()){ 
        std::string addr = tap->_ips[0].toString();
        memcpy(addrstr, addr.c_str(), addr.length());  
    }
    else {
        memcpy(addrstr, "-1.-1.-1.-1/-1", 14);
    }
    /*
    uint64_t nwid_int = strtoull(nwid.c_str(), NULL, 16);
    ZeroTier::NetconEthernetTap * tap = zt1Service->getTaps()[nwid_int];
    std::vector<std::string> ip_strings;
    if(tap) {
        for(int i=0; i<tap->_ips.size(); i++) {
            ip_strings.push_back(tap->_ips[i].toString());
        }
        return ip_strings;
    }
    return ip_strings;
    */
}
int zts_get_device_id() { /* TODO */ return 0; }

bool zts_is_relayed() {
    // TODO
    // zt1Service->getNode()->peers()
    return false;
}

char *zts_get_homepath() {
    return (char*)givenHomeDir.c_str();
}

// Android JNI wrapper
// JNI naming convention: Java_PACKAGENAME_CLASSNAME_METHODNAME
#if defined(__ANDROID__)
    // Join a network
    JNIEXPORT void JNICALL Java_ZeroTier_SDK_zt_1join_1network(JNIEnv *env, jobject thisObj, jstring nwid) {
        const char *nwidstr;
        if(nwid) {
            nwidstr = env->GetStringUTFChars(nwid, NULL);
            zts_join_network(nwidstr);
        }
    }
    // Leave a network
    JNIEXPORT void JNICALL Java_ZeroTier_SDK_zt_1leave_1network(JNIEnv *env, jobject thisObj, jstring nwid) {
        const char *nwidstr;
        if(nwid) {
            nwidstr = env->GetStringUTFChars(nwid, NULL);
            zts_leave_network(nwidstr);
        }
    }
    // Returns whether the ZeroTier service is running
    JNIEXPORT jboolean JNICALL Java_ZeroTier_SDK_zt_1running(JNIEnv *env, jobject thisObj) {
        if(zt1Service)
            return  zts_is_running();
        return false;
    }
    // Shuts down ZeroTier service and SOCKS5 Proxy server
    JNIEXPORT void JNICALL Java_ZeroTier_SDK_zt_1stop_1service(JNIEnv *env, jobject thisObj) {
        if(zt1Service)
            zts_stop_service();
        // TODO: Also terminate SOCKS5 Proxy
        // zts_stop_proxy_server();
    }
    // FIXME: Re-implemented to make it play nicer with the C-linkage required for Xcode integrations
    // Now only returns first assigned address per network. Shouldn't normally be a problem
    JNIEXPORT jobject JNICALL Java_ZeroTier_SDK_zt_1get_1addresses(JNIEnv *env, jobject thisObj, jstring nwid) {
        const char *nwid_str = env->GetStringUTFChars(nwid, NULL);
        char address_string[32];
        memset(address_string, 0, 32);
        zts_get_addresses(nwid_str, address_string);
        jclass clazz = (*env).FindClass("java/util/ArrayList");
        jobject addresses = (*env).NewObject(clazz, (*env).GetMethodID(clazz, "<init>", "()V"));        
        jstring _str = (*env).NewStringUTF(address_string);
        env->CallBooleanMethod(addresses, env->GetMethodID(clazz, "add", "(Ljava/lang/Object;)Z"), _str);
        return addresses;
        /*
        const char *nwid_str = env->GetStringUTFChars(nwid, NULL);
        std::vector<std::string> address_strings = zts_get_addresses(nwid_str);
        jclass clazz = (*env).FindClass("java/util/ArrayList");
        jobject addresses = (*env).NewObject(clazz, (*env).GetMethodID(clazz, "<init>", "()V"));
        if(address_strings.size()) {
            for (int n=0;n<address_strings.size();n++) {
                jstring _str = (*env).NewStringUTF(address_strings[n].c_str());
                env->CallBooleanMethod(addresses, env->GetMethodID(clazz, "add", "(Ljava/lang/Object;)Z"), _str);
            }
        }
        else { // Add dummy value 
            jstring _str = (*env).NewStringUTF("-1.-1.-1.-1/-1");
            env->CallBooleanMethod(addresses, env->GetMethodID(clazz, "add", "(Ljava/lang/Object;)Z"), _str);
        }
        return addresses; 
        */
	}
    // Returns the device is in integer form
    JNIEXPORT jint Java_ZeroTier_SDK_zt_1get_1device_1id() {
        return zts_get_device_id();
    }
    // Returns whether the path to an endpoint is currently relayed by a root server
    JNIEXPORT jboolean JNICALL Java_ZeroTier_SDK_zt_1is_1relayed() {
        return zts_is_relayed();
    }
    // Returns the local address of the SOCKS5 Proxy server
    JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1get_1proxy_1server_1address(JNIEnv *env, jobject thisObj, jstring nwid, jobject ztaddr) {
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
    // Starts a SOCKS5 proxy server for a given ZeroTier network
    JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1start_1proxy_1server(JNIEnv *env, jobject thisObj, jstring nwid, jobject ztaddr) {
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
    JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1stop_1proxy_1server(JNIEnv *env, jobject thisObj, jstring nwid) {
        return zts_stop_proxy_server((char*)env->GetStringUTFChars(nwid, NULL));
    }
    JNIEXPORT jstring JNICALL Java_ZeroTier_SDK_zt_1get_1homepath(JNIEnv *env, jobject thisObj) {
        return (*env).NewStringUTF(zts_get_homepath());
    }
#endif


// Typically used on iOS/OSX 
#if !defined(__ANDROID__)
    /*
     * Starts a service thread and performs basic setup tasks
     */
    void init_service(int key, const char * path) {
        givenHomeDir = path;
        pthread_key_create(&thr_id_key, NULL);
        intercept_thread_id = (int*)malloc(sizeof(int));
        *intercept_thread_id = key;
        pthread_create(&intercept_thread, NULL, zt_start_service, (void *)(intercept_thread_id));
    }
    void init_service_and_rpc(int key, const char * path, const char * nwid) {
        rpcEnabled = true;
        rpcNWID = nwid;
        init_service(key, path);
    }
    /*
     * Enables or disables intercept for current thread using key in thread-local storage
     */
    void set_intercept_status(int mode) {
        fprintf(stderr, "set_intercept_status(mode=%d): tid = %d\n", mode, pthread_mach_thread_np(pthread_self()));
        pthread_key_create(&thr_id_key, NULL);
        intercept_thread_id = (int*)malloc(sizeof(int));
        *intercept_thread_id = mode;
        pthread_setspecific(thr_id_key, intercept_thread_id);
    }
#endif

/*
 * Starts a new service instance
 */
#if defined(__ANDROID__)
    /* NOTE: Since on Android devices the sdcard is formatted as fat32, we can't use just any 
    location to set up the RPC unix domain socket. Rather we must use the application's specific 
    data directory given by getApplicationContext().getFilesDir() */
    JNIEXPORT int JNICALL Java_ZeroTier_SDK_zt_1start_1service(JNIEnv *env, jobject thisObj, jstring path) {
        if(path)
            homeDir = env->GetStringUTFChars(path, NULL);
#else
        void *zt_start_service(void *thread_id) {
#endif

    #if defined(SDK_BUNDLED) && !defined(__ANDROID__)
        set_intercept_status(INTERCEPT_DISABLED); // Ignore network calls from ZT service
    #endif

        #if defined(__UNITY_3D__)
            int MAX_DIR_SZ = 256;
            char current_dir[MAX_DIR_SZ];
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
            homeDir = givenHomeDir;
            localHomeDir = givenHomeDir; // Used for RPC and *can* differ from homeDir on some platforms
        #endif

        dwr(MSG_DEBUG, "homeDir = %s", givenHomeDir.c_str());
        // Where network .conf files will be stored
        netDir = homeDir + "/networks.d";
        zt1Service = (ZeroTier::OneService *)0;
        
        // Construct path for network config and supporting service files
        if (homeDir.length()) {
            dwr(MSG_DEBUG, "start_service(): constructing path...\n");
            std::vector<std::string> hpsp(ZeroTier::Utils::split(homeDir.c_str(),ZT_PATH_SEPARATOR_S,"",""));
            std::string ptmp;
            if (homeDir[0] == ZT_PATH_SEPARATOR)
                ptmp.push_back(ZT_PATH_SEPARATOR);
            for(std::vector<std::string>::iterator pi(hpsp.begin());pi!=hpsp.end();++pi) {
                if (ptmp.length() > 0)
                    ptmp.push_back(ZT_PATH_SEPARATOR);
                ptmp.append(*pi);
                if ((*pi != ".")&&(*pi != "..")) {
                    if (!ZeroTier::OSUtils::mkdir(ptmp)) {
                        dwr(MSG_ERROR, "startOneService(): home path does not exist, and could not create\n");
                    }
                }
            }
        }
        else {
            fprintf(stderr, "start_service(): homeDir is empty, could not construct path\n");
            return NULL;
        }

        #if defined(__IOS__)
            // Go to the app's data directory so we can shorten the sun_path we bind to
            int MAX_DIR_SZ = 256;
            char current_dir[MAX_DIR_SZ];
            getcwd(current_dir, MAX_DIR_SZ);
            std::string targetDir = homeDir + "/../../";
            chdir(targetDir.c_str());
            homeDir = localHomeDir;
        #endif

        //chdir(current_dir); // Return to previous current working directory (at the request of Unity3D)
        #if defined(__UNITY_3D__)
            Debug("Starting service...\n");
        #endif

        // Initialize RPC 
        if(rpcEnabled) {
            zt_init_rpc(localHomeDir.c_str(), rpcNWID.c_str());
        }

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
                    fprintf(stderr,"start_service(): fatal error: %s",zt1Service->fatalErrorMessage().c_str());
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
