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

    // ------------------------------------------------------------------------------
    // ---------------------------- Compilation flag checks -------------------------
    // ------------------------------------------------------------------------------

#define INTERCEPT_ENABLED     111
#define INTERCEPT_DISABLED    222
#define MAX_DIR_SZ            256 // Max path length used for home dir

#if defined(SDK_SERVICE)
	// Sanity checks for compilation
	#if !defined(SDK_LWIP) && !defined(SDK_PICOTCP)
	 #error "No network stack specified, use SDK_LWIP=1, SDK_PICOTCP=1, or similar"
	#endif
	#if defined(SDK_LWIP) && defined(SDK_PICOTCP)
	 #error "Dual stacks is not currently supported, try one or the other"
	#endif
	#if !defined(SDK_IPV4) && !defined(SDK_IPV6)
	 #error "No IP protocol version specified, use SDK_IPV4=1, or SDK_IPV6=1"
	#endif
	#if !defined(SDK_IPV4) && !defined(SDK_IPV6)
	 #error "Dual protocol versions is not currently supported. try one or the other"
	#endif
#endif

    // ------------------------------------------------------------------------------
    // -------------- Socket API function signatures for convenience ----------------
    // ------------------------------------------------------------------------------

#define SETSOCKOPT_SIG int fd, int level, int optname, const void *optval, socklen_t optlen
#define GETSOCKOPT_SIG int fd, int level, int optname, void *optval, socklen_t *optlen
#define SENDMSG_SIG int fd, const struct msghdr *msg, int flags
#define SENDTO_SIG int fd, const void *buf, size_t len, int flags, const struct sockaddr *addr, socklen_t addrlen
#define RECV_SIG int fd, void *buf, size_t len, int flags
#define RECVFROM_SIG int fd, void *buf, size_t len, int flags, struct sockaddr *addr, socklen_t *addrlen
#define RECVMSG_SIG int fd, struct msghdr *msg,int flags
#define SEND_SIG int fd, const void *buf, size_t len, int flags
#define WRITE_SIG int fd, const void *buf, size_t len
#define READ_SIG int fd, void *buf, size_t len
#define SOCKET_SIG int socket_family, int socket_type, int protocol
#define CONNECT_SIG int fd, const struct sockaddr *addr, socklen_t addrlen
#define BIND_SIG int fd, const struct sockaddr *addr, socklen_t addrlen
#define LISTEN_SIG int fd, int backlog
#define ACCEPT4_SIG int fd, struct sockaddr *addr, socklen_t *addrlen, int flags
#define ACCEPT_SIG int fd, struct sockaddr *addr, socklen_t *addrlen
#define CLOSE_SIG int fd
#define GETSOCKNAME_SIG int fd, struct sockaddr *addr, socklen_t *addrlen
#define GETPEERNAME_SIG int fd, struct sockaddr *addr, socklen_t *addrlen
#define FCNTL_SIG int fd, int cmd, int flags
#define SYSCALL_SIG long number, ...

#if defined(__ANDROID__)
    #include <jni.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern void load_symbols();
extern void zts_init_rpc(const char *path, const char *nwid);
extern char *api_netpath;
extern char *debug_logfile;

    // ------------------------------------------------------------------------------
    // ------------------------- Ancient INTERCEPT-related cruft --------------------
    // ------------------------------------------------------------------------------

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
   
    // ------------------------------------------------------------------------------
    // ---------------------------- Direct API call section -------------------------
    // ------------------------------------------------------------------------------

// SOCKS5 Proxy Controls
int zts_start_proxy_server(const char *homepath, const char * nwid, struct sockaddr_storage * addr);
int zts_stop_proxy_server(const char *nwid);
int zts_get_proxy_server_address(const char * nwid, struct sockaddr_storage *addr);
bool zts_proxy_is_running(const char *nwid);
// ZT Service Controls
void zts_start_service(const char *path);
void *zts_start_core_service(void *thread_id);
void zts_stop_service();
void zts_stop();
bool zts_service_is_running();
void zts_join_network(const char * nwid);
void zts_join_network_soft(const char * filepath, const char * nwid);
void zts_leave_network_soft(const char * filepath, const char * nwid);
void zts_leave_network(const char * nwid);
void zts_get_ipv4_address(const char *nwid, char *addrstr);
void zts_get_ipv6_address(const char *nwid, char *addrstr);
bool zts_has_address(const char *nwid);
int zts_get_device_id(char *devID);
int zts_get_device_id_from_file(const char *filepath, char *devID);
char *zts_get_homepath();
void zts_get_6plane_addr(char *addr, const char *nwid, const char *devID);
void zts_get_rfc4193_addr(char *addr, const char *nwid, const char *devID);
// BSD-like socket API
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
    int zts_set_nonblock(int fd); // TODO combine with fcntl()
