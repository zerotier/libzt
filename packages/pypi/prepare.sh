#!/bin/bash

# Move sources into top-level subdirectory so MANIFEST.in can include it in the source distribution

mkdir -p data/libzt
mkdir -p data/zto

# libzt
cp -r ../../src data/libzt/src
cp -r ../../include data/libzt/include
cp -r ../../ext data/libzt/ext

# ZeroTier
cp -r ../../zto/ data/zto

