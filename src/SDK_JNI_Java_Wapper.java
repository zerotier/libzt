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
package ZeroTier;

import java.net.SocketAddress;

public class SDK {

    // Socket families
    public int AF_UNIX = 1;
    public int AF_INET = 2;

    // Socket types
    public int SOCK_STREAM = 1;
    public int SOCK_DGRAM = 2;

    // Loads JNI code
    static { System.loadLibrary("ZeroTierOneJNI"); }

    // ZeroTier service controls
    public native void zt_start_service(String homeDir);
    public native void zt_join_network(String nwid);
    public native void zt_leave_network(String nwid);
    public native boolean zt_running();

    // Direct-call API
    // --- These calls skip the intercept and interface directly via the RPC mechanism
    public native int zt_socket(int family, int type, int protocol);
    public native int zt_connect(int fd, String addr, int port);
    public native int zt_bind(int fd, String addr, int port);
    public native int zt_accept4(int fd, String addr, int port);
    public native int zt_accept(int fd, String addr, int port, int flags);
    public native int zt_listen(int fd, int backlog);
    //public native int zt_getsockopt(int fd, int type, int protocol);
    //public native int zt_setsockopt(int fd, int type, int protocol);
    //public native int zt_getsockname(int fd, int type, int protocol);
    public native int zt_close(int fd);
}