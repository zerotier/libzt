#!/bin/bash

echo "Performing unit tests..."

chmod 755 build/tests/servers.sh
chmod 755 build/tests/clients.sh

./servers.sh $1 $2 $3 &
sleep 3
./clients.sh $1 $2 $3 &
