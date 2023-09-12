import { EventEmitter } from "events";
import { ZtsError, zts } from "./zts";
import { isIPv6 } from "net";


interface UDPSocketEvents {
    "close": () => void
    "message": (msg: Buffer, rinfo: { address: string, family: "udp4" | "udp6", port: number, size: number }) => void
    "listening": () => void
    "error": (err: Error) => void
}



class Socket extends EventEmitter {

    private ipv6;
    private internal;

    constructor(ipv6: boolean) {
        super();
        this.ipv6 = ipv6;

        this.internal = new zts.UDP(ipv6, (data, addr, port) => {
            this.emit("message", data, { address: addr, family: isIPv6(addr) ? "udp6" : "udp4", port, size: data.length });
        });
    }

    address(): { address: string, port: number, family: "udp4" | "udp6" } {
        return { ...this.internal.address(), family: this.ipv6 ? "udp6" : "udp4" };
    }

    bind(port?: number, address?: string, callback?: UDPSocketEvents["listening"]) {
        if (!address) address = this.ipv6 ? "::0" : "0.0.0.0";
        if (!port) port = 0;

        this.internal.bind(address, port);
        void callback; // TODO
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

        if (!port) return handleError(Error("Connect not implemented"));
        if (!address) address = this.ipv6 ? "::1" : "127.0.0.1";

        this.internal.send_to(msg, address, port, (callback??(()=>undefined)));
    }

    close() {
        this.internal.close(()=>undefined);
    }
}

export function createSocket(options: { type: "udp4" | "udp6" }, callback?: UDPSocketEvents["message"]) {
    const ipv6 = options.type === "udp6";
    const s = new Socket(ipv6);
    if (callback) s.on("message", callback);

    return s;
}

