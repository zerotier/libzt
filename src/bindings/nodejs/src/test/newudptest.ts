import { setTimeout } from "timers/promises";
import { setTimeout as cbTimeout } from "timers";

import { startNode, zts , udp as dgram} from "../index";

async function main() {

    const server = process.argv[2] === "server";
    const port = parseInt(process.argv[3]);
    const host = process.argv[4];

    console.log("starting node");
    startNode(`./id/${server?"server" : "client"}`, (event)=> console.log(event));

    console.log("waiting for node to come online");
    while (!zts.node_is_online()) {
        await setTimeout(50);
    }

    console.log(zts.node_get_id());

    const nwid = "ff0000ffff000000";

    zts.net_join(nwid);

    while (!zts.net_transport_is_ready(nwid)) {
        await setTimeout(50);
    }

    try {
        console.log(zts.addr_get_str(nwid, true));
    } catch (error) {
        console.log(error.toString());
    }


    
    if (server) {

        // const udp = new zts.UDP(true, (data, addr, port) => {
        //     console.log(`received: ${data.toString()} from ${addr} at ${port}`);
        // });
        // udp.bind("::1", port);


        const s = dgram.createSocket({ type: "udp6" }, (msg, rinfo) => {
            console.log(`${msg}`);
            console.log(rinfo);
            s.send(msg, rinfo.port, rinfo.address);
        });
        s.bind(port);
        console.log(s.address());
    } else {
        const udp = new zts.UDP(true, (data, addr, port) => {
            console.log(`received: ${data.toString()} from ${addr} at ${port}`);
        });
        for(let i = 0; i < 10000000; i++) {
            console.log(`sending ${i}`);
            udp.send_to(Buffer.from(`hello ${i}`), host, port);
            await setTimeout(500);
        }

        // const s = dgram.createSocket({ type: "udp6" }, (msg, rinfo) => {
        //     console.log(`${msg}`);
        //     console.log(rinfo);
        // });
        // s.on("error", (err) => {
        //     console.log(err);
        // });
        // s.bind(port);
        // console.log(s.address());
        // // eslint-disable-next-line no-constant-condition
        // while (true) {
        //     console.log("sending");
        //     const msg = Buffer.from("abcdefg");
        //     s.send(msg, port, host, () => console.log("sent"));
        //     await setTimeout(1000);
        // }
    }
}

main();
