#!/bin/bash

# Mac OSX XCode Toolset or Linux

# To install command line compiler tools on Mac OSX:
# xcode-select --install

flags="" #"-m32"

if [[ `uname` == "Darwin" ]]; then
   cc $flags -DLIBEXT=\"dylib\" -fPIC -c -o jinter.o jinter.c
else
   cc $flags -DLIBEXT=\"so\"    -fPIC -c -o jinter.o jinter.c
fi

if [[ `uname` == "Darwin" ]]; then
   libtool -dynamic -macosx_version_min 10.6 -o jinter.dylib jinter.o -lc
else
   cc $flags -shared jinter.o -o jinter.so -lc
fi

rm jinter.o
