//
//  ViewController.swift
//  ExampleSwiftApp
//
//  Created by Joseph Henry on 8/1/18.
//  Copyright © 2018 ZeroTier. All rights reserved.
//

import UIKit

class ViewController: UIViewController {

    override func viewDidLoad() {
        super.viewDidLoad()
        
        MyApplication.libzt_example_function();
        
        // Do any additional setup after loading the view, typically from a nib.
    }

    override func didReceiveMemoryWarning() {
        super.didReceiveMemoryWarning()
        // Dispose of any resources that can be recreated.
    }


}

