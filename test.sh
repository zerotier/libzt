# Calls target in makefile to build docker images and execute unit tests

# lwIP
make clean; make -f make-linux.mk unit_test SDK_DEBUG=1 SDK_LWIP=1 SDK_IPV4=1
./tests/unit/docker/start.sh
make clean; make -f make-linux.mk unit_test SDK_DEBUG=1 SDK_LWIP=1 SDK_IPV6=1
./tests/unit/docker/start.sh

# picoTCP
make clean; make -f make-linux.mk unit_test SDK_DEBUG=1 SDK_PICOTCP=1 SDK_IPV4=1
./tests/unit/docker/start.sh
make clean; make -f make-linux.mk unit_test SDK_DEBUG=1 SDK_PICOTCP=1 SDK_IPV6=1
./tests/unit/docker/start.sh