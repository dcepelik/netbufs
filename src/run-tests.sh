#!/usr/bin/env bash
#
# A shell script to run various tests automatically.
#

BAD_DIFF="DIFF!"
BAD_VALGRIND="VALGRIND!"
BAD_RUNTIME="RUNTIME!"
OK="OK"

num_errs=0
num_ok=0

if ! command -v jq 2>&1 >/dev/null; then
	echo "$0: the jq binary is needed to run this test suite"
	exit 1;
fi

echo "stream-echo:"
for testfile in ../tests/stream/*; do
	outfile=$testfile.out
	./test-stream $testfile $outfile

	if ! diff $testfile $outfile >/dev/null; then
		echo -e "\t$BAD_DIFF $testfile"
	else
		echo -e "\t$OK"
	fi

	rm -f $outfile
done

echo "decenc:"
for test in ../tests/libcbor/local/*; do
	testfile=$test/in.cbor
	outfile=$testfile.out
	valgrind_result=$(valgrind ./test-decenc $testfile $outfile 2>&1 >/dev/null | tail -n1 | cut -d ' ' -f4,10)

	err=0

	if [ "$valgrind_result" != "0 0" ]; then
		echo -e "\t$BAD_VALGRIND $testfile"
		err=1
	fi

	if ! diff -q $testfile $outfile >/dev/null; then
		echo -e "\t$BAD_DIFF $testfile"
		err=1
	fi

	if [ $err -eq 0 ]; then
		echo -e "\tOK"
		rm -f $outfile
	fi

	num_errs=$(expr $num_errs + $err)
done

echo "corrupt:"
for test in ../tests/libcbor/corrupt/*; do
	testfile=$test/in.corrupt
	outfile=$testfile.out
	valgrind_result=$(valgrind ./test-corrupt $testfile 2>&1 >/dev/null | tail -n1 | cut -d ' ' -f4,10)

	err=0

	if ! ./test-corrupt $testfile 2>&1 >/dev/null; then
		echo -e "\t$BAD_RUNTIME $testfile"
		err=1
	fi

	if [ "$valgrind_result" != "0 0" ]; then
		echo -e "\t$BAD_VALGRIND $testfile"
		err=1
	fi

	if [ $err -eq 0 ]; then
		echo -e "\tOK"
		rm -f $outfile
	fi

	num_errs=$(expr $num_errs + $err)
done

echo
echo "$num_errs error(s)";
