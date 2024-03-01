import { setTimeout } from "timers/promises";

import { net, startNode, zts } from "../index";
import * as util from "node:util";
import assert = require("node:assert");
import { fork } from "node:child_process";

import * as streamConsumers from "node:stream/consumers";

async function test() {
    
    const payload = "abcdefgh";

    const server = process.argv.length < 3;
    if(server) console.log(`
This test starts a server, opens a client in a child process that connects to the server and tries to verify that cleanup of threads/exiting happens properly.
(Not the case right now.)
    `);


    const log = (...args: unknown[])=>{
        process.stdout.write(server ? "S:  " : " C: ");
        console.log(...args);
    };

    log(`Process started with args: ${process.argv}`);

    const port = 5000;
    
    startNode(`./id/adhoc-test/${server ? "server" : "client"}`, event=>log(`zts event ${event}`));

    while (!zts.node_is_online()) {
        await setTimeout(50);
    }

    log("Node online");

    const nwid = "ff0000ffff000000";
    zts.net_join(nwid);

    while (!zts.net_transport_is_ready(nwid)) {
        await setTimeout(50);
    }
    log("Network joined");


    assert.doesNotThrow(()=>zts.addr_get_str(nwid, true));
    const address = zts.addr_get_str(nwid, true);

    log(`Address: ${address}`);

    if (server) {
        log("Starting server");

        const server = new net.Server({}, socket => {
            log("Connected");
            socket.once("data", data => {
                log(`Received data: ${data}`);
                socket.end(data);
            });
            socket.on("error", () => console.log("error"));
        });
        server.listen(port, "::", () => {
            log(`Listening, address: ${util.format(server.address())}`);
        });
        
        server.on("error", (err)=>assert(false, `Server errored: ${util.format(err)}`));


        const child = fork(__filename, [address]);
        child.on("exit", async code=>{
            log(`Client exited with code ${code}`);
            assert.strictEqual(code, 0);
            await setTimeout(500);
            // server.close();
            log("closed server");
            zts.node_free();
            log("freed node");
            
            
        });

    } else {
        const address = process.argv[2];
        log(`Connecting to: ${address}`);
        const s = net.connect(port, address, () => {
            log("Connected");
            s.end(Buffer.from(payload));
        });

        const result = await streamConsumers.text(s);

        assert.strictEqual(result, payload);

        s.destroy();
        zts.node_free();
        log("Exiting");
    }
}

test();
