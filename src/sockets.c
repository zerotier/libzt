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

#ifdef USE_GNU_SOURCE
    #define _GNU_SOURCE
#endif

// For defining the Android direct-call API
#if defined(__ANDROID__)
    #include <jni.h>
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <strings.h>
#include <errno.h>
#include <stdarg.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
//#include <sys/socket.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <pthread.h>

#if defined(__linux__)
    #include <linux/errno.h>
    #include <sys/syscall.h>
    #include <linux/net.h>
#endif

#ifdef __cplusplus
    extern "C" {
#endif

#if defined(__linux__)
    #define SOCK_MAX (SOCK_PACKET + 1)
#endif
#define SOCK_TYPE_MASK 0xf

#include "sdk.h"
#include "debug.h"
#include "rpc.h"
#include "defs.h"
        
#include "Constants.hpp" // For Tap's MTU
    
// Prototypes
char *api_netpath = (char *)0;
void load_symbols();
void load_symbols_rpc();
int (*realclose)(CLOSE_SIG);

    // ------------------------------------------------------------------------------
    // ---------------------------------- zt_init_rpc -------------------------------
    // ------------------------------------------------------------------------------

    int service_initialized = 0;
        
    // Assembles (and/or) sets the RPC path for communication with the ZeroTier service
    void zts_init_rpc(const char *path, const char *nwid)
    {
        #if !defined(__IOS__)
            // Since we don't use function interposition in iOS 
            if(!realconnect) {
                load_symbols_rpc();
            }
        #endif
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
                pthread_t intercept_thread;
                pthread_create(&intercept_thread, NULL, zts_start_core_service, (void *)(path));
                service_initialized = 1;
                DEBUG_ATTN("waiting for service to assign address to network stack");
                // wait for zt service to assign the network stack an address
                sleep(1);
                while(!zts_has_address(nwid)) { usleep(1000); }
            }
        #endif
    }

    void get_api_netpath() { zts_init_rpc("",""); }
        
    // ------------------------------------------------------------------------------
    // ------------------------------------ send() ----------------------------------
    // ------------------------------------------------------------------------------
    // int fd, const void *buf, size_t len

#if defined(__ANDROID__)
    JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1send(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len, int flags)
    {
        jbyte *body = (*env)->GetByteArrayElements(env, buf, 0);
        char * bufp = (char *)malloc(sizeof(char)*len);
        memcpy(bufp, body, len);
        (*env)->ReleaseByteArrayElements(env, buf, body, 0);
        int written_bytes = write(fd, body, len);
        return written_bytes;
    }
#endif

    // ------------------------------------------------------------------------------
    // ------------------------------------ sendto() --------------------------------
    // ------------------------------------------------------------------------------
    // int fd, const void *buf, size_t len, int flags, 
    // const struct sockaddr *addr, socklen_t addrlen

#if defined(__ANDROID__)
    // TODO: Check result of each JNI call
    // UDP TX
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1sendto(
        JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len, jint flags, jobject ztaddr)
    {
        struct sockaddr_in addr;
        jclass cls = (*env)->GetObjectClass(env, ztaddr);
        jfieldID f = (*env)->GetFieldID(env, cls, "port", "I");
        addr.sin_port = htons((*env)->GetIntField(env, ztaddr, f));
        f = (*env)->GetFieldID(env, cls, "_rawAddr", "J");
        addr.sin_addr.s_addr = (*env)->GetLongField(env, ztaddr, f);
        addr.sin_family = AF_INET;
        //LOGV("zt_sendto(): fd = %d\naddr = %s\nport=%d", fd, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        // TODO: Optimize this
        jbyte *body = (*env)->GetByteArrayElements(env, buf, 0);
        char * bufp = (char *)malloc(sizeof(char)*len);
        memcpy(bufp, body, len);
        (*env)->ReleaseByteArrayElements(env, buf, body, 0);
        // "connect" and send buffer contents
        int sent_bytes = zts_sendto(fd, body, len, flags, (struct sockaddr *)&addr, sizeof(addr));
        return sent_bytes;
    }
#endif

