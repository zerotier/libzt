import zts from "./zts"
import { Duplex } from "stream";



class Socket extends Duplex {
    fd: number
    
    constructor(host: string, port: number) {
        super({
            decodeStrings: true
        });

        this.fd = zts.bsd_socket(2, 1, 0);
    }

    _write(chunk: unknown, encoding: BufferEncoding, callback: (error?: Error) => void): void {
        if(! (chunk instanceof Buffer) ) {
            callback(Error("Why was this not a buffer?"));
            return;
        }
        let written = 0;

        try {
            while (written < chunk.length) {
                written += zts.bsd_send(this.fd, chunk.subarray(written), 0);
            }
        } catch (error) {
            callback(error)
        }
        callback()
    }

    _read(size: number): void {
        zts.bsd_recv_cb(this.fd, size, 0, data => {
            if(data.length === 0) {
                this.push(null);
                return;
            }

            let res = this.push(data);
            if(res) this._read(size);
        })
    }

    _destroy(error: Error, callback: (error: Error) => void): void {
        zts.bsd_close(this.fd);
        callback(error);
    }
}
