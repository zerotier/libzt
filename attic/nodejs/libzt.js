'use strict';

const net = require('net');
const stream = require('stream');
const { types: { isUint8Array } } = require('util');

const nbind = require('@mcesystems/nbind')
const ZeroTier = nbind.init().lib.ZeroTier

const {
  errnoException,
  writevGeneric,
  writeGeneric,
  onStreamRead,
  kAfterAsyncWrite,
  kHandle,
  kUpdateTimer,
  // setStreamTimeout,
  kBuffer,
  kBufferCb,
  kBufferGen
} = require('./stream_commons');

const kLastWriteQueueSize = Symbol('lastWriteQueueSize');


/*
 * EXAMPLE of Low-level usage
 * Usage: `nc -lv 4444`
 */

function example(nwid, address, port) {
  // Start ZeroTier service
  ZeroTier.start(".zerotier", 9994);

  // Join virtual network
  ZeroTier.join(nwid);

  // Connect the socket
  const _connect = (address, port, callback) => {
    // Open the socket
    const fd = ZeroTier.openStream();
    if (fd < 0) { callback(new Error('Could not open socket, errno: ' + fd)); return; }

    // Try connect
    const status = ZeroTier.connect(fd, address, port);

    console.log(status);
    if (status === 0) {
      callback(null, fd);
    } else {
      // Close previous socket
      ZeroTier.close(fd);
      setTimeout(_connect, 250, address, port, callback);
    }
  }

  // Receive some data
  const _read = (fd, callback) => {
    const buf = Buffer.alloc(32)
    let bytes = -1
    do {
      bytes = ZeroTier.recv(fd, buf, 0)
      if (bytes > 0) { callback(null, buf); }
    } while (bytes > 0);

    if (!ZeroTier.getMyNode().online || buf.toString('utf8').includes("exit")) {
      callback('end');
    } else {
      setTimeout(_read, 500, fd, callback)
    }
  }

  _connect(address, port, (err, fd) => {
    if (err) { console.error(err); return; }
    console.debug("Connected.");

    // Send some data
    ZeroTier.send(fd, Buffer.from("Name?\n", 'utf8'), 0);

    // Set blocking read mode
    // ZeroTier.setBlocking(fd, true);
    const heartbeat = setInterval(() => process.stderr.write('.'), 100);

    _read(fd, (stop, buf) => {
      if (stop) {
        // Close the socket
        ZeroTier.close(fd);
        // Stop ZeroTier service
        ZeroTier.stop();
        // Clear the interval
        clearInterval(heartbeat);
        return;
      }

      process.stdout.write(buf.toString('utf8'));
    });
  });
}


// Target API:
//
// let s = net.connect({port: 80, host: 'google.com'}, function() {
//   ...
// });
//
// There are various forms:
//
// connect(options, [cb])
// connect(port, [host], [cb])
// connect(path, [cb]);
//
function connect(...args) {
  const normalized = net._normalizeArgs(args);
  const options = normalized[0];
  // debug('createConnection', normalized);

  const socket = new Socket(Object.assign({ handle: new ZTCP() }, options));

  if (options.timeout) {
    socket.setTimeout(options.timeout);
  }

  return socket.connect(normalized);
}

/*
 * https://github.com/nodejs/node/blob/v12.18.3/lib/net.js#L567
 */
function tryReadStart(socket) {
  // Not already reading, start the flow
  // debug('Socket._handle.readStart');
  socket._handle.reading = true;
  const err = socket._handle.readStart();
  if (err)
    socket.destroy(errnoException(err, 'read'));
}

/*
 * https://github.com/nodejs/node/blob/v12.18.3/lib/net.js#L1107
 */
// function afterConnect(status, self, req, readable, writable) {
//   // const self = handle[owner_symbol];

//   // Callback may come after call to destroy
//   if (self.destroyed) {
//     return;
//   }

//   // debug('afterConnect');

//   // assert(self.connecting);
//   self.connecting = false;
//   self._sockname = null;

//   if (status === 0) {
//     self.readable = readable;
//     if (!self._writableState.ended)
//       self.writable = writable;
//     self._unrefTimer();

//     self.emit('connect');
//     self.emit('ready');

