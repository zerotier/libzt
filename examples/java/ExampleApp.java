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

// Simple Java example for libzt using JNI

import zerotier.*;
import java.net.*;
import java.lang.Thread;

public class ExampleApp {
	
	public native int loadsymbols();
	public native void startOneService();

	static { System.loadLibrary("zt"); } // load libzt.dylib or libzt.so

	static void sleep(int ms)
	{
		try { Thread.sleep(ms); } 
		catch (InterruptedException e) { e.printStackTrace(); }
	}

	public static void main(String[] args) 
	{
	
		final ZeroTier libzt = new ZeroTier();

		new Thread(new Runnable() 
		{
			public void run() 
			{
				String path = "/Users/joseph/op/zt/libzt/ztjni"; // Where node's config files are stored
				long nwid = 0xa09acf0233e4b070L;

				// Test modes				
				boolean blocking_start_call = true;
				boolean client_mode = false; 
				boolean tcp = false;
				boolean loop = true; // RX/TX multiple times
				boolean idle = false; // Idle loop after node comes online. For testing reachability

				int fd = -1, client_fd = -1, err, r, w, lengthToRead = 0, flags = 0;
				byte[] rxBuffer;
				byte[] txBuffer = "welcome to the machine".getBytes();
				String remoteAddrStr = "11.7.7.107";
				String localAddrStr = "0.0.0.0";
				int portNo = 4040;

				ZTSocketAddress remoteAddr, localAddr;
				ZTSocketAddress sockname = new ZTSocketAddress();
				ZTSocketAddress addr = new ZTSocketAddress();

				// METHOD 1 (easy)
				// Blocking call that waits for all components of the service to start
				System.out.println("Starting ZT service...");
				if (blocking_start_call) {
					libzt.startjoin(path, nwid);
				}
				// METHOD 2
				// Optional. Non-blocking call to start service. You'll have to use the below process to determine
				// when you are allowed to start making socket calls.
				if (!blocking_start_call) {
					libzt.start(path, true);
					while(!libzt.ready()) {
						try { // Wait for core service to start
							Thread.sleep(250);
						} 
						catch(InterruptedException ex) {
							Thread.currentThread().interrupt();
						}
					}
					System.out.println("Core started. Networks can be joined after this point");
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
				System.out.println("ZT service ready.");
                // Device/Node address info
                System.out.println("path=" + libzt.get_path());
                long nodeId = libzt.get_node_id();
                System.out.println("nodeId=" + Long.toHexString(nodeId));
                int numAddresses = libzt.get_num_assigned_addresses(nwid);
                System.out.println("this node has (" + numAddresses + ") assigned addresses on network " + Long.toHexString(nwid));
                for (int i=0; i<numAddresses; i++) {
                    libzt.get_address_at_index(nwid, i, sockname);
                    //System.out.println("address[" + i + "] = " + sockname.toString()); // ip:port
                    System.out.println("address[" + i + "] = " + sockname.toCIDR());
                }

                libzt.get_6plane_addr(nwid, nodeId, sockname);
                System.out.println("6PLANE address = " + sockname.toCIDR());

				// Idle loop test
				while(idle) { try { Thread.sleep(3000); } catch (InterruptedException e) { e.printStackTrace(); } }

				// TCP
				if (tcp) {
					System.out.println("mode:tcp");
					if ((fd = libzt.socket(libzt.AF_INET, libzt.SOCK_STREAM, 0)) < 0) {
						System.out.println("error creating socket");
						return; 
					}
					// CLIENT
					if (client_mode) {
						System.out.println("mode:client");
						remoteAddr = new ZTSocketAddress(remoteAddrStr, portNo);
						if ((err = libzt.connect(fd, remoteAddr)) < 0) {
							System.out.println("error connecting (err=" + err + ")");
							return;
						}
						String echo_msg = "echo!";
						w = libzt.write(fd, echo_msg.getBytes(), echo_msg.length());
						rxBuffer = new byte[100];
						lengthToRead = 100;
						System.out.println("reading bytes...");
						r = libzt.read(fd, rxBuffer, lengthToRead);
						System.out.println("r="+r);
						System.out.println("string="+new String(rxBuffer));
					}
					
					// SERVER
					if (!client_mode) {
						System.out.println("mode:server");
						localAddr = new ZTSocketAddress(localAddrStr, portNo);

						if ((err = libzt.bind(fd, localAddr)) < 0) {
							System.out.println("error binding socket to virtual interface");
							return;
						} if ((err = libzt.listen(fd, 1)) < 0) {
							System.out.println("error putting socket into listening state");
							return;
						}
						remoteAddr = new ZTSocketAddress(localAddrStr, 0);
						client_fd = -1;
						if ((client_fd = libzt.accept(fd, remoteAddr)) < 0) {
							System.out.println("error accepting incoming connection (err=" + client_fd + ")");
							return;
						}  
						System.out.println("accepted connection (client_fd=" + client_fd + ")");
						rxBuffer = new byte[100];
						lengthToRead = 100;
						System.out.println("reading bytes...");
						r = libzt.read(client_fd, rxBuffer, lengthToRead);
						System.out.println("r="+r);
						System.out.println("string="+new String(rxBuffer));
						System.out.println("writing bytes...");
						String echo_msg = "echo!";
						w = libzt.write(client_fd, echo_msg.getBytes(), echo_msg.length());
						System.out.println("wrote (" + w + ") bytes");
					}
				}

				// UDP
				if (!tcp) {
					System.out.println("mode:udp");
					if ((fd = libzt.socket(libzt.AF_INET, libzt.SOCK_DGRAM, 0)) < 0) {
						System.out.println("error creating socket");
						return; 
					}
					// CLIENT
					if (client_mode) {
						System.out.println("mode:client");
						localAddr = new ZTSocketAddress(localAddrStr, portNo);
						if ((err = libzt.bind(fd, localAddr)) < 0) {
							System.out.println("error binding socket to virtual interface");
							return;
						}
						remoteAddr = new ZTSocketAddress(remoteAddrStr, portNo);
						System.out.println("sending message to: " + remoteAddr.toString());
						if (loop) {
							while (true) {
								sleep(500);
								if ((w = libzt.sendto(fd, txBuffer, txBuffer.length, flags, remoteAddr)) < 0) {
									System.out.println("error sending bytes");
								} else {
									System.out.println("sendto()=" + w);
								}
							}
						} else {
							if ((w = libzt.sendto(fd, txBuffer, txBuffer.length, flags, remoteAddr)) < 0) {
								System.out.println("error sending bytes");
							} else {
								System.out.println("sendto()=" + w);
							}
						}
					}
					// SERVER
					if (!client_mode) {
						System.out.println("mode:server");
						localAddr = new ZTSocketAddress(localAddrStr, portNo);
						System.out.println("binding to " + localAddr.toString());
						if ((err = libzt.bind(fd, localAddr)) < 0) {
							System.out.println("error binding socket to virtual interface");
							return;
						}

						rxBuffer = new byte[100];
						remoteAddr = new ZTSocketAddress("-1.-1.-1.-1", 0);
						while(true) {
							addr = new ZTSocketAddress();
							r = libzt.recvfrom(fd, rxBuffer, rxBuffer.length, flags, remoteAddr);
							System.out.println("read (" + r + ") bytes from " + remoteAddr.toString() + ", buffer = " + new String(rxBuffer));
						}
					}
				}

				libzt.close(client_fd);
				libzt.close(fd);
			}
		}).start();
		 
		while(true)
		{
			try { Thread.sleep(3000); } 
			catch (InterruptedException e) { e.printStackTrace(); }
		}
	}
}