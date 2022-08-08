#!/bin/sh

set -e
PATH=$PATH:/home/michal/Downloads/osxcross/target/bin
make clean
time make -f Makefile -j32 RELEASE=true OPT=true DEBUG=true STEAMWORKS=true GCC=o64-clang++ OSX=true
ls -l keeper
file keeper
cp keeper ~/steam_content/osx/
