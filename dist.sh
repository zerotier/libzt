#!/bin/bash

# This script works in conjunction with the Makefile and CMakeLists.txt. It is
# intented to be called from the Makefile, it generates projects and builds
# targets as specified in CMakeLists.txt. In addition, this script is
# responsible for packaging all of the resultant builds, licenses, and
# documentation as well as controlling the installation and remote execution of
# tests on mobile devices.

# Example workflow for producing a full release package:
#
# (1) On packaging platform, build most targets (including android and ios):
#    (1a) make all
# (2) On other supported platforms, build remaining supported targets
#     and copy them into a directory structure that is expected by a later stage:
#    (2a) make all
#    (2b) make wrap
# (3) Copy all resultant $(ARCH)_product directories to root project directory
#     of packaging platform. For instance:
#
#  libzt
#    ├── API.md
#    ├── products
#    ├── linux-x86_64_products
#    ├── linux-armv7l_products
#    ├── linux-armv6l_products
#    ├── products
#    ├── win_products
#    └── ...
#
# (4) Merge all builds into single `products` directory and package:
#    (4a) make dist

BUILD_CONCURRENCY=
#"-j 2"
OSNAME=$(uname | tr '[A-Z]' '[a-z]')
BUILD_TMP=$(pwd)/tmp
ANDROID_PROJ_DIR=$(pwd)/ports/android
XCODE_IOS_PROJ_DIR=$(pwd)/ports/xcode_ios
XCODE_IOS_SIMULATOR_PROJ_DIR=$(pwd)/ports/xcode_ios_simulator
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
        if [ ! -d "$XCODE_IOS_PROJ_DIR" ]; then
            mkdir -p $XCODE_IOS_PROJ_DIR
            cd $XCODE_IOS_PROJ_DIR
            cmake -G Xcode ../../ -DIOS_FRAMEWORK=1 -DIOS_ARM64=1
            # Manually replace arch strings in project file
            sed -i '' 's/x86_64/$(CURRENT_ARCH)/g' zt.xcodeproj/project.pbxproj
            cd -
        fi

        if [ ! -d "$XCODE_IOS_SIMULATOR_PROJ_DIR" ]; then
            mkdir -p $XCODE_IOS_SIMULATOR_PROJ_DIR
            cd $XCODE_IOS_SIMULATOR_PROJ_DIR
            cmake -G Xcode ../../ -DIOS_FRAMEWORK=1
            # Manually replace arch strings in project file
            #sed -i '' 's/x86_64/$(CURRENT_ARCH)/g' zt.xcodeproj/project.pbxproj
            cd -
        fi

        # macOS
        if [ ! -d "$XCODE_MACOS_PROJ_DIR" ]; then
            mkdir -p $XCODE_MACOS_PROJ_DIR
            cd $XCODE_MACOS_PROJ_DIR
            cmake -G Xcode ../../ -DMACOS_FRAMEWORK=1
            cd -
        fi
    fi
}

