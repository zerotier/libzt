interface ZTS {
    init_from_storage(path: string): void
    init_set_event_handler(callback: (event: number)=>void ): void

    node_start(): void
    node_is_online(): boolean
    node_get_id(): BigInt
    node_stop(): void
    node_free(): void

    net_join(nwid: BigInt): void
    net_leave(nwid: BigInt): void
    net_transport_is_ready(nwid: BigInt): boolean

    addr_get_str(nwid: BigInt): string

    bsd_socket(family: number, type: number, protocol: number): number
    bsd_close(fd: number): void
    bsd_send(fd: number, data: Uint8Array, flags: number): number
    bsd_recv(fd: number, data: Uint8Array, flags: number): number

    bsd_recv_cb(fd: number, len: number, flags: number, callback: (data: Buffer) => void): void

    bind(fd: number, ipAddr: string, port: number): void
    listen(fd: number, backlog: number): void
    accept(fd: number): {fd: number, address: string, port: number}
    connect(fd: number, ipAddr: string, port: number, timeout: number): void

    util_delay(ms: number): void
}

const zts = require("../build/Release/zts") as ZTS;

export default zts
