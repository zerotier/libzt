# !/bin/bash
# Generates test identities and joins them to a test network

NWID=$1

mkdir -p alice bob carol ted

./../bin/selftest generate_id ${NWID} alice
./../bin/selftest generate_id ${NWID} bob
./../bin/selftest generate_id ${NWID} carol
./../bin/selftest generate_id ${NWID} ted
