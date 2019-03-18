package com.zerotier.libzt;

public class ZeroTierSocketOptionValue
{
	public boolean isBoolean = false;
	public boolean booleanValue;

	public boolean isInteger = false;
	public int integerValue;

	public boolean isTimeval = false;
	public int tv_sec; // seconds
	public int tv_usec; // microseconds
}