/*
 * Copyright (c)2013-2021 ZeroTier, Inc.
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file in the project's root directory.
 *
 * Change Date: 2026-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2.0 of the Apache License.
 */
/****/

package com.zerotier.sockets;

import com.zerotier.sockets.ZeroTierNative;
import java.io.*;
import java.util.Objects;

/**
 * Extends InputStream using ZeroTier as a transport
 */
public class ZeroTierInputStream extends InputStream {
    /**
     * File descriptor used by lower native layer
     */
    public int zfd = -1;

    /**
     * Close the ZeroTierInputStream
     * @exception IOException when an I/O error occurs
     */
    public void close​() throws IOException
    {
        /* Note: this operation currently only stops RX on a socket that is shared
        between both I/OStreams. This means that closing this stream will only shutdown
        that aspect of the socket but not actually close it and free resources. Resources
        will be properly freed when the socket implementation's close() is called or if
        both I/OStreams are closed separately */
        ZeroTierNative.zts_bsd_shutdown(zfd, ZeroTierNative.ZTS_SHUT_RD);
        zfd = -1;
    }

    /**
     * Transfer bytes from this stream to another
     * @param destStream Unused
     * @return Number of bytes transferred
     * @exception IOException when an I/O error occurs
     */
    public long transferTo​(OutputStream destStream) throws IOException
    {
        Objects.requireNonNull(destStream, "destStream must not be null");
        int bytesTransferred = 0, bytesRead;
        byte[] buf = new byte[8192];
        while (((bytesRead = ZeroTierNative.zts_bsd_read(zfd, buf)) >= 0)) {
            destStream.write(buf, 0, bytesRead);
            bytesTransferred += bytesRead;
        }
        return bytesTransferred;
    }

    /**
     * Read a single byte from the stream
     * @return Single byte read
     * @exception IOException when an I/O error occurs
     */
    public int read​() throws IOException
    {
        byte[] buf = new byte[1];
        // Unlike a native read(), if nothing is read we should return -1
        int retval = ZeroTierNative.zts_bsd_read(zfd, buf);
        if ((retval == 0) | (retval == -104) /* EINTR, from SO_RCVTIMEO */) {
            return -1;
        }
        if (retval < 0) {
            throw new IOException("read​(), errno=" + retval);
        }
        return buf[0];
    }

    /**
     * Read from stream into buffer
     * @param destBuffer Destination buffer
     * @return Number of bytes read
     * @exception IOException when an I/O error occurs
     */
    public int read​(byte[] destBuffer) throws IOException
    {
        Objects.requireNonNull(destBuffer, "input byte array must not be null");
        // Unlike a native read(), if nothing is read we should return -1
        int retval = ZeroTierNative.zts_bsd_read(zfd, destBuffer);
        if ((retval == 0) | (retval == -104) /* EINTR, from SO_RCVTIMEO */) {
            return -1;
        }
        if (retval < 0) {
            throw new IOException("read​(destBuffer), errno=" + retval);
        }
        return retval;
    }

    /**
     * Read from stream into buffer at offset
     * @param destBuffer Destination buffer
     * @param offset Where in the destination buffer bytes should be written
     * @param numBytes Number of bytes to read
     * @return Number of bytes read.
     * @exception IOException when an I/O error occurs
     */
    public int read​(byte[] destBuffer, int offset, int numBytes) throws IOException
    {
        Objects.requireNonNull(destBuffer, "input byte array must not be null");
        if (offset < 0) {
            throw new IndexOutOfBoundsException("offset < 0");
        }
        if (numBytes < 0) {
            throw new IndexOutOfBoundsException("numBytes < 0");
        }
        if (numBytes > (destBuffer.length - offset)) {
            throw new IndexOutOfBoundsException("numBytes > (destBuffer.length - offset");
        }
        if (numBytes == 0) {
            return 0;
        }
        // Unlike a native read(), if nothing is read we should return -1
        int retval = ZeroTierNative.zts_bsd_read_offset(zfd, destBuffer, offset, numBytes);
        if ((retval == 0) | (retval == -104) /* EINTR, from SO_RCVTIMEO */) {
            return -1;
        }
        if (retval < 0) {
            throw new IOException("read​(destBuffer, offset, numBytes), errno=" + retval);
        }
        return retval;
    }

    /**
     * Read all available data from stream
     * @return Array of bytes
     * @exception IOException when an I/O error occurs
     */
    public byte[] readAllBytes​() throws IOException
    {
        int pendingDataSize = ZeroTierNative.zts_get_pending_data_size(zfd);
        byte[] buf = new byte[pendingDataSize];
        int retval = ZeroTierNative.zts_bsd_read(zfd, buf);
        if ((retval == 0) | (retval == -104) /* EINTR, from SO_RCVTIMEO */) {
            // No action needed
        }
        if (retval < 0) {
            throw new IOException("readAllBytes​(), errno=" + retval);
        }
        return buf;
    }

    /**
     * Read a given number of bytes from the stream into a buffer
     * @param destBuffer Destination buffer
     * @param offset Where in the destination buffer bytes should be written
     * @param numBytes Number of bytes to read
     * @return Nothing.
     * @exception IOException when an I/O error occurs
     */
    public int readNBytes​(byte[] destBuffer, int offset, int numBytes) throws IOException
    {
        Objects.requireNonNull(destBuffer, "input byte array must not be null");
        if (offset < 0) {
            throw new IndexOutOfBoundsException("offset < 0");
        }
        if (numBytes < 0) {
            throw new IndexOutOfBoundsException("numBytes < 0");
        }
        if (numBytes > (destBuffer.length - offset)) {
            throw new IndexOutOfBoundsException("numBytes > destBuffer.length - offset");
        }
        if (numBytes == 0) {
            return 0;
        }
        int retval = ZeroTierNative.zts_bsd_read_offset(zfd, destBuffer, offset, numBytes);
        if ((retval == 0) | (retval == -104) /* EINTR, from SO_RCVTIMEO */) {
            // No action needed
        }
        if (retval < 0) {
            throw new IOException("readNBytes​(destBuffer, offset, numBytes), errno=" + retval);
        }
        return retval;
    }

    /**
     * Skip a certain number of bytes
     * @param numBytes Unused
     * @return Nothing.
     * @exception IOException when an I/O error occurs
     */
    public long skip​(long numBytes) throws IOException
    {
        if (numBytes <= 0) {
            return 0;
        }
        long bytesRemaining = numBytes, bytesRead;
        int bufSize = (int)Math.min(2048, bytesRemaining);
        byte[] buf = new byte[bufSize];
        while (bytesRemaining > 0) {
            if ((bytesRead = ZeroTierNative.zts_bsd_read_length(zfd, buf, (int)Math.min(bufSize, bytesRemaining)))
                < 0) {
                break;
            }
            bytesRemaining -= bytesRead;
        }
        return numBytes - bytesRemaining;
    }
}
