package com.zerotier.libzt;

import com.zerotier.libzt.ZeroTier;

import java.io.*;
import java.util.Objects;

public class ZeroTierInputStream extends InputStream
{
	private static final int MAX_SKIP_BUFFER_SIZE = 2048;
	private static final int DEFAULT_BUFFER_SIZE = 8192;

	/*
	 * File descriptor used by lower native layer
	 */
	int zfd;

	/*
	 *
	 */
	public int available()
		throws IOException
	{
		return 0; // NOT YET SUPPORTED
	}

	/*
	 *
	 */
	public void close​()
		throws IOException
	{
		/* Note: this operation currently only stops RX on a socket that is shared 
		between both I/OStreams. This means that closing this stream will only shutdown
		that aspect of the socket but not actually close it and free resources. Resources
		will be properly freed when the socket implementation's close() is called or if
		both I/OStreams are closed separately */
		ZeroTier.shutdown(zfd, ZeroTier.SHUT_RD);
		zfd = -1;
	}

	/*
	 *
	 */
	public void mark​(int readlimit)
	{
		System.err.println("mark​: ZeroTierInputStream does not currently support this feature");
	}

	/*
	 *
	 */
	public void reset​()
		throws IOException
	{
		System.err.println("reset​: ZeroTierInputStream does not currently support this feature");
	}

	/*
	 *
	 */
	public boolean markSupported​()
	{
		return false; // mark() is not supported
	}

	/*
	 *
	 */
	public long transferTo​(OutputStream out)
		throws IOException
	{
		Objects.requireNonNull(out, "out must not be null");
		int bytesTransferred = 0, bytesRead;
		byte[] buf = new byte[DEFAULT_BUFFER_SIZE];
		while (((bytesRead = ZeroTier.read(zfd, buf)) >= 0)) {
			out.write(buf, 0, bytesRead);
			bytesTransferred += bytesRead;
		}
		return bytesTransferred;
	}

	/*
	 *
	 */
	public int read​()
		throws IOException
	{		
		byte[] buf = new byte[1];
		// Unlike a native read(), if nothing is read we should return -1
		int retval = ZeroTier.read(zfd, buf);
		if ((retval == 0) | (retval == -104) /* EINTR, from SO_RCVTIMEO */) {
			return -1;
		}
		if (retval < 0) {
			throw new IOException("read​(), errno="+retval);
		}
		return buf[0];
	}

	/*
	 *
	 */
	public int read​(byte[] b)
		throws IOException
	{
		Objects.requireNonNull(b, "input byte array must not be null");
		// Unlike a native read(), if nothing is read we should return -1
		int retval = ZeroTier.read(zfd, b);
		if ((retval == 0) | (retval == -104) /* EINTR, from SO_RCVTIMEO */) {
			return -1;
		}
		if (retval < 0) {
			throw new IOException("read​(b), errno="+retval);
		}
		return retval;
	}

	/*
	 *
	 */
	public int read​(byte[] b, int off, int len)
		throws IOException
	{
		Objects.requireNonNull(b, "input byte array must not be null");
		if ((off < 0) | (len < 0) | (len > b.length - off)) {
			throw new IndexOutOfBoundsException("invalid argument");
		}
		if (len == 0) {
			return 0;
		}
		// Unlike a native read(), if nothing is read we should return -1
		int retval = ZeroTier.read_offset(zfd, b, off, len);
		if ((retval == 0) | (retval == -104) /* EINTR, from SO_RCVTIMEO */) {
			return -1;
		}
		if (retval < 0) {
			throw new IOException("read​(b,off,len), errno="+retval);
		}
		//System.out.println("readNBytes​(byte[] b, int off="+off+", int len="+len+")="+retval);
		return retval;
	}

	/*
	 *
	 */
	public byte[] readAllBytes​()
		throws IOException
	{
		//System.out.println("readAllBytes​()");
		ZeroTierIoctlArg ztarg = new ZeroTierIoctlArg();
		int err = ZeroTier.ioctl(zfd, ZeroTier.FIONREAD, ztarg);
		byte[] buf = new byte[ztarg.integer];
		int retval = ZeroTier.read(zfd, buf);
		if ((retval == 0) | (retval == -104) /* EINTR, from SO_RCVTIMEO */) {
			// No action needed
		}
		if (retval < 0) {
			throw new IOException("readAllBytes​(b,off,len), errno="+retval);
		}
		return buf;
	}

	/*
	 *
	 */
	public int readNBytes​(byte[] b, int off, int len)
		throws IOException
	{
		Objects.requireNonNull(b, "input byte array must not be null");
		if ((off < 0) | (len < 0) | (len > b.length - off)) {
			throw new IndexOutOfBoundsException("invalid argument");
		}
		if (len == 0) {
			return 0;
		}
		int retval = ZeroTier.read_offset(zfd, b, off, len);
		if ((retval == 0) | (retval == -104) /* EINTR, from SO_RCVTIMEO */) {
			// No action needed
		}
		if (retval < 0) {
			throw new IOException("readNBytes​(b,off,len), errno="+retval);
		}
		//System.out.println("readNBytes​(byte[] b, int off="+off+", int len="+len+")="+retval);
		return retval;
	}

	/*
	 *
	 */
	public long skip​(long n)
		throws IOException
	{
		//System.out.println("skip()");
		if (n <= 0) {
			return 0;
		}
		long bytesRemaining = n, bytesRead;
		int bufSize = (int)Math.min(MAX_SKIP_BUFFER_SIZE, bytesRemaining);
		byte[] buf = new byte[bufSize];
		while (bytesRemaining > 0) {
			if ((bytesRead = ZeroTier.read_length(zfd, buf, (int)Math.min(bufSize, bytesRemaining))) < 0) {
				break;
			}
			bytesRemaining -= bytesRead;
		}
		return n - bytesRemaining; // skipped
	}
}