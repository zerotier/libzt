#!/bin/bash

# Call this script from the root project directory via `make dist`
# - submodules will be recursively initialized and updated
# - patches will be applied to submodules if needed
# - this script will call CMake to generate library-building packages if necessary
# - once projects have been generated, this script will use their tooling to build the libraries/packages
# - when all products have been built and moved to `tmp`, they will be compressed and moved to `products`

PROJNAME="zt"
LIBNAME="lib"$PROJNAME
OSNAME=$(uname | tr '[A-Z]' '[a-z]')
LIBZT_VERSION="1.2.0"
LIBZT_REVISION="1"
ZT_CORE_VERSION="1.2.12"
FILENAME_PREFIX=${LIBNAME}"-"${LIBZT_VERSION}"r"${LIBZT_REVISION}

PROJROOT=$(pwd)
BUILD_PRODUCTS_DIR=$(pwd)/bin
LIB_PRODUCTS_DIR=${BUILD_PRODUCTS_DIR}/lib
FINISHED_PRODUCTS_DIR=$(pwd)/products
TMP_PRODUCTS_DIR=${BUILD_PRODUCTS_DIR}/tmp

# previously built, will include in package
WIN_PREBUILT_DIR=${PROJROOT}/prebuilt
WIN_RELEASE_PRODUCTS_DIR=${WIN_PREBUILT_DIR}/release
WIN_DEBUG_PRODUCTS_DIR=${WIN_PREBUILT_DIR}/debug
WIN32_RELEASE_PRODUCTS_DIR=${WIN_RELEASE_PRODUCTS_DIR}/win32
WIN64_RELEASE_PRODUCTS_DIR=${WIN_RELEASE_PRODUCTS_DIR}/win64
WIN32_DEBUG_PRODUCTS_DIR=${WIN_DEBUG_PRODUCTS_DIR}/win32
WIN64_DEBUG_PRODUCTS_DIR=${WIN_DEBUG_PRODUCTS_DIR}/win64

XCODE_IOS_PROJ_DIR=$(pwd)/"packages/xcode_ios"
XCODE_MACOS_PROJ_DIR=$(pwd)/"packages/xcode_macos"

ANDROID_PROJ_DIR=$(pwd)/"packages/android"
ANDROID_ARCHIVE_FILENAME="zt.aar"

mkdir ${FINISHED_PRODUCTS_DIR}
mkdir ${TMP_PRODUCTS_DIR}

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
		echo "Please place previously built windows binaries in ${WIN_PREBUILT_DIR} before running again."
		exit 0
	else
		echo "Projects detected, going to build stage next"
	fi
}

