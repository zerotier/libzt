'use strict';

const net = require('net');
const nbind = require('@mcesystems/nbind')
const ZeroTier = nbind.init().lib.ZeroTier


/*
 * EXAMPLE USAGE
 * Usage: `nc -lv 4444`
 */

function example() {
  // Start ZeroTier service
  ZeroTier.start(".zerotier", 9994);

  // Join virtual network
  ZeroTier.join("8056c2e21c000001");

  // Open the socket
  let fd = ZeroTier.connectStream("29.49.7.203", 4444);

  // Send some data
  ZeroTier.send(fd, Buffer.from("Name?\n", 'utf8'), 0)

  // Set blocking read mode
  // ZeroTier.fcntlSetBlocking(fd, true);
  let heartbeat = setInterval(() => process.stderr.write('.'), 100)

  // Receive some data
  const _read = () => {
    const buf = Buffer.alloc(32)
    let bytes = -1
    do {
      bytes = ZeroTier.recv(fd, buf, 0)
      if (bytes > 0) { process.stdout.write(buf.toString('utf8')) }
    } while (bytes > 0);

    if (!ZeroTier.getMyNode().online || buf.toString('utf8').includes("exit")) {
      // Close the socket
      ZeroTier.close(fd)
      // Stop ZeroTier service
      ZeroTier.stop()
      // Clear the interval
      clearInterval(heartbeat)
    } else {
      setTimeout(_read, 500)
    }
  }
  _read()
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
  const socket = new Socket(options);

  if (options.timeout) {
    socket.setTimeout(options.timeout);
  }

  return socket.connect(normalized);
}

/*
 * https://github.com/nodejs/node/blob/v12.18.3/lib/net.js#L1107
 */
function afterConnect(status, self, req, readable, writable) {
  // const self = handle[owner_symbol];

  // Callback may come after call to destroy
  if (self.destroyed) {
    return;
  }

  // debug('afterConnect');

  // assert(self.connecting);
  self.connecting = false;
  self._sockname = null;

  if (status === 0) {
    self.readable = readable;
    if (!self._writableState.ended)
      self.writable = writable;
    self._unrefTimer();

    self.emit('connect');
    self.emit('ready');

    // Start the first read, or get an immediate EOF.
    // this doesn't actually consume any bytes, because len=0.
    if (readable && !self.isPaused())
      self.read(0);

  } else {
    self.connecting = false;
    let details;
    if (req.localAddress && req.localPort) {
      details = req.localAddress + ':' + req.localPort;
    }
    const ex = new Error(status,
                                     'connect',
                                     req.address,
                                     req.port,
                                     details);
    if (details) {
      ex.localAddress = req.localAddress;
      ex.localPort = req.localPort;
    }
    self.destroy(ex);
  }
}

function writeGeneric(self, chunk, encoding, callback) {
  const buf = (!self.decodeStrings && !Buffer.isBuffer(chunk)) ? Buffer.from(chunk, encoding) : chunk

  let bytes
  const err = ZeroTier.send(self._fd, buf, 0)
  switch (err) {
    case -1:
      callback(new Error("ZeroTier Socket error"))
      break
    case -2:
      callback(new Error("ZeroTier Service error"))
      break
    case -3:
      callback(new Error("ZeroTier Invalid argument"))
      break
    default:
      bytes = err
      callback(null)
  }

  return {
    async: true,
    bytes: bytes,
  }
}

function writevGeneric(self, chunks, callback) {
  const bufs = chunks.map(({ chunk, encoding }) => (!self.decodeStrings && !Buffer.isBuffer(chunk)) ? Buffer.from(chunk, encoding) : chunk)

  let bytes
  const err = ZeroTier.writev(self._fd, bufs)
  switch (err) {
    case -1:
      callback(new Error("ZeroTier Socket error"))
      break
    case -2:
      callback(new Error("ZeroTier Service error"))
      break
    case -3:
      callback(new Error("ZeroTier Invalid argument"))
      break
    default:
      bytes = err
      callback(null)
  }

  return {
    async: true,
    bytes: bytes,
  }
}

