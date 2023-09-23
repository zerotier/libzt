import { setTimeout } from "timers/promises";

import { startNode, zts, net, Socket, connect } from "../index";



async function main() {

    const server = process.argv[2] === "server";
    const port = parseInt(process.argv[3]);
    const host = process.argv[4];

    console.log("starting node");
    startNode(`./id/${server ? "server" : "client"}`, (event) => console.log(event));

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
        console.log(error);
    }



    if (server) {
        const server = new net.Server({}, socket => {
            console.log("connection");
            
            const list: Buffer[] = [];
            socket.on("data", (data: Buffer) => {
                list.push(data);
                console.log(list[0].toString().slice(0,20));
                console.log(data.length);
            });
        });
        server.listen(port, "::", () => {
            console.log("listening");
            console.log(server.address());
        });
    } else {
        const socket = connect(host, port);
        socket.on("connect", async () => {
            console.log("connected");

            for (let i = 0; i < 100000; i++) {
                console.log(`sending ${i}`);

                socket.write(Buffer.from(`hello ${i} it is me :)`.repeat(100)));
                await setTimeout(5);
            }

            socket.write(Buffer.from("hello!"));
        });
        socket.on("error", error => console.log(error));
        socket.on("data", data => console.log(data));


    }
}

main();
