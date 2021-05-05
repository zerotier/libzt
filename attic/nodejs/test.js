'use strict'

const libzt = require('./libzt')

// libzt.example("8056c2e21c000001", "29.49.7.203", 4444)

libzt.start(".zerotier", 9994)

libzt.join("8056c2e21c000001")

// Usage: `nc -lv 4444`
let client = libzt.createConnection({ port: 4444, host: '29.49.7.203' }, () => {
    console.log('connected to server!');
});
client.on('ready', () => {
    client.write("Name?\n", 'utf8');
});
client.on('data', (data) => {
    console.log(data.toString('utf8').trimEnd());
    if (data.toString('utf8').includes("exit")) { client.end(); }
});
client.on('end', () => {
    console.log('disconnected from server');
    libzt.stop()
});
