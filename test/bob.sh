#!/bin/bash

OSTYPE=$(uname -s | tr '[A-Z]' '[a-z]')

./build/$OSTYPE/selftest test/bob.conf &
echo $! >> "test/selftest.bob"
./build/$OSTYPE/echotest test/bob.conf &
echo $! >> "test/selftest.bob"