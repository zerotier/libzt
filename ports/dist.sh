#!/bin/bash

# This script works in conjunction with the Makefile and CMakeLists.txt. It is
# intented to be called from the Makefile, it generates projects and builds
# targets as specified in CMakeLists.txt. This script is also responsible for
# packaging all of the resultant builds, licenses, and documentation.

BUILD_CONCURRENCY=
#"-j 2"
OSNAME=$(uname | tr '[A-Z]' '[a-z]')
BUILD_TMP=$(pwd)/tmp
ANDROID_PROJ_DIR=$(pwd)/ports/android
XCODE_IOS_ARM64_PROJ_DIR=$(pwd)/ports/xcode_ios-arm64
#XCODE_IOS_ARMV7_PROJ_DIR=$(pwd)/ports/xcode_ios-armv7
XCODE_MACOS_PROJ_DIR=$(pwd)/ports/xcode_macos

# Generates projects if needed
generate_projects() 
{
    if [[ ! $OSNAME = *"darwin"* ]]; then
        exit 0
    fi
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
	if [[ $OSNAME = *"darwin"* ]]; then
		# iOS (SDK 11+, 64-bit only, arm64)
		if [ ! -d "$XCODE_IOS_ARM64_PROJ_DIR" ]; then
			mkdir -p $XCODE_IOS_ARM64_PROJ_DIR
			cd $XCODE_IOS_ARM64_PROJ_DIR
			cmake -G Xcode ../../ -DIOS_FRAMEWORK=1 -DIOS_ARM64=1
            # Manually replace arch strings in project file
	        sed -i '' 's/x86_64/$(CURRENT_ARCH)/g' zt.xcodeproj/project.pbxproj 
			cd -
		fi
        # iOS (SDK <11, 32-bit only, armv7, armv7s)
		#if [ ! -d "$XCODE_IOS_ARMV7_PROJ_DIR" ]; then
		#	mkdir -p $XCODE_IOS_ARMV7_PROJ_DIR
		#	cd $XCODE_IOS_ARMV7_PROJ_DIR
		#	cmake -G Xcode ../../ -DIOS_FRAMEWORK=1 -DIOS_ARMV7=1
            # Manually replace arch strings in project file
	    #   sed -i '' 's/x86_64/$(CURRENT_ARCH)/g' zt.xcodeproj/project.pbxproj 
		#	cd -
		#fi
		# macOS
		if [ ! -d "$XCODE_MACOS_PROJ_DIR" ]; then
			mkdir -p $XCODE_MACOS_PROJ_DIR
			cd $XCODE_MACOS_PROJ_DIR
			cmake -G Xcode ../../ -DMACOS_FRAMEWORK=1
			cd -
        fi
	fi
}

