import { zts } from "./module/zts";

export function init(path: string) {
    zts.init_from_storage(path);
    zts.init_set_event_handler(ev => console.log(ev));

    zts.node_start();
}


export * from "./module/zts";
export * from "./module/Socket";
export * from "./module/Server";
export * as udp from "./module/UDPSocket";