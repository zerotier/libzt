#!/bin/bash

# Execute from project root directory

cd ext/lwip
cp ../lwip.patch .
git apply lwip.patch
cd ../..

cd ext/lwip-contrib
cp ../lwip-contrib.patch .
git apply lwip-contrib.patch

cd ../..
