docker stop $(docker ps -a -q)
docker rm $test_container
docker rm $monitor_container
