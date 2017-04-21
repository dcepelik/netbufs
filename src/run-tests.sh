#!/usr/bin/env sh
#
# A shell script to run various tests automatically.
#

BAD_DIFF="DIFF!"
BAD_VALGRIND="VALGRIND!"
OK="OK"

num_errs=0
num_ok=0

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
for testfile in ../tests/libcbor/*.cbor; do
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

echo
echo "$num_errs error(s)";
