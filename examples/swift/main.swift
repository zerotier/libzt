/**
 * I'll order you a pizza if you can rewrite this in modern idomatic Swift
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
 *   established libzt will inform you via ZTS_EVENT_PEER_DIRECT for a specific peer ID. No
 *   action is required on your part for this callback event.
 *
 *   Note: In these initial moments before ZTS_EVENT_PEER_DIRECT has been received for a
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
 *      ZTS_ERR_OK            // No error
 *      ZTS_ERR_SOCKET        // Socket error, see zts_errno
 *      ZTS_ERR_SERVICE       // You probably did something at the wrong time
 *      ZTS_ERR_ARG           // Invalid argument
 *      ZTS_ERR_NO_RESULT     // No result (not necessarily an error)
 *      ZTS_ERR_GENERAL       // Consider filing a bug report
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

let printNodeDetails : @convention(c) (UnsafeMutableRawPointer?) -> Void =
{
	(msgPtr) -> Void in
	let msg = msgPtr?.bindMemory(to: zts_callback_msg.self, capacity: 1)
	let d = msg?.pointee.node;
	print(String(format: "\t- id            : %llx", d!.pointee.address));
	print(String(format: "\t- version       : %d.%d.%d", d!.pointee.versionMajor, d!.pointee.versionMinor, d!.pointee.versionRev));
	print(String(format: "\t- primaryPort   : %d", d!.pointee.primaryPort));
	print(String(format: "\t- secondaryPort : %d", d!.pointee.secondaryPort));
}

/*
func convertTupleToArray<Tuple, Value>(from tuple: Tuple) -> [Value] {
	let tupleMirror = Mirror(reflecting: tuple)
	func convert(child: Mirror.Child) -> Value? {
		let valueMirror = Mirror(reflecting: child.value)
		return child.value as? Value
	}
	return tupleMirror.children.flatMap(convert)
}
*/

let printNetworkDetails : @convention(c) (UnsafeMutableRawPointer?) -> Void =
{
	(msgPtr) -> Void in
	let msg = msgPtr?.bindMemory(to: zts_callback_msg.self, capacity: 1)
	let d = msg?.pointee.network;
	let name = ""; // String(d!.pointee.name);

	print(String(format: "\t- nwid                       : %llx", d!.pointee.nwid));
	print(String(format: "\t- mac                        : %lx", d!.pointee.mac));
	print(String(format: "\t- name                       : %s", name));
	print(String(format: "\t- type                       : %d", Int(d!.pointee.type.rawValue)));
	/* MTU for the virtual network can be set via our web API */
	print(String(format: "\t- mtu                        : %d", d!.pointee.mtu));
	print(String(format: "\t- dhcp                       : %d", d!.pointee.dhcp));
	print(String(format: "\t- bridge                     : %d", d!.pointee.bridge));
	print(String(format: "\t- broadcastEnabled           : %d", d!.pointee.broadcastEnabled));
	print(String(format: "\t- portError                  : %d", d!.pointee.portError));
	print(String(format: "\t- netconfRevision            : %d", d!.pointee.netconfRevision));
	print(String(format: "\t- routeCount                 : %d", d!.pointee.routeCount));
	print(String(format: "\t- multicastSubscriptionCount : %d", d!.pointee.multicastSubscriptionCount));
/*
	var addresses: [zts_sockaddr_storage] = convertTupleToArray(from: d!.pointee.assignedAddresses)

	print("\t- addresses:\n");
	for i in 0...d!.pointee.assignedAddressCount {
		if (addresses[Int(i)].ss_family == ZTS_AF_INET) {
			// Allocate a byte array that can hold the largest possible IPv4 human-readable string
			var ipCharByteArray = Array<Int8>(repeating: 0, count: Int(ZTS_INET_ADDRSTRLEN))
			// Cast unsafe pointer from zts_sockaddr_storage to zts_sockaddr_in
			var addr:zts_sockaddr_in = withUnsafePointer(to: &(addresses[Int(i)])) {
			  $0.withMemoryRebound(to: zts_sockaddr_in.self, capacity: 1) {
				  $0.pointee
			  }
			}
			// Pass unsafe pointer (addr) to a ntop to convert into human-readable byte array
			zts_inet_ntop(ZTS_AF_INET, &(addr.sin_addr), &ipCharByteArray, UInt32(ZTS_INET_ADDRSTRLEN))
			//print(ipCharByteArray) // [49, 55, 50, 46, 50, 55, 46, 49, 49, 54, 46, 49, 54, 55, 0, 0]
			// Somehow convery Int8 byte array to Swift String ???
			//let ipString = String(bytes: ipStr, encoding: .utf8)
			//print(ipString)

			// Pass unsafe pointer (addr) to a ntop to convert into human-readable byte array
			// convert to UInt8 byte array
			let uintArray = ipCharByteArray.map { UInt8(bitPattern: $0) }
			if let string = String(bytes: uintArray, encoding: .utf8) {
				print("\t\t-", string)
			}
		}
		if (addresses[Int(i)].ss_family == ZTS_AF_INET6) {
			// ...
		}
	}
*/
/*
	print("\t- routes:\n");

	for i in 0...d!.pointee.routeCount {
		// ...
	}
*/
}

