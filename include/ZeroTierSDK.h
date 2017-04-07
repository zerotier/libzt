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

/*
 * This defines the external C++ API for the ZeroTier SDK's service
 */

#ifndef ZT_ZEROTIERSDK_H
#define ZT_ZEROTIERSDK_H

#include <sys/socket.h>

/****************************************************************************/
/* Defines                                                                  */
/****************************************************************************/

#define SDK_MTU                       1200 //ZT_MAX_MTU // 2800, usually 
#define UNIX_SOCK_BUF_SIZE            1024*1024
#define ZT_PHY_POLL_INTERVAL          50 // in ms
// picoTCP 
#define MAX_PICO_FRAME_RX_BUF_SZ      ZT_MAX_MTU * 128
// TCP
#define DEFAULT_TCP_TX_BUF_SZ         1024 * 1024
#define DEFAULT_TCP_RX_BUF_SZ         1024 * 1024
#define DEFAULT_TCP_TX_BUF_SOFTMAX    DEFAULT_TCP_TX_BUF_SZ * 0.80
#define DEFAULT_TCP_TX_BUF_SOFTMIN    DEFAULT_TCP_TX_BUF_SZ * 0.20
#define DEFAULT_TCP_RX_BUF_SOFTMAX    DEFAULT_TCP_RX_BUF_SZ * 0.80
#define DEFAULT_TCP_RX_BUF_SOFTMIN    DEFAULT_TCP_RX_BUF_SZ * 0.20
// UDP
#define DEFAULT_UDP_TX_BUF_SZ         ZT_MAX_MTU
#define DEFAULT_UDP_RX_BUF_SZ         ZT_MAX_MTU * 10


/****************************************************************************/
/* Socket API Signatures                                                    */
/****************************************************************************/

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

/****************************************************************************/
/* SDK Socket API                                                           */
/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Start core ZeroTier service (generates identity)
 */
void zts_start(const char *path);


void *zts_start_core_service(void *thread_id);

char *zts_core_version();
void zts_stop_service();
void zts_stop();
int zts_service_is_running();
void zts_join_network(const char * nwid);
void zts_join_network_soft(const char * filepath, const char * nwid);
void zts_leave_network_soft(const char * filepath, const char * nwid);
void zts_leave_network(const char * nwid);
void zts_get_ipv4_address(const char *nwid, char *addrstr);
void zts_get_ipv6_address(const char *nwid, char *addrstr);
int zts_has_address(const char *nwid);
int zts_get_device_id(char *devID);
int zts_get_device_id_from_file(const char *filepath, char *devID);
int zts_get_peer_address(char *peer, const char *devID);
unsigned long zts_get_peer_count();
//int zts_get_peer_list();
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

/****************************************************************************/
/* Debug                                                                    */
/****************************************************************************/

#include <pthread.h>
#include <sys/syscall.h>
#include <sys/types.h>

#define DEBUG_LEVEL     5 // Set this to adjust what you'd like to see in the debug traces

#define MSG_ERROR       1 // Errors
#define MSG_TRANSFER    2 // RX/TX specific statements
#define MSG_INFO        3 // Information which is generally useful to any developer
#define MSG_EXTRA       4 // If nothing in your world makes sense
#define MSG_FLOW        5 // High-level flow messages

#define __SHOW_FILENAMES__    true
#define __SHOW_COLOR__        true

// Colors
#if defined(__APPLE__)
    #include "TargetConditionals.h"
#endif
#if defined(__SHOW_COLOR__) && !defined(__ANDROID__) && !defined(TARGET_OS_IPHONE) && !defined(TARGET_IPHONE_SIMULATOR) && !defined(__APP_FRAMEWORK__)
  #define RED   "\x1B[31m"
  #define GRN   "\x1B[32m"
  #define YEL   "\x1B[33m"
  #define BLU   "\x1B[34m"
  #define MAG   "\x1B[35m"
  #define CYN   "\x1B[36m"
  #define WHT   "\x1B[37m"
  #define RESET "\x1B[0m"
#else
  #define RED
  #define GRN
  #define YEL
  #define BLU
  #define MAG
  #define CYN
  #define WHT
  #define RESET
#endif

// filenames
#if __SHOW_FILENAMES__
  #if __SHOW_FULL_FILENAME_PATH__
    #define __FILENAME__ __FILE__ // show the entire mess
  #else
    #define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__) // shorten
  #endif
#else
  #define __FILENAME__ // omit filename
#endif

#ifdef __linux__
  #define THREAD_ID 0 /*(long)getpid()*/
