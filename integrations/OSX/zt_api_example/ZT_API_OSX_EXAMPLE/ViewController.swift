//
//  ViewController.swift
//  ZT_API_OSX_EXAMPLE
//
//  Created by Joseph Henry on 5/16/16.
//  Copyright © 2016 ZeroTier Inc. All rights reserved.
//

import Cocoa

class ViewController: NSViewController {

    
    func test_network_calls()
    {
        
        var status: Int32 = 0
        let port: UInt16 = 1337
        
            print("sleeping...")
            sleep(15)
            // set_intercept_status(111);
            
            let sd = zt_socket(AF_INET, SOCK_STREAM, 0)

            /*
             let addr = inet_addr("10.242.211.245")
             var server_addr = sockaddr_in()
             server_addr.sa_family = sa_family_t(AF_INET)
             server_addr.sin_port = 1337
             server_addr.sin_addr = addr
             */
            // var addr: UnsafePointer<sockaddr> = UnsafePointer<sockaddr>()
        
            var addr = sockaddr_in(sin_len: UInt8(sizeof(sockaddr_in)),
                                   sin_family: UInt8(AF_INET),
                                   sin_port: port.bigEndian,
                                   sin_addr: in_addr(s_addr: 0),
                                   sin_zero: (0,0,0,0,0,0,0,0))
            
            inet_pton(AF_INET, "10.242.211.245", &(addr.sin_addr));
            
            let connect_fd = zt_connect(sd, UnsafePointer<sockaddr>([addr]), UInt32(addr.sin_len))
            print("connectResult = \(connect_fd),\(errno)")
            
            if connect_fd < 0 {
                let err = errno
                print("Error connecting IPv4 socket \(err)")
                return
            }
            
            /*
             let bindError = zt_bind(sd, UnsafePointer<sockaddr>([addr]), UInt32(addr.sin_len))
             if bindError < 0 {
             let err = errno
             print("Error binding IPv4 socket \(err)")
             return
             }
             */
    }
    
    override func viewDidLoad() {
        super.viewDidLoad()
        test_network_calls()
        // Do any additional setup after loading the view.
    }

    override var representedObject: AnyObject? {
        didSet {
        // Update the view, if already loaded.
        }
    }


}

