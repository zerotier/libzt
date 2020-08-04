/**
 * libzt binding
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <string.h>
#include <string>

#include "ZeroTierSockets.h"
#include "nbind/api.h"

struct Node
{
	Node() : online(false), joinedAtLeastOneNetwork(false), id(0) {}

	bool online;
	bool joinedAtLeastOneNetwork;
	uint64_t id;
	// etc

	bool getOnline() { return online; }
	bool getJoinedAtLeastOneNetwork() { return joinedAtLeastOneNetwork; }
	uint64_t getId() { return id; }
} myNode;

/* Callback handler, you should return control from this function as quickly as you can
to ensure timely receipt of future events. You should not call libzt API functions from
this function unless it's something trivial like zts_inet_ntop() or similar that has
no state-change implications. */
void myZeroTierEventCallback(void *msgPtr)
{
	struct zts_callback_msg *msg = (struct zts_callback_msg *)msgPtr;

	// Node events
	if (msg->eventCode == ZTS_EVENT_NODE_ONLINE) {
		printf("ZTS_EVENT_NODE_ONLINE --- This node's ID is %llx\n", msg->node->address);
		myNode.id = msg->node->address;
		myNode.online = true;
	}
	if (msg->eventCode == ZTS_EVENT_NODE_OFFLINE) {
		printf("ZTS_EVENT_NODE_OFFLINE --- Check your physical Internet connection, router, firewall, etc. What ports are you blocking?\n");
		myNode.online = false;
	}
	if (msg->eventCode == ZTS_EVENT_NODE_NORMAL_TERMINATION) {
		printf("ZTS_EVENT_NODE_NORMAL_TERMINATION\n");
	}

	// Virtual network events
	if (msg->eventCode == ZTS_EVENT_NETWORK_NOT_FOUND) {
		printf("ZTS_EVENT_NETWORK_NOT_FOUND --- Are you sure %llx is a valid network?\n",
			msg->network->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_REQ_CONFIG) {
		printf("ZTS_EVENT_NETWORK_REQ_CONFIG --- Requesting config for network %llx, please wait a few seconds...\n", msg->network->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_ACCESS_DENIED) {
		printf("ZTS_EVENT_NETWORK_ACCESS_DENIED --- Access to virtual network %llx has been denied. Did you authorize the node yet?\n",
			msg->network->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_READY_IP4) {
		printf("ZTS_EVENT_NETWORK_READY_IP4 --- Network config received. IPv4 traffic can now be sent over network %llx\n",
			msg->network->nwid);
		myNode.joinedAtLeastOneNetwork = true;
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_READY_IP6) {
		printf("ZTS_EVENT_NETWORK_READY_IP6 --- Network config received. IPv6 traffic can now be sent over network %llx\n",
			msg->network->nwid);
		myNode.joinedAtLeastOneNetwork = true;
	}
	if (msg->eventCode == ZTS_EVENT_NETWORK_DOWN) {
		printf("ZTS_EVENT_NETWORK_DOWN --- %llx\n", msg->network->nwid);
	}

	// Address events
	if (msg->eventCode == ZTS_EVENT_ADDR_ADDED_IP4) {
		char ipstr[ZTS_INET_ADDRSTRLEN];
		struct zts_sockaddr_in *in4 = (struct zts_sockaddr_in*)&(msg->addr->addr);
		zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), ipstr, ZTS_INET_ADDRSTRLEN);
		printf("ZTS_EVENT_ADDR_NEW_IP4 --- This node's virtual address on network %llx is %s\n",
			msg->addr->nwid, ipstr);
	}
	if (msg->eventCode == ZTS_EVENT_ADDR_ADDED_IP6) {
		char ipstr[ZTS_INET6_ADDRSTRLEN];
		struct zts_sockaddr_in6 *in6 = (struct zts_sockaddr_in6*)&(msg->addr->addr);
		zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
		printf("ZTS_EVENT_ADDR_NEW_IP6 --- This node's virtual address on network %llx is %s\n",
			msg->addr->nwid, ipstr);
	}
	if (msg->eventCode == ZTS_EVENT_ADDR_REMOVED_IP4) {
		char ipstr[ZTS_INET_ADDRSTRLEN];
		struct zts_sockaddr_in *in4 = (struct zts_sockaddr_in*)&(msg->addr->addr);
		zts_inet_ntop(ZTS_AF_INET, &(in4->sin_addr), ipstr, ZTS_INET_ADDRSTRLEN);
		printf("ZTS_EVENT_ADDR_REMOVED_IP4 --- The virtual address %s for this node on network %llx has been removed.\n",
			ipstr, msg->addr->nwid);
	}
	if (msg->eventCode == ZTS_EVENT_ADDR_REMOVED_IP6) {
		char ipstr[ZTS_INET6_ADDRSTRLEN];
		struct zts_sockaddr_in6 *in6 = (struct zts_sockaddr_in6*)&(msg->addr->addr);
		zts_inet_ntop(ZTS_AF_INET6, &(in6->sin6_addr), ipstr, ZTS_INET6_ADDRSTRLEN);
		printf("ZTS_EVENT_ADDR_REMOVED_IP6 --- The virtual address %s for this node on network %llx has been removed.\n",
			ipstr, msg->addr->nwid);
	}
	// Peer events
	if (msg->peer) {
		if (msg->peer->role == ZTS_PEER_ROLE_PLANET) {
			/* Safe to ignore, these are our roots. They orchestrate the P2P connection.
			You might also see other unknown peers, these are our network controllers. */
			return;
		}
		if (msg->eventCode == ZTS_EVENT_PEER_DIRECT) {
			printf("ZTS_EVENT_PEER_DIRECT --- A direct path is known for node=%llx\n",
				msg->peer->address);
		}
		if (msg->eventCode == ZTS_EVENT_PEER_RELAY) {
			printf("ZTS_EVENT_PEER_RELAY --- No direct path to node=%llx\n", msg->peer->address);
		}
		if (msg->eventCode == ZTS_EVENT_PEER_PATH_DISCOVERED) {
			printf("ZTS_EVENT_PEER_PATH_DISCOVERED --- A new direct path was discovered for node=%llx\n",
				msg->peer->address);
		}
		if (msg->eventCode == ZTS_EVENT_PEER_PATH_DEAD) {
			printf("ZTS_EVENT_PEER_PATH_DEAD --- A direct path has died for node=%llx\n",
				msg->peer->address);
		}
	}
}

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
 * - Exceptions to the above rule are:
 *    1) Joining a public network (such as "earth")
 *    2) Joining an Ad-hoc network, (no controller and therefore requires no authorization.)
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
class ZeroTier
{
public:
	/**
	* @brief Starts the ZeroTier service and notifies user application of events via callback
	*
	* @param configPath path directory where configuration files are stored
	* @param servicePort proit which ZeroTier service will listen on
	* @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE or ZTS_ERR_ARG on failure
	*/
    static int start(const char *configPath, uint16_t servicePort)
	{
		// Bring up ZeroTier service

		int err = ZTS_ERR_OK;

		if((err = zts_start(configPath, &myZeroTierEventCallback, servicePort)) != ZTS_ERR_OK) {
			printf("Unable to start service, error = %d.\n", err);
			return err;
		}
		printf("Waiting for node to come online...\n");
		while (!myNode.online) { zts_delay_ms(50); }

		return err;
	}

