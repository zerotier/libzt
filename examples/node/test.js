var fs = require('fs')

var zt = require('./libzt')

var running = zt.running()
console.log('running', running)

var earth = '8056c2e21c000001'
var listenPort = 1234

zt.simpleStart('./tmp/' + earth, earth)

var deviceId = zt.getDeviceId()
console.log('device id', deviceId)

var addr = zt.getIpV4Address(earth).split('/')[0]
console.log('ip a', addr)

var socket = zt.socket()
console.log('socket', socket)

var bindPort = zt.bindPort(socket, addr, listenPort)
console.log('bind', bindPort)

var listen = zt.listen(socket)
console.log('listen', listen)

console.log()
console.log('ready')
console.log(`run this in another terminal:\n\tnc ${addr} ${listenPort}`)
console.log(`then type something`)
console.log()

var fd = zt.accept(socket)

fd = fs.createReadStream(null, { fd: fd })
fd.pipe(process.stdout)

// Start reading from stdin so we don't exit.
process.stdin.resume()
// lib.ZT.stop()
