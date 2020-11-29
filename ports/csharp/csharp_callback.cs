using System.Runtime.InteropServices;

public struct CallbackMessage
{
    public int eventCode;
    /* Pointers to structures that contain details about the 
    subject of the callback */
    public System.IntPtr node;
    public System.IntPtr network;
    public System.IntPtr netif;
    public System.IntPtr route;
    public System.IntPtr path;
    public System.IntPtr peer;
    public System.IntPtr addr;
}

[StructLayout(LayoutKind.Sequential)]
public struct SockAddrStorage
{
    public byte Length;
    public byte Family;
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)]
    public byte[] Data1;
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
    public uint[] Data2;
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
    public uint[] Data3;
}

[StructLayout(LayoutKind.Sequential)]
public struct SockAddr
{
    public ushort Family;
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 14)]
    public byte[] Data;
}

[StructLayout(LayoutKind.Sequential)]
public struct SockAddrIn
{
    public byte Length;
    public byte Family;
    public ushort Port;
    public uint Addr;
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 8)]
    public byte[] Zero;
}

public struct NodeDetails
{
    /**
	 * The node ID
	 */
    public ulong address;

    /**
	 * The current clock value accord to the node
	 */
    public ulong clock;

    /**
	 * Whether or not this node is online
	 */
    public bool online;

    /**
	 * Whether port mapping is enabled
	 */
    public bool portMappingEnabled;

    /**
	 * Whether multipath support is enabled. If true, this node will
	 * be capable of utilizing multiple physical links simultaneosly
	 * to create higher quality or more robust aggregate links.
	 *
	 * See: https://www.zerotier.com/manual.shtml#2_1_5
	 */
    public bool multipathEnabled;

    /**
	 * The port used by the service to send and receive
	 * all encapsulated traffic
	 */
    public ushort primaryPort;

    /**
	 * Planet ID
	 */
    public ulong planetWorldId;
    public ulong planetWorldTimestamp;
    public byte versionMajor;
    public byte versionMinor;
    public byte versionRev;
};
struct AddrDetails
{
    public ulong nwid;
    public SockAddrStorage addr;
};

struct NetifDetails
{
    /**
	 * The virtual network that this interface was commissioned for.
	 */
    public ulong nwid;

    /**
	 * The hardware address assigned to this interface
	 */
    public ulong mac;

    /**
	 * The MTU for this interface
	 */
    public int mtu;
};

struct RouteDetails
{
    /**
	 * Target network / netmask bits (in port field) or NULL or 0.0.0.0/0 for default
	 */
    public System.IntPtr target;

    /**
	 * Gateway IP address (port ignored) or NULL (family == 0) for LAN-local (no gateway)
	 */
    public System.IntPtr via;

	/**
	 * Route flags
	 */
	public ushort flags;

    /**
	 * Route metric (not currently used)
	 */
    public ushort metric;
};

struct NetworkDetails
{
    /**
	 * Network ID
	 */
    public ulong nwid;

    /**
	 * Maximum Transmission Unit size for this network
	 */
    public int mtu;

    /**
	 * Number of addresses (actually) assigned to the node on this network
	 */
    public short num_addresses;

    /**
	 * Array of IPv4 and IPv6 addresses assigned to the node on this network
	 */
    [MarshalAsAttribute(UnmanagedType.ByValArray, SizeConst = 16)]
    public System.IntPtr[] addr;

    /**
	 * Number of routes
	 */
    public uint num_routes;

    /**
	 * Array of IPv4 and IPv6 addresses assigned to the node on this network
	 */
    [MarshalAsAttribute(UnmanagedType.ByValArray, SizeConst = 32)]
    public System.IntPtr[] routes;
};

struct PathDetails
{
    /**
	 * Address of endpoint
	 */
    public System.IntPtr address;

	/**
	 * Time of last send in milliseconds or 0 for never
	 */
	public ulong lastSend;

    /**
	 * Time of last receive in milliseconds or 0 for never
	 */
    public ulong lastReceive;

    /**
	 * Is this a trusted path? If so this will be its nonzero ID.
	 */
    public ulong trustedPathId;

    /**
	 * Is path expired?
	 */
    int expired;

    /**
	 * Is path preferred?
	 */
    int preferred;
};

struct PeerDetails
{
    /**
	 * ZeroTier address (40 bits)
	 */
    public ulong address;

    /**
	 * Remote major version or -1 if not known
	 */
    int versionMajor;

    /**
	 * Remote minor version or -1 if not known
	 */
    int versionMinor;

    /**
	 * Remote revision or -1 if not known
	 */
    int versionRev;

    /**
	 * Last measured latency in milliseconds or -1 if unknown
	 */
    int latency;

    /**
	 * What trust hierarchy role does this device have?
	 */
    public int role;

	/**
	 * Number of paths (size of paths[])
	 */
	public uint pathCount;

    /**
	 * Known network paths to peer
	 */
    [MarshalAsAttribute(UnmanagedType.ByValArray, SizeConst = 16)]
    public System.IntPtr[] paths;
};

[UnmanagedFunctionPointerAttribute(CallingConvention.Cdecl)]
public delegate void CSharpCallback(System.IntPtr msg);
