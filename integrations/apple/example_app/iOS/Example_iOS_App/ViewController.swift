//
//  ViewController.swift
//  Example_iOS_App
//
//  Created by Joseph Henry on 7/10/16.
//  Copyright © 2016 ZeroTier Inc. All rights reserved.
//

import UIKit

class ViewController: UIViewController {

    let zt = ZTSDK();
    
    var serverPort:Int16 = 8080
    var serverAddr:String = "10.9.9.100"
    var selectedProtocol:Int32 = 0
    var sock:Int32 = -1
    var accepted_sock:Int32 = -1
    
    @IBOutlet weak var txtAddr: UITextField!
    @IBOutlet weak var txtPort: UITextField!
    @IBOutlet weak var txtTX: UITextField!
    @IBOutlet weak var txtRX: UITextField!
    @IBOutlet weak var btnTX: UIButton!
    @IBOutlet weak var btnConnect: UIButton!
    @IBOutlet weak var btnBind: UIButton!
    
    @IBOutlet weak var btnRX: UIButton!
    @IBAction func UI_RX(sender: AnyObject) {
        // Use ordinary read/write calls on ZeroTier socket
        
        // TCP
        if(selectedProtocol == SOCK_STREAM) {
            var buffer = [UInt8](count: 100, repeatedValue: 0)
            read(accepted_sock, &buffer, 100);
            print(buffer)
            
        }
        // UDP
        if(selectedProtocol == SOCK_DGRAM) {
            var buffer = [UInt8](count: 100, repeatedValue: 0)
            read(Int32(sock), &buffer, 100);
            print(buffer)
        }
    }
    
    
    @IBAction func UI_TX(sender: AnyObject) {
        // Use ordinary read/write calls on ZeroTier socket
        // TCP
        if(selectedProtocol == SOCK_STREAM) {
            write(Int32(sock), txtTX.description, txtTX.description.characters.count);
        }
        // UDP
        if(selectedProtocol == SOCK_DGRAM) {
            sendto(Int32(sock), txtTX.description, txtTX.description.characters.count, 0, UnsafePointer<sockaddr>([udp_addr]), UInt32(udp_addr.sin_len))
        }
    }
    
    
    @IBOutlet weak var txtNWID: UITextField!
    
    @IBOutlet weak var btnJoinNetwork: UIButton!
    @IBAction func UI_JoinNetwork(sender: AnyObject) {
        zt.join_network(txtNWID.text!)
    }
    
    @IBOutlet weak var btnLeaveNetwork: UIButton!
    @IBAction func UI_LeaveNetwork(sender: AnyObject) {
        zt.leave_network(txtNWID.text!)
    }
    