//     // Start the first read, or get an immediate EOF.
//     // this doesn't actually consume any bytes, because len=0.
//     if (readable && !self.isPaused())
//       self.read(0);

//   } else {
//     self.connecting = false;
//     let details;
//     if (req.localAddress && req.localPort) {
//       details = req.localAddress + ':' + req.localPort;
//     }
//     const ex = new Error(status,
//                                      'connect',
//                                      req.address,
//                                      req.port,
//                                      details);
//     if (details) {
//       ex.localAddress = req.localAddress;
//       ex.localPort = req.localPort;
//     }
//     self.destroy(ex);
//   }
// }

// function afterShutdown(self, _status) {
//   // const self = this.handle[owner_symbol];

//   // debug('afterShutdown destroyed=%j', self.destroyed,
//   //       self._readableState);

//   this.callback();

//   // Callback may come after call to destroy.
//   if (self.destroyed)
//     return;

//   if (!self.readable || self.readableEnded) {
//     // debug('readableState ended, destroying');
//     self.destroy();
//   }
// }

// function writeGeneric(self, chunk, encoding, callback) {
//   const decodeStrings = self._writableState && self._writableState.decodeStrings
//   const buf = (!decodeStrings && !Buffer.isBuffer(chunk)) ? Buffer.from(chunk, encoding) : chunk

//   let bytes
//   const err = ZeroTier.send(self._fd, buf, 0)
//   switch (err) {
//     case -1:
//       callback(new Error("ZeroTier Socket error"))
//       break
//     case -2:
//       callback(new Error("ZeroTier Service error"))
//       break
//     case -3:
//       callback(new Error("ZeroTier Invalid argument"))
//       break
//     default:
//       bytes = err
//       callback()
//   }

//   return {
//     async: true,
//     bytes: bytes,
//   }
// }

// function writevGeneric(self, chunks, callback) {
//   const decodeStrings = self._writableState && self._writableState.decodeStrings
//   const bufs = chunks.map(({ chunk, encoding }) => (!decodeStrings && !Buffer.isBuffer(chunk)) ? Buffer.from(chunk, encoding) : chunk)

//   let bytes
//   const err = ZeroTier.writev(self._fd, bufs)
//   switch (err) {
//     case -1:
//       callback(new Error("ZeroTier Socket error"))
//       break
//     case -2:
//       callback(new Error("ZeroTier Service error"))
//       break
//     case -3:
//       callback(new Error("ZeroTier Invalid argument"))
//       break
//     default:
//       bytes = err
//       callback()
//   }

//   return {
//     async: true,
//     bytes: bytes,
//   }
// }



class ZTCP {
  bytesRead = 0
  bytesWritten = 0
  writeQueueSize = 0

  _fd = null
  _reading = false
  readTimer = null

  get reading() {
    return this._reading;
  }

  set reading(val) {
    return this._reading = val;
  }

  readStart() {
    if (!this._buf) {
      this._buf = Buffer.alloc(128);
    }

    let bytes = 0
    do {
      bytes = ZeroTier.read(this._fd, this._buf)
      if (bytes >= 0) {
        this.bytesRead += bytes;
        bytes = 0;
      }
      switch (bytes) {
        case -2:
          throw new Error("ZeroTier Service error")
        case -3:
          throw new Error("ZeroTier Invalid argument")
        default:
          if (bytes > 0) {
            this.bytesRead += bytes
            this._buf = this.onread(this._buf)
          }
      }
    } while (bytes > 0 && this._reading)

    if (this._reading) { readTimer = setTimeout(() => this._read(size), 500) }
  }

  readStop() {
    if (readTimer) {
      clearTimeout(readTimer);
      readTimer = null;
    }
    this._reading = false
  }

  writev(req, chunks, allBuffers) {
    let bufs = [];

    if (allBuffers) {
      bufs = chunks;
    } else {
      const arr = chunks;
      for (let i = 0; i < arr.length; i+=2) {
        const chunk = arr[i];
        const encoding = arr[i+1];
        chunks.push(Buffer.from(chunk, encoding));
      }
    }

    let bytes = ZeroTier.writev(this._fd, bufs);
    if (bytes >= 0) {
      this.bytesWritten += bytes;
      bytes = 0;
    }

    const status = bytes;
    // https://github.com/nodejs/node/blob/v12.18.3/lib/internal/stream_base_commons.js#L80
    if (req.oncomplete) { req.oncomplete.call(req, status); }
    return status;
  }

