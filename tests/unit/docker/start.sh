#!/bin/bash

# Before running this script, run: make unit_tests [COMPILE_FLAGS] to build the necessary docker images

# Runs test sdk and monitor containers based on the images built by the makefile
test_name="docker_demo"
echo 'Starting containers for: ' "$test_name"
touch "$test_name".name
test_container=$(docker run -d -it -v $PWD/tests/results:/opt/results --privileged --device=/dev/net/tun "$test_name":latest)
monitor_container=$(docker run -d -it -v $PWD/tests/results:/opt/results --privileged --device=/dev/net/tun "$test_name"_monitor:latest)

sleep 90

# By this stage, enough time should have passed for all of the unit tests to conclude
RESULTS_DIR=tests/results

./check.sh $RESULTS_DIR/OK.docker_demo.txt
echo $(cat $RESULTS_DIR/OK.docker_demo.txt)

rm -rf *.tmp
rm -rf *.txt