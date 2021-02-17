/*
 * Copyright (c)2013-2021 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2025-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

using System.Runtime.InteropServices;
using System;

using ZeroTier;

// Prototype of callback used by ZeroTier to signal events to C# application
[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate void CSharpCallbackWithStruct(IntPtr msgPtr);

/// <summary>
/// ZeroTier SDK
/// </summary>
namespace ZeroTier.Core
{
	public delegate void ZeroTierManagedEventCallback(ZeroTier.Core.Event nodeEvent);

	/// <summary>
	/// ZeroTier Node - Virtual network subsystem
	/// </summary>
	public class Node
	{
		static ulong _nodeId = 0x0;
		static bool _isOnline = false;
		static bool _joinedAtLeastOneNetwork = false;
		static bool _hasBeenFreed = false;
		string _configFilePath;
		ushort _servicePort;
		static ZeroTierManagedEventCallback _managedCallback;

		// Callback used internally to ferry events from the C++ layer
		static void OnZeroTierEvent(IntPtr msgPtr)
		{
			// Marshal the callback message pointer to a structure that we can inspect
			zts_callback_msg msg =
				(zts_callback_msg)Marshal.PtrToStructure(msgPtr, typeof(zts_callback_msg));

			ZeroTier.Core.Event newEvent = null;

			// Node events
			if (msg.eventCode == Constants.EVENT_NODE_UP) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NODE_UP");
			}
			if (msg.eventCode == Constants.EVENT_NODE_ONLINE) {
				_isOnline = true;
				// Marshal the node details pointer to a structure
				zts_node_details details =
					(zts_node_details)Marshal.PtrToStructure(msg.node, typeof(zts_node_details));
				_nodeId = details.address;
				newEvent = new ZeroTier.Core.Event(msg.eventCode, "EVENT_NODE_ONLINE");
			}
			if (msg.eventCode == Constants.EVENT_NODE_OFFLINE) {
				_isOnline = false;
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NODE_OFFLINE");
			}
			if (msg.eventCode == Constants.EVENT_NODE_NORMAL_TERMINATION) {
				_isOnline = false;
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NODE_NORMAL_TERMINATION");
			}
			if (msg.eventCode == Constants.EVENT_NODE_DOWN) {
				_isOnline = false;
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NODE_DOWN");
			}
			if (msg.eventCode == Constants.EVENT_NODE_IDENTITY_COLLISION) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NODE_IDENTITY_COLLISION");
				_isOnline = false;
			}
			if (msg.eventCode == Constants.EVENT_NODE_UNRECOVERABLE_ERROR) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NODE_UNRECOVERABLE_ERROR");
				_isOnline = false;
			}

			// Network events
			if (msg.eventCode == Constants.EVENT_NETWORK_NOT_FOUND) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NETWORK_NOT_FOUND");
			}
			if (msg.eventCode == Constants.EVENT_NETWORK_REQ_CONFIG) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NETWORK_REQ_CONFIG");
			}
			if (msg.eventCode == Constants.EVENT_NETWORK_ACCESS_DENIED) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NETWORK_ACCESS_DENIED");
			}
			if (msg.eventCode == Constants.EVENT_NETWORK_READY_IP4) {
				_joinedAtLeastOneNetwork = true;
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NETWORK_READY_IP4");
			}
			if (msg.eventCode == Constants.EVENT_NETWORK_READY_IP6) {
				_joinedAtLeastOneNetwork = true;
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NETWORK_READY_IP6");
			}
			if (msg.eventCode == Constants.EVENT_NETWORK_DOWN) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NETWORK_DOWN");
			}
			if (msg.eventCode == Constants.EVENT_NETWORK_CLIENT_TOO_OLD) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NETWORK_CLIENT_TOO_OLD");
			}
			if (msg.eventCode == Constants.EVENT_NETWORK_REQ_CONFIG) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NETWORK_REQ_CONFIG");
			}
			if (msg.eventCode == Constants.EVENT_NETWORK_OK) {
				zts_network_details unmanagedDetails =
					(zts_network_details)Marshal.PtrToStructure(msg.network, typeof(zts_network_details));
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NETWORK_OK");
				newEvent.networkDetails = new NetworkDetails();
				newEvent.networkDetails.networkId = unmanagedDetails.nwid;
			}
			if (msg.eventCode == Constants.EVENT_NETWORK_ACCESS_DENIED) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NETWORK_ACCESS_DENIED");
			}
			if (msg.eventCode == Constants.EVENT_NETWORK_READY_IP4_IP6) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NETWORK_READY_IP4_IP6");
			}
			if (msg.eventCode == Constants.EVENT_NETWORK_UPDATE) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NETWORK_UPDATE");
			}

			// Stack events
			if (msg.eventCode == Constants.EVENT_STACK_UP) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_STACK_UP");
			}
			if (msg.eventCode == Constants.EVENT_STACK_DOWN) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_STACK_DOWN");
			}

			// Address events
			if (msg.eventCode == Constants.EVENT_ADDR_ADDED_IP4) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_ADDR_ADDED_IP4");
			}
			if (msg.eventCode == Constants.EVENT_ADDR_ADDED_IP6) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_ADDR_ADDED_IP6");
			}
			if (msg.eventCode == Constants.EVENT_ADDR_REMOVED_IP4) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_ADDR_REMOVED_IP4");
			}
			if (msg.eventCode == Constants.EVENT_ADDR_REMOVED_IP6) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_ADDR_REMOVED_IP6");
			}
			// peer events
			if (msg.eventCode == Constants.EVENT_PEER_DIRECT) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_PEER_DIRECT");
			}
			if (msg.eventCode == Constants.EVENT_PEER_RELAY) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_PEER_RELAY");
			}
			// newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_PEER_UNREACHABLE");
			if (msg.eventCode == Constants.EVENT_PEER_PATH_DISCOVERED) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_PEER_PATH_DISCOVERED");
			}
			if (msg.eventCode == Constants.EVENT_PEER_PATH_DEAD) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_PEER_PATH_DEAD");
			}

			// Route events
			if (msg.eventCode == Constants.EVENT_ROUTE_ADDED) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_ROUTE_ADDED");
			}
			if (msg.eventCode == Constants.EVENT_ROUTE_REMOVED) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_ROUTE_REMOVED");
			}

			// Netif events
			if (msg.eventCode == Constants.EVENT_NETIF_UP) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NETIF_UP");
			}
			if (msg.eventCode == Constants.EVENT_NETIF_DOWN) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NETIF_DOWN");
			}
			if (msg.eventCode == Constants.EVENT_NETIF_REMOVED) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NETIF_REMOVED");
			}
			if (msg.eventCode == Constants.EVENT_NETIF_LINK_UP) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NETIF_LINK_UP");
			}
			if (msg.eventCode == Constants.EVENT_NETIF_LINK_DOWN) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_NETIF_LINK_DOWN");
			}
			if (msg.eventCode == Constants.EVENT_ADDR_REMOVED_IP6) {
				newEvent = new ZeroTier.Core.Event(msg.eventCode,"EVENT_ADDR_REMOVED_IP6");
			}

			// Pass the converted Event to the managed callback (visible to user)
			if (newEvent != null) {
				_managedCallback(newEvent);
			}
		}

		/// <summary>
		/// Creates a new Node. Start this object by calling Start().
		/// </summary>
		/// <param name="configFilePath">Where keys and config files will be stored on the filesystem</param>
		/// <param name="managedCallback">Where you would like to receive ZeroTier event notifications</param>
		/// <param name="servicePort">The port ZeroTier will use to send its encrypted </param>
		/// <returns></returns>
		public Node(string configFilePath, ZeroTierManagedEventCallback managedCallback, UInt16 servicePort)
		{
			if (String.IsNullOrEmpty(configFilePath)) {
				throw new ArgumentNullException("configFilePath");
			}
			if (managedCallback == null) {
				throw new ArgumentNullException("managedCallback");
			}
			_nodeId = 0x0;
			_configFilePath = configFilePath;
			_servicePort = servicePort;
			_managedCallback = new ZeroTierManagedEventCallback(managedCallback);
		}

		/// <summary>
		/// Starts the ZeroTier node/service
		/// </summary>
		/// <returns></returns>
		public int Start()
		{
			if (_hasBeenFreed == true) {
				throw new ObjectDisposedException("ZeroTier Node has previously been freed. Restart application to create new instance.");
			}
			return zts_start(_configFilePath,OnZeroTierEvent,_servicePort);
		}

		/// <summary>
		/// Frees (most) resources used by ZeroTier. ZeroTier may not be started again after this call.
		/// </summary>
		/// <returns></returns>
		public int Free()
		{
			_nodeId = 0x0;
			_hasBeenFreed = true;
			return zts_free();
		}

		/// <summary>
		/// Stop all ZeroTier service activity. The service may be started again with Start().
		/// </summary>
		/// <returns></returns>
		public int Stop()
		{
			_nodeId = 0x0;
			return zts_stop();
		}

		/// <summary>
		/// Restarts the ZeroTier service, internal stack and driver. (Mostly used for debugging.)
		/// </summary>
		/// <returns></returns>
		public int Restart()
		{
			_nodeId = 0x0;
			return zts_restart();
		}
		
		/// <summary>
		/// Requests to join a ZeroTier network. Remember to authorize your node/device.
		/// </summary>
		/// <param name="nwid">Network ID</param>
		/// <returns></returns>
		public int Join(ulong nwid)
		{
			return zts_join(nwid);
		}

		/// <summary>
		/// Leaves a ZeroTier network.
		/// </summary>
		/// <param name="nwid"></param>
		/// <returns></returns>
		public int Leave(ulong nwid)
		{
			return zts_leave(nwid);
		}

		/// <summary>
		/// Returns whether the Node is online (able to reach the internet.)
		/// </summary>
		/// <returns></returns>
		public bool IsOnline()
		{
			return _isOnline;
		}

		/// <summary>
		/// Returns whether routing information is available from at least one ZeroTier network.
		/// </summary>
		/// <returns></returns>
		public bool HasRoutes()
		{
			return _joinedAtLeastOneNetwork;
		}

		public ulong NodeId
		{
			get {
				return _nodeId;
			}
		}

		/* Structures and functions used internally to communicate with
		lower-level C API defined in include/ZeroTierSockets.h */

		[DllImport("libzt", EntryPoint="CSharp_zts_start")]
		static extern int zts_start(string arg1, CSharpCallbackWithStruct arg2, ushort arg3);

		[DllImport("libzt", EntryPoint="CSharp_zts_stop")]
		static extern int zts_stop();

		[DllImport("libzt", EntryPoint="CSharp_zts_restart")]
		static extern int zts_restart();

		[DllImport("libzt", EntryPoint="CSharp_zts_free")]
		static extern int zts_free();

		[DllImport("libzt", EntryPoint="CSharp_zts_join")]
		static extern int zts_join(ulong arg1);

		[DllImport("libzt", EntryPoint="CSharp_zts_leave")]
		static extern int zts_leave(ulong arg1);

		[DllImport("libzt", EntryPoint="CSharp_zts_allow_network_caching")]
		static extern int zts_allow_network_caching(byte arg1);

		[DllImport("libzt", EntryPoint="CSharp_zts_allow_peer_caching")]
		static extern int zts_allow_peer_caching(byte arg1);

		[DllImport("libzt", EntryPoint="CSharp_zts_allow_local_conf")]
		static extern int zts_allow_local_conf(byte arg1);

		[DllImport("libzt", EntryPoint="CSharp_zts_orbit")]
		static extern int zts_orbit(ulong arg1, ulong arg2);

		[DllImport("libzt", EntryPoint="CSharp_zts_deorbit")]
		static extern int zts_deorbit(ulong arg1);

		[DllImport("libzt", EntryPoint="CSharp_zts_get_6plane_addr")]
		static extern int zts_get_6plane_addr(IntPtr arg1, ulong arg2, ulong arg3);

		[DllImport("libzt", EntryPoint="CSharp_zts_get_rfc4193_addr")]
		static extern int zts_get_rfc4193_addr(IntPtr arg1, ulong arg2, ulong arg3);

		[DllImport("libzt", EntryPoint="CSharp_zts_generate_adhoc_nwid_from_range")]
		static extern ulong zts_generate_adhoc_nwid_from_range(ushort arg1, ushort arg2);

		[DllImport("libzt", EntryPoint="CSharp_zts_delay_ms")]
		static extern void zts_delay_ms(int arg1);

		[DllImport("libzt", EntryPoint="CSharp_zts_errno_get")]
		static extern int zts_errno_get();

		[StructLayout(LayoutKind.Sequential)]
		struct zts_node_details
		{
			public ulong address;
		}

		/**
		 * Virtual network configuration
		 */
		 [StructLayout(LayoutKind.Sequential)]
		struct zts_network_details
		{
			/**
			 * 64-bit ZeroTier network ID
			 */
			public ulong nwid;

			/**
			 * Ethernet MAC (48 bits) that should be assigned to port
			 */
			public ulong mac;

			/**
			 * Network name (from network configuration master)
			 */
			[MarshalAs(UnmanagedType.ByValArray, SizeConst = 128)]
			public byte[] name;

			/**
			 * Network configuration request status
			 */
			public byte status; // ?

			/**
			 * Network type
			 */
			public byte type; // ?

			/**
			 * Maximum interface MTU
			 */
			public uint mtu;

			/**
			 * If nonzero, the network this port belongs to indicates DHCP availability
			 *
			 * This is a suggestion. The underlying implementation is free to ignore it
			 * for security or other reasons. This is simply a netconf parameter that
			 * means 'DHCP is available on this network.'
			 */
			public int dhcp;

			/**
			 * If nonzero, this port is allowed to bridge to other networks
			 *
			 * This is informational. If this is false (0), bridged packets will simply
			 * be dropped and bridging won't work.
			 */
			public int bridge;

			/**
			 * If nonzero, this network supports and allows broadcast (ff:ff:ff:ff:ff:ff) traffic
			 */
			public int broadcastEnabled;

			/**
			 * If the network is in PORT_ERROR state, this is the (negative) error code most recently reported
			 */
			public int portError;

			/**
			 * Revision number as reported by controller or 0 if still waiting for config
			 */
			public ulong netconfRevision;

			/**
			 * Number of assigned addresses
			 */
			public uint assignedAddressCount;

			/**
			 * ZeroTier-assigned addresses (in sockaddr_storage structures)
			 *
			 * For IP, the port number of the sockaddr_XX structure contains the number
			 * of bits in the address netmask. Only the IP address and port are used.
			 * Other fields like interface number can be ignored.
			 *
			 * This is only used for ZeroTier-managed address assignments sent by the
			 * virtual network's configuration master.
			 */
			//struct zts_sockaddr_storage assignedAddresses[ZTS_MAX_ZT_ASSIGNED_ADDRESSES];

			/**
			 * Number of ZT-pushed routes
			 */
			public uint routeCount;

			/**
			 * Routes (excluding those implied by assigned addresses and their masks)
			 */
			//ZTS_VirtualNetworkRoute routes[ZTS_MAX_NETWORK_ROUTES];

			/**
			 * Number of multicast groups subscribed
			 */
			public uint multicastSubscriptionCount;

			/**
			 * Multicast groups to which this network's device is subscribed
			 */
			//struct {
			//	uint64_t mac; /* MAC in lower 48 bits */
			//	uint32_t adi; /* Additional distinguishing information, usually zero except for IPv4 ARP groups */
			//} multicastSubscriptions[ZTS_MAX_MULTICAST_SUBSCRIPTIONS];
		}

		[StructLayout(LayoutKind.Sequential)]
		struct zts_callback_msg
		{
			public short eventCode;
			//[MarshalAs(UnmanagedType.LPStruct, SizeConst = 4)]
			public IntPtr node;
			public IntPtr network;
		}


		/// <summary>
		/// Gets the value of errno from the unmanaged region
		/// </summary>
		/// <value></value>
		public static int ErrNo {
			get {
				return zts_errno_get();
			}
		}
	}
}
