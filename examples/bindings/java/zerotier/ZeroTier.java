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

package zerotier;

public class ZeroTier {
	
    // socket families
    public static int AF_UNIX = 1;
    public static int AF_INET = 2;
    // socket types
    public static int SOCK_STREAM = 1;
    public static int SOCK_DGRAM = 2;
    // fcntl flags
    public static int O_APPEND = 1024;
    public static int O_NONBLOCK = 2048;
    public static int O_ASYNC = 8192;
    public static int O_DIRECT = 65536;
    public static int O_NOATIME = 262144;
    // fcntl cmds
    public static int F_GETFL = 3;
    public static int F_SETFL = 4;
    // basic service controls
    public native void start(String homeDir, boolean blocking);
    public native void startjoin(String homeDir, String nwid);
    public native void stop();
    public native boolean running();
    public native void join(String nwid);
    public native void leave(String nwid);
    // advanced service controls
    public native void get_path();
    public native int get_id();
    public native void get_6plane_addr();
    public native void get_rfc4193_addr();
    // socket API
    public native int socket(int family, int type, int protocol);
    public native int connect(int fd, String addr, int port);
    public native int bind(int fd, String addr, int port);
    public native int listen(int fd, int backlog);
    public native int accept(int fd, Address addr);
    public native int accept4(int fd, String addr, int port);
    public native int close(int fd);
    //public native int setsockopt();
    //public native int getsockopt();
    public native int read(int fd, byte[] buf, int len);
    public native int write(int fd, byte[] buf, int len);
    public native int sendto(int fd, byte[] buf, int len, int flags, Address addr);
    public native int send(int fd, byte[] buf, int len, int flags);
    public native int recvfrom(int fd, byte[] buf, int len, int flags, Address addr);
    public native int shutdown(int fd, int how);
    //public native int getsockname();
    //public native int getpeername();
    //public native int gethostname();
    ///public native int sethostname();
    //public native int gethostbyname();
    //public native int poll();
    //public native int select();
    public native int fcntl(int sock, int cmd, int flag);
    //public native int ioctl(int fd, long request, ? );
    // stack controls
    public native int add_dns();
    public native int del_dns();
}