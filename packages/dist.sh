#!/bin/bash

# Call this script from the root project directory via `make dist`
# - submodules will be recursively initialized and updated
# - patches will be applied to submodules if needed
# - this script will call CMake to generate library-building packages if necessary
# - once projects have been generated, this script will use their tooling to build the libraries/packages
# - when all products have been built and moved to `tmp`, they will be compressed and moved to `products`

OSNAME=$(uname | tr '[A-Z]' '[a-z]')
BUILD_CONCURRENCY=4
PROJROOT=$(pwd)
BUILD_PRODUCTS_DIR=$(pwd)/bin
LIB_PRODUCTS_DIR=$BUILD_PRODUCTS_DIR/lib
FINISHED_PRODUCTS_DIR=$(pwd)/products
STAGING_DIR=$(pwd)/staging
# Windows (previously built)
WIN_PREBUILT_DIR=$PROJROOT/staging/win
WIN_RELEASE_PRODUCTS_DIR=$WIN_PREBUILT_DIR/release
WIN_DEBUG_PRODUCTS_DIR=$WIN_PREBUILT_DIR/debug
WIN32_RELEASE_PRODUCTS_DIR=$WIN_RELEASE_PRODUCTS_DIR/win32
WIN64_RELEASE_PRODUCTS_DIR=$WIN_RELEASE_PRODUCTS_DIR/win64
WIN32_DEBUG_PRODUCTS_DIR=$WIN_DEBUG_PRODUCTS_DIR/win32
WIN64_DEBUG_PRODUCTS_DIR=$WIN_DEBUG_PRODUCTS_DIR/win64
# Linux
LINUX_PROD_DIR=$PROJROOT/staging/linux
# macOS
MACOS_PROD_DIR=$PROJROOT/staging/macos
MACOS_RELEASE_PROD_DIR=$MACOS_PROD_DIR/release
MACOS_DEBUG_PROD_DIR=$MACOS_PROD_DIR/debug
# iOS
IOS_PROD_DIR=$PROJROOT/staging/ios
# Android
ANDROID_PROJ_DIR=$(pwd)/"packages/android"
ANDROID_ARCHIVE_FILENAME="zt.aar"
# Xcode
XCODE_IOS_PROJ_DIR=$(pwd)/"packages/xcode_ios"
XCODE_MACOS_PROJ_DIR=$(pwd)/"packages/xcode_macos"

mkdir $FINISHED_PRODUCTS_DIR

# Check that projects exist, generate them and exit if they don't exist
generate_projects_if_necessary() 
{
	# iOS
	if [ ! -d "$XCODE_IOS_PROJ_DIR" ]; then
		echo "BUILDING: iOS project"
		should_exit=1
		mkdir -p $XCODE_IOS_PROJ_DIR
		cd $XCODE_IOS_PROJ_DIR
		cmake -G Xcode ../../
		# Bug in CMake requires us to manually replace architecture strings in project file
		sed -i '' 's/x86_64/$(CURRENT_ARCH)/g' $PROJNAME.xcodeproj/project.pbxproj
		cd -
	fi
	# macOS
	if [ ! -d "$XCODE_MACOS_PROJ_DIR" ]; then
		echo "BUILDING: macOS project"
		should_exit=1
		mkdir -p $XCODE_MACOS_PROJ_DIR
		cd $XCODE_MACOS_PROJ_DIR
		cmake -G Xcode ../../
		cd -
	fi
	# android?
	if [[ $should_exit = 1 ]]; then
		echo "Generated projects. Perform necessary modifications and then re-run this script"
		echo "Please place previously built windows binaries in $WIN_PREBUILT_DIR before running again."
		exit 0
	else
		echo "Projects detected, going to build stage next"
	fi
}

