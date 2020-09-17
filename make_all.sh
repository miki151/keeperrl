#!/bin/sh

set -e

sh ./windows_make.sh
sh ./win64_make.sh
sh ./osx_make.sh
sh ./linux_make.sh
rm unity.cpp

cp -R data ~/sdk/tools/ContentBuilder/content/common/
cp -R data_free ~/sdk/tools/ContentBuilder/content/common/
cd ~/sdk/tools/ContentBuilder/
./run_build.sh

cd content
./print_versions.sh

