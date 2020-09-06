#!/bin/sh

set -e

make clean
schroot --chroot steamrt_scout_amd64 -- make OPT=true RELEASE=true GCC=clang++-3.8 -j12
cp keeper ~/steam_content/linux64

