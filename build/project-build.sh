#!/bin/bash

# Intended to be ran from INSIDE the Docker build environment container.

JOBS=20

set -e

cd /source

rm -rf .premake/ bin/ Makefile

premake5 --os=linux gmake

make -j${JOBS} config=debug

for TEST in $(find bin/Debug/ -name "*_test?"); do
    ${TEST}
done

make -j${JOBS} config=release

ls -l ./bin/Release