	/**
	* @brief Stops the ZeroTier service and brings down all virtual network interfaces
	*
	* @usage While the ZeroTier service will stop, the stack driver (with associated timers)
	* will remain active in case future traffic processing is required. To stop all activity
	* and free all resources use zts_free() instead.
	* @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE on failure.
	*/
	static int stop()
	{
		return zts_stop();
	}

	/**
	* @brief Restart the ZeroTier service.
	*
	* @usage This call will block until the service has been brought offline. Then
	* it will return and the user application can then watch for the appropriate
	* startup callback events.
	* @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE on failure.
	*/
	static int restart()
	{
		return zts_restart();
	}

	/**
	* @brief Stop all background services, bring down all interfaces, free all resources. After
	* calling this function an application restart will be required before the library can be
	* used again.
	*
	* @usage This should be called at the end of your program or when you do not anticipate
	*        communicating over ZeroTier
	* @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE on failure.
	*/
	static int free()
	{
		return zts_free();
	}

	/**
	* @brief Join a network
	*
	* @param networkId A 16-digit hexadecimal virtual network ID
	* @return ZTS_ERR_OK on success. ZTS_ERR_SERVICE or ZTS_ERR_ARG on failure.
	*/
	static int join(const char *networkId)
	{
		// Join ZeroTier network

		uint64_t nwid = strtoull(networkId,NULL,16); // Network ID to join

		int err = ZTS_ERR_OK;

		if((err = zts_join(nwid)) != ZTS_ERR_OK) {
			printf("Unable to join network, error = %d.\n", err);
			return err;
		}
		printf("Joining network %llx\n", nwid);
		printf("Don't forget to authorize this device in my.zerotier.com or the web API!\n");
		while (!myNode.joinedAtLeastOneNetwork) { zts_delay_ms(50); }

		return err;
	}