#elif __APPLE__
  #define THREAD_ID 0 /*(long)syscall(SYS_thread_selfid)*/
#endif

#if defined(__JNI_LIB__)
      #include <jni.h>
#endif
#if defined(__ANDROID__)
    #include <android/log.h>
    #define LOG_TAG "ZTSDK"
#endif

 #if DEBUG_LEVEL >= MSG_ERROR
  #define DEBUG_ERROR(fmt, args...) fprintf(stderr, RED "ZT_ERROR[%ld] : %14s:%4d:%25s: " fmt "\n" RESET, THREAD_ID, __FILENAME__, __LINE__, __FUNCTION__, ##args)
 #else
  #define DEBUG_ERROR(fmt, args...)
 #endif
 
 #if DEBUG_LEVEL >= MSG_INFO
  #if defined(__ANDROID__)
    #define DEBUG_INFO(fmt, args...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ZT_INFO : %14s:%4d:%20s: " fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args))
    #define DEBUG_BLANK(fmt, args...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ZT_INFO : %14s:%4d:" fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args))
    #define DEBUG_ATTN(fmt, args...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ZT_INFO : %14s:%4d:%25s: " fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args))
    #define DEBUG_STACK(fmt, args...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ZT_STACK: %14s:%4d:%25s: " fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args))
  #else
    #define DEBUG_INFO(fmt, args...) fprintf(stderr,      "ZT_INFO [%ld] : %14s:%4d:%25s: " fmt "\n", THREAD_ID, __FILENAME__, __LINE__, __FUNCTION__, ##args)
    #define DEBUG_ATTN(fmt, args...) fprintf(stderr, CYN  "ZT_ATTN [%ld] : %14s:%4d:%25s: " fmt "\n" RESET, THREAD_ID, __FILENAME__, __LINE__, __FUNCTION__, ##args)
    #define DEBUG_STACK(fmt, args...) fprintf(stderr, YEL "ZT_STACK[%ld] : %14s:%4d:%25s: " fmt "\n" RESET, THREAD_ID, __FILENAME__, __LINE__, __FUNCTION__, ##args)
    #define DEBUG_BLANK(fmt, args...) fprintf(stderr,     "ZT_INFO [%ld] : %14s:%4d:" fmt "\n", THREAD_ID, __FILENAME__, __LINE__, ##args)
  #endif
 #else
  #define DEBUG_INFO(fmt, args...)
  #define DEBUG_BLANK(fmt, args...)
  #define DEBUG_ATTN(fmt, args...)
  #define DEBUG_STACK(fmt, args...)
 #endif
 
 #if DEBUG_LEVEL >= MSG_TRANSFER
  #if defined(__ANDROID__)
    #define DEBUG_TRANS(fmt, args...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ZT_TRANS : %14s:%4d:%25s: " fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args))
  #else
    #define DEBUG_TRANS(fmt, args...) fprintf(stderr, GRN "ZT_TRANS[%ld] : %14s:%4d:%25s: " fmt "\n" RESET, THREAD_ID, __FILENAME__, __LINE__, __FUNCTION__, ##args)
  #endif
 #else
  #define DEBUG_TRANS(fmt, args...)
 #endif
 
 #if DEBUG_LEVEL >= MSG_EXTRA
   #if defined(__ANDROID__)
    #define DEBUG_EXTRA(fmt, args...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ZT_EXTRA : %14s:%4d:%25s: " fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args))
  #else
    #define DEBUG_EXTRA(fmt, args...) fprintf(stderr, "ZT_EXTRA[%ld] : %14s:%4d:%25s: " fmt "\n", THREAD_ID, __FILENAME__, __LINE__, __FUNCTION__, ##args)
  #endif
 #else
  #define DEBUG_EXTRA(fmt, args...)
 #endif

#if DEBUG_LEVEL >= MSG_FLOW
   #if defined(__ANDROID__)
    #define DEBUG_FLOW(fmt, args...) ((void)__android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, "ZT_FLOW : %14s:%4d:%25s: " fmt "\n", __FILENAME__, __LINE__, __FUNCTION__, ##args))
  #else
    #define DEBUG_FLOW(fmt, args...) fprintf(stderr, "ZT_FLOW [%ld] : %14s:%4d:%25s: " fmt "\n", THREAD_ID, __FILENAME__, __LINE__, __FUNCTION__, ##args)
  #endif
 #else
  #define DEBUG_FLOW(fmt, args...)
 #endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ZT_ZEROTIERSDK_H
