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

#if defined(__ANDROID__)
	// For defining the Android direct-call API
    #include <jni.h>
#endif

#include "SDK_LocalBuild.h"

#ifdef __cplusplus
extern "C" {
#endif

#define INTERCEPT_ENABLED     111
#define INTERCEPT_DISABLED    222
#define MAX_DIR_SZ            256 // Max path length used for home dir

extern void load_symbols();
extern void zts_init_rpc(const char *path, const char *nwid);
extern char *api_netpath;
extern char *debug_logfile;

// Function pointers to original system calls
// - These are used when we detect that either the intercept is not 
//   available or that ZeroTier hasn't administered the given socket
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
    
// Direct call
// - Skips intercept
// - Uses RPC
// - Depending on the target, the API will be exposed as zt_* in 
//   the specific way needed for that platform, but will be implemented 
//   in terms of zts_*

// NOTE: Each platform specific exposed API will be implemented in terms of zts_*
// SOCKS5 Proxy Controls
int zts_start_proxy_server(const char *homepath, const char * nwid, struct sockaddr_storage * addr);
int zts_stop_proxy_server(const char *nwid);
int zts_get_proxy_server_address(const char * nwid, struct sockaddr_storage *addr);
// ZT Service Controls
void *zts_start_service(void *thread_id);
void zts_stop_service();
bool zts_is_running();
void zts_join_network(const char * nwid);
void zts_leave_network(const char * nwid);
void zts_get_addresses(const char * nwid, char * addrstr);
int zts_get_device_id();
bool zts_is_relayed();
char *zts_get_homepath();

// ZT Intercept/RPC Controls
// TODO: Remove any?
void set_intercept_status(int mode); /* TODO: Rethink this */
void init_service(int key, const char * path);
void init_service_and_rpc(int key, const char * path, const char * nwid);
void init_intercept(int key);

int zts_socket(SOCKET_SIG);
int zts_connect(CONNECT_SIG);
int zts_bind(BIND_SIG);
#if defined(__linux__)
	int zts_accept4(ACCEPT4_SIG);
#endif
int zts_accept(ACCEPT_SIG);
int zts_listen(LISTEN_SIG);
int zts_setsockopt(SETSOCKOPT_SIG);
int zts_getsockopt(GETSOCKOPT_SIG);
int zts_getsockname(GETSOCKNAME_SIG);
int zts_getpeername(GETPEERNAME_SIG);
int zts_close(CLOSE_SIG);
int zts_fcntl(FCNTL_SIG);

ssize_t zts_sendto(SENDTO_SIG);
ssize_t zts_sendmsg(SENDMSG_SIG);
ssize_t zts_recvfrom(RECVFROM_SIG);
ssize_t zts_recvmsg(RECVMSG_SIG);
    
#if defined(__UNITY_3D__)
    ssize_t zts_recv(int fd, void *buf, int len);
    ssize_t zts_send(int fd, void *buf, int len);
    int zts_set_nonblock(int fd); /* TODO combine with fcntl() */
#endif  

#if !defined(__IOS__)
    void zt_start_service(const char * path, const char *nwid);
	void zt_join_network(const char * nwid);
    void zt_leave_network(const char * nwid); 
#endif

// Android JNI Direct-call API
// JNI naming convention: Java_PACKAGENAME_CLASSNAME_METHODNAME
/* If you define anything else in this file it that you wish to expose to your Android 
	Java application you *must* follow that convention and any corresponding Java package/classes 
	in your Android project must match this as well */
#if defined(__ANDROID__)
	// Exported JNI : ZT SERVICE CONTROLS
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1start_1service(JNIEnv *env, jobject thisObj, jstring path);
	JNIEXPORT jboolean JNICALL Java_ZeroTier_SDK_zt_1stop_service();
	JNIEXPORT void JNICALL Java_ZeroTier_SDK_zt_1join_1network(JNIEnv *env, jobject thisObj, jstring nwid);
    JNIEXPORT void JNICALL Java_ZeroTier_SDK_zt_1leave_1network(JNIEnv *env, jobject thisObj, jstring nwid);
    JNIEXPORT jboolean JNICALL Java_ZeroTier_SDK_zt_1running(JNIEnv *env, jobject thisObj);
    JNIEXPORT jobject JNICALL Java_ZeroTier_SDK_zt_1get_1addresses(JNIEnv *env, jobject thisObj, jstring nwid);
    JNIEXPORT jboolean JNICALL Java_ZeroTier_SDK_zt_1is_1relayed();
	// Returns the homepath 
	JNIEXPORT jstring JNICALL Java_ZeroTier_SDK_zt_1get_1homepath(JNIEnv *env, jobject thisObj);
	
	// Exported JNI : SOCKS5 PROXY SERVER CONTROLS
	// Stops the SOCKS5 proxy server for a given ZeroTier network
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1start_1proxy_1server(JNIEnv *env, jobject thisObj, jstring nwid, jobject zaddr);
    JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1stop_1proxy_1server(JNIEnv *env, jobject thisObj, jstring nwid);
    JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1get_1proxy_1server_1address(JNIEnv *env, jobject thisObj, jstring nwid, jobject zaddr);
	
	// Exported JNI : SOCKET API
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1socket(JNIEnv *env, jobject thisObj, jint family, jint type, jint protocol);
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1connect(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port);	
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1bind(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port);
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1accept4(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port, jint flags);
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1accept(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port);
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1listen(JNIEnv *env, jobject thisObj, jint fd, int backlog);
	// TCP
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1write(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len);
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1read(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len);
    JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1send(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len, int flags);
	// UDP
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1sendto(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len, jint flags, jobject ztaddr);
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1recvfrom(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len, jint flags, jobject ztaddr);
	// GENERAL UTILITY
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1close(JNIEnv *env, jobject thisObj, jint fd);
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1fcntl(JNIEnv *env, jobject thisObj, jint socket, jint cmd, jint flags);
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1setsockopt(JNIEnv *env, jobject thisObj, jint fd, jint level, jint optname, jint optval, jint optlen);
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1getsockopt(JNIEnv *env, jobject thisObj, jint fd, jint level, jint optname, jint optval, jint optlen);
	JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1getsockname(JNIEnv *env, jobject thisObj, jint fd, jobject ztaddr);
    JNIEXPORT jint JNICALL Java_ZeroTier_SDK_zt_1getpeername(JNIEnv *env, jobject thisObj, jint fd, jobject ztaddr);
#endif


// Prototypes for redefinition of syscalls
// - Implemented in SDK_Intercept.c
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