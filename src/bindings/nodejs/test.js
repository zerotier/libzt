var zts = require('bindings')('node-libzt');

console.log(zts);
console.log(zts.initFromStorage('id')); // 'world'

zts.startNode();

console.log("node started?")

// const nwid = Buffer.from("ff0000ffff0000000", "hex").readBigUInt64BE()
const nwid = Buffer.from("c7c8172af1579292", "hex").readBigUInt64BE()

zts.joinNetwork(nwid)

try {
    console.log(zts.getNetworkIPv4Addr(nwid))
} catch (error) {
    console.log(error)
}
try {
    console.log(zts.getNetworkIPv6Addr(nwid))
} catch (error) {
    console.log(error)
}


process.stdin.resume();