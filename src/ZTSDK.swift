//
//  ZTSDK.swift
//  Example_iOS_App
//
//  Created by Joseph Henry on 9/7/16.
//  Copyright © 2016 ZeroTier Inc. All rights reserved.
//

import Foundation

// Convenience structure for getting address data to/from the native library
struct ZTAddress
{
    var family: Int32
    var addr: String
    var port: Int16
    var data: sockaddr_in?
    
    init(_ family: Int32, _ addr: String, _ port: Int16) {
        self.family = family
        self.addr = addr
        self.port = port
    }
    
    func to_sockaddr_in() -> UnsafePointer<sockaddr> {
        var data = sockaddr_in(sin_len: UInt8(sizeof(sockaddr_in)),
                                    sin_family: UInt8(AF_INET),
                                    sin_port: UInt16(port).bigEndian,
                                    sin_addr: in_addr(s_addr: 0),
                                    sin_zero: (0,0,0,0,0,0,0,0))
        inet_pton(AF_INET, addr, &(data.sin_addr));
        return UnsafePointer<sockaddr>([data]);
    }
    
    func len() -> UInt8 {
        return UInt8(sizeof(sockaddr_in))
    }
}


// Convenience wrapper class for ZeroTier/SDK/Proxy controls
// Implemented in terms of SDK_XcodeWrapper.cpp
class ZTSDK : NSObject
{
    var service_thread : NSThread!
    private func ztnc_start_service(path: String?) {
        if(path == nil) {
            zt_start_service(
                NSSearchPathForDirectoriesInDomains(
                    NSSearchPathDirectory.DocumentDirectory,NSSearchPathDomainMask.UserDomainMask,true)[0])
            return;
        }
        zt_start_service(path!)
    }
    
