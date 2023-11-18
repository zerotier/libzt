import { setTimeout } from "timers/promises";

import { startNode, zts, net, SocketErrors, events } from "../index";
import { createReadStream, createWriteStream, existsSync } from "fs";
import { Duplex, Transform, PassThrough } from "stream";


// eslint-disable-next-line no-constant-condition
const progress = (true) ? new PassThrough() : new Transform({
    transform(chunk, encoding, callback) {
        console.log(chunk.length);
        callback(null, chunk);
    },
});

const sendFile = (socket: Duplex, filename: string) => {
    socket.on("end", () => console.log("socket ended"));
    socket.on("close", () => console.log("socket closed"));

    const input = createReadStream(filename);
    input.pipe(progress).pipe(socket);
};

const recvFile = (socket: Duplex, filename: string) => {
    socket.on("end", () => {
        console.log("socket ended");
        socket.end();
    });
    socket.on("close", () => console.log("socket closed"));

    const output = createWriteStream(filename);
    socket.pipe(progress).pipe(output);
};

const arg = (index: number) => process.argv[index];
const argIndex = (arg: string) => process.argv.indexOf(arg);

async function main() {
    console.log(`
TCP test using ad-hoc network.

usage: <cmd> [options]

available options:
    help                    // prints this help and exits
    client <server ip>      // starts a client, if unspecified starts a server
    port <port>             // specify a port, otherwise 5555
    swap                    // if specified: server receives, client sends
                            // default: server sends, client receives
    filename <filename>     // path to file that should be sent or written to
                            // default: send -> "./send", receive -> "./received"
    network <nwid> [ipv4]   // specify the network id, otherwise adhoc network


example usage: server-client pair on adhoc network, server sends "./send" and client receives into "./received"
<cmd>                       // starts server, wait until it outputs ip
<cmd> client <server ip>    // starts accompaning client
    `);

    if (process.argv.indexOf("help") >= 0) return;

    const server = argIndex("client") < 0;
    const serverIp = server ? "" : arg(argIndex("client") + 1);

    const port = argIndex("port") < 0 ? 5555 : parseInt(arg(argIndex("port") + 1));

    const send = argIndex("swap") < 0 ? server : !server;
    const handleSocket = send ? sendFile : recvFile;
    const filename = argIndex("filename") < 0 ? (send ? "./send" : "./received")
        : arg(argIndex("filename") + 1);

    if (send && (!existsSync(filename))) {
        console.log(`File "${filename}" not found, unable to send it.`);
        return;
    }

    const nwid = argIndex("network") < 0 ? "ff0000ffff000000"
        : arg(argIndex("network") + 1);
    const ipv6 = argIndex("network") < 0 ? true : (argIndex("ipv4") < 0);


    // setting up node

    console.log("starting node");
    startNode(`./id/${server ? "server" : "client"}`, (event) => {
        console.log(`       e: ${event}, ${events[event].replace("ZTS_EVENT_", "").toLowerCase()}`);
    });

    console.log("waiting for node to come online");
    while (!zts.node_is_online()) {
        await setTimeout(50);
    }

    console.log(zts.node_get_id());


    zts.net_join(nwid);

    while (!zts.net_transport_is_ready(nwid)) {
        await setTimeout(50);
    }

    try {
        console.log(`Node ${ipv6 ? "ipv6" : "ipv4"} address: ${zts.addr_get_str(nwid, ipv6)}`);
    } catch (error) {
        console.log(`Couldn't get ip address with ipv6-mode: ${ipv6}`);
        return;
    }


    // const total = 100_000_000;// 1_000_000_000;
    if (server) {
        const server = new net.Server({}, socket => {
            console.log("connection");
            handleSocket(socket, filename);
        });

        server.listen(port, undefined, () => {
            console.log("listening");
            console.log(server.address());
        });
    } else {
        await setTimeout(1000);
        const socket = net.connect(port, serverIp, () => {
            console.log("connected");

            handleSocket(socket, filename);
        });

        socket.on("error", error => {
            console.log(error); 
            if("code" in error) {
                console.log(`code means: ${SocketErrors[error.code as number]}`);
            }
        });
    }
}

main();