//#if !defined(__ANDROID__)
    #ifdef DYNAMIC_LIB
        ssize_t zt_sendto(SENDTO_SIG) // Exposed as API
    #else
        ssize_t zts_sendto(SENDTO_SIG) // Used as internal implementation 
    #endif
        {
            if(len > ZT_UDP_DEFAULT_PAYLOAD_MTU) {
                errno = EMSGSIZE; // Msg is too large
                return -1;
            }
            int socktype = 0;
            socklen_t socktype_len;
            getsockopt(fd,SOL_SOCKET, SO_TYPE, (void*)&socktype, &socktype_len);

            if((socktype & SOCK_STREAM) || (socktype & SOCK_SEQPACKET)) {
                if(addr == NULL || flags != 0) {
                    errno = EISCONN;
                    return -1;
                }
            }
            // EMSGSIZE should be returned if the message is too long to be passed atomically through
            // the underlying protocol, in our case MTU?
            // TODO: More efficient solution
            // This connect call is used to get the address info to the stack for sending the packet
            int err;
            if((err = zts_connect(fd, addr, addrlen)) < 0) {
                DEBUG_ERROR("unknown problem passing address info to stack");
                errno = EISCONN; // double-check this is correct
                return -1;
            }
            return write(fd, buf, len);
        }
//#endif

    // ------------------------------------------------------------------------------
    // ----------------------------------- sendmsg() --------------------------------
    // ------------------------------------------------------------------------------
    // int fd, const struct msghdr *msg, int flags

#if !defined(__ANDROID__)
    #ifdef DYNAMIC_LIB
        ssize_t zt_sendmsg(SENDMSG_SIG)
    #else
        ssize_t zts_sendmsg(SENDMSG_SIG)
    #endif
        {
            //DEBUG_EXTRA("fd=%d",fd);
            char * p, * buf;
            size_t tot_len = 0;
            size_t err;
            struct iovec * iov = msg->msg_iov;
            for(int i=0; i<msg->msg_iovlen; ++i)
                tot_len += iov[i].iov_len;
            if(tot_len > ZT_UDP_DEFAULT_PAYLOAD_MTU) {
                errno = EMSGSIZE; // Message too large to send atomically via underlying protocol, don't send
                return -1;
            }
            buf = (char *)malloc(tot_len);
            if(tot_len != 0 && buf == NULL) {
                errno = ENOMEM; // Unable to allocate space for message
                return -1;
            }
            p = buf;
            for(int i=0; i < msg->msg_iovlen; ++i) {
                memcpy(p, iov[i].iov_base, iov[i].iov_len);
                p += iov[i].iov_len;
            }
            #if defined(__cplusplus)
                #if defined(__APPLE__)
                    err = sendto(fd, buf, tot_len, flags, (const struct sockaddr *)(msg->msg_name), msg->msg_namelen);
                #elif defined (__linux__)
                    err = sendto(fd, buf, tot_len, flags, (__CONST_SOCKADDR_ARG)(msg->msg_name), msg->msg_namelen);
                #endif
            #else
                err = sendto(fd, buf, tot_len, flags, msg->msg_name, msg->msg_namelen);
            #endif
            free(buf);
            return err;
        }
#endif
    
    // ------------------------------------------------------------------------------
    // ---------------------------------- recvfrom() --------------------------------
    // ------------------------------------------------------------------------------
    // int fd, void *restrict buf, size_t len, int flags, struct sockaddr
    // *restrict addr, socklen_t *restrict addrlen

#if defined(__ANDROID__)
    // UDP RX
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1recvfrom(
        JNIEnv *env, jobject thisObj, jint fd, jbyteArray buf, jint len, jint flags, jobject ztaddr)
    {
        struct sockaddr_in addr;
        jbyte *body = (*env)->GetByteArrayElements(env, buf, 0);
        unsigned char buffer[SDK_MTU];
        int payload_offset = sizeof(int) + sizeof(struct sockaddr_storage);
        int rxbytes = zts_recvfrom(fd, &buffer, len, flags, &addr, sizeof(struct sockaddr_storage));
        if(rxbytes > 0)
            memcpy(body, (jbyte*)buffer + payload_offset, rxbytes);
        (*env)->ReleaseByteArrayElements(env, buf, body, 0);
        // Update fields of Java ZTAddress object
        jfieldID fid;
        jclass cls = (*env)->GetObjectClass(env, ztaddr);
        fid = (*env)->GetFieldID(env, cls, "port", "I");
        (*env)->SetIntField(env, ztaddr, fid, addr.sin_port);
        fid = (*env)->GetFieldID(env, cls,"_rawAddr", "J");
        (*env)->SetLongField(env, ztaddr, fid,addr.sin_addr.s_addr);        
        return rxbytes;
    }
