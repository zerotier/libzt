package com.zerotier.libzt;

import com.zerotier.libzt.ZeroTier;

public class ZeroTierProtoStats
{
	public int xmit;             /* Transmitted packets. */
	public int recv;             /* Received packets. */
	public int fw;               /* Forwarded packets. */
	public int drop;             /* Dropped packets. */
	public int chkerr;           /* Checksum error. */
	public int lenerr;           /* Invalid length error. */
	public int memerr;           /* Out of memory error. */
	public int rterr;            /* Routing error. */
	public int proterr;          /* Protocol error. */
	public int opterr;           /* Error in options. */
	public int err;              /* Misc error. */
	public int cachehit;
}