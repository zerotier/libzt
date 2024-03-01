import { setTimeout } from "timers/promises";

import { startNode, zts, net } from "../index";


async function main() {
    console.log(`
TCP example using ad-hoc network.

usage: <cmd> server
usage: <cmd> client <server ip>
    `);

    if (process.argv.length < 3) return;

    // handle cli arguments
    const server = process.argv[2] === "server";
    const serverIP = process.argv[3];

    // Setup the ZT node
    console.log("starting node");
    startNode(`./id/${server ? "server" : "client"}`, (event) => console.log(event));

    /**
     * This is not yet very "node"-like, ideally this would be something like
     * `await node.isOnline()`
     * `await node.transportReady(nwid)`
     */
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
        console.log(zts.addr_get_str(nwid, true)); // this throws if there is no ipv6 address, for example if network only has ipv4
    } catch (error) {
        console.log(error);
    }

    // code for both server and client is almost drop-in an example from the nodejs documentation
    if (server) {
        // https://nodejs.org/dist/latest-v20.x/docs/api/net.html#netcreateserveroptions-connectionlistener
        const server = new net.Server({}, (c) => {
            // 'connection' listener.
            console.log("client connected");
            c.on("end", () => {
                console.log("client disconnected");
            });
            c.write("hello\r\n"); // "hello" is written to the client
            c.pipe(c); // socket is readable for receiving and writable for sending. This pipes the client to itself, i.e. a trivial echo server.
        });
        server.on("error", (err) => {
            throw err;
        });
        server.listen(8124, undefined, () => {
            console.log("server bound");
        });


    } else {
        // https://nodejs.org/dist/latest-v20.x/docs/api/net.html#netcreateconnectionoptions-connectlistener

        const client = net.connect(8124, serverIP, () => {
            // 'connect' listener.
            console.log("connected to server!");
            client.write("world!\r\n");
        });
        client.on("data", (data: Buffer) => { 
            console.log(data.toString()); // data listener, received data is printed out
            client.end();
        });
        client.on("end", () => {
            console.log("disconnected from server");
            zts.node_free();
        });
    }
}

main();
