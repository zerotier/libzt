import { Socket } from "./Socket";
import zts from "./zts"

import * as net from "net"

export function init(path: string) {
    zts.init_from_storage(path);
    zts.init_set_event_handler(ev=>console.log(ev));

    zts.node_start();
}
