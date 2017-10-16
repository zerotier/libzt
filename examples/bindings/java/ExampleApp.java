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
        		System.out.println("starting libzt");
        		libzt.startjoin("/Users/joseph/op/zt/libzt/ztjni", "1212121212121212");
                System.out.println("started.");
                // start(path) will not block
                // startjoin(path, nwid) will block
                int fd = 0, err = 0;
                if ((fd = libzt.socket(libzt.AF_INET, libzt.SOCK_STREAM, 0)) < 0) {
                    System.out.println("error creating socket");
                    return;
                }
                if ((err = libzt.bind(fd, "0.0.0.0", 3000)) < 0) {
                    System.out.println("error binding socket to virtual interface");
                    return;
                }
            }
        }).start();
         
        while(true)
        {
        	try { Thread.sleep(3000); } 
        	catch (InterruptedException e) { e.printStackTrace(); }
        }
    }
}