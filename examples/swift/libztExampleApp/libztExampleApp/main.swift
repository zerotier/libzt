//
//  main.swift
//  libztExampleApp
//
//  Created by Joseph Henry on 10/19/17.
//  Copyright © 2017 ZeroTier, Inc. All rights reserved.
//

print("starting libzt...")
zts_startjoin("xcode_libzt_path", "XXXXXXXXXXXXXXXX")
print("libzt is online.")

// create address structure
var addr_str = "0.0.0.0"
var port = 8080
var in4 = sockaddr_in(sin_len: UInt8(MemoryLayout<sockaddr_in>.size),
                      sin_family: UInt8(AF_INET),
                      sin_port: UInt16(port).bigEndian,
                      sin_addr: in_addr(s_addr: 0),
                      sin_zero: (0,0,0,0,0,0,0,0))
inet_pton(AF_INET, addr_str, &(in4.sin_addr));

// socket()
var fd = zts_socket(2, 1, 0)

// bind()
var addrlen = socklen_t(MemoryLayout.size(ofValue: in4))
let a = withUnsafeMutablePointer(to: &in4) {
    $0.withMemoryRebound(to: sockaddr.self, capacity: 1) {
        zts_bind(fd, $0, addrlen)
    }
}

// listen()
zts_listen(fd, 1)

// accept
var clientAddress: sockaddr_in?

addrlen = socklen_t(MemoryLayout.size(ofValue: clientAddress))
let b = withUnsafeMutablePointer(to: &clientAddress) {
    $0.withMemoryRebound(to: sockaddr.self, capacity: 1) {
        zts_accept(fd, UnsafeMutablePointer<sockaddr>($0), UnsafeMutablePointer<socklen_t>(&addrlen))
    }
}