#endif  

#if !defined(__IOS__)
    void zt_start_service(const char * path, const char *nwid);
	void zt_join_network(const char * nwid);
    void zt_leave_network(const char * nwid); 
#endif

    // ------------------------------------------------------------------------------
    // --------------------- Direct API call section (for Android) ------------------
    // ------------------------------------------------------------------------------

// JNI naming convention: Java_PACKAGENAME_CLASSNAME_METHODNAME
#if defined(__ANDROID__)
	// ZT SERVICE CONTROLS
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1start_1service(JNIEnv *env, jobject thisObj, jstring path);
	JNIEXPORT jboolean JNICALL Java_ZeroTier_ZTSDK_zt_1stop_service();
	JNIEXPORT jboolean JNICALL Java_ZeroTier_ZTSDK_zt_1service_1is_1running(JNIEnv *env, jobject thisObj);
	JNIEXPORT jstring JNICALL Java_ZeroTier_ZTSDK_zt_1get_1homepath(JNIEnv *env, jobject thisObj);
	JNIEXPORT void JNICALL Java_ZeroTier_ZTSDK_zt_1join_1network(JNIEnv *env, jobject thisObj, jstring nwid);
    JNIEXPORT void JNICALL Java_ZeroTier_ZTSDK_zt_1leave_1network(JNIEnv *env, jobject thisObj, jstring nwid);
    JNIEXPORT jobject JNICALL Java_ZeroTier_ZTSDK_zt_1get_1ipv4_1address(JNIEnv *env, jobject thisObj, jstring nwid);
	JNIEXPORT jobject JNICALL Java_ZeroTier_ZTSDK_zt_1get_1ipv6_1address(JNIEnv *env, jobject thisObj, jstring nwid);
    JNIEXPORT jboolean JNICALL Java_ZeroTier_ZTSDK_zt_1is_1relayed();
	// SOCKS5 PROXY SERVER CONTROLS
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1start_1proxy_1server(JNIEnv *env, jobject thisObj, jstring nwid, jobject zaddr);
    JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1stop_1proxy_1server(JNIEnv *env, jobject thisObj, jstring nwid);
    JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1get_1proxy_1server_1address(JNIEnv *env, jobject thisObj, jstring nwid, jobject zaddr);
	JNIEXPORT jboolean JNICALL Java_ZeroTier_ZTSDK_zt_1proxy_1is_1running(JNIEnv *env, jobject thisObj, jstring nwid);
	// SOCKET API
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1socket(JNIEnv *env, jobject thisObj, jint family, jint type, jint protocol);
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1connect(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port);	
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1bind(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port);
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1accept4(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port, jint flags);
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1accept(JNIEnv *env, jobject thisObj, jint fd, jstring addrstr, jint port);
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1listen(JNIEnv *env, jobject thisObj, jint fd, int backlog);
	// TCP
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1write(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len);
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1read(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len);
    JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1send(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len, int flags);
	// UDP
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1sendto(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len, jint flags, jobject ztaddr);
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1recvfrom(JNIEnv *env, jobject thisObj, jint fd, jarray buf, jint len, jint flags, jobject ztaddr);
	// GENERAL UTILITY
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1close(JNIEnv *env, jobject thisObj, jint fd);
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1fcntl(JNIEnv *env, jobject thisObj, jint socket, jint cmd, jint flags);
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1setsockopt(JNIEnv *env, jobject thisObj, jint fd, jint level, jint optname, jint optval, jint optlen);
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1getsockopt(JNIEnv *env, jobject thisObj, jint fd, jint level, jint optname, jint optval, jint optlen);
	JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1getsockname(JNIEnv *env, jobject thisObj, jint fd, jobject ztaddr);
    JNIEXPORT jint JNICALL Java_ZeroTier_ZTSDK_zt_1getpeername(JNIEnv *env, jobject thisObj, jint fd, jobject ztaddr);
#endif


// Prototypes for redefinition of syscalls
// - Implemented in intercept.c
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
