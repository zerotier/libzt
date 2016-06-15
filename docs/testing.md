Testing
====

# Docker Unit Tests

Each unit test will temporarily copy all required ZeroTier binaries into its local directory, then build the `netcon_dockerfile` and `monitor_dockerfile`. Once built, each container will be run and perform tests and monitoring specified in `netcon_entrypoint.sh` and `monitor_entrypoint.sh`

Results will be written to the `tests/docker-test/_results/` directory which is a common shared volume between all containers involved in the test and will be a combination of raw and formatted dumps to files whose names reflect the test performed. In the event of failure, `FAIL.` will be prepended to the result file's name (e.g. `FAIL.my_application_1.0.2.x86_64`), likewise in the event of success, `OK.` will be prepended.

To run unit tests:

1) Disable SELinux. This is so the containers can use a shared volume to exchange MD5 sums and address information. 

2) Set up your own network at [https://my.zerotier.com/](https://my.zerotier.com/). For our example we'll just use the Earth network (8056c2e21c000001). Use its network id as follows:

3) Generate two pairs of identity keys. Each public/private pair will be used by the *netcon* and *monitor* containers:

    mkdir -p /tmp/netcon_first
    cp -f ./netcon/liblwip.so /tmp/netcon_first
    ./zerotier-netcon-service -d -p8100 /tmp/netcon_first
    while [ ! -f /tmp/netcon_first/identity.secret ]; do
      sleep 0.1
    done
    ./zerotier-cli -D/tmp/netcon_first join 8056c2e21c000001
    kill `cat /tmp/netcon_first/zerotier-one.pid`

    mkdir -p /tmp/netcon_second
    cp -f ./netcon/liblwip.so /tmp/netcon_second
    ./zerotier-netcon-service -d -p8101 /tmp/netcon_second
    while [ ! -f /tmp/netcon_second/identity.secret ]; do
      sleep 0.1
    done
    ./zerotier-cli -D/tmp/netcon_second join 8056c2e21c000001
    kill `cat /tmp/netcon_second/zerotier-one.pid`

4) Copy the identity files to your *docker-test* directory. Names will be altered during copy step so the dockerfiles know which identities to use for each image/container:

    cp /tmp/netcon_first/identity.public ./netcon/docker-test/netcon_identity.public
    cp /tmp/netcon_first/identity.secret ./netcon/docker-test/netcon_identity.secret

    cp /tmp/netcon_second/identity.public ./netcon/docker-test/monitor_identity.public
    cp /tmp/netcon_second/identity.secret ./netcon/docker-test/monitor_identity.secret


5) Place a blank network config file in the `netcon/docker-test` directory (e.g. "8056c2e21c000001.conf")
 - This will be used to inform test-specific scripts what network to use for testing

After you've created your network and placed its blank config file in `tests/docker-test` run the following to perform unit tests for httpd:

    ./build.sh httpd
    ./test.sh httpd

It's useful to note that the keyword *httpd* in this example is merely a substring for a test name, this means that if we replaced it with *x86_64* or *fc23*, it would run all unit tests for *x86_64* systems or *Fedora 23* respectively.

