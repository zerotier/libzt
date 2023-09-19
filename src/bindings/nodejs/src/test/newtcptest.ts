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
        console.log(error.toString());
    }



    if (server) {
        const server = new net.Server({});
        server.listen(port, "::", () => {
            console.log("listening");
            console.log(server.address());
        });
    } else {
        const socket = connect(host, port);
        socket.on("connect", () => console.log("connected"));
        socket.on("error", error => console.log(error));
        socket.on("data", data => console.log(data));
    }
}

main();
