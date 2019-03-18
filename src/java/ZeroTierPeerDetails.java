package com.zerotier.libzt;

import com.zerotier.libzt.ZeroTier;
import com.zerotier.libzt.ZeroTierSocketAddress;

public class ZeroTierPeerDetails
{
	/**
	 * ZeroTier address (40 bits)
	 */
	public long address;

	/**
	 * Remote major version or -1 if not known
	 */
	public int versionMajor;

	/**
	 * Remote minor version or -1 if not known
	 */
	public int versionMinor;

	/**
	 * Remote revision or -1 if not known
	 */
	public int versionRev;

	/**
	 * Last measured latency in milliseconds or -1 if unknown
	 */
	public int latency;

	/**
	 * What trust hierarchy role does this device have?
	 */
	public int role;

	/**
	 * Number of paths (size of paths[])
	 */
	public int pathCount;

	/**
	 * Known network paths to peer
	 */
	public ZeroTierSocketAddress[] paths = new ZeroTierSocketAddress[ZeroTier.ZT_MAX_PEER_NETWORK_PATHS];
}