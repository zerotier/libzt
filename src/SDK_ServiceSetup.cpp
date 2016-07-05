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

#include "service/OneService.hpp"
#include "Utils.hpp"
#include "OSUtils.hpp"

#include "SDK.h"
void zt_init_rpc(const char * path, const char * nwid);

#include "SDK_Debug.h"
#include "SDK_ServiceSetup.hpp"

std::string service_path;
pthread_t intercept_thread;
int * intercept_thread_id;
pthread_key_t thr_id_key;
static ZeroTier::OneService *volatile zt1Service;
std::string homeDir;
std::string netDir;

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__UNITY_3D__)
    // .NET Interop-friendly debug mechanism
    typedef void (*FuncPtr)( const char * );
    FuncPtr Debug;
    
    void SetDebugFunction( FuncPtr fp ) { Debug = fp; }
    
    // Starts a service at the specified path
    void unity_start_service(char * path, int len) {
        Debug(path);
        zt_init_rpc(path);
        init_service(INTERCEPT_DISABLED, path);
    }
#endif
    
    void join_network(const char * nwid)
    {
        std::string confFile = netDir + "/" + nwid + ".conf";
        if(!ZeroTier::OSUtils::mkdir(netDir)) {
            LOGV("unable to create %s\n", netDir.c_str());
        }
        if(!ZeroTier::OSUtils::writeFile(confFile.c_str(), "")) {
            LOGV("unable to write network conf file: %s\n", nwid);
        }
        zt1Service->join(nwid);
        zt_init_rpc(homeDir.c_str(), nwid); // This provides the shim API with the RPC information
    }
    
    void leave_network(const char *nwid) { zt1Service->leave(nwid); }
    void zt_join_network(char * nwid) { join_network(nwid); }
    void zt_leave_network(char * nwid) { leave_network(nwid); }
    bool zt_is_running() { return zt1Service->isRunning(); }
    void zt_terminate() { zt1Service->terminate(); }
    
#if !defined(__ANDROID__)
    /*
     * Starts a service thread and performs basic setup tasks
     */
    void init_service(int key, const char * path)
    {
        homeDir = path;
        pthread_key_create(&thr_id_key, NULL);
        intercept_thread_id = (int*)malloc(sizeof(int));
        *intercept_thread_id = key;
        pthread_create(&intercept_thread, NULL, startOneService, (void *)(intercept_thread_id));
    }

    /*
     * Enables or disables intercept for current thread using key in thread-local storage
     */
    void set_intercept_status(int mode)
    {
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
    // JNI naming convention: Java_PACKAGENAME_CLASSNAME_METHODNAME
    JNIEXPORT void JNICALL Java_ZeroTier_SDK_startOneService(JNIEnv *env, jobject thisObj, jstring path) {
        //char * path;
        homeDir = (*env)->GetStringUTFChars(env, path, NULL);
#else
        void *startOneService(void *thread_id) {
#endif

    #if defined(SDK_BUNDLED)
        // Don't intercept network calls originating from ZeroTier service
        set_intercept_status(INTERCEPT_DISABLED);
    #endif
        
        // If no homeDir is provided, attempt a best guess of an appropriate ZeroTier 
        // homeDir according to platform and build type
        if(!homeDir.length())
        {
            #if defined(__UNITY_3D__)
                int MAX_DIR_SZ = 256;
                char current_dir[MAX_DIR_SZ];
                getcwd(current_dir, MAX_DIR_SZ);
                chdir(service_path.c_str());
            #endif
        
            #if defined(__UNITY_3D__) && !defined(__ANDROID__) && !defined(__IOS__)
                // Unity3D on a non-mobile platform
                homeDir = "" + current_dir; // homeDir shall be current dir 
            #endif

            #if defined(__ANDROID__)
                homeDir = "/sdcard/zerotier";
            #endif

            #if defined(__APPLE__)
                #include "TargetConditionals.h"
                #if TARGET_IPHONE_SIMULATOR
                        // homeDir = "dont/run/this/in/the/simulator";
                #elif TARGET_OS_IPHONE
                        homeDir = "ZeroTier/One";
                #endif
            #endif
        }

        LOGV("homeDir = %s", homeDir.c_str());
        // Where network .conf files will be stored
        netDir = homeDir + "/networks.d";

        zt1Service = (ZeroTier::OneService *)0;
        LOGV("Starting ZT service...\n");
        
        if (!homeDir.length()) {
            #if defined(__ANDROID__)
                return;
            #else
                return NULL;
            #endif
        } else {
            LOGV("constructing path...\n");

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
                        std::string homePathErrStr = "home path does not exist, and could not create";
                        throw std::runtime_error(homePathErrStr);
                    }
                }
            }
        }

        //chdir(current_dir); // Return to previous current working directory (at the request of Unity3D)
        
        //Debug(homeDir.c_str());
        
        // Generate random port for new service instance
        unsigned int randp = 0;
        ZeroTier::Utils::getSecureRandom(&randp,sizeof(randp));
        int servicePort = 9000 + (randp % 1000);
        
        LOGV("generated port\n");
        
        for(;;) {
            zt1Service = ZeroTier::OneService::newInstance(homeDir.c_str(),servicePort);
            LOGV("created new instance\n");

            switch(zt1Service->run()) {
                case ZeroTier::OneService::ONE_STILL_RUNNING: // shouldn't happen, run() won't return until done
                case ZeroTier::OneService::ONE_NORMAL_TERMINATION:
                    break;
                case ZeroTier::OneService::ONE_UNRECOVERABLE_ERROR:
                    //fprintf(stderr,"%s: fatal error: %s" ZT_EOL_S,argv[0],zt1Service->fatalErrorMessage().c_str());
                    //returnValue = 1;
                    break;
                case ZeroTier::OneService::ONE_IDENTITY_COLLISION: {
                    delete zt1Service;
                    zt1Service = (ZeroTier::OneService *)0;
                    std::string oldid;
                    //OSUtils::readFile((homeDir + ZT_PATH_SEPARATOR_S + "identity.secret").c_str(),oldid);
                    if (oldid.length()) {
                        //OSUtils::writeFile((homeDir + ZT_PATH_SEPARATOR_S + "identity.secret.saved_after_collision").c_str(),oldid);
                        //OSUtils::rm((homeDir + ZT_PATH_SEPARATOR_S + "identity.secret").c_str());
                        //OSUtils::rm((homeDir + ZT_PATH_SEPARATOR_S + "identity.public").c_str());
                    }
                }   continue; // restart!
            }
            break; // terminate loop -- normally we don't keep restarting
        }
        #if defined(__ANDROID__)
            return;
        #else
            return NULL;
        #endif
    }

#ifdef __cplusplus
}
#endif
