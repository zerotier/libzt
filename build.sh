#!/bin/bash

set -euo pipefail

# -----------------------------------------------------------------------------
# | SYSTEM DISCOVERY AND CONFIGURATION                                        |
# -----------------------------------------------------------------------------

check_submodules()
{
    if [ "$(ls -A ext/lwip)" ] && [ "$(ls -A ext/lwip-contrib)" ] && [ "$(ls -A ext/ZeroTierOne)" ]; then
        :
    else
        echo "Submodules seem to be missing. Please run: git submodule update --init"
        exit 1
    fi
}

CLANG_FORMAT=clang-format-11

PYTHON=python3
PIP=pip3

# Run from the script's own directory so relative paths behave no matter
# where the script is invoked from
libzt="$(cd "$(dirname "$0")" && pwd)"
cd "$libzt"

# Find and set cmake
CMAKE=cmake3
if [[ $(which $CMAKE) = "" ]];
then
    CMAKE=cmake # try this next
fi
if [[ $(which $CMAKE) = "" ]];
then
    echo "CMake (cmake) not found. Please install before continuing."
    exit 1
fi

#
if [[ ! $(which tree) = "" ]];
then
    TREE=tree
else
    TREE="du -a "
fi

# Determine operating system
OSNAME=$(uname | tr '[A-Z]' '[a-z]')
# Defaults, overridden below (explicitly initialized for `set -u`)
SHARED_LIB_NAME="libzt.so"
STATIC_LIB_NAME="libzt.a"
HOST_PLATFORM="unknown"
if [[ $OSNAME = *"darwin"* ]]; then
    SHARED_LIB_NAME="libzt.dylib"
    STATIC_LIB_NAME="libzt.a"
    HOST_PLATFORM="macos"
fi
if [[ $OSNAME = *"linux"* ]]; then
    SHARED_LIB_NAME="libzt.so"
    STATIC_LIB_NAME="libzt.a"
    HOST_PLATFORM="linux"
fi

# Determine and normalize machine type
HOST_MACHINE_TYPE=$(uname -m)
if [[ $HOST_MACHINE_TYPE = *"x86_64"* ]]; then
    HOST_MACHINE_TYPE="x64"
fi

# Determine number of cores. We'll tell CMake to use them all
N_PROCESSORS=1
if [[ $OSNAME = *"darwin"* ]]; then
    N_PROCESSORS=$(sysctl -n hw.ncpu)
fi
if [[ $OSNAME = *"linux"* ]]; then
    N_PROCESSORS=$(nproc --all)
fi

