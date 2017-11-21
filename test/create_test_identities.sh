# !/bin/bash
# Generates test identities and joins them to a test network

NWID=""

mkdir -p test/alice test/bob test/carol test/ted

./build/darwin/selftest generate_id ${NWID} test/alice
./build/darwin/selftest generate_id ${NWID} test/bob
./build/darwin/selftest generate_id ${NWID} test/carol
./build/darwin/selftest generate_id ${NWID} test/ted