    // Starts the ZeroTier background service
    func start_service(path: String?) {
        let queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0)
        dispatch_async(queue) {
            self.ztnc_start_service(path)
        }
        sleep(2)
        while(service_is_running() == false) { /* waiting for service to start */ }
    }
    
    // Stops the ZeroTier background service
    func stop_service() {
        zt_stop_service();
    }
    
    // Returns whether the ZeroTier background service is running
    func service_is_running() -> Bool {
        return zt_service_is_running();
    }
    
    // Joins a ZeroTier network
    func join_network(nwid: String) {
        zt_join_network(nwid);
    }
    
    // Leaves a ZeroTier network
    func leave_network(nwid: String) {
        zt_leave_network(nwid);
    }
    
    // Returns the IPV4 address of this device on a given ZeroTier network
    func get_ipv4_address(nwid: String) -> String? {
        var str_buf = [Int8](count: 16, repeatedValue: 0)
        zt_get_ipv4_address(nwid,&str_buf);
        return String(str_buf);
    }
    
    // Returns the IPV6 address of this device on a given ZeroTier network
    func get_ipv6_address(nwid: String) -> String? {
        var str_buf = [Int8](count: 16, repeatedValue: 0)
        zt_get_ipv6_address(nwid,&str_buf);
        return String(str_buf);
    }
    
    
    // PROXY SERVER CONTROLS
    //
    /*
    func start_proxy_server(homepath: String, nwid: String, struct sockaddr_storage *addr) {
        zt_start_proxy_server(homepath, nwid, addr);
    }
    //
    func stop_proxy_server(nwid: String) {
        zt_stop_proxy_server(nwid);
    }
    //
    func proxy_is_running(const char *homepath, const char *nwid, struct sockaddr_storage *addr) {
        zt_start_proxy_server(homepath, nwid, addr);
    }
    //
    func get_proxy_server_address(const char *nwid, struct sockaddr_storage *addr) {
        zt_get_proxy_server_address(nwid, addr);
    }
    // Explicit ZT API wrappers
    #if !defined(__IOS__)
    // This isn't available for iOS since function interposition isn't as reliable
    func init_rpc(const char *path, const char *nwid) {
        zt_init_rpc(path, nwid);
    }
    #endif
    
    */
    
    // SOCKET API
    func socket(socket_family: Int32,  _ socket_type: Int32, _ socket_protocol: Int32) -> Int32 {
        return zt_socket(socket_family, socket_type, socket_protocol);
    }
    
    func connect(fd: Int32, _ addr: ZTAddress, _ nwid: String? = nil) -> Int32 {
        if(nwid == nil) { // no nwid is provided to check for address, try once and fail
            return zt_connect(Int32(fd), addr.to_sockaddr_in(), UInt32(addr.len()));
        }
        while(true) { // politely wait until an address is provided. simulates a blocking call
            if(self.get_ipv4_address(nwid!) != nil) {
                return zt_connect(Int32(fd), addr.to_sockaddr_in(), UInt32(addr.len()));
            }
        }
    }
    func bind(fd: Int32, _ addr: ZTAddress, _ nwid: String? = nil) -> Int32 {
        if(nwid == nil) { // no nwid is provided to check for address, try once and fail
            return zt_bind(Int32(fd), addr.to_sockaddr_in(), UInt32(addr.len()));
        }
        while(true) { // politely wait until an address is provided. simulates a blocking call
            if(self.get_ipv4_address(nwid!) != nil) {
                return zt_bind(Int32(fd), addr.to_sockaddr_in(), UInt32(addr.len()));
            }
        }
    }
    
    func accept(fd: Int32, _ addr: ZTAddress) -> Int32 {
        return zt_accept(Int32(fd), UnsafeMutablePointer<sockaddr>([addr.data]), UnsafeMutablePointer<UInt32>([addr.len]));
    }
    
    func listen(fd: Int32, _ backlog: Int16) -> Int32 {
        return zt_listen(Int32(fd), Int32(backlog));
    }
    func setsockopt(fd: Int32, _ level: Int32, _ optname: Int32, _ optval: UnsafePointer<Void>, _ optlen: Int32) -> Int32 {
        return zt_setsockopt(fd, level, optname, optval, UInt32(optlen));
    }
    
    func getsockopt(fd: Int32, _ level: Int32, _ optname: Int32, _ optval: UnsafeMutablePointer<Void>, _ optlen: UInt32) -> Int32 {
        return zt_getsockopt(fd, level, optname, optval, UnsafeMutablePointer<UInt32>([optlen]));
    }
    
    func close(fd: Int32) -> Int32 {
        return zt_close(fd);
    }
    
    func getsockname(fd: Int32, _ addr: ZTAddress) -> Int32 {
        return zt_getsockname(fd, UnsafeMutablePointer<sockaddr>([addr.data]), UnsafeMutablePointer<UInt32>([addr.len]));
    }
    
    func getpeername(fd: Int32, _ addr: ZTAddress) -> Int32 {
        return zt_getpeername(fd, UnsafeMutablePointer<sockaddr>([addr.data]), UnsafeMutablePointer<UInt32>([addr.len]));
    }

    func fcntl(fd: Int32, _ cmd: Int32, _ flags: Int32) -> Int32 {
        return zt_fcntl(fd, cmd, flags);
    }

    func recvfrom(fd: Int32, _ buf: UnsafeMutablePointer<Void>, _ len: Int32, _ flags: Int32, _ addr: ZTAddress) -> Int32 {
        return zt_recvfrom(fd, buf, Int(len), flags, UnsafeMutablePointer<sockaddr>([addr.data]), UnsafeMutablePointer<UInt32>([addr.len]));
    }
    
    func sendto(fd: Int32, _ buf: UnsafePointer<Void>, _ len: Int32, _ flags: Int32, _ addr: ZTAddress) -> Int32 {
        return zt_sendto(fd, buf, Int(len), flags, addr.to_sockaddr_in(), UInt32(addr.len()));
    }
}
