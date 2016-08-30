#!/bin/bash

echo "\n\n\n\nStarting server(s)"

# test.sh [udp|tcp|all] [nwid]
TEST_EXECUTABLE_PATH="build/tests"
LIB_PATH="./build/libztintercept.so"

PLATFORM="Linux"
protocol=$1
NWID=$2
ZT_HOME_PATH=$3
HOME_DIR=$(pwd)/$TEST_EXECUTABLE_PATH

BUILD_PATH=$(pwd)/build
TEST_PATH=$(pwd)/build/tests
localAddr="127.0.0.1"

export ZT_NC_NETWORK=$ZT_HOME_PATH/nc_$NWID
export LD_PRELOAD=$LIB_PATH

echo "network = " $NWID
echo "protocol = " $protocol
echo "ZT_NC_NETWORK = " $ZT_NC_NETWORK
echo "ZT_NC_NETWORK = " $ZT_NC_NETWORK
echo "ZT_HOME_PATH = " $ZT_HOME_PATH
echo "DYLD_LIBRARY_PATH = " $DYLD_LIBRARY_PATH

# Start ZeroTier service
echo "Starting ZeroTier background service..."
mkdir -p $ZT_HOME_PATH/networks.d
touch $ZT_HOME_PATH/networks.d/$NWID.conf
$BUILD_PATH/zerotier-sdk-service -U -p$RANDOM $ZT_HOME_PATH &
zt_service_pid=$!

if [ $protocol="tcp" ]; then
    echo "Starting TCP test..."
    random_tcp_server_port=$RANDOM
    echo $random_tcp_server_port > $TEST_EXECUTABLE_PATH/tcp_server.port
    ./$TEST_EXECUTABLE_PATH/$PLATFORM.tcp_server.out $random_tcp_server_port &
    tcp_server_pid=$!    

    # echo "TCP SERVER AT = " $localAddr ":" $random_tcp_server_port
    # sleep 3
    # ./$TEST_EXECUTABLE_PATH/$PLATFORM.tcp_client.out $localAddr $random_tcp_server_port &
    # tcp_client_pid=$!
fi

echo "Waiting for test to conclude..."
echo $random_tcp_server_port
# ,$random_udp_server_port
sleep 10
echo "Cleaning up"
kill -9 $zt_service_PID $tcp_server_pid $tcp_client_pid