# Xcode Frameworks
build_xcode_targets()
{
	CMAKE_CONFIG=${1}
	UPPERCASE_CONFIG="$(tr '[:lower:]' '[:upper:]' <<< ${1:0:1})${1:1}"
	if [[ ${2} = *"jni"* ]]; then
		CMAKE_FLAGS=${CMAKE_FLAGS}" -DJNI=1"
	fi
	if [[ $OSNAME = *"darwin"* && ${2} != *"JNI"* ]]; then
		CURR_BUILD_PRODUCTS_DIR=${LIB_PRODUCTS_DIR}/${UPPERCASE_CONFIG}
		# (iOS)
		echo "BUILDING: iOS"
		cd $XCODE_IOS_PROJ_DIR
		xcodebuild -target zt -configuration "${UPPERCASE_CONFIG}" -sdk "iphoneos"
		xcodebuild -target zt-static -configuration "${UPPERCASE_CONFIG}" -sdk "iphoneos"
		cd -
		CURR_ARCH="arm64" # anything older should be built custom
		CURR_TMP_PRODUCT_DIR=${TMP_PRODUCTS_DIR}/ios-${CURR_ARCH}
		mkdir -p ${CURR_TMP_PRODUCT_DIR}
		mv ${CURR_BUILD_PRODUCTS_DIR}/*.framework ${CURR_TMP_PRODUCT_DIR}
		mv ${CURR_BUILD_PRODUCTS_DIR}/libzt.* ${CURR_TMP_PRODUCT_DIR}

		# (macOS)
		echo "BUILDING: macOS"
		cd $XCODE_MACOS_PROJ_DIR
			xcodebuild -target zt -configuration "${UPPERCASE_CONFIG}" -sdk "macosx"
			xcodebuild -target zt-static -configuration "${UPPERCASE_CONFIG}" -sdk "macosx"
			xcodebuild -target zt-shared -configuration "${UPPERCASE_CONFIG}" -sdk "macosx"
		cd -
		CURR_ARCH=${HOSTTYPE}
		CURR_TMP_PRODUCT_DIR=${TMP_PRODUCTS_DIR}/macos-${CURR_ARCH}
		mkdir -p ${CURR_TMP_PRODUCT_DIR}
		mv ${CURR_BUILD_PRODUCTS_DIR}/*.framework ${CURR_TMP_PRODUCT_DIR}
		mv ${CURR_BUILD_PRODUCTS_DIR}/libzt.* ${CURR_TMP_PRODUCT_DIR}
	fi
}

# Android archive (AAR)
build_aar()
{
	CMAKE_CONFIG=${1}
	UPPERCASE_CONFIG="$(tr '[:lower:]' '[:upper:]' <<< ${1:0:1})${1:1}"
	if [[ ${2} = *"jni"* ]]; then
		CMAKE_FLAGS=${CMAKE_FLAGS}" -DJNI=1"
	fi
	CURR_ARCH="armeabi-v7a"
	CURR_TMP_PRODUCT_DIR=${TMP_PRODUCTS_DIR}/android-${CURR_ARCH}
	mkdir -p ${CURR_TMP_PRODUCT_DIR}
	echo "BUILDING: AAR"
	cd ${ANDROID_PROJ_DIR}
	./gradlew assemble${UPPERCASE_CONFIG} # e.g. assembleRelease
	mv ${ANDROID_PROJ_DIR}/app/build/outputs/aar/app-${CONFIG}.aar ${CURR_TMP_PRODUCT_DIR}/${ANDROID_ARCHIVE_FILENAME}
	cd -
}

# Java archive (JAR)
#Call ordinary CMake build script with JNI flag set, use product in JAR file
build_jar()
{
	CMAKE_CONFIG=${1}
	UPPERCASE_CONFIG="$(tr '[:lower:]' '[:upper:]' <<< ${1:0:1})${1:1}"
	if [[ ${2} = *"jni"* ]]; then
		CMAKE_FLAGS=${CMAKE_FLAGS}" -DJNI=1"
	fi
	CURR_ARCH=${HOSTTYPE}
	CURR_TMP_PRODUCT_DIR=${TMP_PRODUCTS_DIR}/macos-${CURR_ARCH}
	mkdir -p ${CURR_TMP_PRODUCT_DIR}
	echo "BUILDING: JAR"
	rm -rf ${LIB_PRODUCTS_DIR} # clean-lite
	cmake -H. -Bbuild -DCMAKE_BUILD_TYPE=${CMAKE_CONFIG} "-DJNI=1 -DBUILD_TESTS=0"
	cmake --build build
	cd ${PROJROOT}/examples/java
	cp ../../bin/lib/libzt.dylib .
	mv ${LIB_PRODUCTS_DIR}/libzt.a ${CURR_TMP_PRODUCT_DIR}/libzt-jni.a
	mv ${LIB_PRODUCTS_DIR}/libzt.dylib ${CURR_TMP_PRODUCT_DIR}/libzt-jni.dylib
	javac com/zerotier/libzt/*.java
	jar cf libzt.jar libzt.dylib  com/zerotier/libzt/*.class
	mv libzt.jar ${CURR_TMP_PRODUCT_DIR}
	cd -
}

# Build everything (to a specific configuration)
build()
{
	if [[ $OSNAME == *"darwin"* ]]; then
		build_xcode_targets ${1} ${2}
		build_aar ${1} ${2}
	fi
	build_jar ${1} ${2}
}

# Package everything together
package_products()
{
	CONFIG=${1}
	PRODUCT_FILENAME=${FILENAME_PREFIX}-${CONFIG}.tar.gz
	echo "Making: " ${FINISHED_PRODUCTS_DIR}/${PRODUCT_FILENAME}
	cd ${TMP_PRODUCTS_DIR}
	tar -zcvf ${PRODUCT_FILENAME} .
	mv *.tar.gz ${FINISHED_PRODUCTS_DIR}
	cd -
}

copy_windows_targets()
{
	echo "Copying prebuilt windows binaries into temporary staging directory"
	if [ ! -d "$XCODE_MACOS_PROJ_DIR" ]; then
		echo "WARNING: windows products directory appears to be empty. Exiting"
		exit 0
	fi
	CONFIG=${1}
	cp -r ${WIN_PREBUILT_DIR}/${CONFIG}/win32 ${TMP_PRODUCTS_DIR}
	cp -r ${WIN_PREBUILT_DIR}/${CONFIG}/win64 ${TMP_PRODUCTS_DIR}
}

build_all_products()
{
	CONFIG=${1}
	build ${CONFIG}
	copy_windows_targets ${CONFIG}
	package_products ${CONFIG}
}

main()
{
	# prepare environment
	generate_projects_if_necessary
	mkdir -p ${WIN32_RELEASE_PRODUCTS_DIR}
	mkdir -p ${WIN64_RELEASE_PRODUCTS_DIR}
	mkdir -p ${WIN32_DEBUG_PRODUCTS_DIR}
	mkdir -p ${WIN64_DEBUG_PRODUCTS_DIR}
	# build
	build_all_products "release"
	build_all_products "debug"
}

main "$@"