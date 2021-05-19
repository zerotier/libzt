/*
 * Copyright (c)2013-2021 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2026-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

/**
 * @file
 *
 * ZeroTier C Java Socket API
 */

#ifdef ZTS_ENABLE_JAVA

#include "Events.hpp"
#include "ZeroTierSockets.h"
#include "lwip/def.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "lwip/stats.h"

#include <jni.h>

extern int zts_errno;

namespace ZeroTier {

extern uint8_t _state;
extern JavaVM* jvm;

void java_detach_from_thread()
{
    jint rs = jvm->DetachCurrentThread();
}

#ifdef __cplusplus
extern "C" {
#endif

void ss2zta(JNIEnv* env, struct zts_sockaddr_storage* ss, jobject addr);
void zta2ss(JNIEnv* env, struct zts_sockaddr_storage* ss, jobject addr);
void ztfdset2fdset(JNIEnv* env, int nfds, jobject src_ztfd_set, zts_fd_set* dest_fd_set);
void fdset2ztfdset(JNIEnv* env, int nfds, zts_fd_set* src_fd_set, jobject dest_ztfd_set);

/*
 * Called from Java, saves a static reference to the VM so it can be used
 * later to call a user-specified callback method from C.
 */
JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1init(JNIEnv* env, jobject thisObj)
{
    jint rs = env->GetJavaVM(&jvm);
    return rs != JNI_OK ? ZTS_ERR_GENERAL : ZTS_ERR_OK;
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1socket(
    JNIEnv* env,
    jobject thisObj,
    jint family,
    jint type,
    jint protocol)
{
    int retval = zts_bsd_socket(family, type, protocol);
    return retval > -1 ? retval : -(zts_errno);   // Encode lwIP errno into return value for JNI functions only
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1listen(JNIEnv* env, jobject thisObj, jint fd, int backlog)
{
    int retval = zts_bsd_listen(fd, backlog);
    return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1accept(
    JNIEnv* env,
    jobject thisObj,
    jint fd,
    jobject addr,
    jint port)
{
    struct zts_sockaddr_storage ss;
    zts_socklen_t addrlen = sizeof(struct zts_sockaddr_storage);
    int retval = zts_bsd_accept(fd, (zts_sockaddr*)&ss, &addrlen);
    ss2zta(env, &ss, addr);
    return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jboolean JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1getsockname(JNIEnv* env, jobject thisObj, jint fd, jobject addr)
{
    struct zts_sockaddr_storage ss;
    zts_socklen_t addrlen = sizeof(struct zts_sockaddr_storage);
    int retval = zts_bsd_getsockname(fd, (struct zts_sockaddr*)&ss, &addrlen);
    ss2zta(env, &ss, addr);
    return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1getpeername(JNIEnv* env, jobject thisObj, jint fd, jobject addr)
{
    struct zts_sockaddr_storage ss;
    int retval =
        zts_bsd_getpeername(fd, (struct zts_sockaddr*)&ss, (zts_socklen_t*)sizeof(struct zts_sockaddr_storage));
    ss2zta(env, &ss, addr);
    return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1close(JNIEnv* env, jobject thisObj, jint fd)
{
    return zts_bsd_close(fd);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1select(
    JNIEnv* env,
    jobject thisObj,
    jint nfds,
    jobject readfds,
    jobject writefds,
    jobject exceptfds,
    jint timeout_sec,
    jint timeout_usec)
{
    struct zts_timeval _timeout;
    _timeout.tv_sec = timeout_sec;
    _timeout.tv_usec = timeout_usec;
    zts_fd_set _readfds, _writefds, _exceptfds;
    zts_fd_set* r = NULL;
    zts_fd_set* w = NULL;
    zts_fd_set* e = NULL;
    if (readfds) {
        r = &_readfds;
        ztfdset2fdset(env, nfds, readfds, &_readfds);
    }
    if (writefds) {
        w = &_writefds;
        ztfdset2fdset(env, nfds, writefds, &_writefds);
    }
    if (exceptfds) {
        e = &_exceptfds;
        ztfdset2fdset(env, nfds, exceptfds, &_exceptfds);
    }
    int retval = zts_bsd_select(nfds, r, w, e, &_timeout);
    if (readfds) {
        fdset2ztfdset(env, nfds, &_readfds, readfds);
    }
    if (writefds) {
        fdset2ztfdset(env, nfds, &_writefds, writefds);
    }
    if (exceptfds) {
        fdset2ztfdset(env, nfds, &_exceptfds, exceptfds);
    }
    return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1fcntl(JNIEnv* env, jobject thisObj, jint fd, jint cmd, jint flags)
{
    int retval = zts_bsd_fcntl(fd, cmd, flags);
    return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT int JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1ioctl(
    JNIEnv* env,
    jobject thisObj,
    jint fd,
    jlong request,
    jobject argp)
{
    int retval = ZTS_ERR_OK;
    if (request == FIONREAD) {
        int bytesRemaining = 0;
        retval = zts_bsd_ioctl(fd, request, &bytesRemaining);
        // set value in general object
        jclass c = env->GetObjectClass(argp);
        if (! c) {
            return ZTS_ERR_ARG;
        }
        jfieldID fid = env->GetFieldID(c, "integer", "I");
        env->SetIntField(argp, fid, bytesRemaining);
    }
    if (request == FIONBIO) {
        // TODO: double check
        int meaninglessVariable = 0;
        retval = zts_bsd_ioctl(fd, request, &meaninglessVariable);
    }
    return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1send(
    JNIEnv* env,
    jobject thisObj,
    jint fd,
    jbyteArray buf,
    int flags)
{
    void* data = env->GetPrimitiveArrayCritical(buf, NULL);
    int retval = zts_bsd_send(fd, data, env->GetArrayLength(buf), flags);
    env->ReleasePrimitiveArrayCritical(buf, data, 0);
    return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1sendto(
    JNIEnv* env,
    jobject thisObj,
    jint fd,
    jbyteArray buf,
    jint flags,
    jobject addr)
{
    void* data = env->GetPrimitiveArrayCritical(buf, NULL);
    struct zts_sockaddr_storage ss;
    zta2ss(env, &ss, addr);
    zts_socklen_t addrlen =
        ss.ss_family == ZTS_AF_INET ? sizeof(struct zts_sockaddr_in) : sizeof(struct zts_sockaddr_in6);
    int retval = zts_bsd_sendto(fd, data, env->GetArrayLength(buf), flags, (struct zts_sockaddr*)&ss, addrlen);
    env->ReleasePrimitiveArrayCritical(buf, data, 0);
    return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1recv(
    JNIEnv* env,
    jobject thisObj,
    jint fd,
    jbyteArray buf,
    jint flags)
{
    void* data = env->GetPrimitiveArrayCritical(buf, NULL);
    int retval = zts_bsd_recv(fd, data, env->GetArrayLength(buf), flags);
    env->ReleasePrimitiveArrayCritical(buf, data, 0);
    return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1recvfrom(
    JNIEnv* env,
    jobject thisObj,
    jint fd,
    jbyteArray buf,
    jint flags,
    jobject addr)
{
    zts_socklen_t addrlen = sizeof(struct zts_sockaddr_storage);
    struct zts_sockaddr_storage ss;
    void* data = env->GetPrimitiveArrayCritical(buf, NULL);
    int retval = zts_bsd_recvfrom(fd, data, env->GetArrayLength(buf), flags, (struct zts_sockaddr*)&ss, &addrlen);
    env->ReleasePrimitiveArrayCritical(buf, data, 0);
    ss2zta(env, &ss, addr);
    return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1read(JNIEnv* env, jobject thisObj, jint fd, jbyteArray buf)
{
    void* data = env->GetPrimitiveArrayCritical(buf, NULL);
    int retval = zts_bsd_read(fd, data, env->GetArrayLength(buf));
    env->ReleasePrimitiveArrayCritical(buf, data, 0);
    return retval > -1 ? retval : -(zts_errno);
}

ssize_t zts_bsd_read_offset(int fd, void* buf, size_t offset, size_t len)
{
    char* cbuf = (char*)buf;
    return zts_bsd_read(fd, &(cbuf[offset]), len);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1read_1offset(
    JNIEnv* env,
    jobject thisObj,
    jint fd,
    jbyteArray buf,
    jint offset,
    jint len)
{
    void* data = env->GetPrimitiveArrayCritical(buf, NULL);
    int retval = zts_bsd_read_offset(fd, data, offset, len);
    env->ReleasePrimitiveArrayCritical(buf, data, 0);
    return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1read_1length(
    JNIEnv* env,
    jobject thisObj,
    jint fd,
    jbyteArray buf,
    jint len)
{
    void* data = env->GetPrimitiveArrayCritical(buf, NULL);
    int retval = zts_bsd_read(fd, data, len);
    env->ReleasePrimitiveArrayCritical(buf, data, 0);
    return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1write__IB(JNIEnv* env, jobject thisObj, jint fd, jbyteArray buf)
{
    void* data = env->GetPrimitiveArrayCritical(buf, NULL);
    int retval = zts_bsd_write(fd, data, env->GetArrayLength(buf));
    env->ReleasePrimitiveArrayCritical(buf, data, 0);
    return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1write_1offset(
    JNIEnv* env,
    jobject thisObj,
    jint fd,
    jbyteArray buf,
    jint offset,
    jint len)
{
    void* data = env->GetPrimitiveArrayCritical(&(buf[offset]), NULL);   // PENDING: check?
    int retval = zts_bsd_write(fd, data, len);
    env->ReleasePrimitiveArrayCritical(buf, data, 0);
    return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1write_1byte(JNIEnv* env, jobject thisObj, jint fd, jbyte buf)
{
    int retval = zts_bsd_write(fd, &buf, 1);
    return retval > -1 ? retval : -(zts_errno);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1shutdown(JNIEnv* env, jobject thisObj, int fd, int how)
{
    return zts_bsd_shutdown(fd, how);
}

void ztfdset2fdset(JNIEnv* env, int nfds, jobject src_ztfd_set, zts_fd_set* dest_fd_set)
{
    jclass c = env->GetObjectClass(src_ztfd_set);
    if (! c) {
        return;
    }
    ZTS_FD_ZERO(dest_fd_set);
    jfieldID fid = env->GetFieldID(c, "fds_bits", "[B");
    jobject fdData = env->GetObjectField(src_ztfd_set, fid);
    jbyteArray* arr = reinterpret_cast<jbyteArray*>(&fdData);
    char* data = (char*)env->GetByteArrayElements(*arr, NULL);
    for (int i = 0; i < nfds; i++) {
        if (data[i] == 0x01) {
            ZTS_FD_SET(i, dest_fd_set);
        }
    }
    env->ReleaseByteArrayElements(*arr, (jbyte*)data, 0);
    return;
}

void fdset2ztfdset(JNIEnv* env, int nfds, zts_fd_set* src_fd_set, jobject dest_ztfd_set)
{
    jclass c = env->GetObjectClass(dest_ztfd_set);
    if (! c) {
        return;
    }
    jfieldID fid = env->GetFieldID(c, "fds_bits", "[B");
    jobject fdData = env->GetObjectField(dest_ztfd_set, fid);
    jbyteArray* arr = reinterpret_cast<jbyteArray*>(&fdData);
    char* data = (char*)env->GetByteArrayElements(*arr, NULL);
    for (int i = 0; i < nfds; i++) {
        if (ZTS_FD_ISSET(i, src_fd_set)) {
            data[i] = 0x01;
        }
    }
    env->ReleaseByteArrayElements(*arr, (jbyte*)data, 0);
    return;
}

//----------------------------------------------------------------------------//
// Helpers (for moving data across the JNI barrier)                           //
//----------------------------------------------------------------------------//

void ss2zta(JNIEnv* env, struct zts_sockaddr_storage* ss, jobject addr)
{
    jclass c = env->GetObjectClass(addr);
    if (! c) {
        return;
    }
    if (ss->ss_family == ZTS_AF_INET) {
        struct zts_sockaddr_in* in4 = (struct zts_sockaddr_in*)ss;
        jfieldID fid = env->GetFieldID(c, "_port", "I");
        env->SetIntField(addr, fid, lwip_ntohs(in4->sin_port));
        fid = env->GetFieldID(c, "_family", "I");
        env->SetIntField(addr, fid, (in4->sin_family));
        fid = env->GetFieldID(c, "_ip4", "[B");
        jobject ipData = env->GetObjectField(addr, fid);
        jbyteArray* arr = reinterpret_cast<jbyteArray*>(&ipData);
        char* data = (char*)env->GetByteArrayElements(*arr, NULL);
        memcpy(data, &(in4->sin_addr.s_addr), 4);
        env->ReleaseByteArrayElements(*arr, (jbyte*)data, 0);

        return;
    }
    if (ss->ss_family == ZTS_AF_INET6) {
        struct zts_sockaddr_in6* in6 = (struct zts_sockaddr_in6*)ss;
        jfieldID fid = env->GetFieldID(c, "_port", "I");
        env->SetIntField(addr, fid, lwip_ntohs(in6->sin6_port));
        fid = env->GetFieldID(c, "_family", "I");
        env->SetIntField(addr, fid, (in6->sin6_family));
        fid = env->GetFieldID(c, "_ip6", "[B");
        jobject ipData = env->GetObjectField(addr, fid);
        jbyteArray* arr = reinterpret_cast<jbyteArray*>(&ipData);
        char* data = (char*)env->GetByteArrayElements(*arr, NULL);
        memcpy(data, &(in6->sin6_addr.s6_addr), 16);
        env->ReleaseByteArrayElements(*arr, (jbyte*)data, 0);
        return;
    }
}

void zta2ss(JNIEnv* env, struct zts_sockaddr_storage* ss, jobject addr)
{
    jclass c = env->GetObjectClass(addr);
    if (! c) {
        return;
    }
    jfieldID fid = env->GetFieldID(c, "_family", "I");
    int family = env->GetIntField(addr, fid);
    if (family == ZTS_AF_INET) {
        struct zts_sockaddr_in* in4 = (struct zts_sockaddr_in*)ss;
        fid = env->GetFieldID(c, "_port", "I");
        in4->sin_port = lwip_htons(env->GetIntField(addr, fid));
        in4->sin_family = ZTS_AF_INET;
        fid = env->GetFieldID(c, "_ip4", "[B");
        jobject ipData = env->GetObjectField(addr, fid);
        jbyteArray* arr = reinterpret_cast<jbyteArray*>(&ipData);
        char* data = (char*)env->GetByteArrayElements(*arr, NULL);
        memcpy(&(in4->sin_addr.s_addr), data, 4);
        env->ReleaseByteArrayElements(*arr, (jbyte*)data, 0);
        return;
    }
    if (family == ZTS_AF_INET6) {
        struct zts_sockaddr_in6* in6 = (struct zts_sockaddr_in6*)ss;
        jfieldID fid = env->GetFieldID(c, "_port", "I");
        in6->sin6_port = lwip_htons(env->GetIntField(addr, fid));
        fid = env->GetFieldID(c, "_family", "I");
        in6->sin6_family = ZTS_AF_INET6;
        fid = env->GetFieldID(c, "_ip6", "[B");
        jobject ipData = env->GetObjectField(addr, fid);
        jbyteArray* arr = reinterpret_cast<jbyteArray*>(&ipData);
        char* data = (char*)env->GetByteArrayElements(*arr, NULL);
        memcpy(&(in6->sin6_addr.s6_addr), data, 16);
        env->ReleaseByteArrayElements(*arr, (jbyte*)data, 0);
        return;
    }
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1node_1get_1port(JNIEnv* jenv, jobject thisObj)
{
    return zts_node_get_port();
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1node_1stop(JNIEnv* jenv, jobject thisObj)
{
    int res = zts_node_stop();
    java_detach_from_thread();
    return res;
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1node_1free(JNIEnv* jenv, jobject thisObj)
{
    int res = zts_node_free();
    java_detach_from_thread();
    return res;
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1net_1join(JNIEnv* env, jobject thisObj, jlong net_id)
{
    return zts_net_join((uint64_t)net_id);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1net_1leave(JNIEnv* env, jobject thisObj, jlong net_id)
{
    return zts_net_leave((uint64_t)net_id);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1id_1new(JNIEnv* jenv, jobject thisObj, char* key, int* key_buf_len)
{
    return ZTS_ERR_OK;
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1id_1pair_1is_1valid(JNIEnv* jenv, jobject thisObj, char* key, int len)
{
    return ZTS_ERR_OK;
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1init_1from_1storage(JNIEnv* jenv, jobject thisObj, jstring path)
{
    if (! path) {
        return ZTS_ERR_ARG;
    }
    const char* utf_string = jenv->GetStringUTFChars(path, NULL);
    if (! utf_string) {
        return ZTS_ERR_GENERAL;
    }
    int retval = zts_init_from_storage(utf_string);
    jenv->ReleaseStringUTFChars(path, utf_string);
    return retval;
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1init_1set_1event_1handler(JNIEnv* env, jobject thisObj, jobject callback)
{
    jclass eventListenerClass = env->GetObjectClass(callback);
    if (eventListenerClass == NULL) {
        fprintf(stderr, "Couldn't find class for ZeroTierEventListener instance");
        return ZTS_ERR_ARG;
    }
    jmethodID eventListenerCallbackMethod = env->GetMethodID(eventListenerClass, "onZeroTierEvent", "(JI)V");
    if (eventListenerCallbackMethod == NULL) {
        fprintf(stderr, "Couldn't find onZeroTierEvent method");
        return ZTS_ERR_ARG;
    }
    return zts_init_set_event_handler(env->NewGlobalRef(callback), eventListenerCallbackMethod);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1init_1set_1port(JNIEnv* jenv, jobject thisObj, short port)
{
    return zts_init_set_port(port);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1init_1from_1memory(JNIEnv* jenv, jobject thisObj, char* key, int len)
{
    return ZTS_ERR_OK;
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1init_1blacklist_1if(
    JNIEnv* jenv,
    jobject thisObj,
    jstring prefix,
    jint len)
{
    if (! prefix) {
        return ZTS_ERR_ARG;
    }
    const char* utf_string = jenv->GetStringUTFChars(prefix, NULL);
    if (! utf_string) {
        return ZTS_ERR_GENERAL;
    }
    int retval = zts_init_blacklist_if(utf_string, len);
    jenv->ReleaseStringUTFChars(prefix, utf_string);
    return retval;
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1init_1set_roots(JNIEnv* jenv, jobject thisObj, void* roots_data, jint len)
{
    return ZTS_ERR_OK;
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1init_1allow_1net_1cache(JNIEnv* jenv, jobject thisObj, jint allowed)
{
    return zts_init_allow_net_cache(allowed);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1init_1allow_1peer_1cache(JNIEnv* jenv, jobject thisObj, jint allowed)
{
    return zts_init_allow_peer_cache(allowed);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1init_1allow_1roots_1cache(JNIEnv* jenv, jobject thisObj, jint allowed)
{
    return zts_init_allow_roots_cache(allowed);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1init_1allow_1id_1cache(JNIEnv* jenv, jobject thisObj, jint allowed)
{
    return zts_init_allow_id_cache(allowed);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1addr_1is_1assigned(
    JNIEnv* jenv,
    jobject thisObj,
    jlong net_id,
    jint family)
{
    return zts_addr_is_assigned(net_id, family);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1addr_1get(
    JNIEnv* jenv,
    jobject thisObj,
    long net_id,
    jint family,
    struct sockaddr_storage* addr)
{
    // Use Java_com_zerotier_sockets_ZeroTierNative_zts_1addr_1get_1str instead
}

JNIEXPORT jstring JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1addr_1get_1str(JNIEnv* jenv, jobject thisObj, long net_id, jint family)
{
    char ip_str[ZTS_IP_MAX_STR_LEN] = { 0 };
    zts_addr_get_str(net_id, family, ip_str, ZTS_IP_MAX_STR_LEN);
    jstring result = jenv->NewStringUTF(ip_str);
    return result;
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1addr_1get_1all(
    JNIEnv* jenv,
    jobject thisObj,
    long net_id,
    struct sockaddr_storage* addr,
    jint* count)
{
    /* This feature will be implemented once the lower-level
    limitation of one addr per family per network is removed. */
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1addr_1compute_16plane(
    JNIEnv* jenv,
    jobject thisObj,
    jlong net_id,
    jlong node_id,
    struct sockaddr_storage* addr)
{
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1addr_1compute_1rfc4193(
    JNIEnv* jenv,
    jobject thisObj,
    jlong net_id,
    jlong node_id,
    struct sockaddr_storage* addr)
{
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1addr_1compute_1rfc4193_1str(
    JNIEnv* jenv,
    jobject thisObj,
    jlong net_id,
    jlong node_id,
    jstring dst,
    jint len)
{
    return ZTS_ERR_OK;
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1addr_1compute_16plane_1str(
    JNIEnv* jenv,
    jobject thisObj,
    jlong net_id,
    jlong node_id,
    jstring dst,
    jint len)
{
    return ZTS_ERR_OK;
}

JNIEXPORT uint64_t JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1net_1compute_1adhoc_1id(
    JNIEnv* jenv,
    jobject thisObj,
    short start_port,
    short end_port)
{
    return ZTS_ERR_OK;
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1net_1transport_1is_1ready(JNIEnv* jenv, jobject thisObj, jlong net_id)
{
    return zts_net_transport_is_ready(net_id);
}

JNIEXPORT uint64_t JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1net_get_mac(JNIEnv* jenv, jobject thisObj, jlong net_id)
{
    return zts_net_get_mac(net_id);
}

JNIEXPORT jstring JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1net_1get_1mac_1str(JNIEnv* jenv, jobject thisObj, jlong net_id)
{
    char mac_str[ZTS_MAC_ADDRSTRLEN] = { 0 };
    zts_net_get_mac_str(net_id, mac_str, ZTS_MAC_ADDRSTRLEN);
    jstring result = jenv->NewStringUTF(mac_str);
    return result;
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1net_1get_1broadcast(JNIEnv* jenv, jobject thisObj, jlong net_id)
{
    return zts_net_get_broadcast(net_id);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1net_1get_1mtu(JNIEnv* jenv, jobject thisObj, jlong net_id)
{
    return zts_net_get_mtu(net_id);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1net_1get_1name(
    JNIEnv* jenv,
    jobject thisObj,
    jlong net_id,
    jstring dst,
    jint len)
{
    return ZTS_ERR_OK;
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1net_1get_1status(JNIEnv* jenv, jobject thisObj, jlong net_id)
{
    return zts_net_get_status(net_id);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1net_1get_1type(JNIEnv* jenv, jobject thisObj, jlong net_id)
{
    return zts_net_get_type(net_id);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1route_1is_1assigned(
    JNIEnv* jenv,
    jobject thisObj,
    jlong net_id,
    jint family)
{
    return zts_route_is_assigned(net_id, family);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1node_1start(JNIEnv* jenv, jobject thisObj)
{
    return zts_node_start();
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1node_1is_1online(JNIEnv* jenv, jobject thisObj)
{
    return zts_node_is_online();
}

JNIEXPORT uint64_t JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1node_1get_1id(JNIEnv* jenv, jobject thisObj)
{
    return zts_node_get_id();
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1node_1get_1id_1pair(
    JNIEnv* jenv,
    jobject thisObj,
    char* key,
    jint* key_buf_len)
{
    return ZTS_ERR_OK;
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1moon_1orbit(
    JNIEnv* jenv,
    jobject thisObj,
    jlong moon_roots_id,
    jlong moon_seed)
{
    return zts_moon_orbit(moon_roots_id, moon_seed);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1moon_1deorbit(JNIEnv* jenv, jobject thisObj, jlong moon_roots_id)
{
    return zts_moon_deorbit(moon_roots_id);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1connect(
    JNIEnv* jenv,
    jobject thisObj,
    jint fd,
    jstring ipstr,
    jint port,
    jint timeout_ms)
{
    if (! ipstr) {
        return ZTS_ERR_ARG;
    }
    const char* utf_string = jenv->GetStringUTFChars(ipstr, NULL);
    if (! utf_string) {
        return ZTS_ERR_GENERAL;
    }
    int retval = zts_connect(fd, utf_string, port, timeout_ms);
    jenv->ReleaseStringUTFChars(ipstr, utf_string);
    return retval;
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1bind(JNIEnv* jenv, jobject thisObj, jint fd, jstring ipstr, jint port)
{
    if (! ipstr) {
        return ZTS_ERR_ARG;
    }
    const char* utf_string = jenv->GetStringUTFChars(ipstr, NULL);
    if (! utf_string) {
        return ZTS_ERR_GENERAL;
    }
    int retval = zts_bind(fd, utf_string, port);
    jenv->ReleaseStringUTFChars(ipstr, utf_string);
    return retval;
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1accept(
    JNIEnv* jenv,
    jobject thisObj,
    int fd,
    jstring remote_addr,
    jint len,
    jint* port)
{
    // Use Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1accept instead
    return ZTS_ERR_OK;
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1udp_1client(JNIEnv* jenv, jobject thisObj, jstring remote_ipstr)
{
    return ZTS_ERR_OK;
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1set_1no_1delay(JNIEnv* jenv, jobject thisObj, jint fd, jint enabled)
{
    return zts_set_no_delay(fd, enabled);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1get_1no_1delay(JNIEnv* jenv, jobject thisObj, jint fd)
{
    return zts_get_no_delay(fd);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1set_1linger(
    JNIEnv* jenv,
    jobject thisObj,
    jint fd,
    jint enabled,
    jint value)
{
    return zts_set_linger(fd, enabled, value);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1get_1linger_1enabled(JNIEnv* jenv, jobject thisObj, jint fd)
{
    return zts_get_linger_enabled(fd);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1get_1linger_1value(JNIEnv* jenv, jobject thisObj, jint fd)
{
    return zts_get_linger_value(fd);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1get_1pending_1data_1size(JNIEnv* jenv, jobject thisObj, jint fd)
{
    return zts_get_pending_data_size(fd);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1set_1reuse_1addr(JNIEnv* jenv, jobject thisObj, jint fd, jint enabled)
{
    return zts_set_reuse_addr(fd, enabled);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1get_1reuse_1addr(JNIEnv* jenv, jobject thisObj, jint fd)
{
    return zts_get_reuse_addr(fd);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1set_1recv_1timeout(
    JNIEnv* jenv,
    jobject thisObj,
    jint fd,
    jint seconds,
    jint microseconds)
{
    return zts_set_recv_timeout(fd, seconds, microseconds);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1get_1recv_1timeout(JNIEnv* jenv, jobject thisObj, jint fd)
{
    return zts_get_recv_timeout(fd);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1set_1send_1timeout(
    JNIEnv* jenv,
    jobject thisObj,
    jint fd,
    jint seconds,
    jint microseconds)
{
    return zts_set_send_timeout(fd, seconds, microseconds);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1get_1send_1timeout(JNIEnv* jenv, jobject thisObj, jint fd)
{
    return zts_get_send_timeout(fd);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1set_1send_1buf_1size(JNIEnv* jenv, jobject thisObj, jint fd, jint size)
{
    return zts_set_send_buf_size(fd, size);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1get_1send_1buf_1size(JNIEnv* jenv, jobject thisObj, jint fd)
{
    return zts_get_send_buf_size(fd);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1set_1recv_1buf_1size(JNIEnv* jenv, jobject thisObj, jint fd, jint size)
{
    return zts_set_recv_buf_size(fd, size);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1get_1recv_1buf_1size(JNIEnv* jenv, jobject thisObj, jint fd)
{
    return zts_get_recv_buf_size(fd);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1set_1ttl(JNIEnv* jenv, jobject thisObj, jint fd, jint ttl)
{
    return zts_set_ttl(fd, ttl);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1get_1ttl(JNIEnv* jenv, jobject thisObj, jint fd)
{
    return zts_get_ttl(fd);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1set_1blocking(JNIEnv* jenv, jobject thisObj, jint fd, jint enabled)
{
    return zts_set_blocking(fd, enabled);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1get_1blocking(JNIEnv* jenv, jobject thisObj, jint fd)
{
    return zts_get_blocking(fd);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1set_1keepalive(JNIEnv* jenv, jobject thisObj, jint fd, jint enabled)
{
    return zts_set_keepalive(fd, enabled);
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1get_1keepalive(JNIEnv* jenv, jobject thisObj, jint fd)
{
    return zts_get_keepalive(fd);
}

struct hostent*
Java_com_zerotier_sockets_ZeroTierNative_zts_1bsd_1gethostbyname(JNIEnv* jenv, jobject thisObj, jstring name)
{
    return NULL;
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1dns_1set_1server(
    JNIEnv* jenv,
    jobject thisObj,
    uint8_t index,
    ip_addr* addr)
{
    return ZTS_ERR_OK;
}

JNIEXPORT ip_addr* JNICALL dns_1get_1server(JNIEnv* jenv, jobject thisObj, uint8_t index)
{
    return NULL;
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1core_1lock_1obtain(JNIEnv* jenv, jobject thisObj)
{
    return zts_core_lock_obtain();
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1core_1lock_1release(JNIEnv* jenv, jobject thisObj)
{
    return zts_core_lock_release();
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1core_1query_1addr_1count(JNIEnv* jenv, jobject thisObj, jlong net_id)
{
    return zts_core_query_addr_count(net_id);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1core_1query_1addr(
    JNIEnv* jenv,
    jobject thisObj,
    jlong net_id,
    jint idx,
    jstring addr,
    jint len)
{
    return ZTS_ERR_OK;
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1core_1query_1route_1count(JNIEnv* jenv, jobject thisObj, jlong net_id)
{
    return zts_core_query_route_count(net_id);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1core_1query_1route(
    JNIEnv* jenv,
    jobject thisObj,
    jlong net_id,
    jint idx,
    jstring target,
    jstring via,
    jint len,
    short* flags,
    short* metric)
{
    return ZTS_ERR_OK;
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1core_1query_1path_1count(JNIEnv* jenv, jobject thisObj, jlong peer_id)
{
    return zts_core_query_path_count(peer_id);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1core_1query_1path(
    JNIEnv* jenv,
    jobject thisObj,
    jlong peer_id,
    jint idx,
    jstring dst,
    jint len)
{
    return ZTS_ERR_OK;
}

JNIEXPORT jint JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1core_1query_1mc_1count(JNIEnv* jenv, jobject thisObj, jlong net_id)
{
    return zts_core_query_mc_count(net_id);
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1core_1query_1mc(
    JNIEnv* jenv,
    jobject thisObj,
    long net_id,
    jint idx,
    jlong* mac,
    uint32_t* adi)
{
    return ZTS_ERR_OK;
}

JNIEXPORT jint JNICALL Java_com_zerotier_sockets_ZeroTierNative_zts_1util_1roots_1new(
    JNIEnv* jenv,
    jobject thisObj,
    char* roots_out,
    jint* roots_len,
    char* prev_key,
    jint* prev_key_len,
    char* curr_key,
    jint* curr_key_len,
    jlong id,
    jlong ts,
    zts_root_set_t* roots_spec)
{
    return ZTS_ERR_OK;
}

JNIEXPORT void JNICALL
Java_com_zerotier_sockets_ZeroTierNative_zts_1util_1delay(JNIEnv* jenv, jobject thisObj, jlong milliseconds)
{
    zts_util_delay(milliseconds);
}

#ifdef __cplusplus
}
#endif

}   // namespace ZeroTier

#endif   // ZTS_ENABLE_JAVA
