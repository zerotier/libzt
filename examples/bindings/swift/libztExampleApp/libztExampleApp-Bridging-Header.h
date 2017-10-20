/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2017  ZeroTier, Inc.  https://www.zerotier.com/
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
 * You can be released from the requirements of the license by purchasing
 * a commercial license. Buying such a license is mandatory as soon as you
 * develop commercial closed-source software that incorporates or links
 * directly against ZeroTier software without disclosing the source code
 * of your own application.
 */

#ifndef LIBZT_BRIDGING_HEADER_H
#define LIBZT_BRIDGING_HEADER_H

#include <sys/socket.h>
#include "Defs.h"

// ZT SERVICE CONTROLS (documented in include/libzt.h)
void zts_start(const char *path);
void zts_startjoin(const char *path, const char *nwid);
void zts_stop();
int zts_running();
void zts_join(const char *nwid);
void zts_leave(const char *nwid);

// SOCKET API (documented in include/libzt.h)
int zts_connect(ZT_CONNECT_SIG);
int zts_bind(ZT_BIND_SIG);
int zts_accept(ZT_ACCEPT_SIG);
int zts_listen(ZT_LISTEN_SIG);
int zts_socket(ZT_SOCKET_SIG);
int zts_setsockopt(ZT_SETSOCKOPT_SIG);
int zts_getsockopt(ZT_GETSOCKOPT_SIG);
int zts_close(ZT_CLOSE_SIG);
int zts_getsockname(ZT_GETSOCKNAME_SIG);
int zts_getpeername(ZT_GETPEERNAME_SIG);
int zts_recvfrom(ZT_RECVFROM_SIG);
int zts_fcntl(ZT_FCNTL_SIG);
int zts_sendto(ZT_SENDTO_SIG);

#endif /* LIBZT_BRIDGING_HEADER_H */




