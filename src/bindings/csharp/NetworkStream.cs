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

using System;
using System.Threading;
using System.IO;
using System.Runtime.InteropServices;
using System.Net.Sockets;

using ZeroTier;

namespace ZeroTier.Sockets
{
    public class NetworkStream : Stream {
        private ZeroTier.Sockets.Socket _streamSocket;

        private bool _isReadable;
        private bool _isWriteable;
        private bool _ownsSocket;
        private volatile bool _isDisposed = false;

        internal NetworkStream()
        {
            _ownsSocket = true;
        }

        public NetworkStream(ZeroTier.Sockets.Socket socket)
        {
            if (socket == null) {
                throw new ArgumentNullException("socket");
            }
            InitNetworkStream(socket, FileAccess.ReadWrite);
        }

        public NetworkStream(ZeroTier.Sockets.Socket socket, bool ownsSocket)
        {
            if (socket == null) {
                throw new ArgumentNullException("socket");
            }
            InitNetworkStream(socket, FileAccess.ReadWrite);
            _ownsSocket = ownsSocket;
        }

        public NetworkStream(ZeroTier.Sockets.Socket socket, FileAccess accessMode)
        {
            if (socket == null) {
                throw new ArgumentNullException("socket");
            }
            InitNetworkStream(socket, accessMode);
        }

        public NetworkStream(ZeroTier.Sockets.Socket socket, FileAccess accessMode, bool ownsSocket)
        {
            if (socket == null) {
                throw new ArgumentNullException("socket");
            }
            InitNetworkStream(socket, accessMode);
            _ownsSocket = ownsSocket;
        }

        internal NetworkStream(NetworkStream networkStream, bool ownsSocket)
        {
            ZeroTier.Sockets.Socket socket = networkStream.Socket;
            if (socket == null) {
                throw new ArgumentNullException("networkStream");
            }
            InitNetworkStream(socket, FileAccess.ReadWrite);
            _ownsSocket = ownsSocket;
        }

        protected ZeroTier.Sockets.Socket Socket
        {
            get {
                return _streamSocket;
            }
        }

        internal void ConvertToNotSocketOwner()
        {
            _ownsSocket = false;
            GC.SuppressFinalize(this);
        }

        public override int ReadTimeout
        {
            get {
                return _streamSocket.ReceiveTimeout;
            }
            set {
                if (value <= 0) {
                    throw new ArgumentOutOfRangeException("Timeout value must be greater than zero");
                }
                _streamSocket.ReceiveTimeout = value;
            }
        }

        public override int WriteTimeout
        {
            get {
                return _streamSocket.SendTimeout;
            }
            set {
                if (value <= 0) {
                    throw new ArgumentOutOfRangeException("Timeout value must be greater than zero");
                }
                _streamSocket.SendTimeout = value;
            }
        }

        protected bool Readable
        {
            get {
                return _isReadable;
            }
            set {
                _isReadable = value;
            }
        }

        protected bool Writeable
        {
            get {
                return _isWriteable;
            }
            set {
                _isWriteable = value;
            }
        }

        public override bool CanRead
        {
            get {
                return _isReadable;
            }
        }

        public override bool CanSeek
        {
            get {
                return false;
            }
        }

        public override bool CanWrite
        {
            get {
                return _isWriteable;
            }
        }

        public override bool CanTimeout
        {
            get {
                return true;
            }
        }

        public virtual bool DataAvailable
        {
            get {
                if (_streamSocket == null) {
                    throw new IOException("ZeroTier socket is null");
                }
                if (_isDisposed) {
                    throw new ObjectDisposedException("ZeroTier.Sockets.Socket");
                }
                return _streamSocket.Available != 0;
            }
        }

        internal void InitNetworkStream(ZeroTier.Sockets.Socket socket, FileAccess accessMode)
        {
            if (! socket.Connected) {
                throw new IOException("ZeroTier socket must be connected");
            }
            if (! socket.Blocking) {
                throw new IOException("ZeroTier socket must be in blocking mode");
            }
            if (socket.SocketType != SocketType.Stream) {
                throw new IOException("ZeroTier socket must by stream type");
            }

            _streamSocket = socket;

            switch (accessMode) {
                case FileAccess.Write:
                    _isWriteable = true;
                    break;
                case FileAccess.Read:
                    _isReadable = true;
                    break;
                case FileAccess.ReadWrite:
                default:
                    _isReadable = true;
                    _isWriteable = true;
                    break;
            }
        }