	static int connectStream(const char *remoteAddr, const int remotePort)
	{
		return connect(ZTS_AF_INET, ZTS_SOCK_STREAM, 0, remoteAddr, remotePort);
	}

	static int connectDgram(const char *remoteAddr, const int remotePort)
	{
		return connect(ZTS_AF_INET, ZTS_SOCK_DGRAM, 0, remoteAddr, remotePort);
	}

	static int connectRaw(const char *remoteAddr, const int remotePort, const int protocol)
	{
		return connect(ZTS_AF_INET, ZTS_SOCK_RAW, protocol, remoteAddr, remotePort);
	}

	static int connectStream6(const char *remoteAddr, const int remotePort)
	{
		return connect(ZTS_AF_INET6, ZTS_SOCK_STREAM, 0, remoteAddr, remotePort);
	}

	static int connectDgram6(const char *remoteAddr, const int remotePort)
	{
		return connect(ZTS_AF_INET6, ZTS_SOCK_DGRAM, 0, remoteAddr, remotePort);
	}

	static int connectRaw6(const char *remoteAddr, const int remotePort, const int protocol)
	{
		return connect(ZTS_AF_INET6, ZTS_SOCK_RAW, protocol, remoteAddr, remotePort);
	}

	/**
	* @brief Connect a socket to a remote host (sets zts_errno)
	*
	* @param socket_family Address family (ZTS_AF_INET, ZTS_AF_INET6)
	* @param socket_type Type of socket (ZTS_SOCK_STREAM, ZTS_SOCK_DGRAM, ZTS_SOCK_RAW)
	* @param protocol Protocols supported on this socket
	* @param remoteAddr Remote Address to connect to
	* @param remotePort Remote Port to connect to
	* @return ZTS_ERR_OK on success. ZTS_ERR_SOCKET, ZTS_ERR_SERVICE, ZTS_ERR_ARG on failure.
	*/
	static int connect(const int socket_family, const int socket_type, const int protocol, const char *remoteAddr, const int remotePort)
	{
		if (socket_family == ZTS_AF_INET6) {
			printf("IPv6 NOT IMPLEMENTED.\n");
			return -1;
		}

		struct zts_sockaddr_in in4 = sockaddr_in(remoteAddr, remotePort);

		int fd;
		if ((fd = zts_socket(socket_family, socket_type, protocol)) < 0) {
			printf("Error creating ZeroTier socket (fd=%d, zts_errno=%d).\n", fd, zts_errno);
			return -1;
		}

		// Retries are often required since ZT uses transport-triggered links (explained above)
		int err = ZTS_ERR_OK;
		for (;;) {
			printf("Connecting to remote host...\n");
			if ((err = zts_connect(fd, (const struct zts_sockaddr *)&in4, sizeof(in4))) < 0) {
				printf("Error connecting to remote host (fd=%d, ret=%d, zts_errno=%d). Trying again.\n",
					fd, err, zts_errno);
				zts_close(fd);
				// printf("Creating socket...\n");
				if ((fd = zts_socket(socket_family, socket_type, protocol)) < 0) {
					printf("Error creating ZeroTier socket (fd=%d, zts_errno=%d).\n", fd, zts_errno);
					return -1;
				}
				zts_delay_ms(250);
			}
			else {
				printf("Connected.\n");
				break;
			}
		}

		// Set non-blocking mode
		fcntl(fd, ZTS_F_SETFL, ZTS_O_NONBLOCK);

		return fd;
	}