# How many processor cores CMake should use during builds,
# comment out the below line out if you don't want parallelism:
CMAKE_VERSION=$("$CMAKE" --version | head -n 1 | sed 's/[^0-9]*//')
function ver()
# Description: use for comparisons of version strings.
# $1  : a version string of form 1.2.3.4 (any non-numeric suffix
#       such as "-rc1" is ignored)
# use: (( 10#$(ver 1.2.3.4) >= 10#$(ver 1.2.3.3) )) && echo "yes" || echo "no"
# Clever solution from https://stackoverflow.com/users/10682202/christopher
{
    local v="$1"
    # Strip anything from the first character that is not a digit or a dot
    # (e.g. "3.28.3-rc1" -> "3.28.3") so printf only sees numbers
    v="${v%%[!0-9.]*}"
    printf "%02d%02d%02d%02d" ${v//./ }
}
BUILD_CONCURRENCY=""
# 10# forces base-10 so leading zeros aren't interpreted as octal
if (( 10#$(ver "$CMAKE_VERSION") > 10#$(ver "3.12") )); then
    BUILD_CONCURRENCY="-j $N_PROCESSORS"
fi

# -----------------------------------------------------------------------------
# | PATHS                                                                     |
# -----------------------------------------------------------------------------

# Where we place all finished artifacts
BUILD_OUTPUT_DIR=$libzt/dist
# Where we tell CMake to place its build systems and their caches
BUILD_CACHE_DIR=$libzt/cache
# Where package projects, scripts, spec files, etc live
PKG_DIR=$libzt/pkg
# Default location for (host) libraries
DEFAULT_HOST_LIB_OUTPUT_DIR=$BUILD_OUTPUT_DIR/$HOST_PLATFORM-$HOST_MACHINE_TYPE
# Default location for (host) binaries
DEFAULT_HOST_BIN_OUTPUT_DIR=$BUILD_OUTPUT_DIR/$HOST_PLATFORM-$HOST_MACHINE_TYPE
# Default location for (host) packages
DEFAULT_HOST_PKG_OUTPUT_DIR=$BUILD_OUTPUT_DIR/$HOST_PLATFORM-$HOST_MACHINE_TYPE
# Default location for CMake's caches (when building for host)
DEFAULT_HOST_BUILD_CACHE_DIR=$BUILD_CACHE_DIR/$HOST_PLATFORM-$HOST_MACHINE_TYPE

gethosttype()
{
    echo $HOST_PLATFORM-$HOST_MACHINE_TYPE
}

# -----------------------------------------------------------------------------
# | TARGETS                                                                   |
# -----------------------------------------------------------------------------

# Build xcframework
#
# ./build.sh xcframework "debug"
#
# Example output:
#
#    - Cache        : libzt/cache/apple-xcframework-debug
#    - Build output : libzt/dist
#
# apple-xcframework-debug
# └── pkg
#     └── zt.xcframework
#         ├── Info.plist
#         ├── ios-arm64
#         │   └── zt.framework
#         │       └── ...
#         ├── ios-arm64_x86_64-simulator
#         │   └── zt.framework
#         │       └── ...
#         └── macos-arm64_x86_64
#             └── zt.framework
#                 └── ...
#
xcframework()
{
    check_submodules
    if [[ ! $OSNAME = *"darwin"* ]]; then
        echo "Can only build this on a Mac"
        exit 1
    fi
    BUILD_TYPE=${1:-release}
    UPPERCASE_BUILD_TYPE="$(tr '[:lower:]' '[:upper:]' <<< ${BUILD_TYPE:0:1})${BUILD_TYPE:1}"

    # Build all frameworks
    macos-framework $BUILD_TYPE
    iphoneos-framework $BUILD_TYPE
    iphonesimulator-framework $BUILD_TYPE

    ARTIFACT="xcframework"
    TARGET_PLATFORM="apple"
    TARGET_BUILD_DIR=$BUILD_OUTPUT_DIR/$TARGET_PLATFORM-$ARTIFACT-$BUILD_TYPE
    rm -rf $TARGET_BUILD_DIR
    PKG_OUTPUT_DIR=$TARGET_BUILD_DIR/pkg
    mkdir -p $PKG_OUTPUT_DIR

    MACOS_FRAMEWORK_DIR=macos-$HOST_MACHINE_TYPE-framework-$BUILD_TYPE
    IOS_FRAMEWORK_DIR=iphoneos-arm64-framework-$BUILD_TYPE
    IOS_SIM_FRAMEWORK_DIR=iphonesimulator-$HOST_MACHINE_TYPE-framework-$BUILD_TYPE

    # Pack everything
    rm -rf $PKG_OUTPUT_DIR/zt.xcframework # Remove prior to move to prevent error
    xcodebuild -create-xcframework \
        -framework $BUILD_CACHE_DIR/$MACOS_FRAMEWORK_DIR/lib/$UPPERCASE_BUILD_TYPE/zt.framework \
        -framework $BUILD_CACHE_DIR/$IOS_FRAMEWORK_DIR/lib/$UPPERCASE_BUILD_TYPE/zt.framework \
        -framework $BUILD_CACHE_DIR/$IOS_SIM_FRAMEWORK_DIR/lib/$UPPERCASE_BUILD_TYPE/zt.framework \
        -output $PKG_OUTPUT_DIR/zt.xcframework
}

# Build iOS framework
#
# ./build.sh iphonesimulator-framework "debug"
#
# Example output:
#
#    - Cache        : libzt/cache/iphonesimulator-x64-framework-debug
#    - Build output : libzt/dist
#
# libzt/dist/iphonesimulator-x64-framework-debug
# └── pkg
#     └── zt.framework
#         ├── Headers
#         │   └── ZeroTierSockets.h
#         ├── Info.plist
#         ├── Modules
#         │   └── module.modulemap
#         └── zt
#
iphonesimulator-framework()
{
    check_submodules
    if [[ ! $OSNAME = *"darwin"* ]]; then
        echo "Can only build this on a Mac"
        exit 1
    fi
    ARTIFACT="framework"
    BUILD_TYPE=${1:-Release}
    UPPERCASE_BUILD_TYPE="$(tr '[:lower:]' '[:upper:]' <<< ${BUILD_TYPE:0:1})${BUILD_TYPE:1}"
    VARIANT="-DBUILD_IOS_FRAMEWORK=True"
    TARGET_PLATFORM="iphonesimulator"
    TARGET_MACHINE_TYPE=$HOST_MACHINE_TYPE
    CACHE_DIR=$BUILD_CACHE_DIR/$TARGET_PLATFORM-$TARGET_MACHINE_TYPE-$ARTIFACT-$BUILD_TYPE
    TARGET_BUILD_DIR=$BUILD_OUTPUT_DIR/$TARGET_PLATFORM-$TARGET_MACHINE_TYPE-$ARTIFACT-$BUILD_TYPE
    rm -rf $TARGET_BUILD_DIR
    PKG_OUTPUT_DIR=$TARGET_BUILD_DIR/pkg
    mkdir -p $PKG_OUTPUT_DIR
    # Generate project
    mkdir -p $CACHE_DIR
    cd $CACHE_DIR
    # iOS (SDK 11+, 64-bit only, arm64)
    $CMAKE -G Xcode ../../ $VARIANT
    # Build framework
    xcodebuild -target zt -configuration "$UPPERCASE_BUILD_TYPE" -sdk "iphonesimulator"
    cd -
    cp -rf $CACHE_DIR/lib/$UPPERCASE_BUILD_TYPE/*.framework $PKG_OUTPUT_DIR
    echo -e "\n - Build cache  : $CACHE_DIR\n - Build output : $BUILD_OUTPUT_DIR\n"
    $TREE $TARGET_BUILD_DIR
}

# Build macOS framework
#
# ./build.sh macos-framework "debug"
#
# Example output:
#
#    - Cache        : libzt/cache/macos-x64-framework-debug
#    - Build output : libzt/dist
#
# libzt/dist/macos-x64-framework-debug
# └── pkg
#     └── zt.framework
#         ├── Headers
#         │   └── ZeroTierSockets.h
#         ├── Info.plist
#         ├── Modules
#         │   └── module.modulemap
#         └── zt
#
macos-framework()
{
    check_submodules
    if [[ ! $OSNAME = *"darwin"* ]]; then
        echo "Can only build this on a Mac"
        exit 1
    fi
    ARTIFACT="framework"
    BUILD_TYPE=${1:-Release}
    UPPERCASE_BUILD_TYPE="$(tr '[:lower:]' '[:upper:]' <<< ${BUILD_TYPE:0:1})${BUILD_TYPE:1}"
    VARIANT="-DBUILD_MACOS_FRAMEWORK=True"
    CACHE_DIR=$DEFAULT_HOST_BUILD_CACHE_DIR-$ARTIFACT-$BUILD_TYPE
    TARGET_BUILD_DIR=$DEFAULT_HOST_BIN_OUTPUT_DIR-$ARTIFACT-$BUILD_TYPE
    rm -rf $TARGET_BUILD_DIR
    PKG_OUTPUT_DIR=$TARGET_BUILD_DIR/pkg
    mkdir -p $PKG_OUTPUT_DIR
    # Generate project
    mkdir -p $CACHE_DIR
    cd $CACHE_DIR
    $CMAKE -G Xcode ../../ $VARIANT
    # Build framework
    xcodebuild -target zt -configuration $UPPERCASE_BUILD_TYPE -sdk "macosx"
    cd -
    cp -rf $CACHE_DIR/lib/$UPPERCASE_BUILD_TYPE/*.framework $PKG_OUTPUT_DIR
    echo -e "\n - Build cache  : $CACHE_DIR\n - Build output : $BUILD_OUTPUT_DIR\n"
    $TREE $TARGET_BUILD_DIR
}

# Build iOS framework
#
# ./build.sh iphoneos-framework "debug"
#
# Example output:
#
#    - Cache        : libzt/cache/iphoneos-arm64-framework-debug
#    - Build output : libzt/dist
#
# libzt/dist/iphoneos-arm64-framework-debug
# └── pkg
#     └── zt.framework
#         ├── Headers
#         │   └── ZeroTierSockets.h
#         ├── Info.plist
#         ├── Modules
#         │   └── module.modulemap
#         └── zt
#
iphoneos-framework()
{
    check_submodules
    if [[ ! $OSNAME = *"darwin"* ]]; then
        echo "Can only build this on a Mac"
        exit 1
    fi
    ARTIFACT="framework"
    BUILD_TYPE=${1:-Release}
    UPPERCASE_BUILD_TYPE="$(tr '[:lower:]' '[:upper:]' <<< ${BUILD_TYPE:0:1})${BUILD_TYPE:1}"
    VARIANT="-DBUILD_IOS_FRAMEWORK=True -DIOS_ARM64=True"
    TARGET_PLATFORM="iphoneos"
    TARGET_MACHINE_TYPE=arm64
    CACHE_DIR=$BUILD_CACHE_DIR/$TARGET_PLATFORM-$TARGET_MACHINE_TYPE-$ARTIFACT-$BUILD_TYPE
    TARGET_BUILD_DIR=$BUILD_OUTPUT_DIR/$TARGET_PLATFORM-$TARGET_MACHINE_TYPE-$ARTIFACT-$BUILD_TYPE
    rm -rf $TARGET_BUILD_DIR
    PKG_OUTPUT_DIR=$TARGET_BUILD_DIR/pkg
    mkdir -p $PKG_OUTPUT_DIR
    # Generate project
    mkdir -p $CACHE_DIR
    cd $CACHE_DIR
    # iOS (SDK 11+, 64-bit only, arm64)
    $CMAKE -G Xcode ../../ $VARIANT
    sed -i '' 's/x86_64/$(CURRENT_ARCH)/g' zt.xcodeproj/project.pbxproj
    # Build framework
    xcodebuild -arch $TARGET_MACHINE_TYPE -target zt -configuration "$UPPERCASE_BUILD_TYPE" -sdk "iphoneos"
    cd -
    cp -rvf $CACHE_DIR/lib/$UPPERCASE_BUILD_TYPE/*.framework $PKG_OUTPUT_DIR
    echo -e "\n - Build cache  : $CACHE_DIR\n - Build output : $BUILD_OUTPUT_DIR\n"
    $TREE $TARGET_BUILD_DIR
}

# Build standard libraries, examples, and selftest
#
# ./build.sh host "release"
#
# Example output:
#
#    - Cache        : libzt/cache/linux-x64-host-release
#    - Build output : libzt/dist
#
# linux-x64-host-release
# ├── bin
# │   ├── client
# │   └── server
# └── lib
#     ├── libzt.a
#     └── libzt.so # .dylib, .dll
#
host()
{
    check_submodules
    ARTIFACT="host"
    # Default to release
    BUILD_TYPE=${1:-release}
    VARIANT="-DBUILD_HOST=True"
    CACHE_DIR=$DEFAULT_HOST_BUILD_CACHE_DIR-$ARTIFACT-$BUILD_TYPE
    TARGET_BUILD_DIR=$DEFAULT_HOST_BIN_OUTPUT_DIR-$ARTIFACT-$BUILD_TYPE
    rm -rf $TARGET_BUILD_DIR
    LIB_OUTPUT_DIR=$TARGET_BUILD_DIR/lib
    BIN_OUTPUT_DIR=$TARGET_BUILD_DIR/bin
    mkdir -p $LIB_OUTPUT_DIR
    mkdir -p $BIN_OUTPUT_DIR
    $CMAKE $VARIANT -S "$libzt" -B "$CACHE_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE
    $CMAKE --build $CACHE_DIR $BUILD_CONCURRENCY
    rm -f $BUILD_OUTPUT_DIR/native
    ln -s $TARGET_BUILD_DIR $BUILD_OUTPUT_DIR/native
    cp -f $CACHE_DIR/lib/libzt.* $LIB_OUTPUT_DIR
    cp -f $CACHE_DIR/bin/* $BIN_OUTPUT_DIR
    echo -e "\n - Build cache  : $CACHE_DIR\n - Build output : $BUILD_OUTPUT_DIR\n"
    $TREE $TARGET_BUILD_DIR
}
host-install()
{
    if [[ -z "${1:-}" ]]; then
        echo "Usage: ./build.sh host-install <build_type> (e.g. release, debug)" >&2
        exit 1
    fi
    cd "$BUILD_CACHE_DIR/$HOST_PLATFORM-$HOST_MACHINE_TYPE-host-$1" || {
        echo "Build cache directory not found. Run: ./build.sh host \"$1\" first." >&2
        exit 1
    }
    make install
    cd -
}
host-uninstall()
{
    if [[ -z "${1:-}" ]]; then
        echo "Usage: ./build.sh host-uninstall <build_type> (e.g. release, debug)" >&2
        exit 1
    fi
    cd "$BUILD_CACHE_DIR/$HOST_PLATFORM-$HOST_MACHINE_TYPE-host-$1" || {
        echo "Build cache directory not found. Nothing to uninstall." >&2
        exit 1
    }
    xargs rm < install_manifest.txt
    cd -
}

# Build rust crate, libzt.a, link and test
#
host-rust()
{
    BUILD_TYPE=${1:-debug}

    cd pkg/crate/libzt
    cargo build # --verbose

    # Test Rust crate
    if [[ ${2:-} = *"test"* ]]; then
        cargo run --example libzt-test-app -- server $alice_path $testnet 0.0.0.0 $port4 &
        cargo run --example libzt-test-app -- client $bob_path $testnet $alice_ip4 $port4 &
    fi
}

# Build C extension module (*.so), python module, package both into wheel
#
# ./build.sh host-python-wheel "release"
#
# Example output:
#
# libzt/dist/macos-x64-python-debug
# └── pkg
#     └── libzt-1.3.4b1-cp39-cp39-macosx_11_0_x86_64.whl
#
host-python()
{
    # NOTE: This target is currently non-functional. It used to invoke
    # pkg/pypi/build.sh, but that script no longer exists. pkg/pypi now
    # contains a Poetry-based project (build.py, setup.py, pyproject.toml)
    # and the wheel-building invocation has not been wired up here yet.
    echo "ERROR: The host-python target is currently broken." >&2
    echo "pkg/pypi/build.sh does not exist. pkg/pypi contains a Poetry-based" >&2
    echo "project (build.py, setup.py, pyproject.toml); build the wheel from" >&2
    echo "pkg/pypi with your Python packaging tooling instead." >&2
    exit 1

    check_submodules
    ARTIFACT="python"
    # Default to release
    BUILD_TYPE=${1:-release}
    CACHE_DIR=$DEFAULT_HOST_BUILD_CACHE_DIR-$ARTIFACT-$BUILD_TYPE
    TARGET_BUILD_DIR=$DEFAULT_HOST_BIN_OUTPUT_DIR-$ARTIFACT-$BUILD_TYPE
    PKG_OUTPUT_DIR=$TARGET_BUILD_DIR/pkg
    mkdir -p $PKG_OUTPUT_DIR
    # Generate new wrapper
    #swig -c++ -python -o src/bindings/python/zt_wrap.cxx -Iinclude src/bindings/python/zt.i
    # Requires setuptools, etc
    cd pkg/pypi && ./build.sh wheel && cp -f dist/*.whl $PKG_OUTPUT_DIR
    echo -e "\nFinished wheel:\n"
    echo $PKG_OUTPUT_DIR/*.whl

    # Test Python wheel
    if [[ ${2:-} = *"test"* ]]; then
        if [[ -z "${alice_path:-}" ]]; then
            echo "Please set necessary environment variables for test"
            exit 1
        fi
        pip3 uninstall -y libzt
        pip3 install $PKG_OUTPUT_DIR/*.whl
        cd $libzt
        $PYTHON test/selftest.py server $alice_path $testnet $port4 &
        $PYTHON test/selftest.py client $bob_path $testnet $alice_ip4 $port4 &
    fi
}

# Build shared library with python wrapper symbols exported
#host-python()
#{
#    ARTIFACT="python"
#    # Default to release
#    BUILD_TYPE=${1:-release}
#    VARIANT="-DZTS_ENABLE_PYTHON=True"
#    CACHE_DIR=$DEFAULT_HOST_BUILD_CACHE_DIR-$ARTIFACT-$BUILD_TYPE
#    TARGET_BUILD_DIR=$DEFAULT_HOST_BIN_OUTPUT_DIR-$ARTIFACT-$BUILD_TYPE
#    LIB_OUTPUT_DIR=$TARGET_BUILD_DIR/lib
#    rm -rf $LIB_OUTPUT_DIR
#    mkdir -p $LIB_OUTPUT_DIR
#    # Optional step to generate new SWIG wrapper
#    swig -c++ -python -o src/bindings/python/zt_wrap.cxx -Iinclude src/bindings/python/zt.i
#    $CMAKE $VARIANT -S "$libzt" -B "$CACHE_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE
#    $CMAKE --build $CACHE_DIR $BUILD_CONCURRENCY
#    cp -f $CACHE_DIR/lib/$SHARED_LIB_NAME $LIB_OUTPUT_DIR/_libzt.so
#    echo -e "\n - Build cache  : $CACHE_DIR\n - Build output : $BUILD_OUTPUT_DIR\n"
#    $TREE $TARGET_BUILD_DIR
#}

# Build shared library with P/INVOKE wrapper symbols exported
host-pinvoke()
{
    check_submodules
    ARTIFACT="pinvoke"
    # Default to release
    BUILD_TYPE=${1:-release}
    VARIANT="-DZTS_ENABLE_PINVOKE=True"
    CACHE_DIR=$DEFAULT_HOST_BUILD_CACHE_DIR-$ARTIFACT-$BUILD_TYPE
    TARGET_BUILD_DIR=$DEFAULT_HOST_BIN_OUTPUT_DIR-$ARTIFACT-$BUILD_TYPE
    rm -rf $TARGET_BUILD_DIR
    LIB_OUTPUT_DIR=$TARGET_BUILD_DIR/lib
    BIN_OUTPUT_DIR=$TARGET_BUILD_DIR/bin
    mkdir -p $LIB_OUTPUT_DIR
    $CMAKE $VARIANT -S "$libzt" -B "$CACHE_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE
    $CMAKE --build $CACHE_DIR $BUILD_CONCURRENCY
    cp -f $CACHE_DIR/lib/libzt.* $LIB_OUTPUT_DIR
    echo -e "\n - Build cache  : $CACHE_DIR\n - Build output : $BUILD_OUTPUT_DIR\n"
    $TREE $TARGET_BUILD_DIR

    # Test C#
    if [[ ${2:-} = *"test"* ]]; then
        if [[ -z "${alice_path:-}" ]]; then
            echo "Please set necessary environment variables for test"
            exit 1
        fi
        # TODO: This should eventually be converted to a proper dotnet project
        # Build C# managed API library
        # -doc:$LIB_OUTPUT_DIR/ZeroTier.Sockets.xml
        csc -target:library -out:$LIB_OUTPUT_DIR/ZeroTier.Sockets.dll src/bindings/csharp/*.cs
        # Build selftest
        mkdir -p $BIN_OUTPUT_DIR
        csc -out:$BIN_OUTPUT_DIR/selftest.exe -reference:$LIB_OUTPUT_DIR/ZeroTier.Sockets.dll test/selftest.cs
        # Copy shared libraries into bin directory so they can be discovered by dlopen
        cp $LIB_OUTPUT_DIR/* $BIN_OUTPUT_DIR
        # Start Alice as server
        MONO_THREADS_SUSPEND=preemptive; mono --debug "$BIN_OUTPUT_DIR/selftest.exe" server $alice_path $testnet $port4 &
        sleep 3
        # Start Bob as client
        MONO_THREADS_SUSPEND=preemptive; mono --debug "$BIN_OUTPUT_DIR/selftest.exe" client $bob_path $testnet $alice_ip4 $port4 &
    fi
}

# Build shared library with Java JNI wrapper symbols exported (.jar)
host-jar()
{
    check_submodules
    ARTIFACT="jar"
    PKG_VERSION=$(git describe --tags --abbrev=0)
    # Default to release
    BUILD_TYPE=${1:-release}
    if [[ ${1:-} = *"docs"* ]]; then
        # Generate documentation
        javadoc src/bindings/java/com/zerotier/sockets/*.java -d docs/java
        exit 0
    fi
    VARIANT="-DZTS_ENABLE_JAVA=True"
    CACHE_DIR=$DEFAULT_HOST_BUILD_CACHE_DIR-$ARTIFACT-$BUILD_TYPE
    TARGET_BUILD_DIR=$DEFAULT_HOST_BIN_OUTPUT_DIR-$ARTIFACT-$BUILD_TYPE
    rm -rf $TARGET_BUILD_DIR
    PKG_OUTPUT_DIR=$TARGET_BUILD_DIR/pkg
    mkdir -p $PKG_OUTPUT_DIR
    # Share same cache dir with CMake
    JAVA_JAR_DIR=$CACHE_DIR/pkg/jar
    JAVA_JAR_SOURCE_TREE_DIR=$JAVA_JAR_DIR/com/zerotier/sockets/
    mkdir -p $JAVA_JAR_SOURCE_TREE_DIR
    cp -f src/bindings/java/com/zerotier/sockets/*.java $JAVA_JAR_SOURCE_TREE_DIR
    # Build
    $CMAKE $VARIANT -S "$libzt" -B "$CACHE_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE
    $CMAKE --build $CACHE_DIR $BUILD_CONCURRENCY
    # Package everything
    cp -f $CACHE_DIR/lib/libzt.* $JAVA_JAR_DIR
    cd $JAVA_JAR_DIR
    export JAVA_TOOL_OPTIONS=-Dfile.encoding=UTF8
    javac -Xlint:all com/zerotier/sockets/*.java

    jar cf libzt-$PKG_VERSION.jar $SHARED_LIB_NAME com/zerotier/sockets/*.class
    rm -rf com $SHARED_LIB_NAME
    cd -
    # Copy JAR to dist/
    echo -e "\nContents of JAR:\n"
    jar tf $JAVA_JAR_DIR/*.jar
    echo -e
    mv $JAVA_JAR_DIR/*.jar $PKG_OUTPUT_DIR
    echo -e "\n - Build cache  : $CACHE_DIR\n - Build output : $BUILD_OUTPUT_DIR\n"
    $TREE $TARGET_BUILD_DIR

    # Test JAR
    if [[ ${2:-} = *"test"* ]]; then
        if [[ -z "${alice_path:-}" ]]; then
            echo "Please set necessary environment variables for test"
            exit 1
        fi
        cd test
        rm -rf *.dylib
        rm -rf *.jar
        cp -f $PKG_OUTPUT_DIR/*.jar .
        # Unpack JAR to get access to shared library
        jar xf *.jar libzt.dylib
        javac -cp *.jar selftest.java
        # Start Alice as server
        java -cp ".:libzt-"$(git describe --abbrev=0)".jar" selftest server $alice_path $testnet $port4 &
        # Start Bob as client
        java -cp ".:libzt-"$(git describe --abbrev=0)".jar" selftest client $bob_path $testnet $alice_ip4 $port4 &
    fi
}


# -----------------------------------------------------------------------------
# | ANDROID CONFIG                                                            |
# -----------------------------------------------------------------------------

ANDROID_PKG_PROJ_DIR=$libzt/pkg/android

#export PATH=/Library/Java/JavaVirtualMachines/$JDK/Contents/Home/bin/:${PATH}
#export PATH=/Users/$USER/Library/Android/sdk/platform-tools/:${PATH}
GRADLE_ARGS=--stacktrace

# Set ANDROID_HOME because setting sdk.dir in local.properties isn't always
# reliable. Only sets a default when ANDROID_HOME isn't already set, and only
# when building for android (see android-aar), so a pre-existing correct
# value is never clobbered.
set-android-home()
{
    if [[ -z "${ANDROID_HOME:-}" ]]; then
        if [[ $OSNAME = *"linux"* ]]; then
            export ANDROID_HOME=/usr/lib/android-sdk/
        fi
        if [[ $OSNAME = *"darwin"* ]]; then
            export ANDROID_HOME=/Users/$USER/Library/Android/sdk
        fi
    fi
}

# Build shared library with Java JNI wrapper symbols exported (.aar)
#
# ./build.sh android-aar "release"
#
# Example output:
#
#    - Cache        : libzt/cache/android-any-android-release
#    - Build output : libzt/dist
#
# android-any-android-release
# └── libzt-release.aar
#
android-aar()
{
    check_submodules
    set-android-home
    ARTIFACT="android"
    BUILD_TYPE=${1:-release} # Default to release
    TARGET_PLATFORM="android"
    TARGET_MACHINE_TYPE=any
    CACHE_DIR=$BUILD_CACHE_DIR/$TARGET_PLATFORM-$TARGET_MACHINE_TYPE-$ARTIFACT-$BUILD_TYPE
    PKG_OUTPUT_DIR=$BUILD_OUTPUT_DIR/$TARGET_PLATFORM-$TARGET_MACHINE_TYPE-$ARTIFACT-$BUILD_TYPE
    mkdir -p $CACHE_DIR
    mkdir -p $PKG_OUTPUT_DIR
    # Build
    UPPERCASE_BUILD_TYPE="$(tr '[:lower:]' '[:upper:]' <<< ${BUILD_TYPE:0:1})${BUILD_TYPE:1}"
    cd $ANDROID_PKG_PROJ_DIR
    ./gradlew $GRADLE_ARGS assemble$UPPERCASE_BUILD_TYPE # assembleRelease / assembleDebug
    cp $ANDROID_PKG_PROJ_DIR/app/build/outputs/aar/libzt-$BUILD_TYPE.aar \
        $PKG_OUTPUT_DIR/libzt-$BUILD_TYPE.aar
    cd -
    echo -e "\n - Build cache  : $CACHE_DIR\n - Build output : $BUILD_OUTPUT_DIR\n"
    $TREE $PKG_OUTPUT_DIR
}

# Build static library and selftest. Currently this only tests
# the core C API, not any of the language bindings.
# (Named run_tests rather than test so it doesn't shadow the shell builtin.)
run_tests()
{
    check_submodules
    ARTIFACT="test"
    # Default to release
    BUILD_TYPE=${1:-release}
    VARIANT="-DBUILD_HOST_SELFTEST_ONLY=True"
    CACHE_DIR=$DEFAULT_HOST_BUILD_CACHE_DIR-$ARTIFACT-$BUILD_TYPE
    TARGET_BUILD_DIR=$DEFAULT_HOST_BIN_OUTPUT_DIR-$ARTIFACT-$BUILD_TYPE
    rm -rf $TARGET_BUILD_DIR
    LIB_OUTPUT_DIR=$TARGET_BUILD_DIR/lib
    BIN_OUTPUT_DIR=$TARGET_BUILD_DIR/bin
    mkdir -p $BIN_OUTPUT_DIR
    $CMAKE $VARIANT -S "$libzt" -B "$CACHE_DIR" -DCMAKE_BUILD_TYPE=$BUILD_TYPE
    $CMAKE --build $CACHE_DIR $BUILD_CONCURRENCY
    cp -f $CACHE_DIR/bin/* $BIN_OUTPUT_DIR
    echo -e "\n - Build cache  : $CACHE_DIR\n - Build output : $BUILD_OUTPUT_DIR\n"
    $TREE $TARGET_BUILD_DIR
    # Test
    cd $CACHE_DIR
    #ctest -C release
    cd -
}

format-code()
{
    if [[ ${1:-} = *"all"* ]]; then
        format-code "clang"
        format-code "python"
    fi

    # Clang-format
    if [[ ${1:-} = *"clang"* ]]; then
        if [[ ! $(which $CLANG_FORMAT) = "" ]];
        then
            # Eventually: find . -path ./ext -prune -false -o -type f \( -iname \*.c -o -iname \*.h -o -iname \*.cpp -o -iname \*.hpp \) -exec clang-format -i {} \;
            $CLANG_FORMAT -i --verbose include/*.h                        \
                            src/*.c                                       \
                            src/*.cpp                                     \
                            src/*.hpp                                     \
                            examples/c/*.c                                \
                            examples/csharp/*.cs                          \
                            examples/java/*.java                          \
                            test/*.c                                      \
                            test/*.cs                                     \
                            src/bindings/csharp/*.cs                      \
                            src/bindings/csharp/*.cxx                     \
                            src/bindings/java/com/zerotier/sockets/*.java \
                            src/bindings/java/*.cxx                       \
                            examples/csharp/*.cs                          \
                            src/bindings/python/PythonSockets.cxx         \
                            src/bindings/python/PythonSocket.h
            return 0
        else
            echo "Please install clang-format"
        fi
    fi
    # Python
    if [[ ${1:-} = *"python"* ]]; then
        if [[ ! $($PIP list | grep black) = "" ]];
        then
            $PYTHON -m black src/bindings/python/libzt.py
            $PYTHON -m black src/bindings/python/node.py
            $PYTHON -m black src/bindings/python/select.py
            $PYTHON -m black src/bindings/python/sockets.py
            $PYTHON -m black examples/python/
            $PYTHON -m black test/selftest.py
        else
            echo "Please install python module (black)"
        fi
    fi
}

# Test C API
test-c()
{
    if [[ -z "${alice_path:-}" ]]; then
        echo "Please set necessary environment variables for test"
        #exit 0
    fi
    BUILD_TYPE=${1:-debug}
    #BUILD_TYPE="ASAN"
    # Build selftest
    run_tests $BUILD_TYPE
    # Start Alice as server
    "$BIN_OUTPUT_DIR/selftest-c" $alice_path $testnet $port4 $port6 &
    # Start Bob as client
    "$BIN_OUTPUT_DIR/selftest-c" $bob_path $testnet $port4 $alice_ip4 $port6 $alice_ip6 &
}

# Recursive deep clean
clean()
{
    # Clean rust crate
    (cd pkg/crate/libzt && cargo clean) \
        || echo "Warning: could not clean rust crate (is cargo installed?)"
    # Finished artifacts
    rm -rf $BUILD_OUTPUT_DIR
    # CMake's build system cache
    rm -rf $BUILD_CACHE_DIR
    # CMake test output
    rm -rf bin
    rm -rf Testing
    rm -rf CMakeFiles
    rm -rf *.cmake
    rm -rf CMakeCache.txt
    rm -rf Makefile
    # Android AAR project binaries and sources (copied from src/bindings/java)
    rm -rf $ANDROID_PKG_PROJ_DIR/app/build
    rm -rf $ANDROID_PKG_PROJ_DIR/app/src/main/java/com/zerotier/libzt/*.java
    rm -rf $ANDROID_PKG_PROJ_DIR/app/.externalNativeBuild
    # Remove whatever remains (never descend into the submodules in ext/ or
    # into .git, so prebuilt artifacts shipped with the submodules survive)
    find . \
        \( -path ./ext -o -path ./.git \) -prune -o \
        \( -name '*.dylib' \
        -o -name '*.dll'   \
        -o -name '*.aar'   \
        -o -name '*.jar'   \
        -o -name '*.so'    \
        -o -name '*.a'     \
        -o -name '*.o'     \
        -o -name '*.exe'   \
        -o -name '*.o.d'   \
        -o -name '*.out'   \
        -o -name '*.log'   \
        -o -name '*.dSYM'  \
        -o -name '*.class' \
        \) -exec rm -rf {} +

    find . \
        \( -path ./ext -o -path ./.git \) -prune -o \
        -type d -name "__pycache__" -prune -exec rm -rf {} +
    # Python pkg
    # cd pkg/pypi && ./build.sh clean
}

tag_release()
{
    git tag -a $1 -m $2
    git push origin --tags
}

# Internal helpers that are functions but not build targets. Listing them would
# advertise them as things the user can invoke.
INTERNAL_FUNCS="check_submodules gethosttype set-android-home ver list run_tests"

list()
{
    local f name
    {
        IFS=$'\n'
        for f in $(declare -F); do
            name="${f:11}"
            case " $INTERNAL_FUNCS " in
                *" $name "*) continue ;;
            esac
            echo "$name"
        done
        # run_tests is invoked as `test` (see the dispatcher below).
        echo "test"
    } | sort
}

# Dispatch: run the function named by the first argument.
# `test` is kept as an alias for run_tests (the function was renamed so it
# would not shadow the `test` shell builtin).
if [[ -z "${1:-}" ]]; then
    echo "Usage: $0 <target> [build type]"
    echo "Run '$0 list' to see available targets."
    exit 1
elif [[ "${1:-}" = "test" ]]; then
    shift
    run_tests "$@"
elif ! declare -F "$1" > /dev/null; then
    echo "Unknown target: $1"
    echo "Run '$0 list' to see available targets."
    exit 1
else
    "$@"
fi
