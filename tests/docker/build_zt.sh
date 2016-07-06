#!/bin/bash

# Builds Zerotier-One and libraries required for SDK, then
# copies the binaries into the test directory.

cd ../../
make clean
make one
make sdk
cd sdk/docker-test

cp ../../zerotier-cli zerotier-cli
cp ../../zerotier-sdk-service zerotier-sdk-service
cp ../../libztintercept.so libztintercept.so

cp ../liblwip.so liblwip.so
cp ../zerotier-intercept zerotier-intercept

cp ../../zerotier-one zerotier-one

