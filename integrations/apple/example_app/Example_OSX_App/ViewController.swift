//
//  ViewController.swift
//  Example_OSX_App
//
//  Created by Joseph Henry on 6/22/16.
//  Copyright © 2016 ZeroTier Inc. All rights reserved.
//

import Cocoa

class ViewController: NSViewController {
    
    @IBOutlet weak var btnJoinNetwork: NSButton!
    @IBOutlet weak var btnLeaveNetwork: NSButton!
    @IBOutlet weak var txtNWID: NSTextField!
    @IBOutlet weak var segmentAPISelector: NSSegmentedCell!
    @IBOutlet weak var segmentProtocolSelector: NSSegmentedCell!
    @IBOutlet weak var txtAddr: NSTextField!
    @IBOutlet weak var txtPort: NSTextField!
    @IBOutlet weak var btnConnect: NSButton!
    @IBOutlet weak var btnBind: NSButton!
    @IBOutlet weak var txtTX: NSScrollView!
    @IBOutlet weak var txtRX: NSScrollView!
    @IBOutlet weak var btnSend: NSButton!
    
    var serverPort:Int32 = 4545
    var serverAddr:String = "10.147.18.5"
    
    @IBAction func txtAddrChanged(sender: AnyObject) {
        if(sender.value != nil) {
            serverAddr = sender.value
        }
    }
    
    @IBAction func txtPortChanged(sender: AnyObject) {
        serverPort = sender.intValue!
    }
    
    // Join a ZeroTier network
    @IBAction func UI_JoinNetwork(sender: AnyObject) {
        zt_join_network(txtNWID.stringValue);
    }
    
    // Leave a ZeroTier network
    @IBAction func UI_LeaveNetwork(sender: AnyObject) {
        zt_leave_network(txtNWID.stringValue);
    }
    
    // Select an API
    var selectedShim:Int32 = 0
    @IBAction func UI_SelectAPI(sender: AnyObject) {
        selectedShim = sender.intValue // 0 = BSD-style, 1 = SOCKS5 Proxy, etc
    }
    
    // Select a protocol
    // Protocol { TCP / UDP }
    var selectedProtocol:Int32 = 0
    @IBAction func UI_SelectProtocol(sender: AnyObject) {
        switch sender.intValue
        {
        case 0:
            print("Selected TCP (SOCK_STREAM)\n");
            selectedProtocol = SOCK_STREAM
        case 1:
            print("Selected UDP (SOCK_DGRAM)\n");
            selectedProtocol = SOCK_DGRAM
        default:
            break;
        }    }
    
    // Connect to remote host on ZeroTier virtual network
    @IBAction func UI_Connect(sender: AnyObject) {
        // TCP
        if(selectedProtocol == SOCK_STREAM)
        {
            let sd = zts_socket(AF_INET, SOCK_STREAM, 0)
            var addr = sockaddr_in(sin_len: UInt8(sizeof(sockaddr_in)),
                                   sin_family: UInt8(AF_INET),
                                   sin_port: UInt16(serverPort).bigEndian,
                                   sin_addr: in_addr(s_addr: 0),
                                   sin_zero: (0,0,0,0,0,0,0,0))
            
            inet_pton(AF_INET, serverAddr, &(addr.sin_addr));
            
            let connect_fd = zts_connect(sd, UnsafePointer<sockaddr>([addr]), UInt32(addr.sin_len))
            print("connect_fd = \(connect_fd),\(errno)")
            
            if connect_fd < 0 {
                let err = errno
                print("Error connecting IPv4 socket \(err)")
                return
            }
        }
        
        // UDP
        if(selectedProtocol == SOCK_DGRAM)
        {
            
        }
    }
    
    @IBAction func UI_Bind(sender: AnyObject) {
    }
    
    @IBAction func UI_SendData(sender: AnyObject) {
        
    }
    
    /*
    // Mode { Client / Server }
    @IBOutlet weak var ModeControl: UISegmentedControl!
    var selectedMode:UInt16 = 0
    @IBAction func ModeControlSelected(sender: AnyObject) {
        switch sender.selectedSegmentIndex
        {
        case 0:
            print("Selected client\n");
            selectedMode = 0
        case 1:
            print("Selected server\n");
            selectedMode = 1
        default:
            break;
        }
    }

    */
    
    
    var service_thread : NSThread!
    func ztnc_start_service() {
        let path = NSSearchPathForDirectoriesInDomains(NSSearchPathDirectory.DocumentDirectory, NSSearchPathDomainMask.UserDomainMask, true)
        print("start_service()\n")
        // e5cd7a9e1c3511dd
        start_service("/Users/Joseph/utest3")
        //start_service(path[0])
    }
    
    
    override func viewDidLoad() {
        super.viewDidLoad()

        // ZeroTier Service thread
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), {
            self.service_thread = NSThread(target:self, selector:"ztnc_start_service", object:nil)
            self.service_thread.start()
        });

        // Set RPC path for this thread
        zts_init_rpc("/Users/Joseph/utest3/nc_","e5cd7a9e1c2e194f");
    }

    override var representedObject: AnyObject? {
        didSet {
        // Update the view, if already loaded.
        }
    }


}

