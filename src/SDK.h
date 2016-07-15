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

#ifndef _ZT_SDK_H
#define _ZT_SDK_H	1

#include <sys/socket.h>
#include <stdbool.h>
#include "SDK_Signatures.h"

// For defining the Android direct-call API
#if defined(__ANDROID__)
    #include <jni.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define INTERCEPT_ENABLED  111
#define INTERCEPT_DISABLED 222

extern void load_symbols();
extern void zt_init_rpc(const char *path, const char *nwid);
extern char *api_netpath;
extern char *debug_logfile;

#if defined(__linux__)
	extern int (*realaccept4)(ACCEPT4_SIG);
	#if !defined(__ANDROID__)
		extern int (*realsyscall)(SYSCALL_SIG);
	#endif
#endif

#if !defined(__ANDROID__)
    bool check_intercept_enabled();
	extern int (*realbind)(BIND_SIG);
	extern int (*realsendmsg)(SENDMSG_SIG);
	extern ssize_t (*realsendto)(SENDTO_SIG);
	extern int (*realrecvmsg)(RECVMSG_SIG);
	extern int (*realrecvfrom)(RECVFROM_SIG);
#endif
	extern int (*realconnect)(CONNECT_SIG);
	extern int (*realaccept)(ACCEPT_SIG);
	extern int (*reallisten)(LISTEN_SIG);
	extern int (*realsocket)(SOCKET_SIG);
	extern int (*realsetsockopt)(SETSOCKOPT_SIG);
	extern int (*realgetsockopt)(GETSOCKOPT_SIG);
	extern int (*realclose)(CLOSE_SIG);
	extern int (*realgetsockname)(GETSOCKNAME_SIG);

ssize_t zt_sendto(SENDTO_SIG);
ssize_t zt_sendmsg(SENDMSG_SIG);
ssize_t zt_recvfrom(RECVFROM_SIG);
ssize_t zt_recvmsg(RECVMSG_SIG);

    
#if defined(__UNITY_3D__)
    ssize_t zt_recv(int fd, void *buf, int len);
    ssize_t zt_send(int fd, void *buf, int len);
    int zt_set_nonblock(int fd);
#endif    
    
// Android JNI Direct-call API
#if defined(__ANDROID__)
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_ztjniSocket(JNIEnv *env, jobject thisObj, jint family, jint type, jint protocol);
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_ztjniConnect(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port);	
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_ztjniBind(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port);
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_ztjniAccept(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port);
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_ztjniAccept4(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port, jint flags);
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_ztjniListen(JNIEnv *env, jobject thisObj, jint fd, int backlog);
	//JNIEXPORT void JNICALL Java_ZeroTier_SDK_ztjniSetsockopt(JNIEnv *env, jobject thisObj);
	//JNIEXPORT void JNICALL Java_ZeroTier_SDK_ztjniGetsockopt(JNIEnv *env, jobject thisObj);
	//JNIEXPORT void JNICALL Java_ZeroTier_SDK_ztjniGetsockname(JNIEnv *env, jobject thisObj);
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_ztjniClose(JNIEnv *env, jobject thisObj, jint fd);
#endif

int zt_socket(SOCKET_SIG);
int zt_connect(CONNECT_SIG);
int zt_bind(BIND_SIG);
#if defined(__linux__)
	int zt_accept4(ACCEPT4_SIG);
#endif
int zt_accept(ACCEPT_SIG);
int zt_listen(LISTEN_SIG);
int zt_setsockopt(SETSOCKOPT_SIG);
int zt_getsockopt(GETSOCKOPT_SIG);
int zt_getsockname(GETSOCKNAME_SIG);
int zt_close(CLOSE_SIG);


#if defined(SDK_INTERCEPT)
	int socket(SOCKET_SIG);
	int connect(CONNECT_SIG);
	int bind(BIND_SIG);
	#if defined(__linux__)
		int accept4(ACCEPT4_SIG);
	#endif
	int accept(ACCEPT_SIG);
	int listen(LISTEN_SIG);
	int setsockopt(SETSOCKOPT_SIG);
	int getsockopt(GETSOCKOPT_SIG);
	int getsockname(GETSOCKNAME_SIG);
	int close(CLOSE_SIG);
#endif
	
#ifdef __cplusplus
}
#endif

#endif // _ZT_SDK_H