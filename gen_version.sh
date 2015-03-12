#!/bin/sh

VERSION="`git describe --abbrev=4 --dirty --always`"
DATE="`date +%F`"

echo "#define BUILD_VERSION \"$VERSION\"" > version.h
echo "#define BUILD_DATE \"$DATE\"" >> version.h
