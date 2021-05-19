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

import com.zerotier.sockets.*;
import java.io.*;
import java.util.Arrays;
import java.util.Objects;

/**
 * Extends OutputStream using ZeroTier as a transport
 */
public class ZeroTierOutputStream extends OutputStream {
    /**
     * File descriptor used by lower native layer. No touch!
     */
    public int zfd = -1;

    /**
     * Close the stream
     * @exception IOException when an I/O error occurs
     */
    public void close() throws IOException
    {
        /* Note: this operation currently only stops RX on a socket that is
        shared between both I/OStreams. This means that closing this stream
        will only shutdown that aspect of the socket but not actually close
        it and free resources. Resources will be properly freed when the
        socket implementation's native close() is called or if both I/OStreams
        are closed separately */
        ZeroTierNative.zts_bsd_shutdown(zfd, ZeroTierNative.ZTS_SHUT_WR);
        zfd = -1;
    }

    /**
     * Write a buffer
     * @param originBuffer Source buffer
     * @exception IOException when an I/O error occurs
     */
    public void write(byte[] originBuffer) throws IOException
    {
        int bytesWritten = ZeroTierNative.zts_bsd_write(zfd, originBuffer);
        if (bytesWritten < 0) {
            throw new IOException("write(originBuffer[]), errno=" + bytesWritten);
        }
    }

    /**
     * Write a buffer at offset
     * @param originBuffer Source buffer
     * @param offset Where in the buffer to start
     * @param numBytes Number of bytes to write
     * @exception IOException when an I/O error occurs
     */
    public void write(byte[] originBuffer, int offset, int numBytes) throws IOException
    {
        Objects.requireNonNull(originBuffer, "input byte array (originBuffer) must not be null");
        if (offset < 0) {
            throw new IndexOutOfBoundsException("offset < 0");
        }
        if (numBytes < 0) {
            throw new IndexOutOfBoundsException("numBytes < 0");
        }
        if ((offset + numBytes) > originBuffer.length) {
            throw new IndexOutOfBoundsException("(offset+numBytes) > originBuffer.length");
        }
        int bytesWritten = ZeroTierNative.zts_bsd_write_offset(zfd, originBuffer, offset, numBytes);
        if (bytesWritten < 0) {
            throw new IOException("write(originBuffer[],offset,numBytes), errno=" + bytesWritten);
        }
    }

    /**
     * Write low byte
     * @param d Integer containing low byte to write
     * @exception IOException when an I/O error occurs
     */
    public void write(int d) throws IOException
    {
        byte lowByte = (byte)(d & 0xFF);
        int bytesWritten = ZeroTierNative.zts_bsd_write_byte(zfd, lowByte);
        if (bytesWritten < 0) {
            throw new IOException("write(d), errno=" + bytesWritten);
        }
    }
}
