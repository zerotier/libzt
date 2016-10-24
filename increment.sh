#!/bin/bash

BUILD_NUMBER_STR=$(cat .buildnum)
BUILD_NUMBER=$((BUILD_NUMBER_STR + 1))
echo $BUILD_NUMBER > .buildnum
echo "#define ZTSDK_BUILD_VERSION " $BUILD_NUMBER > src/build.h 
echo $BUILD_NUMBER 
