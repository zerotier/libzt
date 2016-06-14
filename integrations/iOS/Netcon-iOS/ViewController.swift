//
//  ViewController.swift
//  Netcon-iOS
//
//  Created by Joseph Henry on 2/14/16.
//  Copyright © 2016 ZeroTier. All rights reserved.
//

import UIKit

class ViewController: UIViewController {

    @IBOutlet weak var myWebView: UIWebView!
    
    @IBOutlet weak var btnTcpServerTest: UIButton!
    @IBOutlet weak var btnTcpClientTest: UIButton!
    @IBOutlet weak var btnUdpServerTest: UIButton!
    @IBOutlet weak var btnUdpClientTest: UIButton!
    
    @IBOutlet weak var btnExecuteTest: UIButton!
    @IBOutlet weak var txtPort: UITextField!
    @IBOutlet weak var txtAddr: UITextField!
    
    @IBOutlet weak var urlTextField: UITextField!
    
    var serverPort:UInt16 = 8888
    var serverAddr:String = "10.5.5.2"

    // Test Network Join
    @IBOutlet weak var txtNWID: UITextField!
    @IBOutlet weak var btnJoinNetwork: UIButton!
    @IBAction func joinNetworkClicked(sender: AnyObject) {
        zt_join_network(txtNWID.text!);
        zt_join_network("e5cd7a9e1c2e194f");
    }
    

    
    
    // Shim { Hook, Proxy, Changeling, Direct Call }
    @IBOutlet weak var ShimControl: UISegmentedControl!
    var selectedShim:UInt16 = 0
    @IBAction func ShimControlSelected(sender: AnyObject) {
        switch sender.selectedSegmentIndex
        {
        case 0:
            print("Selected Hook\n");
            selectedShim = 0
        case 1:
            print("Selected Proxy\n");
            selectedShim = 1
        case 2:
            print("Selected Changeling\n");
            selectedShim = 2
        case 3:
            print("Selected Direct\n");
            selectedShim = 3
        default:
            break;
        }
    }
    
    
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
    
    
    // Protocol { TCP / UDP }
    @IBOutlet weak var ProtocolControl: UISegmentedControl!
    var selectedProtocol:Int32 = SOCK_STREAM
    @IBAction func ProtocolControlSelected(sender: AnyObject) {
        switch sender.selectedSegmentIndex
        {
        case 0:
            print("Selected TCP (SOCK_STREAM)\n");
            selectedProtocol = SOCK_STREAM
        case 1:
            print("Selected UDP (SOCK_DGRAM)\n");
            selectedProtocol = SOCK_DGRAM
        default:
            break;
        }
    }
    
    
    
    
    @IBAction func ExecuteTest(sender: AnyObject) {
        print("Running Test...\n")
        switch selectedShim
        {
        case 0:
            print("test_client_hook_bsd_socket_api\n");
            test_client_hook_bsd_socket_api()
        case 1:
            print("test_intercepted_proxy_streams\n");
            test_client_proxy_nsstream()
        case 2:
            print("test_client_changeling\n");
            test_client_changeling()
        case 3:
            print("test_client_direct_call_zt_socket\n");
            test_client_direct_call_zt_socket()
        default:
            break;
        }
    }
    


    @IBOutlet weak var btnSockTest: UIButton!
    @IBAction func SocksTestAction(sender: AnyObject) {
        // Remove
    }
    
    @IBOutlet weak var WebRequest: UIButton!
    @IBAction func WebRequestAction(sender: AnyObject) {
        // TODO: Re-test
        let url_str = "http://" + txtAddr.text! + "/"
        let url = NSURL (string: url_str);
        //urlTextField.text = url_str;
        let requestObj = NSURLRequest(URL: url!);
        myWebView.loadRequest(requestObj);
    }
    

    // Mode: Client Test
    // Shim: SOCKS5 Proxy
    // Method: NSStream
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
        
