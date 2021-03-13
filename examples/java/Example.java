//package com.zerotier.libzt.javasimpleexample;

import com.zerotier.libzt.ZeroTier;
import com.zerotier.libzt.ZeroTierEventListener;
import java.math.BigInteger;

public class Example {

	static void sleep(int ms) {
		try {
			Thread.sleep(ms);
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
	}

	public static void main(String[] args) {
		String keyPath="."; // Default to current directory
		BigInteger networkId;
		Integer servicePort=0;
		if (args.length != 3) {
			System.err.println(" Invalid arguments");
			System.err.println(" Usage: example <path_to_write_keys> <nwid> <ztServicePort>");
			System.exit(1);
		}
		keyPath = args[0];
		networkId = new BigInteger(args[1], 16);
		servicePort = Integer.parseInt(args[2]);
		System.out.println("networkId   = " + Long.toHexString(networkId.longValue()));
		System.out.println("keyPath     = " + keyPath);
		System.out.println("servicePort = " + servicePort);

		//
		// BEGIN SETUP
		//

		// Set up event listener and start service
		MyZeroTierEventListener listener = new MyZeroTierEventListener();
		ZeroTier.start(keyPath, (ZeroTierEventListener)listener, servicePort);
		// Wait for EVENT_NODE_ONLINE
		System.out.println("waiting for node to come online...");
		while (listener.isOnline == false) { sleep(50); }
		System.out.println("joining network");
		ZeroTier.join(networkId.longValue());
		// Wait for EVENT_NETWORK_READY_IP4/6
		System.out.println("waiting for network config...");
		while (listener.isNetworkReady == false) { sleep(50); }
		System.out.println("joined");

		//
		// END SETUP
		//

		sleep(120000);

		/**
		 *
		 * Begin using socket API after this point:
		 *
		 *    ZeroTier.ZeroTierSocket, ZeroTier.ZeroTierSocketFactory, etc
		 *
		 * (or)
		 *
		 *    ZeroTier.socket(), ZeroTier.connect(), etc
		 */
	}
}

/**
 * Example event handling
 */
class MyZeroTierEventListener implements ZeroTierEventListener {

	public boolean isNetworkReady = false;
	public boolean isOnline = false;

	public void onZeroTierEvent(long id, int eventCode) {
		if (eventCode == ZeroTier.EVENT_NODE_UP) {
			System.out.println("EVENT_NODE_UP: nodeId=" + Long.toHexString(id));
		}
		if (eventCode == ZeroTier.EVENT_NODE_ONLINE) {
			// The core service is running properly and can join networks now
			System.out.println("EVENT_NODE_ONLINE: nodeId=" + Long.toHexString(id));
			isOnline = true;
		}
		if (eventCode == ZeroTier.EVENT_NODE_OFFLINE) {
			// Network does not seem to be reachable by any available strategy
			System.out.println("EVENT_NODE_OFFLINE");
		}
		if (eventCode == ZeroTier.EVENT_NODE_DOWN) {
			// Called when the node is shutting down
			System.out.println("EVENT_NODE_DOWN");
		}
		if (eventCode == ZeroTier.EVENT_NODE_IDENTITY_COLLISION) {
			// Another node with this identity already exists
			System.out.println("EVENT_NODE_IDENTITY_COLLISION");
		}
		if (eventCode == ZeroTier.EVENT_NODE_UNRECOVERABLE_ERROR) {
			// Try again
			System.out.println("EVENT_NODE_UNRECOVERABLE_ERROR");
		}
		if (eventCode == ZeroTier.EVENT_NODE_NORMAL_TERMINATION) {
			// Normal closure
			System.out.println("EVENT_NODE_NORMAL_TERMINATION");
		}
		if (eventCode == ZeroTier.EVENT_NETWORK_READY_IP4) {
			// We have at least one assigned address and we've received a network configuration
			System.out.println("ZTS_EVENT_NETWORK_READY_IP4: nwid=" + Long.toHexString(id));
			isNetworkReady = true;
		}
		if (eventCode == ZeroTier.EVENT_NETWORK_READY_IP6) {
			// We have at least one assigned address and we've received a network configuration
			System.out.println("ZTS_EVENT_NETWORK_READY_IP6: nwid=" + Long.toHexString(id));
			//isNetworkReady = true;
		}
		if (eventCode == ZeroTier.EVENT_NETWORK_DOWN) {
			// Someone called leave(), we have no assigned addresses, or otherwise cannot use this interface
			System.out.println("EVENT_NETWORK_DOWN: nwid=" + Long.toHexString(id));
		}
		if (eventCode == ZeroTier.EVENT_NETWORK_REQ_CONFIG) {
			// Waiting for network configuration
			System.out.println("EVENT_NETWORK_REQ_CONFIG: nwid=" + Long.toHexString(id));
		}
		if (eventCode == ZeroTier.EVENT_NETWORK_OK) {
			// Config received and this node is authorized for this network
			System.out.println("EVENT_NETWORK_OK: nwid=" + Long.toHexString(id));
		}
		if (eventCode == ZeroTier.EVENT_NETWORK_ACCESS_DENIED) {
			// You are not authorized to join this network
			System.out.println("EVENT_NETWORK_ACCESS_DENIED: nwid=" + Long.toHexString(id));
		}
		if (eventCode == ZeroTier.EVENT_NETWORK_NOT_FOUND) {
			// The virtual network does not exist
			System.out.println("EVENT_NETWORK_NOT_FOUND: nwid=" + Long.toHexString(id));
		}
		if (eventCode == ZeroTier.EVENT_NETWORK_CLIENT_TOO_OLD) {
			// The core version is too old
			System.out.println("EVENT_NETWORK_CLIENT_TOO_OLD: nwid=" + Long.toHexString(id));
		}
		if (eventCode == ZeroTier.EVENT_PEER_DIRECT) {
			System.out.println("EVENT_PEER_DIRECT: id=" + Long.toHexString(id));
/*
			ZeroTierPeerDetails details = new ZeroTierPeerDetails();
			ZeroTier.get_peer(id, details);
			System.out.println("address="+Long.toHexString(details.address));
			System.out.println("pathCount="+details.pathCount);
			System.out.println("version="+details.versionMajor+"."+details.versionMinor+"."+details.versionRev);
			System.out.println("latency="+details.latency); // Not relevant
			System.out.println("role="+details.role); // Not relevent
			// Print all known paths
			for (int i=0; i<details.pathCount; i++) {
				System.out.println("addr="+details.paths[i].toString());
			}
*/
		}
		if (eventCode == ZeroTier.EVENT_PEER_RELAY) {
			System.out.println("EVENT_PEER_RELAY: id=" + Long.toHexString(id));
		}
	}
}

