#!/bin/bash
curdir=$pwd
mydir="${0%/*}"

cd $mydir

echo "Downloding cadical"
git clone https://github.com/arminbiere/cadical cadical --depth=1
cd cadical
CXXFLAGS=-fPIC ./configure -static
make -j8
