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

import com.zerotier.libzt.ZeroTier;
import com.zerotier.libzt.ZeroTierEventListener;

public class ExampleApp
{
	static void sleep(int ms)
	{
		try { Thread.sleep(ms); } 
		catch (InterruptedException e) { e.printStackTrace(); }
	}

	public static void main(String[] args) 
	{
		// Set up event listener and start service
		MyZeroTierEventListener listener = new MyZeroTierEventListener();
		int servicePort = 9994;
		ZeroTier.start("test/path", listener, servicePort);
		// Wait for EVENT_NODE_ONLINE
		System.out.println("waiting for node to come online...");
		while (listener.isOnline == false) { sleep(50); }
		System.out.println("joinging network");
		ZeroTier.join(0x0123456789abcdefL);
		// Wait for EVENT_NETWORK_READY_IP4/6
		System.out.println("waiting for network config...");
		while (listener.isNetworkReady == false) { sleep(50); }
		System.out.println("joined");

		/*
		
		Begin using socket API after this point

		Use ZeroTier.ZeroTierSocket, ZeroTier.ZeroTierSocketFactory, etc

		(or)

		ZeroTier.socket(), ZeroTier.connect(), etc

		*/
	}
}
