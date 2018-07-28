/*
 * ZeroTier SDK - Network Virtualization Everywhere
 * Copyright (C) 2011-2018  ZeroTier, Inc.  https://www.zerotier.com/
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

package com.zerotier.libzt;

import java.net.*;

public class ZeroTier {

    public static int AF_INET = 2;
    public static int AF_INET6 = 30;
    public static int SOCK_STREAM = 1;
    public static int SOCK_DGRAM = 2;
    public static int O_APPEND = 1024;
    public static int O_NONBLOCK = 2048;
    public static int O_ASYNC = 8192;
    public static int O_DIRECT = 65536;
    public static int O_NOATIME = 262144;
    public static int F_GETFL = 3;
    public static int F_SETFL = 4;

    public native void start(String homePath, boolean blocking);
    public native void startjoin(String homePath, long nwid);
    public native void stop();
    public native boolean core_running();
    public native boolean stack_running();
    public native boolean ready();
    public native int join(long nwid);
    public native int leave(long nwid);
    public native String get_path();
    public native long get_node_id();
    public native int get_num_assigned_addresses(long nwid);
    public native boolean get_address_at_index(long nwid, int index, ZTSocketAddress addr);
    public native boolean has_address(long nwid);
    public native boolean get_address(long nwid, int address_family, ZTSocketAddress addr);
    public native void get_6plane_addr(long nwid, long nodeId, ZTSocketAddress addr);
    public native void get_rfc4193_addr(long nwid, long nodeId, ZTSocketAddress addr);

    public native int socket(int family, int type, int protocol);
    public native int connect(int fd, ZTSocketAddress addr);
    public native int bind(int fd, ZTSocketAddress addr);
    public native int listen(int fd, int backlog);
    public native int accept(int fd, ZTSocketAddress addr);
    public native int accept4(int fd, String addr, int port);
    public native int close(int fd);
    public native int setsockopt(int fd, int level, int optname, int optval, int optlen);
    public native int getsockopt(int fd, int level, int optname, int optval, int optlen);
    public native int sendto(int fd, byte[] buf, int len, int flags, ZTSocketAddress addr);
    public native int send(int fd, byte[] buf, int len, int flags);
    public native int recv(int fd, byte[] buf, int len, int flags);
    public native int recvfrom(int fd, byte[] buf, int len, int flags, ZTSocketAddress addr);
    public native int read(int fd, byte[] buf, int len);
    public native int write(int fd, byte[] buf, int len);
    public native int shutdown(int fd, int how);
    public native boolean getsockname(int fd, ZTSocketAddress addr);
    public native int getpeername(int fd, ZTSocketAddress addr);
    public native int fcntl(int sock, int cmd, int flag);
    public native int select(int nfds, ZTFDSet readfds, ZTFDSet writefds, ZTFDSet exceptfds, int timeout_sec, int timeout_usec);
}