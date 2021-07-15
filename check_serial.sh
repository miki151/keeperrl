#!/bin/bash

GET_ARGUMENTS='cut -d"(" -f 2- | rev | cut -d ")" -f 2- | rev'

grep -o "SERIAL(.*)" *.{h,cpp} | grep -v "serialization\." | cut -d"(" -f 2 | cut -d")" -f 1 | cut -d" " -f 1 | cut -d"," -f 1 | sort > /tmp/decl
grep -E "(SERIALIZE_ALL\(|SERIALIZE_ALL_NO_VERSION\(|COMPARE_ALL\()" *.{h,cpp} | grep -v define | eval $GET_ARGUMENTS | sed "s/, /\n/g" > /tmp/def1
grep " ar(" *.{cpp,h} | grep -v define | eval $GET_ARGUMENTS | sed "s/, /\n/g" >> /tmp/def1
grep -E "(SERIALIZE_DEF\(|SERIALIZE_TMPL\()" *.cpp | grep -v define | eval $GET_ARGUMENTS | grep , | cut -d" " -f 2- | sed "s/, /\n/g" >> /tmp/def1
sed -i "s/NAMED(\(.*\))/\1/" /tmp/def1
sed -i "s/SKIP(\(.*\))/\1/" /tmp/def1
sed -i "s/OPTION(\(.*\))/\1/" /tmp/def1
sed -i "s/withRoundBrackets(\(.*\))/\1/" /tmp/def1
sed -i "s/serializeAsValue(\(.*\))/\1/" /tmp/def1
grep -v SUBCLASS < /tmp/def1 | grep -v __VA_ARGS__ | grep -v 'roundBracket()' | grep -v "endInput()" | sort > /tmp/def

diff /tmp/decl /tmp/def
if [ "$?" != "0" ]; then
  echo "======== Serialization check failed ========"
  exit 1
fi

grep -o "HASH(.*)" *.{h,cpp} | grep -v "hashing\." | cut -d"(" -f 2 | cut -d")" -f 1 | cut -d" " -f 1 | cut -d"," -f 1 | sort > /tmp/decl
grep "HASH_ALL(" *.{h,cpp} | grep -v define | cut -d"(" -f 2 | cut -d ")" -f 1 | sed "s/, /\n/g" > /tmp/def1
grep "HASH_DEF(" *.{h,cpp} | grep -v define | cut -d"(" -f 2 | cut -d ")" -f 1 | cut -d" " -f 2- | sed "s/, /\n/g" >> /tmp/def1
sort < /tmp/def1 > /tmp/def

diff /tmp/decl /tmp/def
if [ "$?" != "0" ]; then
  echo "======== Hashing check failed ========"
  exit 1
fi

