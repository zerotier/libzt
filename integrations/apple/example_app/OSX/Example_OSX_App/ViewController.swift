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

    @IBOutlet weak var txtTX: NSTextField!
    @IBOutlet weak var txtRX: NSTextField!
    
    var serverPort:Int32 = 5658
    var serverAddr:String = "10.9.9.203"
    
    var sock:Int32 = -1
    var accepted_sock:Int32 = -1
    
    @IBAction func txtAddrChanged(sender: AnyObject) {
        if(sender.stringValue != nil) {
            serverAddr = sender.stringValue
        }
    }
    
    @IBAction func txtPortChanged(sender: AnyObject) {
        serverPort = sender.intValue
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
            selectedProtocol = SOCK_STREAM
        case 1:
            selectedProtocol = SOCK_DGRAM
        default:
            break;
        }
    }
    
    // Connect to remote host on ZeroTier virtual network
    @IBAction func UI_Connect(sender: AnyObject) {

        // TCP
        if(selectedProtocol == SOCK_STREAM)
        {
            sock = zts_socket(AF_INET, SOCK_STREAM, 0)
            var addr = sockaddr_in(sin_len: UInt8(sizeof(sockaddr_in)),
                                   sin_family: UInt8(AF_INET),
                                   sin_port: UInt16(serverPort).bigEndian,
                                   sin_addr: in_addr(s_addr: 0),
                                   sin_zero: (0,0,0,0,0,0,0,0))
            
            inet_pton(AF_INET, serverAddr, &(addr.sin_addr));
            
            let connect_err = zts_connect(sock, UnsafePointer<sockaddr>([addr]), UInt32(addr.sin_len))
            print("connect_err = \(connect_err),\(errno)")
            
            if connect_err < 0 {
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
    
    // Bind a ZeroTier socket
    @IBAction func UI_Bind(sender: AnyObject) {
        // TCP
        if(selectedProtocol == SOCK_STREAM)
        {
            sock = zts_socket(AF_INET, SOCK_STREAM, 0)
            var addr = sockaddr_in(sin_len: UInt8(sizeof(sockaddr_in)),
                                   sin_family: UInt8(AF_INET),
                                   sin_port: UInt16(serverPort).bigEndian,
                                   sin_addr: in_addr(s_addr: 0),
                                   sin_zero: (0,0,0,0,0,0,0,0))
            
            inet_pton(AF_INET, serverAddr, &(addr.sin_addr));
            
            let bind_err = zts_bind(sock, UnsafePointer<sockaddr>([addr]), UInt32(addr.sin_len))
            
            print("bind_err = \(bind_err),\(errno)")
            
            if bind_err < 0 {
                let err = errno
                print("Error binding IPv4 socket \(err)")
                return
            }
            
            // Put socket into listening state
            zts_listen(sock, 1);
            
            // Accept connection
            var len:socklen_t = 0;
            var legIntPtr = withUnsafeMutablePointer(&len, { $0 })
            while(accepted_sock < 0) {
                accepted_sock = zts_accept(sock, UnsafeMutablePointer<sockaddr>([addr]), legIntPtr)
            }
            print("accepted connection")
        }
        
        // UDP
        if(selectedProtocol == SOCK_DGRAM)
        {
            
        }
    }
    
    // TX
    @IBOutlet weak var btnSend: NSButton!
    @IBAction func UI_SendData(sender: AnyObject) {
        // Use ordinary read/write calls on ZeroTier socket
        
        // TCP
        if(selectedProtocol == SOCK_STREAM)
        {
            write(sock, txtTX.description, 4);
        }
        // UDP
        if(selectedProtocol == SOCK_DGRAM)
        {
            // sendto
        }
    }
    
    // RX
    @IBOutlet weak var btnReadData: NSButton!
    @IBAction func UI_ReadData(sender: AnyObject) {
        // Use ordinary read/write calls on ZeroTier socket
        
        // TCP
        if(selectedProtocol == SOCK_STREAM)
        {
            var buffer = [UInt8](count: 100, repeatedValue: 0)
            let str = "GET / HTTP/1.0\r\n\r\n"
            //let str = "Welcome to the machine"
            print("strlen = %d\n", str.characters.count)
            let encodedDataArray = [UInt8](str.utf8)
            
//            read(accepted_sock, UnsafeMutablePointer<Void>([txtTX.stringValue]), 128);
            read(accepted_sock, &buffer, 100);
            print(buffer)

        }
        // UDP
        if(selectedProtocol == SOCK_DGRAM)
        {
            // recvfrom
        }
    }
    
    func test_client_proxy_nsstream()
    {
        // For HTTP request
        var buffer = [UInt8](count: 100, repeatedValue: 0)
        let str = "GET / HTTP/1.0\r\n\r\n"
        //let str = "Welcome to the machine"
        print("strlen = %d\n", str.characters.count)
        let encodedDataArray = [UInt8](str.utf8)
        
        var inputStream:NSInputStream?
        var outputStream:NSOutputStream?
        
        // As usual, get our streams to our desired "local" address
        NSStream.getStreamsToHostWithName(serverAddr, port: Int(serverPort), inputStream: &inputStream, outputStream: &outputStream)
        
        // SOCKS Proxy config dictionary
        let myDict:NSDictionary = [NSStreamSOCKSProxyHostKey : "0.0.0.0",
                                   NSStreamSOCKSProxyPortKey : 1337,
                                   NSStreamSOCKSProxyVersionKey : NSStreamSOCKSProxyVersion5]
        
        // Give configuration to NSStreams
        inputStream!.setProperty(myDict, forKey: NSStreamSOCKSProxyConfigurationKey)
        outputStream!.setProperty(myDict, forKey: NSStreamSOCKSProxyConfigurationKey)
        
        inputStream!.open()
        outputStream!.open()
     
        // TX
        outputStream?.write(encodedDataArray, maxLength: encodedDataArray.count)
     
        // RX
        //sleep(5)
        //inputStream?.read(&buffer, maxLength: 100)
        //print("buffer = \(buffer)\n")
    }
 
    var service_thread : NSThread!
    func ztnc_start_service() {
        
        // If you plan on using SOCKS Proxy
        //start_service("/Users/Joseph/utest3")
        
        // If you plan on using direct calls via RPC
        start_service_and_rpc("/Users/Joseph/utest3","565799d8f65063e5");
    }
    
    
    override func viewDidLoad() {
        super.viewDidLoad()

        // Set initial UI values for demo
        txtAddr.stringValue = serverAddr
        txtPort.intValue = serverPort
        txtNWID.stringValue = "565799d8f65063e5"
        
        // ZeroTier Service thread
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), {
            self.service_thread = NSThread(target:self, selector:"ztnc_start_service", object:nil)
            self.service_thread.start()
        });
    }

    override var representedObject: AnyObject? {
        didSet {
        // Update the view, if already loaded.
        }
    }
}

