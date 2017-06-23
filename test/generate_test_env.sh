#!/bin/bash

# create dirs to house the identities/config
mkdir -p test_identities
mkdir test_identities/alice
mkdir test_identities/bob

# generate identities
zerotier-one test_identities/alice -d
echo $! >> "test_identities/zto.alice"
zerotier-one test_identities/bob -d
echo $! >> "test_identities/zto.bob"

# should be done by now
sleep(30)

# kill daemons
echo "killing daemons"

pid=$(cat test_identities/alice/zto.alice)
kill -9 $pid
pid=$(cat test_identities/bob/zto.bob)
kill -9 $pid