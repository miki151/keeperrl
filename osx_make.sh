#!/bin/sh

set -e

make clean
time make -f Makefile -j12 RELEASE=true OPT=true DEBUG=true STEAMWORKS=true GCC=~/Downloads/osxcross/target/bin/o64-clang++ OSX=true
ls -l keeper
file keeper
cp keeper ~/steam_content/osx/
