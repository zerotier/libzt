import { EventEmitter } from "events";
import { zts } from "./zts";

export class Server extends EventEmitter {
    private listening = false;
    private internal;


    constructor(options: Record<string, never>, connectionListener?: () => void) {
        super();
        if (connectionListener) this.on("connection", connectionListener);

        this.internal = new zts.Server((error, socket) => {
            console.log(error);
            console.log(socket);

            // todo add listeners to socket, emit
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

