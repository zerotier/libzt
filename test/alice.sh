#!/bin/bash

OSTYPE=$(uname -s | tr '[A-Z]' '[a-z]')

./build/$OSTYPE/selftest test/alice.conf &
echo $! >> "test/selftest.alice"
./build/$OSTYPE/echotest test/alice.conf &
echo $! >> "test/echotest.alice"