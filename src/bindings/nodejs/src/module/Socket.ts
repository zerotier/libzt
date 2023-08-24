import { ZtsError, defs, errnos, errors, zts } from "./zts";
import { Duplex } from "stream";

import { isIPv6 } from "net";

export interface SocketEvents {
    "connect": () => void
    "timeout": () => void
    "error": (err: ZtsError) => void
    "data": (data: Buffer) => void
}

export declare interface Socket {
    on<E extends keyof SocketEvents>(event: E, listener: SocketEvents[E]): this
    once<E extends keyof SocketEvents>(event: E, listener: SocketEvents[E]): this
    emit<E extends keyof SocketEvents>(event: E, ...args: Parameters<SocketEvents[E]>): boolean
}

export class Socket extends Duplex {
    private fd: number;
    private reading = false;

    localAddress?: string;
    localPort?: number;

    remoteAddress?: string;
    remotePort?: number;

    timeout?: number;

    constructor(fd: number) {
        super({
            decodeStrings: true
        });
        this.fd = fd;

        this.once("connect", () => {
            const sockname = zts.getsockname(this.fd);
            this.localAddress = sockname.address;
            this.localPort = sockname.port;

            const peername = zts.getpeername(this.fd);
            this.remoteAddress = peername.address;
            this.remotePort = peername.port;
        });
    }

    _write(chunk: unknown, encoding: BufferEncoding, callback: (error?: Error) => void): void {
        if (!(chunk instanceof Buffer)) {
            callback(Error("Why was this not a buffer?"));
            return;
        }

        zts.bsd_send(this.fd, chunk, 0, (err) => {
            // if(err) {
            //     if(this.timeout && err.code === errors.ZTS_ERR_SOCKET && err.errno === errnos.ZTS_EAGAIN) {
            //         process.nextTick(()=>this.emit("timeout"));
            //         callback();
            //         return;
            //     }
            // }
            callback(err);
        });
    }

    _final(callback: (error?: Error) => void): void {
        zts.shutdown_wr(this.fd);
        callback();
    }

    _read(size: number): void {
        if (this.reading) return;
        this.reading = true;

        zts.bsd_recv(this.fd, size, 0, (err, data) => {
            this.reading = false;

            if (err) {
                if (this.timeout && err.code === errors.ZTS_ERR_SOCKET && err.errno === errnos.ZTS_EAGAIN) {
                    process.nextTick(() => this.emit("timeout"));
                    return;
                }

                this.destroy(err);
                return;
            }

            if (data.length === 0) {
                this.push(null);
                return;
            }

            const res = this.push(data);
            if (res) this._read(size);
        });
    }

    _destroy(error: Error, callback: (error: Error) => void): void {
        zts.bsd_close(this.fd);
        callback(error);
    }

    address() {
        try {
            return zts.getsockname(this.fd);
        } catch (error) {
            return {};
        }
    }

    setTimeout(timeout: number, callback?: () => void) {
        if (callback) this.once("timeout", callback);
        this.timeout = timeout;

        zts.set_recv_timeout(this.fd, Math.floor(timeout / 1000), (timeout % 1000) * 1000);
        zts.set_send_timeout(this.fd, Math.floor(timeout / 1000), (timeout % 1000) * 1000);
    }
}

export function connect(host: string, port: number): Socket {
    const fd = zts.bsd_socket(isIPv6(host) ? defs.ZTS_AF_INET6 : defs.ZTS_AF_INET, defs.ZTS_SOCK_STREAM, 0);
    
    const s = new Socket(fd);

    zts.connect(fd, host, port, 0, err => {
        if (err) {
            s.destroy(err);
            return;
        }
        s.emit("connect");
    });

    return s;
}