export interface ZtsError extends Error { code?: number, errno?: number }

interface ZTS {
    init_from_storage(path: string): void
    init_set_event_handler(callback: (event: number) => void): void

    node_start(): void
    node_is_online(): boolean
    node_get_id(): bigint
    node_stop(): void
    node_free(): void

    net_join(nwid: bigint): void
    net_leave(nwid: bigint): void
    net_transport_is_ready(nwid: bigint): boolean

    addr_get_str(nwid: bigint, ipv6: boolean): string

    bsd_socket(family: number, type: number, protocol: number): number
    bsd_close(fd: number): void

    bsd_send(fd: number, data: Uint8Array, flags: number, callback: (err: ZtsError | undefined, bytesWritten: number) => void): void;
    bsd_recv(fd: number, len: number, flags: number, callback: (err: ZtsError | null, data: Buffer) => void): void

    bsd_sendto(fd: number, data: Uint8Array, flags: number, ipaddr: string, port: number, callback: (err: ZtsError | undefined, bytesWritten: number) => void): void
    bsd_recvfrom(fd: number, len: number, flags: number, callback: (err: ZtsError | undefined, data: Buffer, address: string, port: number)=>void): void

    bind(fd: number, ipAddr: string, port: number): void
    listen(fd: number, backlog: number): void
    accept(fd: number, callback: (err: ZtsError | null, fd: number) => void): void
    connect(fd: number, ipAddr: string, port: number, timeout: number, callback: (err: ZtsError | null) => void): void
    shutdown_wr(fd: number): void

    getpeername(fd: number): { port: number, address: string }
    getsockname(fd: number): { port: number, address: string }
    set_recv_timeout(fd: number, seconds: number, microseconds: number): void
    set_send_timeout(fd: number, seconds: number, microseconds: number): void
}

// eslint-disable-next-line @typescript-eslint/no-var-requires
export const zts = require("../build/Release/zts") as ZTS;

export enum errors {
    /** No ZtsError */
    ZTS_ERR_OK = 0,
    /** Socket ZtsError, see `zts_errno` */
    ZTS_ERR_SOCKET = -1,
    /** This operation is not allowed at this time. Or possibly the node hasn't been started */
    ZTS_ERR_SERVICE = -2,
    /** Invalid argument */
    ZTS_ERR_ARG = -3,
    /** No result (not necessarily an ZtsError) */
    ZTS_ERR_NO_RESULT = -4,
    /** Consider filing a bug report */
    ZTS_ERR_GENERAL = -5
}

export enum events {
    /**
     * Node has been initialized
     *
     * This is the first event generated, and is always sent. It may occur
     * before node's constructor returns.
     *
     */
    ZTS_EVENT_NODE_UP = 200,

    /**
     * Node is online -- at least one upstream node appears reachable
     *
     */
    ZTS_EVENT_NODE_ONLINE = 201,

    /**
     * Node is offline -- network does not seem to be reachable by any available
     * strategy
     *
     */
    ZTS_EVENT_NODE_OFFLINE = 202,

    /**
     * Node is shutting down
     *
     * This is generated within Node's destructor when it is being shut down.
     * It's done for convenience, since cleaning up other state in the event
     * handler may appear more idiomatic.
     *
     */
    ZTS_EVENT_NODE_DOWN = 203,

