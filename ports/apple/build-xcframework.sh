#!/bin/sh

xcodebuild archive -scheme zt -sdk macosx -archivePath macosx

xcodebuild archive -scheme zt -sdk iphoneos -archivePath iphoneos

xcodebuild archive -scheme zt -sdk iphonesimulator -archivePath iphonesimulator

xcodebuild -create-xcframework \
    -framework macosx.xcarchive/Products/Library/Frameworks/zt.framework \
    -framework iphoneos.xcarchive/Products/Library/Frameworks/zt.framework \
    -framework iphonesimulator.xcarchive/Products/Library/Frameworks/zt.framework \
    -output zt.xcframework
    
rm -rf macosx.xcarchive
rm -rf iphoneos.xcarchive
rm -rf iphonesimulator.xcarchive
