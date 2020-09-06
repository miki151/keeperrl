#!/bin/sh

set -e

make clean
time make -f Makefile-win -j10 AMD64=true UNITY=true RELEASE=true OPT=true DEBUG=true STEAMWORKS=true PREFIX=~/Downloads/mxe/usr/bin/x86_64-w64-mingw32.shared-
mv keeper keeper.exe
ls -l keeper.exe
file keeper.exe
mv keeper.exe ~/steam_content/windows/

