var http = require('http')

var zt = require('./libzt')

var earth = '8056c2e21c000001'
var listenPort = 8766

zt.simpleStart('./tmp/' + earth, earth)
var addr = zt.getIpV4Address(earth).split('/')[0]
var socket = zt.socket()
zt.bindPort(socket, addr, listenPort)
// zt.listen(socket)

console.log('socket fd', socket)
console.log('ip a', addr)
console.log(`http://${addr}:${listenPort}`)

var server = http.createServer(function (request, response) {
  response.writeHead(200, { 'Content-Type': 'text/plain' })
  response.end('Hello World\n')
})

// attempt to listen to file descriptor.
// doesn't work, but would be cool!
server.listen({ fd: socket })

// listen on localhost:8765. works
// server.listen(8765)
