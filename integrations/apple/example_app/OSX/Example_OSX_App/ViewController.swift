//
//  ViewController.swift
//  Example_OSX_App
//
//  Created by Joseph Henry on 6/22/16.
//  Copyright © 2016 ZeroTier Inc. All rights reserved.
//

import Cocoa

class ViewController: NSViewController {
    
    let zt = ZTSDK();

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
    
    var serverPort:Int32 = 8080
    var serverAddr:String = "0.0.0.0"
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
        zt.join_network(txtNWID.stringValue);
    }
    // Leave a ZeroTier network
    @IBAction func UI_LeaveNetwork(sender: AnyObject) {
        zt.leave_network(txtNWID.stringValue);
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
    
    // CONNECT
    var connect_thread : NSThread!
    func attempt_connect()
    {
        // TCP
        if(selectedProtocol == SOCK_STREAM)
        {
            sock = zt.socket(AF_INET, SOCK_STREAM, 0)
            let ztaddr: ZTAddress = ZTAddress(AF_INET, serverAddr, Int16(serverPort))
            let connect_err = zt.connect(sock, ztaddr)
            
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
    
    // Connect to remote host on ZeroTier virtual network
    @IBAction func UI_Connect(sender: AnyObject) {
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), {
            self.connect_thread = NSThread(target:self, selector:#selector(ViewController.attempt_connect), object:nil)
            self.connect_thread.start()
        });
    }
    
    // BIND
    var bind_thread : NSThread!
    func attempt_bind()
    {
        sock = zt.socket(AF_INET, SOCK_STREAM, 0)
        let ztaddr: ZTAddress = ZTAddress(AF_INET, serverAddr, Int16(serverPort))
        let bind_err = zt.bind(sock, ztaddr)
        
        print("bind_err = \(bind_err),\(errno)")
        if bind_err < 0 {
            let err = errno
            print("Error binding IPv4 socket \(err)")
            return
        }
            
        // Put socket into listening state
        zt.listen(sock, 1);
        
        // TCP
        if(selectedProtocol == SOCK_STREAM) {
            while(accepted_sock < 0) {
                accepted_sock = zt.accept(sock, ztaddr)
            }
            print("accepted connection")
        }
        // UDP
        if(selectedProtocol == SOCK_DGRAM) {
            // nothing
        }
    }
    
    // Bind a ZeroTier socket
    @IBAction func UI_Bind(sender: AnyObject) {
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), {
            self.bind_thread = NSThread(target:self, selector:#selector(ViewController.attempt_bind), object:nil)
            self.bind_thread.start()
        });
    }
    
    // TX
    @IBOutlet weak var btnSend: NSButton!
    @IBAction func UI_SendData(sender: AnyObject) {
        // Use ordinary read/write calls on ZeroTier socket
        // TCP
        if(selectedProtocol == SOCK_STREAM)
        {
            write(sock, txtTX.description, txtTX.description.characters.count);
        }
        // UDP
        if(selectedProtocol == SOCK_DGRAM)
        {
            // zt_sendto
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
            read(accepted_sock, &buffer, 1024);
            print(buffer)

        }
        // UDP
        if(selectedProtocol == SOCK_DGRAM)
        {
            // zt_recvfrom
        }
    }
    
    // Watch for incoming data
    var rx_thread : NSThread!
    func update_rx() {
        while(true)
        {
            sleep(1)
            dispatch_async(dispatch_get_main_queue()) {
                let str_buf = [Int8](count: 16, repeatedValue: 0)
                print("addr = ", String.fromCString(str_buf))
            }

            // TCP
            if(selectedProtocol == SOCK_STREAM)
            {
                let len = 32
                var buffer = [UInt8](count: len, repeatedValue: 0)
                let n = read(accepted_sock, &buffer, len);
                if(n > 0)
                {
                    if let str = String(data: NSData(bytes: &buffer, length: len), encoding: NSUTF8StringEncoding) {
                        dispatch_async(dispatch_get_main_queue()) {
                            self.txtRX.stringValue = str
                        }
                    } else {
                        print("not a valid UTF-8 sequence")
                    }
                }
            }
            // UDP
            if(selectedProtocol == SOCK_DGRAM)
            {
                // zt_recvfrom
            }
        }
    }

    
    // Built-in SOCKS5 Proxy Server Test
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
        sleep(5)
        inputStream?.read(&buffer, maxLength: 100)
        print("buffer = \(buffer)\n")
    }
 
    override func viewDidLoad() {
        super.viewDidLoad()
        // Set initial UI values for demo
        txtAddr.stringValue = serverAddr
        txtPort.intValue = serverPort
        txtNWID.stringValue = "8056c2e21c000001"
        selectedProtocol = SOCK_STREAM // Just for selecting test mode. Not necessary for zt API
        
        zt.start_service(".");
        zt.join_network(txtNWID.stringValue);
        
        // Update UI on RX of data
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), {
            self.rx_thread = NSThread(target:self, selector:#selector(ViewController.update_rx), object:nil)
            self.rx_thread.start()
        });
    }

    override var representedObject: AnyObject? {
        didSet {
        // Update the view, if already loaded.
        }
    }
}

