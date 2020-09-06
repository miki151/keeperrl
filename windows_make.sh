#!/bin/sh

set -e

make clean
time make -f Makefile-win -j12 UNITY=true RELEASE=true OPT=true DEBUG=true STEAMWORKS=true PREFIX=~/Downloads/mxe/usr/bin/i686-w64-mingw32.shared.dw2-
mv keeper keeper.exe
ls -l keeper.exe
file keeper.exe
mv keeper.exe ~/steam_content/win32/

