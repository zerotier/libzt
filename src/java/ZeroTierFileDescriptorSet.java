package com.zerotier.libzt;

public class ZeroTierFileDescriptorSet
{
	byte[] fds_bits = new byte[1024];

	public void CLR(int fd)
	{
		fds_bits[fd] = 0x00;
	}

	public boolean ISSET(int fd)
	{
		return fds_bits[fd] == 0x01;
	}

	public void SET(int fd)
	{
		fds_bits[fd] = 0x01;
	}

	public void ZERO()
	{
		for (int i=0; i<1024; i++) {
			fds_bits[i] = 0x00;
		}
	}
}