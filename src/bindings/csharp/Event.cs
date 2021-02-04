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

using System.Net;

using ZeroTier;

namespace ZeroTier
{
	/* Convenience structures for exposing lower level operational details to the user.
	These structures do not have the same memory layout or content as those found in
	include/ZeroTierSockets.h */

	/// <summary>
	/// Structure containing information about the local Node.
	/// </summary>
	public class NodeDetails
	{
		// Node ID
		public ulong nodeId;

		/**
		 * The port used by the service to send and receive
		 * all encapsulated traffic
		 */
		public ushort primaryPort;
		public ushort secondaryPort;
		public ushort tertiaryPort;

		/**
		 * ZT version
		 */
		public byte versionMajor;
		public byte versionMinor;
		public byte versionRev;
	}

	public class MulticastSubscription
	{
		ulong macAddress;
		uint AdditionalDistinguishingInformation;
	}

	/// <summary>
	/// Structure containing information about virtual networks.
	/// </summary>
	public class NetworkDetails
	{
		public ulong networkId;
		public ulong macAddress;
		public string networkName;
		//public byte status;
		public byte type;
		public ushort mtu;
		public bool bridgingAllowed;
		public bool broadcastEnabled;
		//public int portError;
		public IPAddress[] assignedAddresses;
		public IPAddress[] routes;
		public MulticastSubscription[] multicastSubscroptions;
	}

	/// <summary>
	/// Structure containing state information about the low-level ethernet driver.
	/// </summary>
	public class NetifDetails
	{
		public ulong networkId;
		public ulong macAddress;
		public int mtu;
	}

	/// <summary>
	/// Structure containing routing information for networks.
	/// </summary>
	public class RouteDetails
	{
		public EndPoint target;
		public EndPoint via;
		public ushort flags;
		public ushort metric;
	}

	/// <summary>
	/// Structure containing information about remote peer reachability.
	/// </summary>
	public class PeerDetails
	{
		ulong nodeId;
		byte versionMajor;
		byte versionMinor;
		byte versionRev;
		int latency;
		byte role;
		IPAddress[] paths;
	}

	/// <summary>
	/// Structure containing information about assigned addresses.
	/// </summary>
	public class AddrDetails
	{
		ulong networkId;
		IPAddress address;
	}

	/// <summary>
	/// Class used to convey details of a low-level network event to the user.
	/// </summary>
	public class Event
	{
		int _eventCode;
		string _eventName;

		public NodeDetails nodeDetails;
		public NetworkDetails networkDetails;
		public NetifDetails netifDetails;
		public RouteDetails routeDetails;
		public PeerDetails peerDetails;
		public AddrDetails addrDetails;

		public Event(int eventCode, string eventName)
		{
			_eventCode = eventCode;
			_eventName = eventName;
			nodeDetails = null;
			networkDetails = null;
			netifDetails = null;
			routeDetails = null;
			peerDetails = null;
			addrDetails = null;
		}

		public int EventCode {
			get {
				return _eventCode;
			}
		}

		public string EventName {
			get {
				return _eventName;
			}
		}
	}
}