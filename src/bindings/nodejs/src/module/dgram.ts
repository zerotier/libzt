import { EventEmitter } from "events";
import { UDPError, zts } from "./zts";
import { isIPv6 } from "net";


interface UDPSocketEvents {
    "close": () => void
    "message": (msg: Buffer, rinfo: { address: string, family: "udp4" | "udp6", port: number, size: number }) => void
    "listening": () => void
    "error": (err: UDPError) => void
}


class Socket extends EventEmitter {

    private ipv6;
    private internal;
    private bound = false;

    constructor(ipv6: boolean) {
        super();
        this.ipv6 = ipv6;
        
        this.internal = new zts.UDP(ipv6, (data, addr, port) => {
            this.emit("message", data, { address: addr, family: isIPv6(addr) ? "udp6" : "udp4", port, size: data.length });
        });
        // unbound socket should not hold process open
        this.unref();

        this.once("listening", () => {
            this.bound = true;
            this.ref();
        });

    }

    ref() { this.internal.ref(); return this; }

    unref() { this.internal.unref(); return this; }

    address() {
        if(!this.bound) throw Error("Unbound socket");
        return this.internal.address();
    }

    bind(port?: number, address?: string, callback?: UDPSocketEvents["listening"]) {
        // TODO what if already bound?
        if (!address) address = "";
        if (!port) port = 0;

        if(callback) this.once("listening", ()=>callback());

        this.internal.bind(address, port, (err) => {
            if (err) this.emit("error", err);
            else this.emit("listening");
        });
    }

    send(msg: Buffer, port?: number, address?: string, callback?: (err?: UDPError) => void) {
        if (!port) return this.handleError(callback)(Error("Connect not implemented, port must be specified"));
        if (!address) address = this.ipv6 ? "::1" : "127.0.0.1";

        const cb = this.bound ? this.handleError(callback) : (err?: UDPError) => {
            if (!err) this.emit("listening");

            this.handleError(callback)(err);
        };
        this.internal.send_to(msg, address, port, cb);
    }

    close(callback?: () => void) {
        if (callback) this.on("close", callback);
        this.internal.close(() => this.emit("close"));
    }

    private handleError(callback?: (err?: UDPError) => void) {
        return (err?: Error) => {
            if (callback) callback(err);
            else {
                if (!err) return;

                if (!this.emit("error", err)) {
                    throw (err);
                }
            }
        };
    }
}

export function createSocket(options: { type: "udp4" | "udp6" }, callback?: UDPSocketEvents["message"]) {
    const ipv6 = options.type === "udp6";
    const s = new Socket(ipv6);
    if (callback) s.on("message", callback);

    return s;
}