#endif

//#if !defined(__ANDROID__)
    #ifdef DYNAMIC_LIB
        ssize_t zt_recvfrom(RECVFROM_SIG)
    #else
        ssize_t zts_recvfrom(RECVFROM_SIG)
    #endif
        {
            int read_chunk_sz = 0, payload_offset, tmpsz=0, pnum=0; // payload size
            char tmpbuf[SDK_MTU];
            memset(tmpbuf, 0, SDK_MTU);
            
            // Attempt to read SDK_MTU sized chunk
            int total_read = 0, n=0;

            // Read the entire SDK_MTU-sized chunk from the service socket
            while(total_read < SDK_MTU) {
                n = read(fd, tmpbuf+total_read, SDK_MTU);
                
                if(n>0) {
                    total_read += n;
                    total_socket_rx += n;
                }
                else if (n < 0)
                    return n;
            }

            if(n > 0) {                
                // TODO: case for address size mismatch?
                memcpy(addr, tmpbuf, *addrlen);
                memcpy(&tmpsz, tmpbuf + sizeof(struct sockaddr_storage), sizeof(tmpsz));
                memcpy(&pnum, tmpbuf + sizeof(struct sockaddr_storage) + sizeof(int), sizeof(int));
                if(tmpsz > SDK_MTU || tmpsz < 0) {
                    DEBUG_ERROR("An error occured somewhere in the SDK, read=%d", n);
                    return -1;
                }
                payload_offset = sizeof(int) + sizeof(struct sockaddr_storage);

                // No matter how much we read from the service, only copy 'read_chunk_sz'
                // into the app's buffer
                read_chunk_sz = len < SDK_MTU ? len : SDK_MTU;
                read_chunk_sz = tmpsz < read_chunk_sz ? tmpsz : read_chunk_sz; // FIXME
                memcpy(buf, tmpbuf + payload_offset, read_chunk_sz);
            }
            else {
                return -1;
            }
            return read_chunk_sz;
        }
//#endif

    // ------------------------------------------------------------------------------
    // ----------------------------------- recvmsg() --------------------------------
    // ------------------------------------------------------------------------------
    // int fd, struct msghdr *msg, int flags

#if !defined(__ANDROID__)
    #ifdef DYNAMIC_LIB
        ssize_t zt_recvmsg(RECVMSG_SIG)
    #else
        ssize_t zts_recvmsg(RECVMSG_SIG)
    #endif
        {
            //DEBUG_EXTRA("fd=%d", fd);
            ssize_t err, n, tot_len = 0;
            char *buf, *p;
            struct iovec *iov = msg->msg_iov;
            
            for(int i = 0; i < msg->msg_iovlen; ++i)
                tot_len += iov[i].iov_len;
            buf = (char *)malloc(tot_len);
            if(tot_len != 0 && buf == NULL) {
                errno = ENOMEM;
                return -1;
            }
            #if defined(__cplusplus)
                #if defined(__APPLE__)
                    n = err = recvfrom(fd, buf, tot_len, flags, (struct sockaddr * __restrict)(msg->msg_name), &msg->msg_namelen);
                #elif defined(__linux__)
                    n = err = recvfrom(fd, buf, tot_len, flags, (__SOCKADDR_ARG)(msg->msg_name), &msg->msg_namelen);
                #endif
            #else
                n = err = recvfrom(fd, buf, tot_len, flags, msg->msg_name, &msg->msg_namelen);
            #endif
            p = buf;
            
            // According to: http://pubs.opengroup.org/onlinepubs/009695399/functions/recvmsg.html
            if(err > msg->msg_controllen && !( msg->msg_flags & MSG_PEEK)) {
                // excess data should be disgarded
                msg->msg_flags |= MSG_TRUNC; // Indicate that the buffer has been truncated
            }
            
            while (n > 0) {
                ssize_t count = n < iov->iov_len ? n : iov->iov_len;
                memcpy (iov->iov_base, p, count);
                p += count;
                n -= count;
                ++iov;
            }
            free(buf);
            return err;
        }