class Socket extends net.Socket {
  /*
   * https://github.com/nodejs/node/blob/v12.18.3/lib/net.js#L929
   */
  connect(...args) {
    let normalized;
    // If passed an array, it's treated as an array of arguments that have
    // already been normalized (so we don't normalize more than once). This has
    // been solved before in https://github.com/nodejs/node/pull/12342, but was
    // reverted as it had unintended side effects.
    if (Array.isArray(args[0])) {
      normalized = args[0];
    } else {
      normalized = net._normalizeArgs(args);
    }
    const options = normalized[0];
    const cb = normalized[1];

    // if (this.write !== net.Socket.prototype.write)
    //     this.write = net.Socket.prototype.write;

    if (this.destroyed) {
      this._handle = null;
      this._peername = null;
      this._sockname = null;
    }

    // const { path } = options;
    // const pipe = !!path;
    // debug('pipe', pipe, path);

    // if (!this._handle) {
    //     this._handle = pipe ?
    //         new Pipe(PipeConstants.SOCKET) :
    //         new TCP(TCPConstants.SOCKET);
    //     initSocketHandle(this);
    // }

    if (cb !== null) {
      this.once('connect', cb);
    }

    this._unrefTimer();

    this.connecting = true;
    this.writable = true;

    // if (pipe) {
    //     validateString(path, 'options.path');
    //     defaultTriggerAsyncIdScope(
    //         this[async_id_symbol], internalConnect, this, path
    //     );
    // } else {
    //     lookupAndConnect(this, options);
    // }

    const { host, port } = options;
    // If host is an IP, skip performing a lookup
    const addressType = net.isIP(host);
    if (addressType) {
      this._fd = ZeroTier.connectStream(host, port);
      afterConnect(0, this, {}, true, true);
    } else {
      throw new Error("DNS LOOKUP NOT IMPLEMENTED");
    }

    return this;
  }

  /*
   * https://nodejs.org/docs/latest-v12.x/api/stream.html#stream_readable_read_size_1
   */
  _read(size) {
    // debug('_read');

    if (this.connecting) {
      // debug('_read wait for connection');
      this.once('connect', () => this._read(size));
      return
    }

    if (!this.readChunk || this.readChunk.length < size) {
      this.readChunk = Buffer.alloc(size)
    }

    let bytes = -1
    let moreData = true
    do {
      bytes = ZeroTier.recv(this._fd, this.readChunk, 0)
      switch (bytes) {
        case -2:
          throw new Error("ZeroTier Service error")
        case -3:
          throw new Error("ZeroTier Invalid argument")
        default:
          if (bytes > 0) {
            // this.bytesRead += bytes
            moreData = this.push(this.readChunk)
          }
      }
    } while (bytes > 0 && moreData)

    if (moreData) { setTimeout(() => this._read(size), 500) }
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
  _final(callback) {
    const err = ZeroTier.close(this._fd)

    switch (err) {
      case -1:
        return callback(new Error("ZeroTier Socket error"))
        break
      case -2:
        return callback(new Error("ZeroTier Service error"))
        break
      default:
        return super._final(callback)
    }
  }

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

    // if (!this._handle) {
    //   cb(new ERR_SOCKET_CLOSED());
    //   return false;
    // }

    this._unrefTimer();

    let req;
    if (writev)
      req = writevGeneric(this, data, cb);
    else
      req = writeGeneric(this, data, encoding, cb);
    if (req.async) {
      // this[kLastWriteQueueSize] = req.bytes;
    }
  }
}

module.exports = {
  example,
  start: ZeroTier.start,
  join: ZeroTier.join,
  connect,
  createConnection: connect,
  Socket,
  Stream: Socket, // Legacy naming
};
