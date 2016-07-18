//
//  ViewController.swift
//  Example_iOS_App
//
//  Created by Joseph Henry on 7/10/16.
//  Copyright © 2016 ZeroTier Inc. All rights reserved.
//

import UIKit

class ViewController: UIViewController {

    @IBOutlet weak var txtNWID: UITextField!
    
    @IBOutlet weak var btnJoinNetwork: UIButton!
    @IBAction func UI_JoinNetwork(sender: AnyObject) {
        zt_join_network("565799d8f65063e5")
    }
    
    @IBOutlet weak var btnLeaveNetwork: UIButton!
    @IBAction func UI_LeaveNetwork(sender: AnyObject) {
        zt_leave_network("565799d8f65063e5")
    }
    
    var service_thread : NSThread!
    func ztnc_start_service() {
        let path = NSSearchPathForDirectoriesInDomains(NSSearchPathDirectory.DocumentDirectory, NSSearchPathDomainMask.UserDomainMask, true)
        start_service(path[0])
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        
        // ZeroTier Service thread
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0), {
            self.service_thread = NSThread(target:self, selector:"ztnc_start_service", object:nil)
            self.service_thread.start()
        });
                
        // Do any additional setup after loading the view, typically from a nib.
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }
}