ios()
{
    if [[ ! $OSNAME = *"darwin"* ]]; then
        exit 0
    fi
    generate_projects # if needed
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    UPPERCASE_CONFIG="$(tr '[:lower:]' '[:upper:]' <<< ${1:0:1})${1:1}"

    # 64-bit
    cd $XCODE_IOS_ARM64_PROJ_DIR
    # Framework
    xcodebuild -arch arm64 -target zt -configuration "$UPPERCASE_CONFIG" -sdk "iphoneos" 
    cd -
    OUTPUT_DIR=$(pwd)/lib/$1/ios-arm64
    mkdir -p $OUTPUT_DIR
    rm -rf $OUTPUT_DIR/zt.framework # Remove prior to move to prevent error
    mv $XCODE_IOS_ARM64_PROJ_DIR/$UPPERCASE_CONFIG-iphoneos/* $OUTPUT_DIR
    
    # 32-bit
    #cd $XCODE_IOS_ARMV7_PROJ_DIR
    # Framework
    #xcodebuild -target zt -configuration "$UPPERCASE_CONFIG" -sdk "iphoneos10.0"
    # Manually replace arch strings in project file
	#sed -i '' 's/x86_64/$(CURRENT_ARCH)/g' zt.xcodeproj/project.pbxproj  
    #cd -
    #OUTPUT_DIR=$(pwd)/lib/$1/ios-armv7
    #mkdir -p $OUTPUT_DIR
    #rm -rf $OUTPUT_DIR/*
    #mv $XCODE_IOS_ARMV7_PROJ_DIR/$UPPERCASE_CONFIG-iphoneos/* $OUTPUT_DIR
}

macos()
{
    if [[ ! $OSNAME = *"darwin"* ]]; then
        exit 0
    fi
    generate_projects # if needed
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    UPPERCASE_CONFIG="$(tr '[:lower:]' '[:upper:]' <<< ${1:0:1})${1:1}"
    cd $XCODE_MACOS_PROJ_DIR
    # Framework
    xcodebuild -target zt -configuration "$UPPERCASE_CONFIG" -sdk "macosx"
    # NOTE: We build the static and dynamic editions in host()
    # Static library (libzt.a)
    #xcodebuild -target zt-static -configuration "$UPPERCASE_CONFIG" -sdk "macosx"
    # Dynamic library (libzt.dylib)
    #xcodebuild -target zt-shared -configuration "$UPPERCASE_CONFIG" -sdk "macosx"
    cd -
    OUTPUT_DIR=$(pwd)/lib/$1/macos-$(uname -m)
    mkdir -p $OUTPUT_DIR
    rm -rf $OUTPUT_DIR/zt.framework # Remove prior to move to prevent error
    mv $XCODE_MACOS_PROJ_DIR/$UPPERCASE_CONFIG/* $OUTPUT_DIR
}

host_jar()
{
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    NORMALIZED_OSNAME=$OSNAME
    if [[ $OSNAME = *"darwin"* ]]; then
        DYNAMIC_LIB_NAME="libzt.dylib"
        NORMALIZED_OSNAME="macos"
    fi
    if [[ $OSNAME = *"linux"* ]]; then
        DYNAMIC_LIB_NAME="libzt.so"
    fi
    LIB_OUTPUT_DIR=$(pwd)/lib/$1/${NORMALIZED_OSNAME}-$(uname -m)
    mkdir -p $LIB_OUTPUT_DIR
    # Build dynamic library
    BUILD_DIR=$(pwd)/tmp/${NORMALIZED_OSNAME}-$(uname -m)-jni-$1
    UPPERCASE_CONFIG="$(tr '[:lower:]' '[:upper:]' <<< ${1:0:1})${1:1}"
    cmake -H. -B$BUILD_DIR -DCMAKE_BUILD_TYPE=$UPPERCASE_CONFIG "-DJNI=1"
    cmake --build $BUILD_DIR $BUILD_CONCURRENCY
    # Copy dynamic library from previous build step
    # And, remove any lib that may exist prior. We don't want accidental successes
    cd $(pwd)/ports/java
    rm $DYNAMIC_LIB_NAME
    mv $BUILD_DIR/lib/$DYNAMIC_LIB_NAME .
    # Begin constructing JAR
    export JAVA_TOOL_OPTIONS=-Dfile.encoding=UTF8
    javac com/zerotier/libzt/*.java
    jar cf zt.jar $DYNAMIC_LIB_NAME com/zerotier/libzt/*.class
    rm $DYNAMIC_LIB_NAME
    cd -
    # Move completed JAR
    LIB_OUTPUT_DIR=$(pwd)/lib/$1/${NORMALIZED_OSNAME}-$(uname -m)
    mkdir -p $LIB_OUTPUT_DIR
    mv $(pwd)/ports/java/zt.jar $LIB_OUTPUT_DIR
}

host()
{
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    NORMALIZED_OSNAME=$OSNAME
    if [[ $OSNAME = *"darwin"* ]]; then
        NORMALIZED_OSNAME="macos"
    fi
    # CMake build files
    BUILD_DIR=$(pwd)/tmp/${NORMALIZED_OSNAME}-$(uname -m)-$1
    mkdir -p $BUILD_DIR
    # Where to place results
    BIN_OUTPUT_DIR=$(pwd)/bin/$1/${NORMALIZED_OSNAME}-$(uname -m)
    mkdir -p $BIN_OUTPUT_DIR
    LIB_OUTPUT_DIR=$(pwd)/lib/$1/${NORMALIZED_OSNAME}-$(uname -m)
    mkdir -p $LIB_OUTPUT_DIR
    # Build
    cmake -H. -B$BUILD_DIR -DCMAKE_BUILD_TYPE=$1
    cmake --build $BUILD_DIR $BUILD_CONCURRENCY
    # Move and clean up
    mv $BUILD_DIR/bin/* $BIN_OUTPUT_DIR
    mv $BUILD_DIR/lib/* $LIB_OUTPUT_DIR
    cleanup
}

android()
{
    # NOTE: There's no reason this won't build on linux, it's just that
    # for our purposes we limit this to execution on macOS
    if [[ ! $OSNAME = *"darwin"* ]]; then
        exit 0
    fi
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    ARCH="armeabi-v7a"
    # CMake build files
    BUILD_DIR=$(pwd)/tmp/android-$ARCH-$1
    mkdir -p $BUILD_DIR
    # If clean requested, remove temp build dir
    if [[ $1 = *"clean"* ]]; then
        rm -rf $BUILD_DIR
        exit 0
    fi
    # Where to place results
    LIB_OUTPUT_DIR=$(pwd)/lib/$1/android-$ARCH
    mkdir -p $LIB_OUTPUT_DIR
    # Build
    UPPERCASE_CONFIG="$(tr '[:lower:]' '[:upper:]' <<< ${1:0:1})${1:1}"
    CMAKE_FLAGS=$CMAKE_FLAGS" -DSDK_JNI=1"
    cd $ANDROID_PROJ_DIR
    ./gradlew assemble$UPPERCASE_CONFIG # assembleRelease / assembleDebug
    mv $ANDROID_PROJ_DIR/app/build/outputs/aar/app-$1.aar \
        $LIB_OUTPUT_DIR/libzt-$1.aar
    cd -
}

cleanup()
{
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    find $(pwd)/lib -type f -name 'liblwip_pic.a' -exec rm {} +
    find $(pwd)/lib -type f -name 'liblwip.a' -exec rm {} +
    find $(pwd)/lib -type f -name 'libminiupnpc.a' -exec rm {} +
    find $(pwd)/lib -type f -name 'libnatpmp.a' -exec rm {} +
    find $(pwd)/lib -type f -name 'libzto_pic.a' -exec rm {} +
    find $(pwd)/lib -type f -name 'libzerotiercore.a' -exec rm {} +
}

package_everything()
{
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    LIBZT_VERSION=$(git describe)
    ZT_CORE_VERSION="1.2.12"
    PROD_NAME=$LIBZT_VERSION-$1
    PROD_DIR=$(pwd)/products/$PROD_NAME/
    # Make products directory
    LICENSE_DIR=$PROD_DIR/licenses
    mkdir -p $LICENSE_DIR
    # Licenses
    cp $(pwd)/ext/lwip/COPYING $LICENSE_DIR/LWIP-LICENSE.BSD
    cp $(pwd)/ext/concurrentqueue/LICENSE.md $LICENSE_DIR/CONCURRENTQUEUE-LICENSE.BSD
    cp $(pwd)/LICENSE.GPL-3 $LICENSE_DIR/ZEROTIER-LICENSE.GPL-3
    cp $(pwd)/include/net/ROUTE_H-LICENSE.APSL $LICENSE_DIR/ROUTE_H-LICENSE.APSL
    cp $(pwd)/include/net/ROUTE_H-LICENSE $LICENSE_DIR/ROUTE_H-LICENSE

    # Documentation
    #mkdir -p $PROD_DIR/doc
    #cp $(pwd)/README.md $PROD_DIR/doc
    # Header(s)
    mkdir -p $PROD_DIR/include
    cp $(pwd)/include/*.h $PROD_DIR/include
    # Libraries
    mkdir -p $PROD_DIR/lib
    cp -r $(pwd)/lib/$1/* $PROD_DIR/lib
    # Clean
    find $PROD_DIR -type f \( -name '*.DS_Store' -o -name 'thumbs.db' \) -delete
    # Emit a README file
#    echo $'* libzt version: '${LIBZT_VERSION}$'\n* Core ZeroTier version:
#'${ZT_CORE_VERSION}$'\n* Date: '$(date)$'\n
#- ZeroTier Manual: https://www.zerotier.com/manual.shtml
#- libzt Manual: https://www.zerotier.com/manual.shtml#5
#- libzt Repo: https://github.com/zerotier/libzt
#- Other Downloads: https://www.zerotier.com/download.shtml
#- For more assistance, visit https://my.zerotier.com and ask your
#question in our Community section' > $PROD_DIR/README.FIRST
    # Tar everything
    PROD_FILENAME=$(pwd)/products/$PROD_NAME.tar.gz
    tar --exclude=$PROD_FILENAME -zcvf $PROD_FILENAME $PROD_DIR
    md5 $PROD_FILENAME
}

dist()
{
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    package_everything "debug"
    package_everything "release"
}

"$@"