#!/bin/bash

# create dirs to house the identities/config
mkdir alice
mkdir bob
mkdir ted 
mkdir carol

# generate identities
zerotier-one alice -d
echo $! >> "zto.alice"
zerotier-one bob -d
echo $! >> "zto.bob"
zerotier-one ted -d
echo $! >> "zto.ted"
zerotier-one carol -d
echo $! >> "zto.carol"

# should be done by now
sleep(30)

# kill daemons
echo "killing daemons"

pid=$(cat alice/zto.alice)
kill -9 $pid
pid=$(cat bob/zto.bob)
kill -9 $pid
pid=$(cat ted/zto.ted)
kill -9 $pid
pid=$(cat carol/zto.carol)
kill -9 $pid