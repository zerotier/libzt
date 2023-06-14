import { EventEmitter } from "events";
import { ZtsError, zts } from "./zts";
import { ZTS_AF_INET, ZTS_AF_INET6, ZTS_SOCK_DGRAM } from "./defs";
import { isIPv6 } from "net";


interface UDPSocketEvents {
    "close": () => void
    "message": (msg: Buffer, rinfo: { address: string, family: "udp4" | "udp6", port: number, size: number }) => void
    "listening": () => void
    "error": (err: Error) => void
}

declare interface UDPSocket {
    on<E extends keyof UDPSocketEvents>(event: E, listener: UDPSocketEvents[E]): this
    once<E extends keyof UDPSocketEvents>(event: E, listener: UDPSocketEvents[E]): this
    emit<E extends keyof UDPSocketEvents>(event: E, ...args: Parameters<UDPSocketEvents[E]>): boolean
}

class UDPSocket extends EventEmitter {
    private fd: number;
    private ipv6;
    private bound = false;
    private connection: { connected: false } | { connected: true, address: string, port: number } = { connected: false };

    constructor(ipv6: boolean) {
        super();
        this.ipv6 = ipv6;
    }

    address(): { address: string, port: number, family: "udp4" | "udp6" } {
        if (!this.bound) throw (Error("Socket not connected"));

        const sname = zts.getsockname(this.fd);
        return { address: sname.address, port: sname.port, family: this.ipv6 ? "udp6" : "udp4" };
    }

    bind(port?: number, address?: string, callback?: UDPSocketEvents["listening"]) {
        if (this.bound) throw Error("UDPSocket already bound");
        if (callback) this.once("listening", callback);

        if (!address) address = this.ipv6 ? "::0" : "0.0.0.0";
        if (!port) port = 0;

        this.fd = zts.bsd_socket(this.ipv6 ? ZTS_AF_INET6 : ZTS_AF_INET, ZTS_SOCK_DGRAM, 0);
        zts.bind(this.fd, address, port);
        this.bound = true;

        process.nextTick(() => this.emit("listening"));
        this._recv();
    }

    private _recv() {
        zts.bsd_recvfrom(this.fd, 16384, 0, (err, data, address, port) => {
            if (err) return this._err(err);

            this._recv();
            this.emit("message", data, { address, family: isIPv6(address) ? "udp6" : "udp4", port, size: data.length });
        });
    }

    private _err(err: Error) {
        if (!this.emit("error", err)) {
            throw (err);
        }
    }

    send(msg: Buffer, port?: number, address?: string, callback?: (err?: ZtsError) => void) {

        const handleError = (err: Error) => {
            if (callback) callback(err);
            else this._err(err);
        };

        if (!this.bound) this.bind();

        if (this.connection.connected) {
            address = this.connection.address;
            port = this.connection.port;
        } else {
            if (!port) return handleError(Error("Port not specified on send of unconnected UDP Socket"));
            if (!address) address = this.ipv6 ? "::1" : "127.0.0.1";
        }

        zts.bsd_sendto(this.fd, msg, 0, address, port, (err) => {
            if (err) return handleError(err);

            if (callback) callback();
        });

    }
}

export function createSocket(options: { type: "udp4" | "udp6" }, callback?: UDPSocketEvents["message"]) {
    const ipv6 = options.type === "udp6";
    const s = new UDPSocket(ipv6);
    if (callback) s.on("message", callback);

    return s;
}