    @IBOutlet weak var segmentProtocol: UISegmentedControl!
    @IBAction func protocolSelected(sender: AnyObject) {
        switch sender.selectedSegmentIndex
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
            let ztaddr: ZTAddress = ZTAddress(family: AF_INET, addr: serverAddr, port: serverPort)
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
            self.connect_thread = NSThread(target:self, selector:"attempt_connect", object:nil)
            self.connect_thread.start()
        });
    }
    
    // BIND
    var bind_thread : NSThread!
    func attempt_bind()
    {
        var err:Int32
        
        // TCP
        if(selectedProtocol == SOCK_STREAM)
        {
            sock = zt_socket(AF_INET, SOCK_STREAM, 0)
            let ztaddr: ZTAddress = ZTAddress(family: AF_INET, addr: serverAddr, port: serverPort)
            let bind_err = zt.bind(sock, ztaddr)
            
            print("bind_err = \(bind_err),\(errno)")
            
            if bind_err < 0 {
                let err = errno
                print("Error binding IPv4 socket \(err)")
                return
            }
            
            // Put socket into listening state
            zt_listen(Int32(sock), 1);
            
            // Accept connection
            var len:socklen_t = 0;
            var legIntPtr = withUnsafeMutablePointer(&len, { $0 })
            while(accepted_sock < 0) {
                accepted_sock = zt.accept(sock, ztaddr)
            }
            print("accepted connection")
        }
        
        // UDP
        if(selectedProtocol == SOCK_DGRAM)
        {
            let ztaddr: ZTAddress = ZTAddress(family: AF_INET, addr: serverAddr, port: serverPort)

            sock = zt_socket(AF_INET, SOCK_DGRAM, 0)
            err = zt.bind(sock, ztaddr)
            print("bind_err = ", err)
            
            err = zt.listen(sock, 0)
            print("listen_err = ", err)
        }
    }
    
    @IBOutlet weak var txtAddress: UITextField!
    // Bind a ZeroTier socket
    @IBAction func UI_Bind(sender: AnyObject) {
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), {
            self.bind_thread = NSThread(target:self, selector:"attempt_bind", object:nil)
            self.bind_thread.start()
        });
    }

    @IBOutlet weak var lblAddress: UILabel!
    
    
    var udp_addr:sockaddr_in!
    
    // Watch for incoming data
    var rx_thread : NSThread!
    func update_rx() {
        while(true)
        {
            sleep(1)
            
            dispatch_async(dispatch_get_main_queue()) {
                var str_buf = [Int8](count: 16, repeatedValue: 0)
                print(self.zt.get_address(self.txtNWID.text!)) //, &str_buf);
                self.lblAddress.text = String.fromCString(str_buf)
                // print("IPV4 = ", String.fromCString(str_buf))
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
                            self.txtRX.text = str
                        }
                    } else {
                        print("not a valid UTF-8 sequence")
                    }
                }
            }
            // UDP
            /*
            if(selectedProtocol == SOCK_DGRAM)
            {
                let len = 32
                var buffer = [UInt8](count: len, repeatedValue: 0)

                //udp_addr = sockaddr_in(sin_len: UInt8(sizeof(sockaddr_in)),
                //            sin_family: UInt8(AF_INET),
                //            sin_port: UInt16(0).bigEndian,
                //            sin_addr: in_addr(s_addr: 0),
                //            sin_zero: (0,0,0,0,0,0,0,0))
                
                //var addrlen:socklen_t = 0;
                //var legIntPtr = withUnsafeMutablePointer(&addrlen, { $0 })
                //let n = recvfrom(sock, &buffer, len, 0, UnsafeMutablePointer<sockaddr>([udp_addr]), legIntPtr)
 
                
                var socketAddress = sockaddr_storage()
                var socketAddressLength = socklen_t(sizeof(sockaddr_storage.self))
                
                let bytesRead = withUnsafeMutablePointers(&socketAddress, &socketAddressLength) {
                    recvfrom(sock, UnsafeMutablePointer<Void>(buffer), len, 0, UnsafeMutablePointer($0), UnsafeMutablePointer($1))
                    //recvfrom(<#T##Int32#>, <#T##UnsafeMutablePointer<Void>#>, <#T##Int#>, <#T##Int32#>, <#T##UnsafeMutablePointer<sockaddr>#>, <#T##UnsafeMutablePointer<socklen_t>#>)
                }
                
                if(bytesRead > 0)
                {
                    print("socketAddressLength = ", socketAddressLength);
                    let bytesWritten = withUnsafePointer(&socketAddress) {
                        print("TXing...\n");
                        sendto(sock, UnsafePointer(buffer), bytesRead, 0, UnsafePointer<sockaddr>($0), socketAddressLength)
                    }
                    
                    print("bytesWritten = ", bytesWritten);
                    
                    //let bytesWritten = withUnsafePointer(&socketAddress.sin) {
                    //    sendto(sock, buffer, len, 0, UnsafePointer($0), socklen_t(socketAddress.sin.sin_len))
                    //}

                    if let str = String(data: NSData(bytes: &buffer, length: len), encoding: NSUTF8StringEncoding) {
                        dispatch_async(dispatch_get_main_queue()) {
                            self.txtRX.text = str
                        }
                    } else {
                        print("not a valid UTF-8 sequence")
                    }
                }
            }
             */
        }
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        
        txtNWID.text = "8056c2e21c000001"
        txtTX.text = "welcome to the machine"
        txtAddr.text = "0.0.0.0"
        serverAddr = "0.0.0.0"
        txtPort.text = "8080"
        serverPort = 8080
        
        selectedProtocol = SOCK_STREAM
        
        sleep(3)
        print("Starting ZeroTier...\n");
        zt.start_service(nil);
        
        print("Joining network...\n");
        zt.join_network(txtNWID.text!);
        
        print("Complete\n");
        
        // UI RX update
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), {
            self.rx_thread = NSThread(target:self, selector:"update_rx", object:nil)
            self.rx_thread.start()
        });
        
        // Do any additional setup after loading the view, typically from a nib.
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
}

