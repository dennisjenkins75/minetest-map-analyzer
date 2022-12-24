#!/bin/bash

set -e

rm -rf tmp/test

[[ -f Makefile ]] && make clean

premake5 --os=linux gmake

make -j20

for TEST in $(find bin/Debug/ -name "*_test?"); do
    ${TEST}
done

make -j20 config=release
