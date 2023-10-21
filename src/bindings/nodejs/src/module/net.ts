import { EventEmitter } from "node:events";
import { nativeSocket, zts } from "./zts";
import { Duplex, PassThrough } from "node:stream";


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
    private internalEvents = new EventEmitter();
    private internal: nativeSocket;

    private connected = false;

    bytesRead = 0;
    bytesWritten = 0;

    // internal socket writes to receiver, receiver writes to duplex (which is read by client application)
    private receiver = new PassThrough();

    constructor(options: { allowHalfOpen?: boolean }, internal?: nativeSocket) {
        super({
            ...options
        });

        if (internal) this.connected = true;
        this.internal = internal ?? new zts.Socket();

        // events from native socket
        this.internalEvents.on("data", (data) => {
            if (data) {
                console.log(`received ${data.length}`);

                this.bytesRead += data.length;
                this.receiver.write(data, undefined, () => {
                    console.log(`acking ${data.length}`);
                    if (data) this.internal.ack(data.length);
                });
            } else {
                this.receiver.end();
            }
        });
        this.internalEvents.on("sent", (length) => {
            console.log(`internal has sent ${length}`);
            this.bytesWritten += length;
        });
        this.internalEvents.on("error", (error) => {
            console.log(error);
        });
        this.internal.init((event: string, ...args: unknown[]) => this.internalEvents.emit(event, ...args));

        // setup passthrough for receiving data
        this.receiver.on("data", chunk => {
            if (!this.push(chunk)) this.receiver.pause();
        });
        this.receiver.on("end", ()=>this.push(null));
        this.receiver.pause();
    }

    _construct(callback: (error?: Error | null | undefined) => void): void {
        if (this.connected) {
            callback();
            return;
        } else {
            this.internalEvents.once("connect", () => {
                this.connected = true;
                callback();
            });
        }
    }

    _read() {
        this.receiver.resume();
    }

    _write(chunk: Buffer, _: unknown, callback: (error?: Error | null) => void): void {
        this.realWrite(chunk, callback);
    }

    private realWrite(chunk: Buffer, callback: (error?: Error | null) => void): void {
        const currentWritten = this.bytesWritten;
        this.internal.send(chunk, (length) => {
            console.log(`written ${length}`);
            // everything was written out
            if (length === chunk.length) {
                callback();
                return;
            }

            // not everything was written out
            const continuation = () => {
                this.realWrite(chunk.subarray(length), callback);
            };

            // new space became available in the time it took to sync threads
            if (currentWritten !== this.bytesWritten) continuation();
            // wait for more space to become available
            else this.internalEvents.once("sent", continuation);
        });
    }

    _final(callback: (error?: Error | null | undefined) => void): void {
        console.log("final called");
        this.internal.shutdown_wr();
        callback();
    }

    connect(port: number, address: string) {
        if (this.connected) throw Error("Already connected");
        this.internalEvents.on("connect", () => {
            this.emit("connect");
        });
        this.internal.connect(port, address);
    }

}

export function connect(port: number, address: string, connectionListener?: () => void): Socket {
    const socket = new Socket({});

    if (connectionListener) socket.on("connect", () => connectionListener());
    process.nextTick(() => socket.connect(port, address));

    return socket;
}