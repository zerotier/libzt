/**
 * libzt Swift example
 *
 * swiftc -lc++ -import-objc-header ../../include/ZeroTierSockets.h -L. -lzt main.swift -o main;
 * ./main
 *
 * TODO: This example is incomplete
 */

import Swift
import Foundation

/**
 *
 *   IDENTITIES and AUTHORIZATION:
 *
 * - Upon the first execution of this code, a new identity will be generated and placed in
 *   the location given in the first argument to zts_start(path, ...). If you accidentally
 *   duplicate the identity files and use them simultaneously in a different node instance
 *   you will experience undefined behavior and it is likely nothing will work.
 *
 * - You must authorize the node ID provided by the ZTS_EVENT_NODE_ONLINE callback to join
 *   your network, otherwise nothing will happen. This can be done manually or via
 *   our web API: https://my.zerotier.com/help/api
 *
 * - An exception to the above rule is if you are using an Ad-hoc network, it has no
 *   controller and therefore requires no authorization.
 *
 *
 *   ESTABLISHING A CONNECTION:
 *
 * - Creating a standard socket connection generally works the same as it would using
 *   an ordinary socket interface, however with libzt there is a subtle difference in
 *   how connections are established which may cause confusion:
 *
 *   The underlying virtual ZT layer creates what are called "transport-triggered links"
 *   between nodes. That is, links are not established until an attempt to communicate
 *   with a peer has taken place. The side effect is that the first few packets sent from
 *   a libzt instance are usually relayed via our free infrastructure and it isn't until a
 *   root server has passed contact information to both peers that a direct connection will be
 *   established. Therefore, it is required that multiple connection attempts be undertaken
 *   when initially communicating with a peer. After a transport-triggered link is
 *   established libzt will inform you via ZTS_EVENT_PEER_P2P for a specific peer ID. No
 *   action is required on your part for this callback event.
 *
 *   Note: In these initial moments before ZTS_EVENT_PEER_P2P has been received for a
 *         specific peer, traffic may be slow, jittery and there may be high packet loss.
 *         This will subside within a couple of seconds.
 *
 *
 *   ERROR HANDLING:
 *
 * - libzt's API is actually composed of two categories of functions with slightly
 *   different error reporting mechanisms.
 *
 *   Category 1: Control functions (zts_start, zts_join, zts_get_peer_status, etc). Errors
 *                returned by these functions can be any of the following:
 *
 *      ZTS_ERR_OK            0 // No error
 *      ZTS_ERR_SOCKET       -1 // Socket error, see zts_errno
 *      ZTS_ERR_SERVICE      -2 // You probably did something at the wrong time
 *      ZTS_ERR_ARG          -3 // Invalid argument
 *      ZTS_ERR_NO_RESULT    -4 // No result (not necessarily an error)
 *      ZTS_ERR_GENERAL      -5 // Consider filing a bug report
 *
 *   Category 2: Sockets (zts_socket, zts_bind, zts_connect, zts_listen, etc).
 *               Errors returned by these functions can be the same as the above. With
 *               the added possibility of zts_errno being set. Much like standard
 *               errno this will provide a more specific reason for an error's occurrence.
 *               See ZeroTierSockets.h for values.
 *
 *
 *   API COMPATIBILITY WITH HOST OS:
 *
 * - While the ZeroTier socket interface can coexist with your host OS's own interface in
 *   the same file with no type and naming conflicts, try not to mix and match host
 *   OS/libzt structures, functions, or constants. It may look similar and may even work
 *   some of the time but there enough differences that it will cause headaches. Here
 *   are a few guidelines:
 *
 *   If you are calling a zts_* function, use the appropriate ZTS_* constants:
 *
 *          zts_socket(ZTS_AF_INET6, ZTS_SOCK_DGRAM, 0); (CORRECT)
 *          zts_socket(AF_INET6, SOCK_DGRAM, 0);         (INCORRECT)
 *
 *   If you are calling a zts_* function, use the appropriate zts_* structure:
 *
 *          struct zts_sockaddr_in in4;  <------ Note the zts_* prefix
 *             ...
 *          zts_bind(fd, (struct zts_sockaddr *)&in4, sizeof(struct zts_sockaddr_in)) < 0)
 *
 */

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

	case ZTS_EVENT_NETWORK_REQ_CONFIG:
		let networkId:UInt64 = network!.pointee.nwid
		print(String(format: "ZTS_EVENT_NETWORK_REQ_CONFIG (%llx)", networkId))

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
	zts_start("config_path", myZeroTierEventCallback, 0)
	while(!nodeReady) {
		sleep(1)
	}
	print("Joining network")

}

main()
