#!/bin/bash

echo "Performing unit tests..."

chmod 755 build/tests/servers.sh
chmod 755 build/tests/clients.sh

./build/tests/servers.sh $1 $2 &
sleep 3
./build/tests/clients.sh $1 $2 &