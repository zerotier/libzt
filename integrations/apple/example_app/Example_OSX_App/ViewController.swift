//
//  ViewController.swift
//  Example_OSX_App
//
//  Created by Joseph Henry on 6/22/16.
//  Copyright © 2016 ZeroTier Inc. All rights reserved.
//

import Cocoa
i

class ViewController: NSViewController {
    
    
    var service_thread : NSThread!
    func ztnc_start_service() {
        let path = NSSearchPathForDirectoriesInDomains(NSSearchPathDirectory.DocumentDirectory, NSSearchPathDomainMask.UserDomainMask, true)
        print("start_service()\n")
        // e5cd7a9e1c3511dd
        start_service(path[0])
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

