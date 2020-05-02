/**
 * libzt Swift example
 *
 * swiftc -lc++ -import-objc-header ../../include/ZeroTierSockets.h -L. -lzt main.swift -o main;
 * ./main
 */

import Swift
import Foundation

var nodeReady:Bool = false
var networkReady:Bool = false

let myZeroTierEventCallback : @convention(c) (UnsafeMutableRawPointer?) -> Void =
{
	(msgPtr) -> Void in
	let msg = msgPtr?.bindMemory(to: zts_callback_msg.self, capacity: 1)

	var eventCode = msg!.pointee.eventCode

	let node = msg?.pointee.node;
	let network = msg?.pointee.network;

    switch Int32(eventCode)
    {
	case ZTS_EVENT_NODE_ONLINE:
		let nodeId:UInt64 = node!.pointee.address
		print(String(format: "ZTS_EVENT_NODE_ONLINE (%llx)", nodeId))
		nodeReady = true;

	case ZTS_EVENT_NODE_OFFLINE:
		print("ZTS_EVENT_NODE_OFFLINE\n")
		nodeReady = false;

	case ZTS_EVENT_NODE_NORMAL_TERMINATION:
		print("ZTS_EVENT_NODE_NORMAL_TERMINATION\n")

	case ZTS_EVENT_NETWORK_NOT_FOUND:
		let networkId:UInt64 = network!.pointee.nwid
		print(String(format: "ZTS_EVENT_NETWORK_NOT_FOUND (%llx)", networkId))

	case ZTS_EVENT_NETWORK_REQUESTING_CONFIG:
		let networkId:UInt64 = network!.pointee.nwid
		print(String(format: "ZTS_EVENT_NETWORK_REQUESTING_CONFIG (%llx)", networkId))

	case ZTS_EVENT_NETWORK_ACCESS_DENIED:
		let networkId:UInt64 = network!.pointee.nwid
		print(String(format: "ZTS_EVENT_NETWORK_ACCESS_DENIED (%llx)", networkId))

	case ZTS_EVENT_NETWORK_READY_IP4:
		let networkId:UInt64 = network!.pointee.nwid
		print(String(format: "ZTS_EVENT_NETWORK_READY_IP4 (%llx)", networkId))
		networkReady = true;

	case ZTS_EVENT_NETWORK_READY_IP6:
		let networkId:UInt64 = network!.pointee.nwid
		print(String(format: "ZTS_EVENT_NETWORK_READY_IP6 (%llx)", networkId))
		networkReady = true;

	case ZTS_EVENT_NETWORK_DOWN:
		let networkId:UInt64 = network!.pointee.nwid
		print(String(format: "ZTS_EVENT_NETWORK_DOWN (%llx)", networkId))

/*
	// Network stack events
	case ZTS_EVENT_NETIF_UP:
		print("ZTS_EVENT_NETIF_UP --- network=%llx, mac=%llx, mtu=%d\n", 
			msg.netif->nwid,
			msg.netif->mac,
			msg.netif->mtu)
		//networkReady = true;

	case ZTS_EVENT_NETIF_DOWN:
		print("ZTS_EVENT_NETIF_DOWN --- network=%llx, mac=%llx\n", 
			msg.netif->nwid,
			msg.netif->mac)
		//networkReady = true;

	// Address events
	case ZTS_EVENT_ADDR_ADDED_IP4:
		print("ZTS_EVENT_ADDR_ADDED_IP4")
	/*
		char ipstr[INET_ADDRSTRLEN];
		struct zts_sockaddr_in *in4 = (struct zts_sockaddr_in*)&(msg.addr->addr);
		inet_ntop(AF_INET, &(in4->sin_addr), ipstr, INET_ADDRSTRLEN);
		print("ZTS_EVENT_ADDR_NEW_IP4 --- This node's virtual address on network %llx is %s\n", 
			msg.addr->nwid, ipstr)
			*/

	case ZTS_EVENT_ADDR_ADDED_IP6:
		print("ZTS_EVENT_ADDR_ADDED_IP6")
		/*
		char ipstr[INET6_ADDRSTRLEN];
		struct zts_sockaddr_in6 *in6 = (struct zts_sockaddr_in6*)&(msg.addr->addr);
		inet_ntop(AF_INET6, &(in6->sin6_addr), ipstr, INET6_ADDRSTRLEN);
		print("ZTS_EVENT_ADDR_NEW_IP6 --- This node's virtual address on network %llx is %s\n", 
			msg.addr->nwid, ipstr)
			*/

	case ZTS_EVENT_ADDR_REMOVED_IP4:
		print("ZTS_EVENT_ADDR_REMOVED_IP4")
		/*
		char ipstr[INET_ADDRSTRLEN];
		struct zts_sockaddr_in *in4 = (struct zts_sockaddr_in*)&(msg.addr->addr);
		inet_ntop(AF_INET, &(in4->sin_addr), ipstr, INET_ADDRSTRLEN);
		print("ZTS_EVENT_ADDR_REMOVED_IP4 --- The virtual address %s for this node on network %llx has been removed.\n", 
			ipstr, msg.addr->nwid)
			*/

	case ZTS_EVENT_ADDR_REMOVED_IP6:
		print("ZTS_EVENT_ADDR_REMOVED_IP6")
	/*
		char ipstr[INET6_ADDRSTRLEN];
		struct zts_sockaddr_in6 *in6 = (struct zts_sockaddr_in6*)&(msg.addr->addr);
		inet_ntop(AF_INET6, &(in6->sin6_addr), ipstr, INET6_ADDRSTRLEN);
		print("ZTS_EVENT_ADDR_REMOVED_IP6 --- The virtual address %s for this node on network %llx has been removed.\n", 
			ipstr, msg.addr->nwid)
		*/
	// Peer events
	case ZTS_EVENT_PEER_P2P:
		print("ZTS_EVENT_PEER_P2P --- node=%llx\n", msg.peer->address)
	case ZTS_EVENT_PEER_RELAY:
		print("ZTS_EVENT_PEER_RELAY --- node=%llx\n", msg.peer->address)


*/
	default:
		print("UNKNOWN_EVENT")
	}
}

func main()
{
	print("waiting for node to come online...")
	zts_start("../../config_path_a", myZeroTierEventCallback, 0)
	while(!nodeReady) {
		sleep(1)
	}
	print("Joining network")

}

main()