#endif

    // ------------------------------------------------------------------------------
    // --------------------- Exposed RX/TX API for .NET Wrapper ---------------------
    // ------------------------------------------------------------------------------
        
#if defined(__UNITY_3D__)
        // Just expose some basic calls for configuring and RX/TXing through ZT sockets
        ssize_t zt_send(int fd, void *buf, int len) {
            return write(fd, buf, len);
        }
        
        ssize_t zt_recv(int fd, void *buf, int len) {
            return read(fd, buf, len);
        }
        
        int zt_set_nonblock(int fd) {
            return 	fcntl(fd, F_SETFL, O_NONBLOCK);
        }
#endif

    // ------------------------------------------------------------------------------
    // ----------------------- Exposed RX/TX API for Java JNI -----------------------
    // ------------------------------------------------------------------------------

#if defined(__ANDROID__)
    // TCP TX
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1write(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len)
    {
        jbyte *body = (*env)->GetByteArrayElements(env, buf, 0);
        char * bufp = (char *)malloc(sizeof(char)*len);
        memcpy(bufp, body, len);
        (*env)->ReleaseByteArrayElements(env, buf, body, 0);
        int written_bytes = write(fd, body, len);
        return written_bytes;
    }
    // TCP RX
    JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1read(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len)
    {
        jbyte *body = (*env)->GetByteArrayElements(env, buf, 0);
        int read_bytes = read(fd, body, len);
        (*env)->ReleaseByteArrayElements(env, buf, body, 0);
        return read_bytes;
    }    
#endif

    // ------------------------------------------------------------------------------
    // --------------------------------- setsockopt() -------------------------------
    // ------------------------------------------------------------------------------
    // int fd, int level, int optname, const void *optval, socklen_t optlen

#if defined(__ANDROID__)
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1setsockopt(
        JNIEnv *env, jobject thisObj, jint fd, jint level, jint optname, jint optval, jint optlen) {
        return zts_setsockopt(fd, level, optname, optval, optlen);
    }
#endif

    #ifdef DYNAMIC_LIB
    int zt_setsockopt(SETSOCKOPT_SIG)
    #else
    int zts_setsockopt(SETSOCKOPT_SIG)
    #endif
    {
        DEBUG_INFO("fd=%d", fd);
        return 0;
    }
    
    // ------------------------------------------------------------------------------
    // --------------------------------- getsockopt() -------------------------------
    // ------------------------------------------------------------------------------
    // int fd, int level, int optname, void *optval, socklen_t *optlen 

#if defined(__ANDROID__)
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1getsockopt(
        JNIEnv *env, jobject thisObj, jint fd, jint level, jint optname, jint optval, jint optlen) {
        return zts_getsockopt(fd, level, optname, optval, optlen);
    }
#endif

#ifdef DYNAMIC_LIB
    int zt_getsockopt(GETSOCKOPT_SIG)
#else
    int zts_getsockopt(GETSOCKOPT_SIG)
#endif
    {
        DEBUG_INFO("fd=%d", fd);
        if(optname == SO_TYPE) {
            int* val = (int*)optval;
            *val = 2;
            optval = (void*)val;
        }
        return 0;
    }
    
    // ------------------------------------------------------------------------------
    // ----------------------------------- socket() ---------------------------------
    // ------------------------------------------------------------------------------
    // int socket_family, int socket_type, int protocol

#if defined(__ANDROID__)
    JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1socket(JNIEnv *env, jobject thisObj, jint family, jint type, jint protocol) {
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
        int err = rpc_send_command(api_netpath, RPC_SOCKET, -1, &rpc_st, sizeof(struct socket_st));
        DEBUG_INFO("err=%d", err);
        return err;
    }

    // ------------------------------------------------------------------------------
    // ---------------------------------- connect() ---------------------------------
    // ------------------------------------------------------------------------------
    // int fd, const struct sockaddr *addr, socklen_t addrlen

