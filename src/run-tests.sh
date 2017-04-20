#!/usr/bin/env sh
#
# A shell script to run various tests automatically.
#

echo "stream-echo:"
for testfile in ../tests/stream/*; do
	outfile=$testfile.out
	./test-stream $testfile $outfile

	if ! diff $testfile $outfile; then
		echo -e "\tFAILED $testfile"
	else
		echo -e "\tOK"
	fi

	rm -f $outfile
done
