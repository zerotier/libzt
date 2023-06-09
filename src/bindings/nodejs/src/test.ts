import { init } from ".";
import { Socket } from "./Socket";
import zts from "./zts"

init("id");

while(! zts.node_is_online()) {
    zts.util_delay(50);
}

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
    console.log(error.toString())
}
try {
    console.log(zts.addr_get_str(nwid, true))
} catch (error) {
    console.log(error.toString())
}

const s = new Socket("172.24.144.58", 6000)
// const s = net.connect(6000);

let i = 0;

s.on("connect", ()=> {
    console.log("connected");
    s.write(Buffer.from("ping"+i));
    i++;
});

s.on("data", data=> {
    console.log(`${data.length}`);
    setTimeout(()=>{
        s.write(Buffer.alloc(10_000))
        i++
    }, 0);
});

s.on("error", err => console.log(err));
