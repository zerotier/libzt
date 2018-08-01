//
//  Example.swift
//  ExampleSwiftApp
//
//  Created by Joseph Henry on 8/1/18.
//  Copyright © 2018 ZeroTier. All rights reserved.
//

import Foundation
import UIKit

class MyApplication: UIResponder, UIApplicationDelegate {
    static func libzt_example_function()
    {
        var fd: Int32 = -1;
        var err: Int32 = -1;
        let clientMode = true;
        let remotePort = 4545;
        let remoteAddress = "192.168.195.1";
        let appDir = String(NSSearchPathForDirectoriesInDomains(.libraryDirectory, .userDomainMask, true)[0])
        
        // Start up and create socket
        print("starting...");
        zts_startjoin(appDir, 0x1c33c1ceb0aa9251);
        print("I am ", NSString(format:"%llx", zts_get_node_id()));
        fd = zts_socket(2, 1, 0);
        if(fd < 0) {
            print("error creating socket");
        }
        print("fd = ", fd);
        
        // Remote address
        var in4 = sockaddr_in(sin_len: UInt8(MemoryLayout<sockaddr_in>.size),
                              sin_family: UInt8(AF_INET),
                              sin_port: UInt16(remotePort).bigEndian,
                              sin_addr: in_addr(s_addr: 0),
                              sin_zero: (0,0,0,0,0,0,0,0))
        inet_pton(AF_INET, remoteAddress, &(in4.sin_addr));
        
        // CLIENT
        if (clientMode)
        {
            print("connecting...");
            let addrlen = socklen_t(MemoryLayout.size(ofValue: in4))
            let a = withUnsafeMutablePointer(to: &in4) {
                $0.withMemoryRebound(to: sockaddr.self, capacity: 1) {
                    err = zts_connect(fd, $0, addrlen)
                }
            }
            if(err < 0) {
                print("error connecting to remote server");
            }
            print("connected");
            
            print("sending message to server");
            var msg: String = "Hello from Swift!";
            err = zts_write(fd, msg, msg.count)
            if(err < 0) {
                print("error creating socket");
            }
            print("wrote ", err, " bytes");
            zts_close(fd);
        }
        else // SERVER
        {
            
        }
        
        zts_stop();
    }
}

 
