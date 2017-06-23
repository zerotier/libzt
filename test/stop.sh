#!/bin/bash

if [[ $1 == "" ]]
then
	echo "Usage: stop <alice|bob>."
	exit
fi

echo "killing $1"

for f in "test/*.$1";
do
	pid=$(cat $f)
	kill -9 $pid
	rm -rf $f
done