	static int fcntlSetBlocking(int fd, bool isBlocking)
	{
		int flags = fcntl(fd, ZTS_F_GETFL, 0);
		if (isBlocking) {
			flags &= ~ZTS_O_NONBLOCK;
		} else {
			flags &= ZTS_O_NONBLOCK;
		}
		return fcntl(fd, ZTS_F_SETFL, flags);
	}

	/**
	* @brief Issue file control commands on a socket
	*
	* @param fd File descriptor
	* @param cmd
	* @param flags
	* @return
	*/
	static int fcntl(int fd, int cmd, int flags)
	{
		return zts_fcntl(fd, cmd, flags);
	}

	static ssize_t read(int fd, nbind::Buffer buf)
	{
		return zts_read(fd, buf.data(), buf.length());
	}

	static ssize_t write(int fd, nbind::Buffer buf)
	{
		return zts_write(fd, buf.data(), buf.length());
	}

	static ssize_t writev(int fd, std::vector<nbind::Buffer> bufs)
	{
		std::size_t size = bufs.size();
		zts_iovec iov[size];
		for (std::size_t i = 0; i != size; ++i) {
			iov[i].iov_base = bufs[i].data();
			iov[i].iov_len = bufs[i].length();
		}
		return zts_writev(fd, iov, bufs.size());
	}

	static ssize_t recv(int fd, nbind::Buffer buf, int flags)
	{
		return zts_recv(fd, buf.data(), buf.length(), flags);
	}

	static ssize_t send(int fd, nbind::Buffer buf, int flags)
	{
		return zts_send(fd, buf.data(), buf.length(), flags);
	}

	static int close(int fd)
	{
		return zts_close(fd);
	}

    static zts_sockaddr_in sockaddr_in(const char *remoteAddr, const int remotePort)
    {
        struct zts_sockaddr_in in4;
		in4.sin_port = zts_htons(remotePort);
	#if defined(_WIN32)
		in4.sin_addr.S_addr = zts_inet_addr(remoteAddr);
	#else
		in4.sin_addr.s_addr = zts_inet_addr(remoteAddr);
	#endif
		in4.sin_family = ZTS_AF_INET;
		return in4;
    }

	static Node getMyNode() { return myNode; }
};

#include "nbind/nbind.h"

NBIND_CLASS(Node) {
	getter(getOnline);
	getter(getJoinedAtLeastOneNetwork);
	getter(getId);
}

NBIND_CLASS(ZeroTier) {
	method(start);
	method(join);
	method(connectStream);
	method(connectDgram);
	method(connectRaw);
	method(connectStream6);
	method(connectDgram6);
	method(connectRaw6);
	method(connect);
	method(read);
	method(write);
	method(writev);
	method(recv);
	method(send);
	method(fcntlSetBlocking);
	method(fcntl);
	method(close);
	method(restart);
	method(stop);
	method(free);
	method(getMyNode);
}
