'use strict'

const libzt = require('./libzt')

// libzt.example()

libzt.start(".zerotier", 9994)

libzt.join("8056c2e21c000001")

// Usage: `nc -lv 4444`
let client = libzt.createConnection({ port: 4444, host: '29.49.7.203' }, () => {
    // 'connect' listener.
    console.log('connected to server!');
    // client.write('world!\r\n');
});
client.write("Name?\n", 'utf8');
client.on('data', (data) => {
    console.log(data.toString());
    client.end();
});
client.on('end', () => {
    console.log('disconnected from server');
});