#if defined(__ANDROID__)
    JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1connect(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port) {
        struct sockaddr_in addr;
        const char *str = (*env)->GetStringUTFChars(env, addrstr, 0);
        DEBUG_INFO("fd=%d, addr=%s, port=%d", fd, str, port);
        addr.sin_addr.s_addr = inet_addr(str);
        addr.sin_family = AF_INET;
        addr.sin_port = htons( port );
        (*env)->ReleaseStringUTFChars(env, addrstr, str);
        return zts_connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    }
#endif

#ifdef DYNAMIC_LIB
    int zt_connect(CONNECT_SIG)
#else
    int zts_connect(CONNECT_SIG)
#endif
    {
        get_api_netpath();
        //DEBUG_INFO("fd=%d", fd);
        struct connect_st rpc_st;
        rpc_st.fd = fd;
        memcpy(&rpc_st.addr, addr, sizeof(struct sockaddr_storage));
        memcpy(&rpc_st.addrlen, &addrlen, sizeof(socklen_t));
        return rpc_send_command(api_netpath, RPC_CONNECT, fd, &rpc_st, sizeof(struct connect_st));
    }
    
    // ------------------------------------------------------------------------------
    // ------------------------------------ bind() ----------------------------------
    // ------------------------------------------------------------------------------
    // int fd, const struct sockaddr *addr, socklen_t addrlen

#if defined(__ANDROID__)
    JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1bind(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port) {
        struct sockaddr_in addr;
        const char *str = (*env)->GetStringUTFChars(env, addrstr, 0);
        DEBUG_INFO("fd=%d, addr=%s, port=%d", fd, str, port);
        addr.sin_addr.s_addr = inet_addr(str);
        addr.sin_family = AF_INET;
        addr.sin_port = htons( port );
        (*env)->ReleaseStringUTFChars(env, addrstr, str);
        return zts_bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    }
#endif

#ifdef DYNAMIC_LIB
    int zt_bind(BIND_SIG)
#else
    int zts_bind(BIND_SIG)
#endif
    {
        get_api_netpath();
        DEBUG_INFO("fd=%d", fd);
        struct bind_st rpc_st;
        rpc_st.fd = fd;
        memcpy(&rpc_st.addr, addr, sizeof(struct sockaddr_storage));
        memcpy(&rpc_st.addrlen, &addrlen, sizeof(socklen_t));
        return rpc_send_command(api_netpath, RPC_BIND, fd, &rpc_st, sizeof(struct bind_st));
    }

    // ------------------------------------------------------------------------------
    // ----------------------------------- accept4() --------------------------------
    // ------------------------------------------------------------------------------
    // int fd, struct sockaddr *addr, socklen_t *addrlen, int flags

#if defined(__ANDROID__)
    JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1accept4(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port, jint flags) {
        struct sockaddr_in addr;
        char *str;
        // = env->GetStringUTFChars(addrstr, NULL);
        (*env)->ReleaseStringUTFChars(env, addrstr, str);
        addr.sin_addr.s_addr = inet_addr(str);
        addr.sin_family = AF_INET;
        addr.sin_port = htons( port );
        return zts_accept4(fd, (struct sockaddr *)&addr, sizeof(addr), flags);
    }
#endif

#if defined(__linux__)
    #ifdef DYNAMIC_LIB
        int zt_accept4(ACCEPT4_SIG)
    #else
        int zts_accept4(ACCEPT4_SIG)
    #endif
        {
            get_api_netpath();
            DEBUG_INFO("fd=%d", fd);
        #if !defined(__ANDROID__)
            if ((flags & SOCK_CLOEXEC))
                fcntl(fd, F_SETFL, FD_CLOEXEC);
            if ((flags & SOCK_NONBLOCK))
               fcntl(fd, F_SETFL, O_NONBLOCK);
        #endif
            addrlen = !addr ? 0 : addrlen;
            return accept(fd, addr, addrlen);
        }
#endif
    
    // ------------------------------------------------------------------------------
    // ----------------------------------- accept() ---------------------------------
    // ------------------------------------------------------------------------------
    // int fd struct sockaddr *addr, socklen_t *addrlen

#if defined(__ANDROID__)
    JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1accept(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port) {
        struct sockaddr_in addr;
        // TODO: Send addr info back to Javaland
        addr.sin_addr.s_addr = inet_addr("");
        addr.sin_family = AF_INET;
        addr.sin_port = htons( port );
        return zts_accept(fd, (struct sockaddr *)&addr, sizeof(addr));    
    }
