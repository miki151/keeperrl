#!/bin/bash

grep -o "SERIAL.\?(.*)" *.{h,cpp} | grep -v "serialization\." | cut -d"(" -f 2 | cut -d")" -f 1 | cut -d" " -f 1 | cut -d"," -f 1 | sort > /tmp/decl
grep -o "SVAR(\w*)" *.{h,cpp} | grep -v "serialization\." | cut -d"(" -f 2 | cut -d")" -f 1 > /tmp/def1
grep "SERIALIZE_ALL(" *.{h,cpp} | grep -v define | cut -d"(" -f 2 | cut -d ")" -f 1 | sed "s/, /\n/g" >> /tmp/def1
grep "SERIALIZE_ALL2(" *.{h,cpp} | grep -v define | cut -d"(" -f 2 | cut -d ")" -f 1 | grep , | cut -d" " -f 2- | sed "s/, /\n/g" >> /tmp/def1
sort < /tmp/def1 > /tmp/def

diff /tmp/decl /tmp/def
if [ "$?" != "0" ]; then
  echo "======== Serialization check failed ========"
  exit 1
fi