        /* If you're interested in what happens next:
        
        NSStream objects will generate native sockets internally which then connect to
        the SOCKS proxy on 'localhost'. Once this connection is established the Proxy server
        will handle a connection request to the "local address" of your choice. The subsequent
        socket(), and connect() calls will be intercepted and sent to the Netcon service via
        an RPC mechanism mediated by unix domain sockets. These RPC calls are dissected and
        sent to the lwIP stack and finally to the ZeroTierOne service
        */
        
        inputStream!.open()
        outputStream!.open()
        outputStream?.write(encodedDataArray, maxLength: encodedDataArray.count)
        //sleep(5)
        //inputStream?.read(&buffer, maxLength: 100)
        //print("buffer = \(buffer)\n")
    }
    
    
    // Mode: Client Test
    // Shim: Hook
    // Method: BSD-like socket API
    func test_client_hook_bsd_socket_api()
    {
        // TCP
        if(selectedProtocol == SOCK_STREAM)
        {
            let sd = socket(AF_INET, SOCK_STREAM, 0)
            var addr = sockaddr_in(sin_len: UInt8(sizeof(sockaddr_in)),
                               sin_family: UInt8(AF_INET),
                               sin_port: serverPort.bigEndian,
                               sin_addr: in_addr(s_addr: 0),
                               sin_zero: (0,0,0,0,0,0,0,0))
        
            inet_pton(AF_INET, serverAddr, &(addr.sin_addr));
        
            let connect_fd = connect(sd, UnsafePointer<sockaddr>([addr]), UInt32(addr.sin_len))
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
    
    // Mode: Client Test
    // Shim: N/A
    // Method: Direct Call to ZT API
    func test_client_direct_call_zt_socket()
    {
        // TCP
        if(selectedProtocol == SOCK_STREAM)
        {
            // Note: We merely added the 'zt_' prefix to the standard native bsd socket calls
            // This gets you direct access to ZeroTier Sockets
            let sd = zts_socket(AF_INET, SOCK_STREAM, 0)
            var addr = sockaddr_in(sin_len: UInt8(sizeof(sockaddr_in)),
                                   sin_family: UInt8(AF_INET),
                                   sin_port: serverPort.bigEndian,
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
    
    // Mode: Client Test
    // Shim: Changeling
    // Method: BSD-like socket API
    func test_client_changeling()
    {
        // Technically this scenario is using the same bsd socket API as the 
        // 'test_client_hook_bsd_socket_api' test, we're just handling the native 
        // sockets in a different way, so we'll just call the same test function
        test_client_hook_bsd_socket_api()
    }
    
    
    // -------- BEGIN ZEROTIER SERVICE AND PROXY THREAD DEFINITIONS
    
    var service_thread : NSThread!
    func ztnc_start_service() {
        // FIXME: We use this to get a path for the ZeroTierOne service to use, this should be done differently for production
        let path = NSSearchPathForDirectoriesInDomains(NSSearchPathDirectory.DocumentDirectory, NSSearchPathDomainMask.UserDomainMask, true)
        //disable_intercept() // We don't want the ZeroTier service to use intercepted calls
        print("start_service()\n")
        start_service(path[0])
    }
    
    // ------- END
    
    
    override func viewDidLoad() {
        
        txtNWID.text = "e5cd7a9e1c3511dd"
        
        // Style
        self.view.backgroundColor = UIColor.blackColor()
        btnExecuteTest.setTitleColor(UIColor.greenColor(), forState: UIControlState.Normal)
        btnExecuteTest.layer.cornerRadius = 6
        btnExecuteTest.layer.backgroundColor = UIColor.grayColor().CGColor
        btnExecuteTest.layer.borderColor = UIColor.grayColor().CGColor

        super.viewDidLoad()
        
        // ------- BEGIN INITIALIZATION OF ZEROTIER SERVICE AND PROXY
        
        // ZeroTier Service thread
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), {
            self.service_thread = NSThread(target:self, selector:"ztnc_start_service", object:nil)
            self.service_thread.start()
        });
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
}

