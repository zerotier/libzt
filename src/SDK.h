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

#ifdef __cplusplus
extern "C" {
#endif

#define INTERCEPT_ENABLED  111
#define INTERCEPT_DISABLED 222

void zt_init_rpc(char *nwid);
const char *get_netpath();

#if defined(__linux__)
	static int (*realaccept4)(ACCEPT4_SIG) = 0;
	#if !defined(__ANDROID__)
		static int (*realsyscall)(SYSCALL_SIG) = 0;
	#endif
#endif

#if !defined(__ANDROID__)
    bool check_intercept_enabled_for_thread();
	static int (*realbind)(BIND_SIG) = 0;
	static int (*realsendmsg)(SENDMSG_SIG) = 0;
	static ssize_t (*realsendto)(SENDTO_SIG) = 0;
	static int (*realrecvmsg)(RECVMSG_SIG) = 0;
	static int (*realrecvfrom)(RECVFROM_SIG) = 0;
#endif
	static int (*realconnect)(CONNECT_SIG) = 0;
	static int (*realaccept)(ACCEPT_SIG) = 0;
	static int (*reallisten)(LISTEN_SIG) = 0;
	static int (*realsocket)(SOCKET_SIG) = 0;
	static int (*realsetsockopt)(SETSOCKOPT_SIG) = 0;
	static int (*realgetsockopt)(GETSOCKOPT_SIG) = 0;
	static int (*realclose)(CLOSE_SIG) = 0;
	static int (*realgetsockname)(GETSOCKNAME_SIG) = 0;

ssize_t zt_sendto(SENDTO_SIG);
ssize_t zt_sendmsg(SENDMSG_SIG);
ssize_t zt_recvfrom(RECVFROM_SIG);
ssize_t zt_recvmsg(RECVMSG_SIG);

    
#if defined(__UNITY_3D__)
    ssize_t zt_recv(int fd, void *buf, int len);
    ssize_t zt_send(int fd, void *buf, int len);
    int zt_set_nonblock(int fd);
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

#ifdef __cplusplus
}
#endif

#endif // _ZT_SDK_H