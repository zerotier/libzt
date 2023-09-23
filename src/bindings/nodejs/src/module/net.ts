import { EventEmitter } from "node:events";
import { nativeSocket, zts } from "./zts";
import { Duplex } from "node:stream";

export class Server extends EventEmitter {
    private listening = false;
    private internal;


    constructor(options: Record<string, never>, connectionListener?: (socket: Socket) => void) {
        super();
        if (connectionListener) this.on("connection", connectionListener);

        this.internal = new zts.Server((error, socket) => {
            const s = new Socket({}, socket);

            process.nextTick(() => this.emit("connection", s));
        });
    }


    listen(port: number, host = "::", callback?: () => void) {
        if (callback) this.on("listening", callback);

        this.internal.listen(port, host, (error) => {
            if (error) console.log(error);

            this.emit("listening");
        });
    }

    address() {
        return this.internal.address();
    }
}

/**
 * 
 */


class Socket extends Duplex {
    private internal: nativeSocket;
    private reading = false;

    constructor(options: { allowHalfOpen?: boolean }, internal?: nativeSocket) {
        super({
            ...options
        });

        this.internal = internal ?? new zts.Socket();
        this.internal.init((data) => {
            this.push(data);
        });
    }

    _read() {
        //
    }
}

