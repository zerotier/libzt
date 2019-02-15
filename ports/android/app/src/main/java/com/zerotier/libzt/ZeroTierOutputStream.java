package com.zerotier.libzt;

import com.zerotier.libzt.ZeroTier;

import java.io.*;
import java.util.Arrays;
import java.util.Objects;

class ZeroTierOutputStream extends OutputStream
{
	/*
	 * File descriptor used by lower native layer
	 */
	int zfd;

	/*
	 *
	 */
	public void flush()
		throws IOException
	{
		// System.err.println("flush: ZeroTierOutputStream does not currently support this feature");
		// Not fully supported since we don't maintain a buffer
	}
	
	/*
	 *
	 */
	public void close()
		throws IOException 
	{
		/* Note: this operation currently only stops RX on a socket that is shared 
		between both I/OStreams. This means that closing this stream will only shutdown
		that aspect of the socket but not actually close it and free resources. Resources
		will be properly freed when the socket implementation's close() is called or if
		both I/OStreams are closed separately */
		ZeroTier.shutdown(zfd, ZeroTier.SHUT_WR);
		zfd = -1;
	}

	/*
	 *
	 */
	public void write(byte[] b)
		throws IOException
	{
		int err = ZeroTier.write(zfd, b);
		if (err < 0) {
			throw new IOException("write(b[]), errno="+err);
		}
	}

	/*
	 *
	 */
	public void write(byte[] b, int off, int len)
		throws IOException
	{
		Objects.requireNonNull(b, "input byte array must not be null");
		if ((off < 0) | (len < 0) | (off+len > b.length)) {
			throw new IndexOutOfBoundsException("write(b,off,len)");
		}
		int err = ZeroTier.write_offset(zfd, b, off, len);
		if (err < 0) {
			throw new IOException("write(b[],off,len), errno="+err);
		}
	}

	/*
	 *
	 */
	public void write(int b)
		throws IOException
	{
		byte lowByte = (byte)(b & 0xFF);
		int err = ZeroTier.write_byte(zfd, lowByte);
		if (err < 0) {
			throw new IOException("write(b), errno="+err);
		}
	}
}