  writeBuffer(req, buf) {
    let bytes = ZeroTier.write(this._fd, buf);
    if (bytes >= 0) {
      this.bytesWritten += bytes;
      bytes = 0;
    }

    const status = bytes;
    // https://github.com/nodejs/node/blob/v12.18.3/lib/internal/stream_base_commons.js#L80
    if (req.oncomplete) { req.oncomplete.call(req, status); }
    return status;
  }

  writeLatin1String(req, data) {
    return this.writeBuffer(req, Buffer.from(data, 'latin1'));
  }

  writeUtf8String(req, data) {
    return this.writeBuffer(req, Buffer.from(data, 'utf8'));
  }

  writeAsciiString(req, data) {
    return this.writeBuffer(req, Buffer.from(data, 'ascii'));
  }

  writeUcs2String(req, data) {
    return this.writeBuffer(req, Buffer.from(data, 'ucs2'));
  }

  getAsyncId() {
    return -1;
  }

  useUserBuffer(buf) {
    this._buf = buf;
  }

  setBlocking(newValue) {
    return ZeroTier.setBlocking(this._fd, newValue);
  }

  setNoDelay(newValue) {
    return ZeroTier.setNoDelay(this._fd, newValue);
  }

  setKeepalive(enable, initialDelay) {
    ZeroTier.setKeepidle(initialDelay);
    return ZeroTier.setKeepalive(this._fd, +enable);
  }

  bind(localAddress, localPort) {
    return ZeroTier.bind(this._fd, localAddress, localPort);
  }

  bind6(localAddress, localPort, _flags) {
    return ZeroTier.bind6(this._fd, localAddress, localPort);
  }

  open(fd) {
    if (fd) {
      this._fd = fd;
      return 0;
    } else {
      const err = ZeroTier.openStream();
      if (err < 0) {
        return err;
      } else {
        this._fd = err;
        return 0;
      }
    }
  }

  close(callback) {
    const err = ZeroTier.close(this._fd);
    this._fd = null;
    if (callback) { callback(err); }
  }

  shutdown(req) {
    const status = ZeroTier.shutdown(this._fd);
    // https://github.com/nodejs/node/blob/v12.18.3/test/parallel/test-tcp-wrap-connect.js
    if (req.oncomplete) { req.oncomplete.call(req, status, this); }
    return status;
  }

  connect(req, address, port) {
    let status = ZeroTier.connect(this._fd, address, port);
    
    // Retries are often required since ZT uses transport-triggered links
    if (status !== 0) {
      let count = 0;
      while (count < 10) {
        // Close previous socket
        this.close();
        status = this.open();
        if (status !== 0) {
          // Break if reopen-socket fails
          break;
        }

        // Reconnect
        status = ZeroTier.connect(this._fd, address, port);
        if (status === 0) { break; }

        count++;
      }
    }

    // https://github.com/nodejs/node/blob/v12.18.3/test/parallel/test-tcp-wrap-connect.js
    if (req && req.oncomplete) { req.oncomplete.call(status, this, req, true, true); }

    return status;
  }

  connect6(req, address, port) {
    let status = ZeroTier.connect6(this._fd, address, port);

    // Retries are often required since ZT uses transport-triggered links
    if (status !== 0) {
      let count = 0;
      while (count < 10) {
        // Close previous socket
        this.close();
        status = this.open();
        if (status !== 0) {
          // Break if reopen-socket fails
          break;
        }

        // Reconnect
        status = ZeroTier.connect6(this._fd, address, port);
        if (status === 0) { break; }

        count++;
      }
    }

    // https://github.com/nodejs/node/blob/v12.18.3/test/parallel/test-tcp-wrap-connect.js
    if (req.oncomplete) { req.oncomplete.call(status, this, req, true, true); }

    return status;
  }

  getpeername(out) {
    const in4 = ZeroTier.getpeername(this._fd);
    out.address = ZeroTier.inet_ntop(in4);
    out.family = in4.sin_family;
    out.port = in4.sin_port;
    return 0
  }

