import { init } from ".";
import { Server } from "./Server";
import { Socket, connect } from "./Socket";
import zts from "./zts"

const server = process.argv[2] === "server"
const port = parseInt(process.argv[3])
const host = process.argv[4]

init("id" + (server ? "server" : ""));

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


if(server) {
    console.log("starting server")
    const server = new Server({}, socket => {
        console.log("new connection")
        socket.on("data", data=>socket.write(data));
        socket.on("error", ()=>console.log("error"))
    });
    server.listen(port, "::", ()=>console.log("listening"));
    server.on("error", err=>console.log(err));
} else {

    const s = connect(host, port)
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
            s.write(Buffer.from("ping"+i));
            i++
        }, 100);
    });
    
    s.on("error", err => console.log(err));
}
