import { setTimeout } from "timers/promises";

import { startNode, zts, net } from "../index";
import { createReadStream, createWriteStream } from "fs";


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


    // const total = 100_000_000;// 1_000_000_000;
    if (server) {
        const server = new net.Server({}, socket => {
            console.log("connection");
            socket.on("end", ()=>console.log("socket ended"));
            socket.on("close", ()=>console.log("socket closed"));

            const input = createReadStream("./randombytes");
            input.pipe(socket);
        });
        server.listen(port, "::", () => {
            console.log("listening");
            console.log(server.address());
        });
    } else {
        const socket = net.connect(port, host, async () => {
            console.log("connected");

            const output = createWriteStream("./randomreceived");
            socket.pipe(output);
            socket.on("end", () => {
                console.log("socket ended");
                socket.end();
            });
        });

        socket.on("error", error => console.log(error));
        socket.on("close", () => console.log("socket closed in main"));

    }
}

main();