  getsockname(out) {
    const in4 = ZeroTier.getsockname(this._fd);
    out.address = ZeroTier.inet_ntop(in4);
    out.family = in4.sin_family;
    out.port = in4.sin_port;
    return 0;
  }

  listen(port) {
    // TODO
    // this.onconnection
  }

  fchmod(mode) {
    // TODO
    return 0;
  }
}

class Socket extends net.Socket {
  [kLastWriteQueueSize] = 0;
  [kBuffer] = null;
  [kBufferCb] = null;
  [kBufferGen] = null;

  [kHandle] = null;
  get _handle() { return this[kHandle]; }
  set _handle(v) { return this[kHandle] = v; }

  /*
   * https://github.com/nodejs/node/blob/v12.18.3/lib/net.js#L929
   */
  // connect(...args) {
  //   let normalized;
  //   // If passed an array, it's treated as an array of arguments that have
  //   // already been normalized (so we don't normalize more than once). This has
  //   // been solved before in https://github.com/nodejs/node/pull/12342, but was
  //   // reverted as it had unintended side effects.
  //   if (Array.isArray(args[0])) {
  //     normalized = args[0];
  //   } else {
  //     normalized = net._normalizeArgs(args);
  //   }
  //   const options = normalized[0];
  //   const cb = normalized[1];

  //   if (this.write !== net.Socket.prototype.write)
  //       this.write = net.Socket.prototype.write;

  //   if (this.destroyed) {
  //     this._handle = null;
  //     this._peername = null;
  //     this._sockname = null;
  //   }

  //   if (!this._handle) {
  //       this._handle = new ZTCP();
  //       initSocketHandle(this);
  //   }

  //   if (cb !== null) {
  //     this.once('connect', cb);
  //   }

  //   this._unrefTimer();

  //   this.connecting = true;
  //   this.writable = true;

  //   const { host, port } = options;
  //   // If host is an IP, skip performing a lookup
  //   const addressType = net.isIP(host);
  //   if (addressType) {
  //     this._fd = ZeroTier.connectStream(host, port);
  //     afterConnect(0, this, {}, true, true);
  //   } else {
  //     throw new Error("DNS LOOKUP NOT IMPLEMENTED");
  //   }

  //   return this;
  // }

  /*
   * https://github.com/nodejs/node/blob/v12.18.3/lib/net.js#L596
   */
  pause() {
    if (this[kBuffer] && !this.connecting && this._handle &&
        this._handle.reading) {
      this._handle.reading = false;
      if (!this.destroyed) {
        const err = this._handle.readStop();
        if (err)
          this.destroy(errnoException(err, 'read'));
      }
    }
    return stream.Duplex.prototype.pause.call(this);
  }
  
  /*
   * https://github.com/nodejs/node/blob/v12.18.3/lib/net.js#L610
   */
  resume() {
    if (this[kBuffer] && !this.connecting && this._handle &&
        !this._handle.reading) {
      tryReadStart(this);
    }
    return stream.Duplex.prototype.resume.call(this);
  }
  
  /*
   * https://github.com/nodejs/node/blob/v12.18.3/lib/net.js#L619
   */
  read(n) {
    if (this[kBuffer] && !this.connecting && this._handle &&
        !this._handle.reading) {
      tryReadStart(this);
    }
    return stream.Duplex.prototype.read.call(this, n);
  }

  /*
   * https://nodejs.org/docs/latest-v12.x/api/stream.html#stream_readable_read_size_1
   */
  _read(n) {
    // debug('_read');

    if (this.connecting || !this._handle) {
      // debug('_read wait for connection');
      this.once('connect', () => this._read(n));
    } else if (!this._handle.reading) {
      tryReadStart(this);
    }

    // if (!this.readChunk || this.readChunk.length < n) {
    //   this.readChunk = Buffer.alloc(n)
    // }

    // let bytes = -1
    // let moreData = true
    // do {
    //   bytes = ZeroTier.recv(this._fd, this.readChunk, 0)
    //   switch (bytes) {
    //     case -2:
    //       throw new Error("ZeroTier Service error")
    //     case -3:
    //       throw new Error("ZeroTier Invalid argument")
    //     default:
    //       if (bytes > 0) {
    //         // this.bytesRead += bytes
    //         moreData = this.push(this.readChunk)
    //       }
    //   }
    // } while (bytes > 0 && moreData)

    // if (moreData) { setTimeout(() => this._read(n), 500) }
  }

