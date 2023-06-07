var zts = require('bindings')('node-libzt');

console.log(zts);
console.log(zts.init_from_storage('id')); // 'world'

zts.init_set_event_handler((ev) => console.log(ev));

zts.node_start();

while(! zts.node_is_online()) {
    zts.util_delay(50);
}

console.log("node started?")
console.log(`node id: ${zts.node_get_id().toString(16)}`)

const nwid = BigInt("0xff0000ffff000000")


// 40 node id + 24 net id

console.log(nwid.toString(16))
zts.net_join(nwid)

while(! zts.net_transport_is_ready(nwid)) {
    zts.util_delay(50);
}

try {
    console.log(zts.addr_get_str(nwid, false))
} catch (error) {
    console.log(error)
}
try {
    console.log(zts.addr_get_str(nwid, true))
} catch (error) {
    console.log(error)
}

const fd = zts.bsd_socket(2, 1, 0)
console.log(fd)
zts.connect(fd, "172.24.144.58", 6000, 0)

zts.bsd_send(fd, Buffer.from("hello!"), 0)

const data = Buffer.alloc(10)
const n = zts.bsd_recv(fd, data, 0)

console.log(data.subarray(0, n).toString())


zts.bsd_close(fd)

process.stdin.resume();