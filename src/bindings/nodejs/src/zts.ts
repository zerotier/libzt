interface ZTS {
    init_from_storage(path: string): void
    init_set_event_handler(callback: (event: number)=>void ): void

    node_start(): void
    node_is_online(): boolean
    node_get_id(): bigint
    node_stop(): void
    node_free(): void

    net_join(nwid: bigint): void
    net_leave(nwid: bigint): void
    net_transport_is_ready(nwid: bigint): boolean

    addr_get_str(nwid: bigint, ipv6: boolean): string

    bsd_socket(ipv6: boolean, type: number, protocol: number): number
    bsd_close(fd: number): void

    bsd_send(fd: number, data: Uint8Array, flags: number, callback: (err: Error | null, bytesWritten: number)=>void): void;
    bsd_recv(fd: number, len: number, flags: number, callback: (err: Error | null, data: Buffer) => void): void

    bind(fd: number, ipAddr: string, port: number): void
    listen(fd: number, backlog: number): void
    accept(fd: number, callback: (err: Error | null, fd: number)=>void): void
    connect(fd: number, ipAddr: string, port: number, timeout: number, callback: (err: Error | null) => void): void
    shutdown_wr(fd: number): void

    util_delay(ms: number): void
}

// eslint-disable-next-line @typescript-eslint/no-var-requires
const zts = require("../build/Release/zts") as ZTS;

export default zts;
