import { setTimeout } from "timers/promises";

import { startNode, zts,  dgram} from "../index";
// import * as dgram from "../module/UDPSocket";


async function main() {
    console.log(`
Tests UDP using ad-hoc network."
Usage: <cmd> server <port>"
Usage: <cmd> client <port> <server ip>"
Server echoes received messages in upper case.
Client sends 10 messages, 5 unconnected (specifying recipient) and 5 connected.
`);

    if(process.argv.length < 3) return;

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
        console.log(`ipv6 address: ${zts.addr_get_str(nwid, true)}`);
    } catch (error) {
        console.log(error);
    }

    
    if (server) {
        const server = dgram.createSocket({type: "udp6"}, (msg, rinfo) => {
            // message listener
            console.log(`received: ${msg.length} from ${rinfo.address} at ${rinfo.port}, rinfo size ${rinfo.size}: ${msg.toString().slice(0,16)}`);
            server.send(Buffer.from(msg.toString().toUpperCase()), rinfo.port, rinfo.address, (err)=> {
                if(err) console.log(err);
                console.log("sent");
            });

        });
        server.bind(port, undefined, ()=>console.log(server.address()));
    } else {

        const socket = dgram.createSocket({type: "udp6"}, (msg, rinfo) => {
            // message listener
            console.log(`received: ${rinfo.size} from ${rinfo.address} at ${rinfo.port}: ${msg.toString().slice(0,16)}`);
        });

        for(let i = 0; i < 5; i++) {
            console.log(`sending ${i}`);
            
            socket.send(Buffer.from(`unconnected hello ${i}`.repeat(100)), port, host, (err)=>{
                console.log("sent");
                if(err) console.log(err);
            });
            await setTimeout(50);
        }

        socket.connect(port, host, async (err)=>{
            console.log(err);

            console.log(socket.remoteAddress());

            for(let i = 0; i < 5; i++) {
                console.log(`sending ${i}`);
                
                socket.send(Buffer.from(`connected hello ${i}`.repeat(100)));
                await setTimeout(50);
            }

            socket.disconnect();

            await setTimeout(100);

            socket.close();
            socket.on("close", ()=>{
                console.log("closed!");

                zts.node_free();
            });
    
            console.log("closed?");
        });


    }
}

main();
