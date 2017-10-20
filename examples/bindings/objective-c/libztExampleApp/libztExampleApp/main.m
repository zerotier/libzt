//
//  main.m
//  libztExampleApp
//
//  Created by Joseph Henry on 10/19/17.
//  Copyright © 2017 ZeroTier, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        // insert code here...
        NSLog(@"Hello, World!");
        zts_startjoin("libzt_config_path", "XXXXXXXXXXXXXXXX");
        zts_socket(2, 1, 0);
    }
    return 0;
}
