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

import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.ArrayList;
import java.util.zip.ZipError;

import android.os.ParcelFileDescriptor;
import android.util.Log;
import android.util.Pair;

public class SDK {

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
    static { System.loadLibrary("ZeroTierOneJNI"); }

    // ZeroTier service controls
    public native void zt_start_service(String homeDir);
    public void start_service(String homeDir) {
        zt_start_service(homeDir);
    }

    public native void zt_join_network(String nwid);
    public void join_network(String nwid) {
        zt_join_network(nwid);
    }

    public native void zt_leave_network(String nwid);
    public void leave_network(String nwid) {
        zt_leave_network(nwid);
    }

    // ------------------------------------------------------------------------------
    // ------------------------------- get_addresses() ------------------------------
    // ------------------------------------------------------------------------------

    public native ArrayList<String> zt_get_addresses(String nwid);
    public ArrayList<String> get_addresses(String nwid) {
        int err = -1;
        ArrayList<String> addresses;
        while (err < 0) {
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) {
            }
            addresses = zt_get_addresses(nwid);
            if (addresses.size() > 0) {
                return addresses;
            }
        }
        return null;
    }

    public native int zt_get_proxy_port(String nwid);
    public int get_proxy_port(String nwid) {
        return zt_get_proxy_port(nwid);
    }

    public native boolean zt_running();
    public boolean running() {
        return zt_running();
    }


    // ------------------------------------------------------------------------------
    // ----------------------------------- socket() ---------------------------------
    // ------------------------------------------------------------------------------

    public native int zt_socket(int family, int type, int protocol);
    public int socket(int family, int type, int protocol) {
        return zt_socket(family, type, protocol);
    }


    // ------------------------------------------------------------------------------
    // ----------------------------------- connect() --------------------------------
    // ------------------------------------------------------------------------------

    public native int zt_connect(int fd, String addr, int port);

    public int connect(int sock, ZTAddress zaddr, String nwid) {
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
            addresses = zt_get_addresses(nwid);
            if (addresses.size() > 0) {
                if(!addresses.get(0).startsWith("-1.-1.-1.-1/-1")) {
                    err = zt_connect(sock, addr, port);
                }
            }
        }
        return err;
    }

    // ------------------------------------------------------------------------------
    // ------------------------------------ bind() ----------------------------------
    // ------------------------------------------------------------------------------

    public native int zt_bind(int fd, String addr, int port);

    public int bind(int sock, ZTAddress zaddr, String nwid) {
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
            addresses = zt_get_addresses(nwid);
            if (addresses.size() > 0) {
                if(!addresses.get(0).startsWith("-1.-1.-1.-1/-1")) {
                    err = zt_bind(sock, addr, port);
                }
            }
        }
        return err;
    }


    // ------------------------------------------------------------------------------
    // ---------------------------------- accept4() ---------------------------------
    // ------------------------------------------------------------------------------

    public native int zt_accept4(int fd, String addr, int port);
    public int accept4(int fd, String addr, int port) {
        return zt_accept4(fd,addr,port);
    }


    // ------------------------------------------------------------------------------
    // ---------------------------------- accept() ----------------------------------
    // ------------------------------------------------------------------------------

    public native int zt_accept(int fd, ZeroTier.ZTAddress addr);
    public int accept(int fd, ZeroTier.ZTAddress addr) {
        return zt_accept(fd, addr);
    }

    // ------------------------------------------------------------------------------
    // ----------------------------------- listen() ---------------------------------
    // ------------------------------------------------------------------------------

    public native int zt_listen(int fd, int backlog);
    public int listen(int fd, int backlog) {
        return zt_listen(fd,backlog);
    }

    // ------------------------------------------------------------------------------
    // ----------------------------------- close() ----------------------------------
    // ------------------------------------------------------------------------------

    public native int zt_close(int fd);
    public int close(int fd) {
        return close(fd);
    }


    // ------------------------------------------------------------------------------
    // ------------------------------------ read() ----------------------------------
    // ------------------------------------------------------------------------------

    public native int zt_read(int fd, byte[] buf, int len);
    public int read(int fd, byte[] buf, int len) {
        return zt_read(fd, buf, len);
    }

    // ------------------------------------------------------------------------------
    // ----------------------------------- write() ----------------------------------
    // ------------------------------------------------------------------------------

    public native int zt_write(int fd, byte[] buf, int len);
    public int write(int fd, byte[] buf, int len) {
        return zt_write(fd, buf, len);
    }

    // ------------------------------------------------------------------------------
    // ----------------------------------- sendto() ---------------------------------
    // ------------------------------------------------------------------------------

    public native int zt_sendto(int fd, byte[] buf, int len, int flags, ZeroTier.ZTAddress addr);
    public int sendto(int fd, byte[] buf, int len, int flags, ZeroTier.ZTAddress addr){
        return zt_sendto(fd,buf,len,flags,addr);
    }

    // ------------------------------------------------------------------------------
    // ----------------------------------- send() -----------------------------------
    // ------------------------------------------------------------------------------

    public native int zt_send(int fd, byte[] buf, int len, int flags);
    public int send(int fd, byte[] buf, int len, int flags) {
        return zt_send(fd, buf, len, flags);
    }

    // ------------------------------------------------------------------------------
    // ---------------------------------- recvfrom() --------------------------------
    // ------------------------------------------------------------------------------

    public native int zt_recvfrom(int fd, byte[] buf, int len, int flags, ZeroTier.ZTAddress addr);
    public int recvfrom(int fd, byte[] buf, int len, int flags, ZeroTier.ZTAddress addr){
        return zt_recvfrom(fd,buf,len,flags,addr);
    }

    // ------------------------------------------------------------------------------
    // ---------------------------------- recvfrom() --------------------------------
    // ------------------------------------------------------------------------------

    public native int zt_fcntl(int sock, int cmd, int flag);
    public int fcntl(int sock, int cmd, int flag) {
        return  zt_fcntl(sock, F_SETFL, O_NONBLOCK);
    }



    //public static native int zt_getsockopt(int fd, int type, int protocol);
    //public static native int zt_setsockopt(int fd, int type, int protocol);
    //public static native int zt_getsockname(int fd, int type, int protocol);
}