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

// Simple Java example for libzt using JNI

import zerotier.ZeroTier;
import java.net.*;
import java.lang.Thread;

public class ExampleApp {
	
    public native int loadsymbols();
    public native void startOneService();
    
    // load libzt.dylib or libzt.so
	static {
        System.loadLibrary("zt");
    }

	public static void main(String[] args) {
	
        final ZeroTier libzt = new ZeroTier();
                
        new Thread(new Runnable() {
            public void run() {
                String path = "/Users/joseph/op/zt/libzt/ztjni";
                long nwid = 0xa09acf0233ac70fdL;

                // METHOD 1 (easy)
                // Blocking call that waits for the core, userspace stack and IP assignment before unblocking
                if (true)
                {
                    libzt.startjoin(path, nwid);
                }

                // METHOD 2
                // Optionally-nonblocking call. You'll have to use the below process to determine when you 
                // are allowed to stack making socket calls. The advantage of this method is that you can 
                // get your nodeId before joining the network.
                if (false) {
                    libzt.start(path, true);
                    // Wait for core service to start
                    while(!libzt.core_running()) {
                        try {
                            Thread.sleep(1000);
                        } 
                        catch(InterruptedException ex) {
                            Thread.currentThread().interrupt();
                        }
                    }
                    System.out.println("core started");
                    long nodeId = libzt.get_node_id();
                    System.out.println("nodeId=" + Long.toHexString(nodeId));
                    libzt.join(nwid);
                    // Wait for userspace stack to start, we trigger this by joining a network
                    while(!libzt.stack_running()) {
                        try {
                            Thread.sleep(1000);
                        } 
                        catch(InterruptedException ex) {
                            Thread.currentThread().interrupt();
                        }
                    }
                }

                System.out.println("core and stack started, now ready for socket API calls");

                int num_addresses = libzt.get_num_assigned_addresses(nwid);
                System.out.println("number of assigned addresses for this node on this network = " + String.valueOf(num_addresses));
                
                // get IPv4 address
                //InetAddress assigned = libzt.get_address(nwid, libzt.AF_INET6);
                //System.out.println("assigned address = " + assigned.toString());

                // get address at arbitrary (index < num_addresses)
                //assigned = libzt.get_address_at_index(nwid, 0);
                //System.out.println("assigned address = " + assigned.toString());

                // get IPv6 address
                //assigned = libzt.get_address(nwid, libzt.AF_INET6);
                //System.out.println("assigned address = " + assigned.toString());

                String homePath = libzt.get_path();
                System.out.println("homePath=" + homePath);

                while(!libzt.has_address(nwid)) {
                    try {
                        Thread.sleep(1000);
                    } 
                    catch(InterruptedException ex) {
                        Thread.currentThread().interrupt();
                    }
                }


                //InetAddress assigned = libzt.get_address(nwid);
                //System.out.println("assigned address = " + assigned.toString());

                int fd = 0, err = 0;
                if ((fd = libzt.socket(libzt.AF_INET, libzt.SOCK_STREAM, 0)) < 0) {
                    System.out.println("error creating socket");
                    return;
                }
                System.out.println("Created socket");

        while(true)
        {
            try { Thread.sleep(3000); } 
            catch (InterruptedException e) { e.printStackTrace(); }
        }

        /*
                InetSocketAddress remoteAddr = new InetSocketAddress("172.27.54.9", 3434);

                if ((err = libzt.connect(fd, remoteAddr)) < 0) {
                    System.out.println("error connecting");
                    return;
                }
        */ 
/*
                InetSocketAddress localAddr = new InetSocketAddress("0.0.0.0", 3434);

                if ((err = libzt.bind(fd, addr)) < 0) {
                    System.out.println("error binding socket to virtual interface");
                    return;
                }  
                */
            }
        }).start();
         
        while(true)
        {
        	try { Thread.sleep(3000); } 
        	catch (InterruptedException e) { e.printStackTrace(); }
        }
    }
}