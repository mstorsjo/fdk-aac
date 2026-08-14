#!/bin/sh

set -ex

cd $(dirname $0)

./autogen.sh

[ -z "$CLEAN" ] || rm -rf build-archive
mkdir -p build-archive
cd build-archive

(cd .. && git archive --prefix=git/ HEAD ) > archive.tar

../configure
make dist
make distclean

rm -rf git
tar -xf archive.tar
mkdir unpack
tar -C unpack -zxf fdk-aac-*.tar.gz
rm -rf autotools
mv unpack/fdk-aac-* autotools
rmdir unpack

cd git
find . | sed 's/^..//' | sort | grep -v '^\.git' | grep -v '\.gitkeep' | grep -v '^m4' | grep -v '^check-.*\.sh$' > ../git-listing.txt
cd ../autotools
find . | sed 's/^..//' | sort | grep -v '^m4' | grep -v -E '^(aclocal.m4|compile|depcomp|install-sh|ltmain.sh|Makefile.in|missing)$' | grep -v -E '^config(.guess|.sub|ure)$' > ../autotools-listing.txt
cd ..

diff -u git-listing.txt autotools-listing.txt
