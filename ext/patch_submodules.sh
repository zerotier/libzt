#!/bin/bash

# Execute from project root directory
cd ext/lwip
cp ../lwip.patch .
git apply lwip.patch
cd -