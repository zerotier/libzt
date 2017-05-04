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

package zerotier;

import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.ArrayList;
import java.util.zip.ZipError;

public class ZeroTier {

	public static String Version()
	{
		return "1.2.2";
	}
	
    // Socket families
    public static int AF_UNIX = 1;
    public static int AF_INET = 2;

    // Socket types
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

    // Loads JNI code
    static { System.loadLibrary("zt"); }

    // ZeroTier service controls
    public native void ztjni_start(String homeDir);
    public void start(String homeDir) {
    	ztjni_start(homeDir);
    }

    public native void ztjni_join(String nwid);
    public void join(String nwid) {
    	ztjni_join(nwid);
    }

    public native void ztjni_leave(String nwid);
    public void leave(String nwid) {
    	ztjni_leave(nwid);
    }

    public native ArrayList<String> ztjni_get_addresses(String nwid);
    public ArrayList<String> get_addresses(String nwid) {
        int err = -1;
        ArrayList<String> addresses;
        while (err < 0) {
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
            }
            addresses = ztjni_get_addresses(nwid);
            if (addresses.size() > 0) {
                return addresses;
            }
        }
        return null;
    }
    
    public native boolean ztjni_running();
    public boolean running() {
        return ztjni_running();
    }

    public native int ztjni_socket(int family, int type, int protocol);
    public int socket(int family, int type, int protocol) {
        return ztjni_socket(family, type, protocol);
    }

    public native int ztjni_connect(int fd, String addr, int port);

    public int connect(int sock, Address zaddr, String nwid) {
        return connect(sock, zaddr.Address(), zaddr.Port(), nwid);
    }

    public int connect(int sock, String addr, int port, String nwid)
    {
        int err = -1;
        ArrayList<String> addresses;
        while (err < 0) {
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
            }
            addresses = ztjni_get_addresses(nwid);
            if (addresses.size() > 0) {
                if(!addresses.get(0).startsWith("-1.-1.-1.-1/-1")) {
                    err = ztjni_connect(sock, addr, port);
                }
            }
        }
        return err;
    }

    public native int ztjni_bind(int fd, String addr, int port);

    public int bind(int sock, Address zaddr, String nwid) {
        return bind(sock, zaddr.Address(), zaddr.Port(), nwid);
    }
    public int bind(int sock, String addr, int port, String nwid) {
        int err = -1;
        ArrayList<String> addresses;
        while (err < 0) {
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
            }
            addresses = ztjni_get_addresses(nwid);
            if (addresses.size() > 0) {
                if(!addresses.get(0).startsWith("-1.-1.-1.-1/-1")) {
                    err = ztjni_bind(sock, addr, port);
                }
            }
        }
        return err;
    }

    public native int ztjni_accept4(int fd, String addr, int port);
    public int accept4(int fd, String addr, int port) {
        return ztjni_accept4(fd,addr,port);
    }

    public native int ztjni_accept(int fd, zerotier.Address addr);
    public int accept(int fd, zerotier.Address addr) {
        return ztjni_accept(fd, addr);
    }

    public native int ztjni_listen(int fd, int backlog);
    public int listen(int fd, int backlog) {
        return ztjni_listen(fd,backlog);
    }

    public native int ztjni_close(int fd);
    public int close(int fd) {
        return ztjni_close(fd);
    }

    public native int ztjni_read(int fd, byte[] buf, int len);
    public int read(int fd, byte[] buf, int len) {
        return ztjni_read(fd, buf, len);
    }

    public native int ztjni_write(int fd, byte[] buf, int len);
    public int write(int fd, byte[] buf, int len) {
        return ztjni_write(fd, buf, len);
    }

    public native int ztjni_sendto(int fd, byte[] buf, int len, int flags, zerotier.Address addr);
    public int sendto(int fd, byte[] buf, int len, int flags, zerotier.Address addr){
        return ztjni_sendto(fd,buf,len,flags,addr);
    }

    public native int ztjni_send(int fd, byte[] buf, int len, int flags);
    public int send(int fd, byte[] buf, int len, int flags) {
        return ztjni_send(fd, buf, len, flags);
    }

    public native int ztjni_recvfrom(int fd, byte[] buf, int len, int flags, zerotier.Address addr);
    public int recvfrom(int fd, byte[] buf, int len, int flags, zerotier.Address addr){
        return ztjni_recvfrom(fd,buf,len,flags,addr);
    }

    public native int ztjni_fcntl(int sock, int cmd, int flag);
    public int fcntl(int sock, int cmd, int flag) {
        return  ztjni_fcntl(sock, F_SETFL, O_NONBLOCK);
    }
}