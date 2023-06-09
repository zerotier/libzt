import zts from "./zts";
import { Duplex } from "stream";

import { isIPv6 } from "net";

export class Socket extends Duplex {
    private fd: number;
    private reading = false;
    
    constructor(fd: number) {
        super({
            decodeStrings: true
        });
        this.fd = fd;
    }

    _write(chunk: unknown, encoding: BufferEncoding, callback: (error?: Error) => void): void {
        if(! (chunk instanceof Buffer) ) {
            callback(Error("Why was this not a buffer?"));
            return;
        }

        zts.bsd_send(this.fd, chunk, 0, callback);
    }

    _final(callback: (error?: Error) => void): void {
        zts.shutdown_wr(this.fd);
        callback();
    }

    _read(size: number): void {
        if(this.reading) return;
        this.reading = true;

        zts.bsd_recv(this.fd, size, 0, (err, data) => {
            this.reading=false;

            if(err) {
                console.log(err);
                this.destroy(err);
                return;
            }

            if(data.length === 0) {
                this.push(null);
                return;
            }

            const res = this.push(data);
            if(res) this._read(size);
        });
    }

    _destroy(error: Error, callback: (error: Error) => void): void {
        zts.bsd_close(this.fd);
        callback(error);
    }
}

export function connect(host: string, port: number): Socket {
    const fd = zts.bsd_socket(isIPv6(host), 1, 0);
    console.log(fd);
    const s = new Socket(fd);

    zts.connect(fd, host, port, 0, err => {
        if(err) {
            s.destroy(err);
            return;
        }
        s.emit("connect");
    });

    return s;
}