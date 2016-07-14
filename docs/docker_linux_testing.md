Testing
====

### Docker Unit Tests

**Running all docker tests**
Build the docker images:
`make docker_images`

Run the docker tests from the docker containers
`make docker_test`

Check the results of the completed tests:
`make docker_test_check`

***
Each unit test will temporarily copy all required ZeroTier binaries into its local directory, then build the `sdk_dockerfile` and `monitor_dockerfile`. Once built, each container will be run and perform tests and monitoring specified in `sdk_entrypoint.sh` and `monitor_entrypoint.sh`

Results will be written to the `tests/docker/_results/` directory which is a common shared volume between all containers involved in the test and will be a combination of raw and formatted dumps to files whose names reflect the test performed. In the event of failure, `FAIL.` will be prepended to the result file's name (e.g. `FAIL.my_application_1.0.2.x86_64`), likewise in the event of success, `OK.` will be prepended.

To run unit tests:

1) Disable SELinux. This is so the containers can use a shared volume to exchange MD5 sums and address information. 

2) Set up your own network at [https://my.zerotier.com/](https://my.zerotier.com/). For our example we'll just use the Earth network (8056c2e21c000001). Use its network id as follows:

3) Generate two pairs of identity keys. Each public/private pair will be used by the *sdk* and *monitor* containers:

    mkdir -p /tmp/sdk_first
    cp -f ./sdk/liblwip.so /tmp/sdk_first
    ./zerotier-sdk-service -d -p8100 /tmp/sdk_first
    while [ ! -f /tmp/sdk_first/identity.secret ]; do
      sleep 0.1
    done
    ./zerotier-cli -D/tmp/sdk_first join 8056c2e21c000001
    kill `cat /tmp/sdk_first/zerotier-one.pid`

    mkdir -p /tmp/sdk_second
    cp -f ./sdk/liblwip.so /tmp/sdk_second
    ./zerotier-sdk-service -d -p8101 /tmp/sdk_second
    while [ ! -f /tmp/sdk_second/identity.secret ]; do
      sleep 0.1
    done
    ./zerotier-cli -D/tmp/sdk_second join 8056c2e21c000001
    kill `cat /tmp/sdk_second/zerotier-one.pid`

4) Copy the identity files to *tests/docker*. Names will be altered during copy step so the dockerfiles know which identities to use for each image/container:

    cp /tmp/sdk_first/identity.public ./sdk/tests/docker/sdk_identity.public
    cp /tmp/sdk_first/identity.secret ./sdk/tests/docker/sdk_identity.secret

    cp /tmp/sdk_second/identity.public ./sdk/tests/docker/monitor_identity.public
    cp /tmp/sdk_second/identity.secret ./sdk/tests/docker/monitor_identity.secret


5) Place a blank network config file in the `tests/docker` directory (e.g. "8056c2e21c000001.conf")
 - This will be used to inform test-specific scripts what network to use for testing

After you've created your network and placed its blank config file in `tests/docker` run the following to perform unit tests for httpd:

    ./build.sh httpd
    ./test.sh httpd

It's useful to note that the keyword *httpd* in this example is merely a substring for a test name, this means that if we replaced it with *x86_64*, *fc23*, or *nginx*, it would run all unit tests for *x86_64*, *Fedora 23*, or *nginx* respectively.

