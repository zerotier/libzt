import { zts } from "./zts";

export function init(path: string) {
    zts.init_from_storage(path);
    zts.init_set_event_handler(ev => console.log(ev));

    zts.node_start();
}


export * from "./zts";
export * from "./Socket";
export * from "./Server";