#!/usr/bin/env sh
#
# A shell script to generate some of the test files
#

sizes="1 1k 8k 1M 16M"
for size in $sizes; do
	testfile=stream/random-$size
	if [ ! -f $testfile ]; then
		dd if=/dev/urandom of=$testfile bs=$size count=1
	fi
done
