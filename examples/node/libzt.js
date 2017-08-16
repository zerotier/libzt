var nbind = require('nbind')
var ZT = nbind.init().lib.ZT

module.exports = {
  accept: ZT.accept,
  bindPort: ZT.bind,
  getDeviceId: ZT.getDeviceId,
  getIpV4Address: ZT.getIpV4Address,
  listen: ZT.listen,
  running: ZT.running,
  simpleStart: ZT.simpleStart,
  socket: ZT.socket
}