# Build framework for iOS (with embedded static library)
ios()
{
    if [[ ! $OSNAME = *"darwin"* ]]; then
        exit 0
    fi
    generate_projects # if needed
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    UPPERCASE_CONFIG="$(tr '[:lower:]' '[:upper:]' <<< ${1:0:1})${1:1}"

    cd $XCODE_IOS_PROJ_DIR
    # Framework
    xcodebuild -arch arm64 -target zt -configuration "$UPPERCASE_CONFIG" -sdk "iphoneos"
    cd -
    IOS_OUTPUT_DIR=$(pwd)/lib/$1/ios
    mkdir -p $IOS_OUTPUT_DIR
    rm -rf $IOS_OUTPUT_DIR/zt.framework # Remove prior to move to prevent error
    mv $XCODE_IOS_PROJ_DIR/$UPPERCASE_CONFIG-iphoneos/* $IOS_OUTPUT_DIR

    cd $XCODE_IOS_SIMULATOR_PROJ_DIR
    # Framework
    xcodebuild -target zt -configuration "$UPPERCASE_CONFIG" -sdk "iphonesimulator"
    cd -
    SIMULATOR_OUTPUT_DIR=$(pwd)/lib/$1/ios-simulator
    mkdir -p $SIMULATOR_OUTPUT_DIR
    rm -rf $SIMULATOR_OUTPUT_DIR/zt.framework # Remove prior to move to prevent error
    mv $XCODE_IOS_SIMULATOR_PROJ_DIR/$UPPERCASE_CONFIG-iphonesimulator/* $SIMULATOR_OUTPUT_DIR

    # Combine the two archs
    lipo -create $IOS_OUTPUT_DIR/zt.framework/zt $SIMULATOR_OUTPUT_DIR/zt.framework/zt -output $IOS_OUTPUT_DIR/zt.framework/zt

    # Clean up
    rm -rf $SIMULATOR_OUTPUT_DIR
}

# Build framework for current host (macOS only)
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

# Build Java JAR for current host (uses JNI)
host_jar()
{
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    copy_root_java_sources_to_projects
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
    rm -rf $LIB_OUTPUT_DIR/zt.jar
    # Build dynamic library
    BUILD_DIR=$(pwd)/tmp/${NORMALIZED_OSNAME}-$(uname -m)-jni-$1
    UPPERCASE_CONFIG="$(tr '[:lower:]' '[:upper:]' <<< ${1:0:1})${1:1}"
    cmake -H. -B$BUILD_DIR -DCMAKE_BUILD_TYPE=$UPPERCASE_CONFIG -DSDK_JNI=ON "-DSDK_JNI=1"
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
    # Build sample app classes
    # Remove old dynamic library if it exists
    rm -rf $(pwd)/examples/java/$DYNAMIC_LIB_NAME
    javac -cp ".:"$LIB_OUTPUT_DIR/zt.jar $(pwd)/examples/java/src/main/java/*.java
    # To run:
    # jar xf $LIB_OUTPUT_DIR/zt.jar libzt.dylib
    # cp libzt.dylib examples/java/
    # java -cp "lib/debug/macos-x86_64/zt.jar:examples/java/src/main/java" ExampleApp
}

# Build all ordinary library types for current host
host()
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
    # CMake build files
    BUILD_DIR=$(pwd)/tmp/${NORMALIZED_OSNAME}-$(uname -m)-$1
    mkdir -p $BUILD_DIR
    # Where to place results
    BIN_OUTPUT_DIR=$(pwd)/bin/$1/${NORMALIZED_OSNAME}-$(uname -m)
    mkdir -p $BIN_OUTPUT_DIR
    rm -rf $BIN_OUTPUT_DIR/*
    LIB_OUTPUT_DIR=$(pwd)/lib/$1/${NORMALIZED_OSNAME}-$(uname -m)
    mkdir -p $LIB_OUTPUT_DIR
    rm -rf $LIB_OUTPUT_DIR/libzt.a $LIB_OUTPUT_DIR/$DYNAMIC_LIB_NAME $LIB_OUTPUT_DIR/libztcore.a
    # Build
    cmake -H. -B$BUILD_DIR -DCMAKE_BUILD_TYPE=$1
    cmake --build $BUILD_DIR $BUILD_CONCURRENCY
    # Move and clean up
    mv $BUILD_DIR/bin/* $BIN_OUTPUT_DIR
    mv $BUILD_DIR/lib/* $LIB_OUTPUT_DIR
    clean_post_build
}

# Set important variables for Android builds
set_android_env()
{
    #JDK=jdk1.8.0_202.jdk
    # Set ANDROID_HOME because setting sdk.dir in local.properties isn't always reliable
    export ANDROID_HOME=/Users/$USER/Library/Android/sdk
    export PATH=/Library/Java/JavaVirtualMachines/$JDK/Contents/Home/bin/:${PATH}
    export PATH=/Users/$USER/Library/Android/sdk/platform-tools/:${PATH}
    GRADLE_ARGS=--stacktrace
    ANDROID_APP_NAME=com.example.mynewestapplication
}

# Build android AAR from ports/android
android()
{
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    set_android_env
    copy_root_java_sources_to_projects
    # NOTE: There's no reason this won't build on linux, it's just that
    # for our purposes we limit this to execution on macOS
    if [[ ! $OSNAME = *"darwin"* ]]; then
        exit 0
    fi
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
    CMAKE_FLAGS="-DSDK_JNI=1 -DSDK_JNI=ON"
    cd $ANDROID_PROJ_DIR
    ./gradlew $GRADLE_ARGS --recompile-scripts
    ./gradlew $GRADLE_ARGS assemble$UPPERCASE_CONFIG # assembleRelease / assembleDebug
    mv $ANDROID_PROJ_DIR/app/build/outputs/aar/app-$1.aar \
        $LIB_OUTPUT_DIR/libzt-$1.aar
    cd -
}

# Remove intermediate object files and/or libraries
clean_post_build()
{
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    find $(pwd)/lib -type f -name 'liblwip_pic.a' -exec rm {} +
    find $(pwd)/lib -type f -name 'liblwip.a' -exec rm {} +
    find $(pwd)/lib -type f -name 'libminiupnpc.a' -exec rm {} +
    find $(pwd)/lib -type f -name 'libminiupnpc_pic.a' -exec rm {} +
    find $(pwd)/lib -type f -name 'libnatpmp.a' -exec rm {} +
    find $(pwd)/lib -type f -name 'libnatpmp_pic.a' -exec rm {} +
    find $(pwd)/lib -type f -name 'libzto_pic.a' -exec rm {} +
    find $(pwd)/lib -type f -name 'libzt_pic.a' -exec rm {} +
}

# General clean
clean()
{
    # Remove all temporary build files, products, etc
    rm -rf tmp lib bin products
    rm -f *.o *.s *.exp *.lib *.core core
    # Generally search for and remove object files, libraries, etc
    find . -type f \( -name '*.dylib' -o -name '*.so' -o -name \
        '*.a' -o -name '*.o' -o -name '*.o.d' -o -name \
        '*.out' -o -name '*.log' -o -name '*.dSYM' -o -name '*.class' \) -delete
    # Remove any sources copied to project directories
    rm -rf ports/android/app/src/main/java/com/zerotier/libzt/*.java
    rm -rf ports/java/com/zerotier/libzt/*.java
}

# Copy and rename Android AAR from lib to example app directory
prep_android_example()
{
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    mkdir -p examples/android/ExampleAndroidApp/app/libs/
    cp -f lib/$1/android-armeabi-v7a/libzt-$1.aar \
    examples/android/ExampleAndroidApp/app/libs/libzt.aar
}
# Clean Android project
clean_android_project()
{
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    set_android_env
    ANDROID_EXAMPLE_PROJ_DIR="examples/android/ExampleAndroidApp"
    cd $ANDROID_EXAMPLE_PROJ_DIR
    ./gradlew $GRADLE_ARGS clean
    ./gradlew $GRADLE_ARGS cleanBuildCache
    cd -
}
# Build APK from AAR and sources
build_android_app()
{
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    set_android_env
    ANDROID_EXAMPLE_PROJ_DIR="examples/android/ExampleAndroidApp"
    UPPERCASE_CONFIG="$(tr '[:lower:]' '[:upper:]' <<< ${1:0:1})${1:1}"
    cd $ANDROID_EXAMPLE_PROJ_DIR
    ./gradlew assemble$UPPERCASE_CONFIG
    cd -
}
# Stops an Android app that is already installed on device
stop_android_app()
{
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    set_android_env
    adb shell am force-stop $ANDROID_APP_NAME
}
# Starts an Android app that is already installed on device
start_android_app()
{
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    set_android_env
    adb shell monkey -p $ANDROID_APP_NAME 1
}
# Copy and install example Android app on device
install_android_app()
{
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    set_android_env
    if [[ $1 = "release" ]]; then
        APKNAME=app-$1-"unsigned"
    else
        APKNAME=app-$1
    fi
    APK=examples/android/ExampleAndroidApp/app/build/outputs/apk/$1/$APKNAME.apk
    echo "Installing $APK ..."
    adb install -r $APK
}
# Perform all steps necessary to run a new instance of the app on device
run_android_app()
{
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    stop_android_app
    prep_android_example $1
    clean_android_project
    # The following two functions take 'debug' as an argument regardless
    # of the build type since the native code is built with the proper
    # configuration anyway.
    build_android_app "debug"
    install_android_app "debug"
    start_android_app
}
# View ADB logs of running Android app
android_app_log()
{
    set_android_env
    if [[ $OSNAME = *"darwin"* ]]; then
        adb logcat
    fi
}
# View ADB logs of running Android app (filtered, must restart for each app re-launch)
android_app_log_filtered()
{
    set_android_env
    if [[ $OSNAME = *"darwin"* ]]; then
        adb logcat | grep -F "`adb shell ps | grep $ANDROID_APP_NAME | cut -c10-15`"
    fi
}


# Copy java sources to projects before build process. This is so
# that we only have to maintain one set of sources for multiple java-
# based projects.
copy_root_java_sources_to_projects()
{
    cp -f src/java/*.java ports/android/app/src/main/java/com/zerotier/libzt
    cp -f src/java/*.java ports/java/com/zerotier/libzt/
}

# At the end of build stage, print contents and trees for inspection
display()
{
    find $(pwd)/lib -type f -name 'zt.jar' -exec echo -e "\n" \; -exec ls {} \; -exec jar tf {} +
    echo -e "\n"
    tree $(pwd)/lib
}

# Merge all remotely-built targets. This is used before dist()
merge()
{
    #if [ -d "darwin-x86_64_products" ]; then
    #    rsync -a darwin-x86_64_products/ products/
    #else
    #    echo "Warning: darwin-x86_64_products is missing"
    #fi
    # x86_64 64-bit linux
    REMOTE_PRODUCTS_DIR=linux-x86_64_products
    if [ -d "$REMOTE_PRODUCTS_DIR" ]; then
        rsync -a $REMOTE_PRODUCTS_DIR/ products/
        echo "Merged products from " $REMOTE_PRODUCTS_DIR " to " products
    else
        echo "Warning: $REMOTE_PRODUCTS_DIR is missing"
    fi
    # armv7l linux
    REMOTE_PRODUCTS_DIR=linux-armv7l_products
    if [ -d "$REMOTE_PRODUCTS_DIR" ]; then
        rsync -a $REMOTE_PRODUCTS_DIR/ products/
        echo "Merged products from " $REMOTE_PRODUCTS_DIR " to " products
    else
        echo "Warning: $REMOTE_PRODUCTS_DIR is missing"
    fi
    # armv6l linux
    REMOTE_PRODUCTS_DIR=linux-armv6l_products
    if [ -d "$REMOTE_PRODUCTS_DIR" ]; then
        rsync -a $REMOTE_PRODUCTS_DIR/ products/
        echo "Merged products from " $REMOTE_PRODUCTS_DIR " to " products
    else
        echo "Warning: $REMOTE_PRODUCTS_DIR is missing"
    fi
    # 32/64-bit windows
    REMOTE_PRODUCTS_DIR=win_products
    if [ -d "$REMOTE_PRODUCTS_DIR" ]; then
        rsync -a $REMOTE_PRODUCTS_DIR/ products/
        echo "Merged products from " $REMOTE_PRODUCTS_DIR " to " products
    else
        echo "Warning: $REMOTE_PRODUCTS_DIR is missing"
    fi
}

# On hosts which are not the final packaging platform (e.g. armv7, armv6l, etc)
# we will rename the products directory so that we can merge() it at a later
# stage on the packaging platform
wrap()
{
    ARCH_WRAP_DIR=$OSNAME"-"$(uname -m)_products
    cp -rf lib $ARCH_WRAP_DIR
    echo "Copied products to:" $ARCH_WRAP_DIR
    PROD_FILENAME=$ARCH_WRAP_DIR.tar.gz
    tar --exclude=$PROD_FILENAME -zcvf $PROD_FILENAME -C $ARCH_WRAP_DIR .
}

# Renames and copies licenses for libzt and each of its dependencies
package_licenses()
{
    CURR_DIR=$1
    DEST_DIR=$2
    mkdir -p $DEST_DIR
    cp $CURR_DIR/ext/lwip/COPYING $DEST_DIR/LWIP-LICENSE.BSD
    cp $CURR_DIR/ext/concurrentqueue/LICENSE.md $DEST_DIR/CONCURRENTQUEUE-LICENSE.BSD
    cp $CURR_DIR/LICENSE.GPL-3 $DEST_DIR/ZEROTIER-LICENSE.GPL-3
    cp $CURR_DIR/include/net/ROUTE_H-LICENSE.APSL $DEST_DIR/ROUTE_H-LICENSE.APSL
    cp $CURR_DIR/include/net/ROUTE_H-LICENSE $DEST_DIR/ROUTE_H-LICENSE
}

# Copies binaries, documentation, licenses, etc into a products
# dir and then tarballs everything together
package_everything()
{
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    LIBZT_VERSION=$(git describe)
    PROD_NAME=$LIBZT_VERSION-$(date '+%Y%m%d_%H-%M')-$1
    PROD_DIR=$(pwd)/products/$PROD_NAME/
    # Make products directory
    # Licenses
    package_licenses $(pwd) $PROD_DIR/licenses
    # Documentation
    mkdir -p $PROD_DIR/doc
    # Copy the errno header from lwIP for customer reference
    cp ext/lwip/src/include/lwip/errno.h $PROD_DIR/doc
    cp $(pwd)/API.pdf $PROD_DIR/API.pdf
    # Header(s)
    mkdir -p $PROD_DIR/include
    cp $(pwd)/include/*.h $PROD_DIR/include
    cp $(pwd)/ext/ZeroTierOne/include/ZeroTierOne.h $PROD_DIR/include
    # Libraries
    mkdir -p $PROD_DIR/lib
    cp -r $(pwd)/lib/$1/* $PROD_DIR/lib
    # Clean
    find $PROD_DIR -type f \( -name '*.DS_Store' -o -name 'thumbs.db' \) -delete
    # Emit a README file
    echo 'See API.md for more information on how to use the SDK
- ZeroTier Manual: https://www.zerotier.com/manual.shtml
- libzt Manual: https://www.zerotier.com/manual.shtml#5
- libzt Repo: https://github.com/zerotier/libzt
- ZeroTierOne Repo: https://github.com/zerotier/ZeroTierOne
- Downloads: https://www.zerotier.com/download.shtml' > $PROD_DIR/README
    # Record the version (and each submodule's version)
    echo "$(git describe)" > $PROD_DIR/VERSION
    echo -e "$(git submodule status | awk '{$1=$1};1')" >> $PROD_DIR/VERSION
    echo -e "$(cat ext/ZeroTierOne/version.h | grep ZEROTIER_ONE_VERSION | sed 's/\#define//g' | awk '{$1=$1};1')" >> $PROD_DIR/VERSION
    echo "$(date)" >> $PROD_DIR/VERSION
    # Tar everything
    PROD_FILENAME=$(pwd)/products/$PROD_NAME.tar.gz
    tar --exclude=$PROD_FILENAME -zcvf $PROD_FILENAME -C $PROD_DIR .
    if [[ $OSNAME = *"darwin"* ]]; then
        md5 $PROD_FILENAME
    fi
    if [[ $OSNAME = *"linux"* ]]; then
        md5sum $PROD_FILENAME
    fi
    # Print results for post-build inspection
    echo -e "\n"
    tree $PROD_DIR
    cat $PROD_DIR/VERSION
    # Final check. Display warnings if anything is missing
    FILES="README
    VERSION
    API.pdf
    doc/errno.h
    licenses/LWIP-LICENSE.BSD
    licenses/CONCURRENTQUEUE-LICENSE.BSD
    licenses/ZEROTIER-LICENSE.GPL-3
    licenses/ROUTE_H-LICENSE.APSL
    licenses/ROUTE_H-LICENSE
    licenses/LWIP-LICENSE.BSD"
    for f in $FILES
    do
        if [ ! -f "$PROD_DIR$f" ]; then
            echo "Warning: $PROD_DIR$f is missing"
        fi
    done
}

# Generates a source-only tarball
sdist()
{
    VERSION=$(git describe --abbrev=0)
    TARBALL_DIR="libzt-${VERSION}"
    TARBALL_NAME=libzt-${VERSION}-source.tar.gz
    PROD_DIR=$(pwd)/products/
    mkdir -p $PROD_DIR
    #
    mkdir ${TARBALL_DIR}
    # primary sources
    cp -rf src ${TARBALL_DIR}/src
    cp -rf include ${TARBALL_DIR}/include
    # important build scripts
    cp Makefile ${TARBALL_DIR}
    cp CMakeLists.txt ${TARBALL_DIR}
    cp *.md ${TARBALL_DIR}
    cp *.sh ${TARBALL_DIR}
    cp *.bat ${TARBALL_DIR}
    # submodules/dependencies
    # lwIP
    mkdir ${TARBALL_DIR}/ext
    mkdir -p ${TARBALL_DIR}/ext/lwip/src
    cp -rf ext/lwip/src/api ${TARBALL_DIR}/ext/lwip/src
    cp -rf ext/lwip/src/core ${TARBALL_DIR}/ext/lwip/src
    cp -rf ext/lwip/src/include ${TARBALL_DIR}/ext/lwip/src
    cp -rf ext/lwip/src/netif ${TARBALL_DIR}/ext/lwip/src
    # lwIP ports
    mkdir -p ${TARBALL_DIR}/ext/lwip-contrib/ports
    cp -rf ext/lwip-contrib/ports/unix ${TARBALL_DIR}/ext/lwip-contrib/ports
    cp -rf ext/lwip-contrib/ports/win32 ${TARBALL_DIR}/ext/lwip-contrib/ports
    # ZeroTierOne
    mkdir ${TARBALL_DIR}/ext/ZeroTierOne
    cp -rf ext/ZeroTierOne/*.h ${TARBALL_DIR}/ext/ZeroTierOne
    cp -rf ext/ZeroTierOne/controller ${TARBALL_DIR}/ext/ZeroTierOne
    cp -rf ext/ZeroTierOne/ext ${TARBALL_DIR}/ext/ZeroTierOne
    cp -rf ext/ZeroTierOne/include ${TARBALL_DIR}/ext/ZeroTierOne
    cp -rf ext/ZeroTierOne/node ${TARBALL_DIR}/ext/ZeroTierOne
    cp -rf ext/ZeroTierOne/osdep ${TARBALL_DIR}/ext/ZeroTierOne
    #
    # Perform selective removal
    rm -rf ${TARBALL_DIR}/ext/ZeroTierOne/ext/bin
    rm -rf ${TARBALL_DIR}/ext/ZeroTierOne/ext/tap-mac
    rm -rf ${TARBALL_DIR}/ext/ZeroTierOne/ext/librethinkdbxx
    rm -rf ${TARBALL_DIR}/ext/ZeroTierOne/ext/installfiles
    rm -rf ${TARBALL_DIR}/ext/ZeroTierOne/ext/curl-*
    rm -rf ${TARBALL_DIR}/ext/ZeroTierOne/ext/http-parser
    #
    mkdir ${TARBALL_DIR}/ext/concurrentqueue
    cp -rf ext/concurrentqueue/*.h ${TARBALL_DIR}/ext/concurrentqueue
    # Licenses
    package_licenses $(pwd) $TARBALL_DIR/licenses
    # Tarball everything and display the results
    tar -cvf ${TARBALL_NAME} ${TARBALL_DIR}
    tree ${TARBALL_DIR}
    rm -rf ${TARBALL_DIR}
    mv ${TARBALL_NAME} ${PROD_DIR}
}

# Package both debug and release
bdist()
{
    echo "Executing task: " ${FUNCNAME[ 0 ]} "(" $1 ")"
    package_everything "debug"
    package_everything "release"
}

# Generate a markdown CHANGELOG from git-log
update_changelog()
{
    first_commit=$(git rev-list --max-parents=0 HEAD)
    git for-each-ref --sort=-refname --format="## [%(refname:short)] - %(taggerdate:short) &(newline)*** &(newline)- %(subject) %(body)" refs/tags > CHANGELOG.md
    gsed -i '''s/\&(newline)/\n/' CHANGELOG.md # replace first instance
    gsed -i '''s/\&(newline)/\n/' CHANGELOG.md # replace second instance
    echo -e "\n" >> CHANGELOG.md
    for curr_tag in $(git tag -l --sort=-v:refname)
    do
        prev_tag=$(git describe --abbrev=0 ${curr_tag}^)
        if [ -z "${prev_tag}" ]
        then
            prev_tag=${first_commit}
        fi
        echo "[${curr_tag}]: https://github.com/zerotier/libzt/compare/${prev_tag}..${curr_tag}" >> CHANGELOG.md
    done
}

# List all functions in this script (just for convenience)
list()
{
    IFS=$'\n'
    for f in $(declare -F); do
        echo "${f:11}"
    done
}

"$@"
