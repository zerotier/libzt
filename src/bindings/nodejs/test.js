var zts = require('bindings')('node-libzt');

console.log(zts);
console.log(zts.init_from_storage('id')); // 'world'

zts.init_set_event_handler((ev) => console.log(ev));

zts.start_node();

while(! zts.node_is_online()) {
    zts.util_delay(50);
}

console.log("node started?")

const nwid = Buffer.from("ff0000ffff0000000", "hex").readBigUInt64BE()


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


process.stdin.resume();