#endif

#ifdef DYNAMIC_LIB
    int zt_accept(ACCEPT_SIG)
#else
    int zts_accept(ACCEPT_SIG)
#endif
    {
        get_api_netpath();
        DEBUG_INFO("fd=%d", fd);
    #if !defined(__UNITY_3D__)
        if(addr)
            addr->sa_family = AF_INET;
    #endif
        int new_fd = get_new_fd(fd);
        DEBUG_INFO("newfd=%d", new_fd);

        if(new_fd > 0) {
            errno = ERR_OK;
            return new_fd;
        }
        errno = EAGAIN;
        return -EAGAIN;
    }
    
    // ------------------------------------------------------------------------------
    // ------------------------------------- listen()--------------------------------
    // ------------------------------------------------------------------------------
    // int fd, int backlog

#if defined(__ANDROID__)
    JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1listen(JNIEnv *env, jobject thisObj, jint fd, int backlog) {
        return zts_listen(fd, backlog);
    }
#endif

#ifdef DYNAMIC_LIB
    int zt_listen(LISTEN_SIG)
#else
    int zts_listen(LISTEN_SIG)
#endif
    {
        get_api_netpath();
        DEBUG_INFO("fd=%d", fd);
        struct listen_st rpc_st;
        rpc_st.fd = fd;
        rpc_st.backlog = backlog;
        return rpc_send_command(api_netpath, RPC_LISTEN, fd, &rpc_st, sizeof(struct listen_st));
    }
    
    // ------------------------------------------------------------------------------
    // ------------------------------------- close() --------------------------------
    // ------------------------------------------------------------------------------
    // int fd

#if defined(__ANDROID__)
    JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1close(JNIEnv *env, jobject thisObj, jint fd) {
        return zts_close(fd);
    }
#endif

#ifdef DYNAMIC_LIB
    int zt_close(CLOSE_SIG) 
#else
    int zts_close(CLOSE_SIG) 
#endif
    {
        get_api_netpath();
        DEBUG_INFO("fd=%d", fd);
        return realclose(fd);
    }
    
    // ------------------------------------------------------------------------------
    // -------------------------------- getsockname() -------------------------------
    // ------------------------------------------------------------------------------
    // int fd, struct sockaddr *addr, socklen_t *addrlen
    
#if defined(__ANDROID__)
    JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1getsockname(JNIEnv *env, jobject thisObj, jint fd, jobject ztaddr) {
        struct sockaddr_in addr;
        int err = zts_getsockname(fd, &addr, sizeof(struct sockaddr));
        jfieldID fid;
        jclass cls = (*env)->GetObjectClass(env, ztaddr);
        fid = (*env)->GetFieldID(env, cls, "port", "I");
        (*env)->SetIntField(env, ztaddr, fid, addr.sin_port);
        fid = (*env)->GetFieldID(env, cls,"_rawAddr", "J");
        (*env)->SetLongField(env, ztaddr, fid,addr.sin_addr.s_addr);        
        return err;    
    }
#endif

#ifdef DYNAMIC_LIB
    int zt_getsockname(GETSOCKNAME_SIG)
#else
    int zts_getsockname(GETSOCKNAME_SIG)
#endif
    {
        get_api_netpath();
        DEBUG_EXTRA("fd=%d", fd);
        struct getsockname_st rpc_st;
        rpc_st.fd = fd;
        memcpy(&rpc_st.addrlen, &addrlen, sizeof(socklen_t));
        int rpcfd = rpc_send_command(api_netpath, RPC_GETSOCKNAME, fd, &rpc_st, sizeof(struct getsockname_st));
        // read address info from service
        char addrbuf[sizeof(struct sockaddr_storage)];
        memset(&addrbuf, 0, sizeof(struct sockaddr_storage));
        if(rpcfd > -1) {
            int err = read(rpcfd, &addrbuf, sizeof(struct sockaddr));
            close(rpcfd);
            if(err > 0) {
                int sum=0;
                for(int i=0;i<sizeof(struct sockaddr_storage);i++) {
                    sum|=addrbuf[i];
                }
                if(!sum) { // RXed a zero-ed address buffer, currently the only way to signal a problem
                    errno = ENOTSOCK; // TODO: general error, needs to be more specific
                    DEBUG_ERROR("no address info given by service.");
                    return -1;
                }
            } 
            else {
                errno = ENOTSOCK; // TODO: general error, needs to be more specific
                DEBUG_ERROR("unable to read address info from service. err=%d", err);
                return -1;
            }
        }
        struct sockaddr_storage sock_storage;
        memcpy(&sock_storage, addrbuf, sizeof(struct sockaddr));
        *addrlen = sizeof(struct sockaddr_in);
        memcpy(addr, &sock_storage, sizeof(struct sockaddr));
        addr->sa_family = AF_INET;
        return 0;
    }

    // ------------------------------------------------------------------------------
    // -------------------------------- getpeername() -------------------------------
    // ------------------------------------------------------------------------------
    // int fd, struct sockaddr *addr, socklen_t *addrlen
    
