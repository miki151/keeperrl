#!/bin/sh

set -e

if [ "$1" != "" ]; then
	if [ "$1" = "linux" ]; then
		sh ./linux_make.sh
	fi
	if [ "$1" = "osx" ]; then
		sh ./osx_make.sh
	fi
	if [ "$1" = "windows" ]; then
		sh ./windows_make.sh
		sh ./win64_make.sh
	fi
else
	sh ./windows_make.sh
	sh ./win64_make.sh
	sh ./osx_make.sh
	sh ./linux_make.sh
fi

rm -Rf ~/sdk/tools/ContentBuilder/content/common/data/
rm -Rf ~/sdk/tools/ContentBuilder/content/common/data_free/
cp -R -L data ~/sdk/tools/ContentBuilder/content/common/
cp -R data_free ~/sdk/tools/ContentBuilder/content/common/
cd ~/sdk/tools/ContentBuilder/
./run_build.sh

cd content
./print_versions.sh


rm unity.cpp
