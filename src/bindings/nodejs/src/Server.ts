import { EventEmitter } from "events";
import zts from "./zts";
import { Socket } from "./Socket";

import { isIPv6 } from "net";


export interface ServerEvents {
    "connection": (socket: Socket)=>void
    "listening": ()=>void
    "error": (err: Error)=>void
}

export declare interface Server {
    on<E extends keyof ServerEvents>(event: E, listener: ServerEvents[E]): this
    emit<E extends keyof ServerEvents>(event: E, ...args: Parameters<ServerEvents[E]>): boolean
}

export class Server extends EventEmitter {
    private fd: number;
    private listening = false;

    constructor(options: Record<string, never>, connectionListener?: ServerEvents["connection"]) {
        super();
        if(connectionListener) this.on("connection", connectionListener);
    }

    
    listen(port: number, host = "::", callback?: ServerEvents["listening"]) {
        if(callback) this.on("listening", callback);
        
        this.fd = zts.bsd_socket(isIPv6(host), 1, 0);
        console.log(this.fd);
        zts.bind(this.fd, host, port);
        zts.listen(this.fd, 20);

        this.listening = true;
        process.nextTick(()=>this.emit("listening"));

        const accept = () => {
            zts.accept(this.fd, (err, fd) => {
                if(err) {
                    this.emit("error", err);
                    return;
                }
                const s = new Socket(fd);
                process.nextTick(() => this.emit("connection", s));
                if(this.listening) accept();
            });
        };

        accept();
    }
}