let printPeerDetails : @convention(c) (UnsafeMutableRawPointer?) -> Void =
{
	(msgPtr) -> Void in
	let msg = msgPtr?.bindMemory(to: zts_callback_msg.self, capacity: 1)
	let d = msg?.pointee.peer;
	print(String(format: "\t- peer                       : %llx", d!.pointee.address));
	print(String(format: "\t- role                       : %d", Int(d!.pointee.role.rawValue)));
	print(String(format: "\t- latency                    : %llx", d!.pointee.latency));
	print(String(format: "\t- pathCount                  : %llx", d!.pointee.pathCount));
	print(String(format: "\t- version                    : %d.%d.%d", d!.pointee.versionMajor, d!.pointee.versionMinor, d!.pointee.versionRev));
	print(String(format: "\t- paths:\n"));

/*
	for i in 0...d!.pointee.pathCount {
		// ...
	}
*/
}

let printNetifDetails : @convention(c) (UnsafeMutableRawPointer?) -> Void =
{
	(msgPtr) -> Void in
	let msg = msgPtr?.bindMemory(to: zts_callback_msg.self, capacity: 1)
	let d = msg?.pointee.netif;
	print(String(format: "\t- nwid : %llx", d!.pointee.nwid));
	print(String(format: "\t- mac  : %llx", d!.pointee.mac));
	print(String(format: "\t- mtu  : %d", d!.pointee.mtu));
}

var nodeReady:Bool = false
var networkReady:Bool = false

let myZeroTierEventCallback : @convention(c) (UnsafeMutableRawPointer?) -> Void =
{
	(msgPtr) -> Void in
	let msg = msgPtr?.bindMemory(to: zts_callback_msg.self, capacity: 1)

	let eventCode = msg!.pointee.eventCode
	let network = msg?.pointee.network;
	let peer = msg?.pointee.peer;

	switch Int32(eventCode)
	{
	case ZTS_EVENT_NODE_UP:
		print("ZTS_EVENT_NODE_UP (you can ignore this)\n")

	case ZTS_EVENT_NODE_ONLINE:
		print("ZTS_EVENT_NODE_ONLINE\n")
		printNodeDetails(msg)
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

	case ZTS_EVENT_NETWORK_UPDATE:
		print("ZTS_EVENT_NETWORK_UPDATE\n")
		printNetworkDetails(msg)


	case ZTS_EVENT_ADDR_ADDED_IP4:
		print("ZTS_EVENT_ADDR_ADDED_IP4\n")

	case ZTS_EVENT_ADDR_ADDED_IP6:
		print("ZTS_EVENT_ADDR_ADDED_IP6\n")

	case ZTS_EVENT_ADDR_REMOVED_IP4:
		print("ZTS_EVENT_ADDR_REMOVED_IP4\n")

	case ZTS_EVENT_ADDR_REMOVED_IP6:
		print("ZTS_EVENT_ADDR_REMOVED_IP6\n")


	case ZTS_EVENT_PEER_DIRECT:
		let peerId:UInt64 = peer!.pointee.address
		print(String(format: "ZTS_EVENT_PEER_DIRECT (%llx)", peerId))
		printPeerDetails(msg)

	case ZTS_EVENT_PEER_RELAY:
		let peerId:UInt64 = peer!.pointee.address
		print(String(format: "ZTS_EVENT_PEER_RELAY (%llx)", peerId))
		printPeerDetails(msg)

	case ZTS_EVENT_PEER_PATH_DISCOVERED:
		let peerId:UInt64 = peer!.pointee.address
		print(String(format: "ZTS_EVENT_PEER_PATH_DISCOVERED (%llx)", peerId))
		printPeerDetails(msg)

	case ZTS_EVENT_PEER_PATH_DEAD:
		let peerId:UInt64 = peer!.pointee.address
		print(String(format: "ZTS_EVENT_PEER_PATH_DEAD (%llx)", peerId))
		printPeerDetails(msg)


	case ZTS_EVENT_NETIF_UP:
		print("ZTS_EVENT_NETIF_UP\n")

	case ZTS_EVENT_NETIF_DOWN:
		print("ZTS_EVENT_NETIF_DOWN\n")

	case ZTS_EVENT_NETIF_REMOVED:
		print("ZTS_EVENT_NETIF_REMOVED\n")

	case ZTS_EVENT_NETIF_LINK_UP:
		print("ZTS_EVENT_NETIF_LINK_UP\n")

	case ZTS_EVENT_NETIF_LINK_DOWN:
		print("ZTS_EVENT_NETIF_LINK_DOWN\n")

	case ZTS_EVENT_STACK_UP:
		print("ZTS_EVENT_STACK_UP\n")

	case ZTS_EVENT_STACK_DOWN:
		print("ZTS_EVENT_STACK_DOWN\n")

	default:
		print("UNKNOWN_EVENT: ", eventCode)
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

	let nwId : UInt64 = 0x0123456789abcdef; // Specify your network ID here
	zts_join(nwId);

	// create address structure
	let addr_str = "0.0.0.0"
	let port = 8080
	var in4 = zts_sockaddr_in(sin_len: UInt8(MemoryLayout<zts_sockaddr_in>.size),
						  sin_family: UInt8(ZTS_AF_INET),
						  sin_port: UInt16(port).bigEndian,
						  sin_addr: zts_in_addr(s_addr: 0),
						  sin_zero: (0,0,0,0,0,0,0,0))
	zts_inet_pton(ZTS_AF_INET, addr_str, &(in4.sin_addr));

	print("fd=", zts_socket(ZTS_AF_INET, ZTS_SOCK_STREAM, 0));

	// ...

	while(true) {
		sleep(1);
	}
}

main()
