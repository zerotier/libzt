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
    

    @IBAction func UI_JoinNetwork(sender: AnyObject) {
        zt_join_network(txtNWID.stringValue);
    }
    
    @IBAction func UI_LeaveNetwork(sender: AnyObject) {
        zt_leave_network(txtNWID.stringValue);
    }
    
    @IBAction func UI_SelectAPI(sender: AnyObject) {
    }
    
    @IBAction func UI_SelectProtocol(sender: AnyObject) {
    }
    
    @IBAction func UI_Connect(sender: AnyObject) {
    }
    
    @IBAction func UI_Bind(sender: AnyObject) {
    }
    
    @IBAction func UI_SendData(sender: AnyObject) {
    }
    
    
    
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
        // Do any additional setup after loading the view.
    }

    override var representedObject: AnyObject? {
        didSet {
        // Update the view, if already loaded.
        }
    }


}

