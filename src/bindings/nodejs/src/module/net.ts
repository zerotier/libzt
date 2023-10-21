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
    private internal: nativeSocket;

    bytesRead = 0;
    bytesWritten = 0;
    private receiver = new PassThrough();

    constructor(options: { allowHalfOpen?: boolean }, internal?: nativeSocket) {
        super({
            ...options
        });

        this.internal = internal ?? new zts.Socket();

        this.receiver.on("data", chunk => {
            if (!this.push(chunk)) this.receiver.pause();
        });
        this.receiver.pause();
    }

    _construct(callback: (error?: Error | null | undefined) => void): void {
        this.internal.init((data) => {
            if (data) {
                this.bytesRead += data.length;
                this.receiver.write(data, undefined, () => {
                    if (data) this.internal.ack(data.length);
                });
            } else {
                this.receiver.end();
            }
        }, (length) => {
            this.bytesWritten += length;
            this.emit("wroteData");
        });

        callback();
    }

    _read() {
        this.receiver.resume();
    }

    _write(chunk: Buffer, _: unknown, callback: (error?: Error | null) => void): void {
        const currentWritten = this.bytesWritten;

        this.internal.send(chunk, (length) => {
            // everything was written out
            if (length === chunk.length) {
                callback();
                return;
            }

            // not everything was written out
            const continuation = () => { this._write(chunk.subarray(length), undefined, callback); };

            // new space became available in the time it took to sync threads
            if (currentWritten !== this.bytesWritten) continuation();
            // wait for more space to become available
            else this.once("wroteData", continuation);
        });
    }

    _final(callback: (error?: Error | null | undefined) => void): void {
        this.internal.shutdown_wr();
        callback();
    }

}