  /*
  * https://nodejs.org/docs/latest-v12.x/api/stream.html#stream_writable_writev_chunks_callback
  */
  _writev(chunks, cb) {
    this._writeGeneric(true, chunks, '', cb);
  }

  /*
  * https://nodejs.org/docs/latest-v12.x/api/stream.html#stream_writable_write_chunk_encoding_callback_1
  */
  _write(data, encoding, cb) {
    this._writeGeneric(false, data, encoding, cb);
  }

  /*
   * https://nodejs.org/docs/latest-v12.x/api/stream.html#stream_writable_final_callback
   */
  // _final(cb) {
  //   // If still connecting - defer handling `_final` until 'connect' will happen
  //   if (this.pending) {
  //     // debug('_final: not yet connected');
  //     return this.once('connect', () => this._final(cb));
  //   }
  
  //   if (!this._handle)
  //     return cb();
  
  //   // debug('_final: not ended, call shutdown()');
  
  //   // const req = new ShutdownWrap();
  //   const req = {};
  //   req.oncomplete = afterShutdown;
  //   req.handle = this._handle;
  //   req.callback = cb;
  //   // const err = this._handle.shutdown(req);
  //   const err = ZeroTier.shutdown(this._fd);
  //   return afterShutdown.call(req, this, 0);
  // }

  /*
   * https://github.com/nodejs/node/blob/v12.18.3/lib/net.js#L760
   */
  _writeGeneric(writev, data, encoding, cb) {
    // If we are still connecting, then buffer this for later.
    // The Writable logic will buffer up any more writes while
    // waiting for this one to be done.
    if (this.connecting) {
      this._pendingData = data;
      this._pendingEncoding = encoding;
      this.once('connect', function connect() {
        this._writeGeneric(writev, data, encoding, cb);
      });
      return;
    }
    this._pendingData = null;
    this._pendingEncoding = '';
  
    if (!this._handle) {
      cb(new Error('ERR_SOCKET_CLOSED'));
      return false;
    }
  
    this._unrefTimer();
  
    let req;
    if (writev)
      req = writevGeneric(this, data, cb);
    else
      req = writeGeneric(this, data, encoding, cb);
    if (req.async)
      this[kLastWriteQueueSize] = req.bytes;
  }

  /*
   * https://github.com/nodejs/node/blob/v12.18.3/lib/net.js#L552
   */
  get bufferSize() {
    if (this._handle) {
      return this[kLastWriteQueueSize] + this.writableLength;
    }
  }

  /*
   * https://github.com/nodejs/node/blob/v12.18.3/lib/net.js#L756
   */
  [kAfterAsyncWrite]() {
    this[kLastWriteQueueSize] = 0;
  }

  /*
   * https://github.com/nodejs/node/blob/v12.18.3/lib/net.js#L468
   */
  _onTimeout() {
    const handle = this._handle;
    const lastWriteQueueSize = this[kLastWriteQueueSize];
    if (lastWriteQueueSize > 0 && handle) {
      // `lastWriteQueueSize !== writeQueueSize` means there is
      // an active write in progress, so we suppress the timeout.
      const { writeQueueSize } = handle;
      if (lastWriteQueueSize !== writeQueueSize) {
        this[kLastWriteQueueSize] = writeQueueSize;
        this._unrefTimer();
        return;
      }
    }
    // debug('_onTimeout');
    this.emit('timeout');
  }

  get [kUpdateTimer]() {
    return this._unrefTimer;
  }
}

module.exports = {
  start: ZeroTier.start,
  join: ZeroTier.join,
  restart: ZeroTier.restart,
  stop: ZeroTier.stop,
  free: ZeroTier.free,
  example,
  connect,
  createConnection: connect,
  Socket,
  Stream: Socket, // Legacy naming
  TCP: ZTCP,
};