    /**
     * A fatal ZtsError has occurred. One possible reason is:
     *
     * Your identity has collided with another node's ZeroTier address
     *
     * This happens if two different public keys both hash (via the algorithm
     * in Identity::generate()) to the same 40-bit ZeroTier address.
     *
     * This is something you should "never" see, where "never" is defined as
     * once per 2^39 new node initializations / identity creations. If you do
     * see it, you're going to see it very soon after a node is first
     * initialized.
     *
     * This is reported as an event rather than a return code since it's
     * detected asynchronously via ZtsError messages from authoritative nodes.
     *
     * If this occurs, you must shut down and delete the node, delete the
     * identity.secret record/file from the data store, and restart to generate
     * a new identity. If you don't do this, you will not be able to communicate
     * with other nodes.
     *
     * We'd automate this process, but we don't think silently deleting
     * private keys or changing our address without telling the calling code
     * is good form. It violates the principle of least surprise.
     *
     * You can technically get away with not handling this, but we recommend
     * doing so in a mature reliable application. Besides, handling this
     * condition is a good way to make sure it never arises. It's like how
     * umbrellas prevent rain and smoke detectors prevent fires. They do, right?
     *
     * Meta-data: none
     */
    ZTS_EVENT_NODE_FATAL_ZtsError = 204,

    /** Network ID does not correspond to a known network */
    ZTS_EVENT_NETWORK_NOT_FOUND = 210,
    /** The version of ZeroTier inside libzt is too old */
    ZTS_EVENT_NETWORK_CLIENT_TOO_OLD = 211,
    /** The configuration for a network has been requested (no action needed) */
    ZTS_EVENT_NETWORK_REQ_CONFIG = 212,
    /** The node joined the network successfully (no action needed) */
    ZTS_EVENT_NETWORK_OK = 213,
    /** The node is not allowed to join the network (you must authorize node) */
    ZTS_EVENT_NETWORK_ACCESS_DENIED = 214,
    /** The node has received an IPv4 address from the network controller */
    ZTS_EVENT_NETWORK_READY_IP4 = 215,
    /** The node has received an IPv6 address from the network controller */
    ZTS_EVENT_NETWORK_READY_IP6 = 216,
    /** Deprecated */
    ZTS_EVENT_NETWORK_READY_IP4_IP6 = 217,
    /** Network controller is unreachable */
    ZTS_EVENT_NETWORK_DOWN = 218,
    /** Network change received from controller */
    ZTS_EVENT_NETWORK_UPDATE = 219,

    /** TCP/IP stack (lwIP) is up (for debug purposes) */
    ZTS_EVENT_STACK_UP = 220,
    /** TCP/IP stack (lwIP) id down (for debug purposes) */
    ZTS_EVENT_STACK_DOWN = 221,

    /** lwIP netif up (for debug purposes) */
    ZTS_EVENT_NETIF_UP = 230,
    /** lwIP netif down (for debug purposes) */
    ZTS_EVENT_NETIF_DOWN = 231,
    /** lwIP netif removed (for debug purposes) */
    ZTS_EVENT_NETIF_REMOVED = 232,
    /** lwIP netif link up (for debug purposes) */
    ZTS_EVENT_NETIF_LINK_UP = 233,
    /** lwIP netif link down (for debug purposes) */
    ZTS_EVENT_NETIF_LINK_DOWN = 234,

    /** A direct P2P path to peer is known */
    ZTS_EVENT_PEER_DIRECT = 240,
    /** A direct P2P path to peer is NOT known. Traffic is now relayed  */
    ZTS_EVENT_PEER_RELAY = 241,
    /** A peer is unreachable. Check NAT/Firewall settings */
    ZTS_EVENT_PEER_UNREACHABLE = 242,
    /** A new path to a peer was discovered */
    ZTS_EVENT_PEER_PATH_DISCOVERED = 243,
    /** A known path to a peer is now considered dead */
    ZTS_EVENT_PEER_PATH_DEAD = 244,

    /** A new managed network route was added */
    ZTS_EVENT_ROUTE_ADDED = 250,
    /** A managed network route was removed */
    ZTS_EVENT_ROUTE_REMOVED = 251,

    /** A new managed IPv4 address was assigned to this peer */
    ZTS_EVENT_ADDR_ADDED_IP4 = 260,
    /** A managed IPv4 address assignment was removed from this peer  */
    ZTS_EVENT_ADDR_REMOVED_IP4 = 261,
    /** A new managed IPv4 address was assigned to this peer  */
    ZTS_EVENT_ADDR_ADDED_IP6 = 262,
    /** A managed IPv6 address assignment was removed from this peer  */
    ZTS_EVENT_ADDR_REMOVED_IP6 = 263,

