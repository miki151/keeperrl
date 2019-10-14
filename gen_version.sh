#!/bin/sh

VERSION="`git describe --abbrev=4 --dirty --always`"
EPOCH=`git log --format=%ct -1`
[ -n "$EPOCH" ] || EPOCH=$SOURCE_DATE_EPOCH
[ -n "$EPOCH" ] || EPOCH=`date +%s`
DATE_FMT="+%F"
DATE=`date -u -d "@$EPOCH" $DATE_FMT 2>/dev/null || date -u -r "$EPOCH" $DATE_FMT 2>/dev/null || date -u $DATE_FMT`

echo "#define BUILD_VERSION \"$VERSION\"" > version.h
echo "#define BUILD_DATE \"$DATE\"" >> version.h
