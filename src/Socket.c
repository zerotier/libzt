/*
 * ZeroTier One - Network Virtualization Everywhere
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

#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>

// For defining the Android direct-call API
#if defined(__ANDROID__) || defined(__JNI_LIB__)
    #include <jni.h>
#endif

#ifdef __cplusplus
    extern "C" {
#endif

    
#if defined(__linux__)
    #define SOCK_MAX (SOCK_PACKET + 1)
#endif
#define SOCK_TYPE_MASK 0xf

#include "ZeroTierSDK.h"
#include "RPC.h"

char *api_netpath;

/****************************************************************************/
/* zts_init_rpc()                                                           */
/****************************************************************************/

    int service_initialized = 0;
        
    // Assembles (and/or) sets the RPC path for communication with the ZeroTier service
    void zts_init_rpc(const char *path, const char *nwid)
    {
        // If no path, construct one or get it fron system env vars
        if(!api_netpath) {
            rpc_mutex_init();
            // Provided by user
            #if defined(SDK_BUNDLED)
                // Get the path/nwid from the user application
                // netpath = [path + "/nc_" + nwid] 
                char *fullpath = (char *)malloc(strlen(path)+strlen(nwid)+1+4);
                if(fullpath) {
                    zts_join_network_soft(path, nwid);
                    strcpy(fullpath, path);
                    strcat(fullpath, "/nc_");
                    strcat(fullpath, nwid);
                    api_netpath = fullpath;
                }
            // Provided by Env
            #else
                // Get path/nwid from environment variables
                if (!api_netpath) {
                    api_netpath = getenv("ZT_NC_NETWORK");
                    DEBUG_INFO("$ZT_NC_NETWORK=%s", api_netpath);
                }
            #endif
        }

        // start the SDK service if this is bundled
        #if defined(SDK_BUNDLED)
            if(!service_initialized) {
                DEBUG_ATTN("api_netpath = %s", api_netpath);
                pthread_t service_thread;
                pthread_create(&service_thread, NULL, zts_start_core_service, (void *)(path));
                service_initialized = 1;
                DEBUG_ATTN("waiting for service to assign address to network stack");
                // wait for zt service to assign the network stack an address
                sleep(1);
                while(!zts_has_address(nwid)) { usleep(1000); }
            }
        #endif
    }

    void get_api_netpath() { zts_init_rpc("",""); }

/****************************************************************************/
/* socket()                                                                 */
/****************************************************************************/

    // int socket_family, int socket_type, int protocol

#if defined(SDK_LANG_JAVA)
    JNIEXPORT jint JNICALL Java_zerotier_ZeroTier_zt_1socket(JNIEnv *env, jobject thisObj, jint family, jint type, jint protocol) {
        return zts_socket(family, type, protocol);
    }
#endif

#ifdef DYNAMIC_LIB
    int zt_socket(SOCKET_SIG)
#else
    int zts_socket(SOCKET_SIG)
#endif
    {
        get_api_netpath();
        DEBUG_INFO("");
        // Check that type makes sense
#if defined(__linux__) && !defined(__ANDROID__)
        int flags = socket_type & ~SOCK_TYPE_MASK;
        if (flags & ~(SOCK_CLOEXEC | SOCK_NONBLOCK)) {
            errno = EINVAL;
            return -1;
        }
#endif
        socket_type &= SOCK_TYPE_MASK;
        // Check protocol is in range
#if defined(__linux__)
        if (socket_family < 0 || socket_family >= NPROTO){
            errno = EAFNOSUPPORT;
            return -1;
        }
        if (socket_type < 0 || socket_type >= SOCK_MAX) {
            errno = EINVAL;
            return -1;
        }
#endif
        // Assemble and send RPC
        struct socket_st rpc_st;
        rpc_st.socket_family = socket_family;
        rpc_st.socket_type = socket_type;
        rpc_st.protocol = protocol;
        // -1 is passed since we we're generating the new socket in this call
        return rpc_send_command(api_netpath, RPC_SOCKET, -1, &rpc_st, sizeof(struct socket_st));
    }






#ifdef __cplusplus
    }
#endif