#if defined(__ANDROID__)
    JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1getpeername(JNIEnv *env, jobject thisObj, jint fd, jobject ztaddr) {
        struct sockaddr_in addr;
        int err = zts_getpeername(fd, &addr, sizeof(struct sockaddr));
        jfieldID fid;
        jclass cls = (*env)->GetObjectClass(env, ztaddr);
        fid = (*env)->GetFieldID(env, cls, "port", "I");
        (*env)->SetIntField(env, ztaddr, fid, addr.sin_port);
        fid = (*env)->GetFieldID(env, cls,"_rawAddr", "J");
        (*env)->SetLongField(env, ztaddr, fid,addr.sin_addr.s_addr);        
        return err;
    }
#endif

#ifdef DYNAMIC_LIB
    int zt_getpeername(GETSOCKNAME_SIG)
#else
    int zts_getpeername(GETSOCKNAME_SIG)
#endif
    {
        get_api_netpath();
        DEBUG_EXTRA("fd=%d", fd);
        struct getsockname_st rpc_st;
        rpc_st.fd = fd;
        memcpy(&rpc_st.addrlen, &addrlen, sizeof(socklen_t));
        int rpcfd = rpc_send_command(api_netpath, RPC_GETPEERNAME, fd, &rpc_st, sizeof(struct getsockname_st));
        // read address info from service
        char addrbuf[sizeof(struct sockaddr_storage)];
        memset(&addrbuf, 0, sizeof(struct sockaddr_storage));

        if(rpcfd > -1) {
            int err = read(rpcfd, &addrbuf, sizeof(struct sockaddr));
            close(rpcfd);
            if(err > 0) {
                int sum=0;
                for(int i=0;i<sizeof(struct sockaddr_storage);i++) {
                    sum|=addrbuf[i];
                }
                if(!sum) { // RXed a zero-ed address buffer, currently the only way to signal a problem
                    errno = ENOTSOCK; // TODO: general error, needs to be more specific
                    DEBUG_ERROR("no address info given by service.");
                    return -1;
                }
            } 
            else {
                errno = ENOTSOCK; // TODO: general error, needs to be more specific
                DEBUG_ERROR("unable to read address info from service. err=%d", err);
                return -1;
            }
        }
        struct sockaddr_storage sock_storage;
        memcpy(&sock_storage, addrbuf, sizeof(struct sockaddr));
        *addrlen = sizeof(struct sockaddr_in);
        memcpy(addr, &sock_storage, sizeof(struct sockaddr));
        addr->sa_family = AF_INET;
        return 0;
    }

    // ------------------------------------------------------------------------------
    // ------------------------------------ fcntl() ---------------------------------
    // ------------------------------------------------------------------------------
    // int fd, int cmd, int flags

    #if defined(__ANDROID__)
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1fcntl(JNIEnv *env, jobject thisObj, jint fd, jint cmd, jint flags) {
        return zts_fcntl(fd,cmd,flags);
    }
#endif

#ifdef DYNAMIC_LIB
    int zt_fcntl(FCNTL_SIG)
#else
    int zts_fcntl(FCNTL_SIG)
#endif
    {
        DEBUG_EXTRA("fd=%d, cmd=%d, flags=%d", fd, cmd, flags);
        return fcntl(fd,cmd,flags);
    }

#ifdef __cplusplus
}
#endif