build_all_products()
{
	CONFIG=$1
	UPPERCASE_CONFIG="$(tr '[:lower:]' '[:upper:]' <<< $1:0:1)$1:1"

	# Targets to build on and for darwin
	if [[ $OSNAME = *"darwin"* ]]; then
		# Xcode Frameworks --- Builds targets from a CMake-generated Xcode project
		if true; then
			if [[ $2 != *"JNI"* ]]; then
				CURR_BUILD_PRODUCTS_DIR=$LIB_PRODUCTS_DIR/$UPPERCASE_CONFIG
				# (iOS)
				echo "BUILDING: iOS"
				cd $XCODE_IOS_PROJ_DIR
				xcodebuild -target zt -configuration "$UPPERCASE_CONFIG" -sdk "iphoneos"
				xcodebuild -target zt-static -configuration "$UPPERCASE_CONFIG" -sdk "iphoneos"
				cd -
				CURR_ARCH="arm64" # anything older should be built custom
				CURR_TMP_PRODUCT_DIR=$STAGING_DIR/$CONFIG/ios-$CURR_ARCH
				mkdir -p $CURR_TMP_PRODUCT_DIR
				mv $CURR_BUILD_PRODUCTS_DIR/*.framework $CURR_TMP_PRODUCT_DIR
				mv $CURR_BUILD_PRODUCTS_DIR/libzt.* $CURR_TMP_PRODUCT_DIR

				# (macOS)
				echo "BUILDING: macOS"
				cd $XCODE_MACOS_PROJ_DIR
					xcodebuild -target zt -configuration "$UPPERCASE_CONFIG" -sdk "macosx"
					xcodebuild -target zt-static -configuration "$UPPERCASE_CONFIG" -sdk "macosx"
					xcodebuild -target zt-shared -configuration "$UPPERCASE_CONFIG" -sdk "macosx"
				cd -
				CURR_ARCH=$HOSTTYPE
				CURR_TMP_PRODUCT_DIR=$STAGING_DIR/$CONFIG/macos-$CURR_ARCH
				mkdir -p $CURR_TMP_PRODUCT_DIR
				mv $CURR_BUILD_PRODUCTS_DIR/*.framework $CURR_TMP_PRODUCT_DIR
				mv $CURR_BUILD_PRODUCTS_DIR/libzt.* $CURR_TMP_PRODUCT_DIR
			fi
		fi
		# Android Archive (AAR) --- Executes a Gradle task
		if true; then
			CMAKE_FLAGS=$CMAKE_FLAGS" -DJNI=1"
			CURR_ARCH="armeabi-v7a"
			CURR_TMP_PRODUCT_DIR=$STAGING_DIR/$CONFIG/android-$CURR_ARCH
			mkdir -p $CURR_TMP_PRODUCT_DIR
			echo "BUILDING: AAR"
			cd $ANDROID_PROJ_DIR
			./gradlew assemble$UPPERCASE_CONFIG # e.g. assembleRelease
			mv $ANDROID_PROJ_DIR/app/build/outputs/aar/app-$CONFIG.aar $CURR_TMP_PRODUCT_DIR/$ANDROID_ARCHIVE_FILENAME
			cd -
		fi
		# Java Archive (JAR)
		if true; then
			CMAKE_FLAGS=$CMAKE_FLAGS" -DJNI=1"
			CURR_ARCH=$HOSTTYPE
			CURR_TMP_PRODUCT_DIR=$STAGING_DIR/$CONFIG/macos-$CURR_ARCH
			mkdir -p $CURR_TMP_PRODUCT_DIR
			echo "BUILDING: JAR"
			rm -rf $LIB_PRODUCTS_DIR # clean-lite
			cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=$CONFIG "-DJNI=1 -DBUILD_TESTS=0"
			cmake --build build
			cd $PROJROOT/packages/java
			#cp $CURR_BUILD_PRODUCTS_DIR/libzt.dylib .
			javac com/zerotier/libzt/*.java
			jar cf zt.jar $CURR_BUILD_PRODUCTS_DIR/libzt.dylib com/zerotier/libzt/*.class
			mv zt.jar $CURR_TMP_PRODUCT_DIR
			cd -
		fi
	fi
	# Linux targets
	if [[ $OSNAME = *"linux"* ]]; then
		CURR_BUILD_PRODUCTS_DIR=$LIB_PRODUCTS_DIR/
		# Ordinary libraries
		if false; then
			rm -rf $LIB_PRODUCTS_DIR
			cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=$CONFIG "-DBUILD_TESTS=0"
			cmake --build build -j $BUILD_CONCURRENCY
			CURR_ARCH=$HOSTTYPE
			CURR_TMP_PRODUCT_DIR=$STAGING_DIR/$CONFIG/linux-$CURR_ARCH
			mv $CURR_BUILD_PRODUCTS_DIR/libzt.* $CURR_TMP_PRODUCT_DIR
		fi
		# Java JAR file
		if true; then
			rm -rf $LIB_PRODUCTS_DIR
			cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=$CONFIG "-DJNI=1 -DBUILD_TESTS=0"
			cmake --build build -j $BUILD_CONCURRENCY
			CURR_ARCH=$HOSTTYPE
			CURR_TMP_PRODUCT_DIR=$STAGING_DIR/$CONFIG/linux-$CURR_ARCH
			mkdir -p $CURR_TMP_PRODUCT_DIR
			cd $PROJROOT/packages/java
			#cp $CURR_BUILD_PRODUCTS_DIR/libzt.so .
			javac com/zerotier/libzt/*.java
			jar cf zt.jar $CURR_BUILD_PRODUCTS_DIR/libzt.so com/zerotier/libzt/*.class
			mv zt.jar $CURR_TMP_PRODUCT_DIR
			cd -
		fi
	fi
}

main()
{
	generate_projects_if_necessary
	build_all_products "release"
	build_all_products "debug"
}

main "$@"