        public override int Read([In, Out] byte[] buffer, int offset, int size)
        {
            bool canRead = CanRead;
            if (_isDisposed) {
                throw new ObjectDisposedException("ZeroTier.Sockets.Socket");
            }
            if (! canRead) {
                throw new InvalidOperationException("Cannot read from ZeroTier socket");
            }

            if (buffer == null) {
                throw new ArgumentNullException("buffer");
            }
            if (offset < 0 || offset > buffer.Length) {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (size < 0 || size > buffer.Length - offset) {
                throw new ArgumentOutOfRangeException("size");
            }

            if (_streamSocket == null) {
                throw new IOException("ZeroTier socket is null");
            }

            try {
                int bytesTransferred = _streamSocket.Receive(buffer, offset, size, 0);
                return bytesTransferred;
            }
            catch (Exception exception) {
                throw new IOException("Cannot read from ZeroTier socket", exception);
            }
        }

        public override void Write(byte[] buffer, int offset, int size)
        {
            bool canWrite = CanWrite;
            if (_isDisposed) {
                throw new ObjectDisposedException("ZeroTier.Sockets.Socket");
            }
            if (! canWrite) {
                throw new InvalidOperationException("Cannot write to ZeroTier socket");
            }
            if (buffer == null) {
                throw new ArgumentNullException("buffer");
            }
            if (offset < 0 || offset > buffer.Length) {
                throw new ArgumentOutOfRangeException("offset");
            }
            if (size < 0 || size > buffer.Length - offset) {
                throw new ArgumentOutOfRangeException("size");
            }
            if (_streamSocket == null) {
                throw new IOException("ZeroTier socket is null");
            }

            try {
                _streamSocket.Send(buffer, offset, size, SocketFlags.None);
            }
            catch (Exception exception) {
                throw new IOException("Cannot write to ZeroTier socket", exception);
            }
        }

        internal bool Poll(int microSeconds, SelectMode mode)
        {
            if (_streamSocket == null) {
                throw new IOException("ZeroTier socket is null");
            }
            if (_isDisposed) {
                throw new ObjectDisposedException("ZeroTier.Sockets.Socket");
            }
            return _streamSocket.Poll(microSeconds, mode);
        }

        internal bool PollRead()
        {
            if (_streamSocket == null) {
                return false;
            }
            if (_isDisposed) {
                return false;
            }
            return _streamSocket.Poll(0, SelectMode.SelectRead);
        }

        public override void Flush()
        {
            // Not applicable
        }

        public override void SetLength(long value)
        {
            throw new NotSupportedException("Not supported");
        }

        public override long Length
        {
            get {
                throw new NotSupportedException("Not supported");
            }
        }

        public override long Position
        {
            get {
                throw new NotSupportedException("Not supported");
            }

            set {
                throw new NotSupportedException("Not supported");
            }
        }

        public override long Seek(long offset, SeekOrigin origin)
        {
            throw new NotSupportedException("Not supported");
        }

        public void Close(int timeout)
        {
            if (timeout < 0) {
                throw new ArgumentOutOfRangeException("Timeout value must be greater than zero");
            }
            _streamSocket.Close(timeout);
        }

        internal bool Connected
        {
            get {
                if (! _isDisposed && _streamSocket != null && _streamSocket.Connected) {
                    return true;
                }
                else {
                    return false;
                }
            }
        }

        protected override void Dispose(bool disposing)
        {
            bool cleanedUp = _isDisposed;
            _isDisposed = true;
            if (! cleanedUp && disposing) {
                if (_streamSocket != null) {
                    _isWriteable = false;
                    _isReadable = false;
                    if (_ownsSocket) {
                        if (_streamSocket != null) {
                            _streamSocket.Shutdown(SocketShutdown.Both);
                            _streamSocket.Close();
                        }
                    }
                }
            }
            base.Dispose(disposing);
        }

        ~NetworkStream()
        {
            Dispose(false);
        }
    }
}