    /** The node's secret key (identity) */
    ZTS_EVENT_STORE_IDENTITY_SECRET = 270,
    /** The node's public key (identity) */
    ZTS_EVENT_STORE_IDENTITY_PUBLIC = 271,
    /** The node has received an updated planet config */
    ZTS_EVENT_STORE_PLANET = 272,
    /** New reachability hints and peer configuration */
    ZTS_EVENT_STORE_PEER = 273,
    /** New network config */
    ZTS_EVENT_STORE_NETWORK = 274
}

export enum errnos {
    ZTS_EPERM = 1,
    /** No such file or directory */
    ZTS_ENOENT = 2,
    /** No such process */
    ZTS_ESRCH = 3,
    /** Interrupted system call */
    ZTS_EINTR = 4,
    /** I/O ZtsError */
    ZTS_EIO = 5,
    /** No such device or address */
    ZTS_ENXIO = 6,
    /** Bad file number */
    ZTS_EBADF = 9,
    /** Try again */
    ZTS_EAGAIN = 11,
    /** Operation would block */
    ZTS_EWOULDBLOCK = ZTS_EAGAIN,
    /** Out of memory */
    ZTS_ENOMEM = 12,
    /** Permission denied */
    ZTS_EACCES = 13,
    /** Bad address */
    ZTS_EFAULT = 14,
    /** Device or resource busy */
    ZTS_EBUSY = 16,
    /** File exists */
    ZTS_EEXIST = 17,
    /** No such device */
    ZTS_ENODEV = 19,
    /** Invalid argument */
    ZTS_EINVAL = 22,
    /** File table overflow */
    ZTS_ENFILE = 23,
    /** Too many open files */
    ZTS_EMFILE = 24,
    /** Function not implemented */
    ZTS_ENOSYS = 38,
    /** Socket operation on non-socket */
    ZTS_ENOTSOCK = 88,
    /** Destination address required */
    ZTS_EDESTADDRREQ = 89,
    /** Message too long */
    ZTS_EMSGSIZE = 90,
    /** Protocol wrong type for socket */
    ZTS_EPROTOTYPE = 91,
    /** Protocol not available */
    ZTS_ENOPROTOOPT = 92,
    /** Protocol not supported */
    ZTS_EPROTONOSUPPORT = 93,
    /** Socket type not supported */
    ZTS_ESOCKTNOSUPPORT = 94,
    /** Operation not supported on transport endpoint */
    ZTS_EOPNOTSUPP = 95,
    /** Protocol family not supported */
    ZTS_EPFNOSUPPORT = 96,
    /** Address family not supported by protocol */
    ZTS_EAFNOSUPPORT = 97,
    /** Address already in use */
    ZTS_EADDRINUSE = 98,
    /** Cannot assign requested address */
    ZTS_EADDRNOTAVAIL = 99,
    /** Network is down */
    ZTS_ENETDOWN = 100,
    /** Network is unreachable */
    ZTS_ENETUNREACH = 101,
    /** Software caused connection abort */
    ZTS_ECONNABORTED = 103,
    /** Connection reset by peer */
    ZTS_ECONNRESET = 104,
    /** No buffer space available */
    ZTS_ENOBUFS = 105,
    /** Transport endpoint is already connected */
    ZTS_EISCONN = 106,
    /** Transport endpoint is not connected */
    ZTS_ENOTCONN = 107,
    /** Connection timed out */
    ZTS_ETIMEDOUT = 110,
    /* Connection refused */
    ZTS_ECONNREFUSED = 111,
    /** No route to host */
    ZTS_EHOSTUNREACH = 113,
    /** Operation already in progress */
    ZTS_EALREADY = 114,
    /** Operation now in progress */
    ZTS_EINPROGRESS = 115
}

export * as defs from "./defs";
