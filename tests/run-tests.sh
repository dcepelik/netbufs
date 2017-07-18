#!/usr/bin/env sh
#
# An automated test runner.
#

CBOR_DIR=cbor
CBOR_RANDFILES_DIR=$CBOR_DIR/random
CBOR_RFC_DIR=$CBOR_DIR/rfc
CBOR_LOCAL_DIR=$CBOR_DIR/local
CBOR_CORRUPT_DIR=$CBOR_DIR/corrupt
CBOR_NUM_RANDFILES=20
CBOR_RFC_TESTS_URI=https://raw.githubusercontent.com/cbor/test-vectors/master/appendix_a.json
CBOR_RFC_TESTS_JSON=rfc-tests.json

NBDIAG=../build/nbdiag

IO_DIR=io
IO_RAND_FILES="1 5117 1k 8k 1M 16M"

setup_test_files() {
	if ! command -v jq >/dev/null; then
		echo "$0: jq binary not found"
		exit 1;
	fi

	if ! command -v curl >/dev/null; then
		echo "$0: curl binary not found"
		exit 1;
	fi

	if ! command -v xxd >/dev/null; then
		echo "$0: xxd binary not found"
		exit 1;
	fi

	#mkdir $IO_DIR
	for size in $IO_RAND_FILES; do
		testfile=$IO_DIR/$size.random
		dd if=/dev/urandom of=$testfile bs=$size count=1
	done

	if [ ! -d $CBOR_RANDFILES_DIR ]; then
		mkdir $CBOR_RANDFILES_DIR
		for i in $(seq 1 $CBOR_NUM_RANDFILES); do
			dir=$CBOR_RANDFILES_DIR/$i
			mkdir $dir

			testfile=$dir/in
			dd if=/dev/urandom bs=256 count=1 2>&1 | xxd -p | tr -d '\n' > $testfile
		done
	fi

	# generate tests from RFC test suite

	if [ ! -d $CBOR_RFC_DIR ]; then
		if ! curl -o $CBOR_RFC_TESTS_JSON $CBOR_RFC_TESTS_URI; then
			echo "Cannot download RFC test vectors from $CBOR_RFC_TESTS_URI"
			exit 1
		fi

		i=0
		jq -c ".[] | (.hex,.decoded,.diagnostic)" $CBOR_RFC_TESTS_JSON |
			while IFS= read -r hex && IFS= read -r decoded && IFS= read diag; do
				i=$(($i + 1))

				dirname=$CBOR_RFC_DIR/$(printf %02i $i)
				mkdir -p $dirname

				echo -n $hex | tr -d '"' > $dirname/in

				if [ "$decoded" != "null" ]; then
					echo "$decoded" | jq "." > $dirname/out.json
				fi

				if [ "$diag" != "null" ]; then
					echo "$diag" | sed 's/^"\|"$//g' > $dirname/out
				fi
			done

		rm $CBOR_RFC_TESTS_JSON
	fi

	# disable some RFC tests

	for i in $(seq 19 40); do
		echo "Skip float test" > $CBOR_RFC_DIR/$i/skip
	done
	echo "Skip float test" > $CBOR_RFC_DIR/50/skip

	echo "No positive bignum support" > $CBOR_RFC_DIR/12/skip
	echo "The integer doesn't fit into int64 range" > $CBOR_RFC_DIR/13/skip
	echo "No negative bignum support" > $CBOR_RFC_DIR/14/skip
	echo "No way to compare outputs easily" > $CBOR_RFC_DIR/68/skip
	echo "cbordump cats bytestreams implicitly" > $CBOR_RFC_DIR/72/skip
}

runtime_error() {
	echo RUNTIME! $1
	num_errs=$(($num_errs + 1))
}

skip() {
	printf "SKIPPED %i (%s)\n" $i "$(cat $2)"
	num_skipped=$(($num_skipped + 1))
}

valgrind_error() {
	echo VALGRIND! $1
	num_errs=$(($num_errs + 1))
}

diff_error() {
	echo DIFF! $1
	num_errs=$(($num_errs + 1))
}

should_fail() {
	echo SHOULD FAIL! $1
	num_errs=$(($num_errs + 1))
}

pass() {
	num_ok=$(($num_ok + 1))
	echo + $1
}

cbordump_retval() {
	xxd -p -r $1 | $NBDIAG -J3 2>/dev/null >/dev/null
	return $?
}

cbordump_vg() {
	vg_errs=$(xxd -p -r $1 | valgrind $NBDIAG -J3 2>&1 | tail -n1 | cut -d ' ' -f4,10)

	if [ "$vg_errs" != "0 0" ]; then
		valgrind_error $test
		got_err=1
		return 1
	else
		pass $test
	fi
}

print_results() {
	echo "$num_ok tests OK, $num_errs ERRORS, $num_skipped suites SKIPPED"
}

run_cbor_positive_tests() {
	for test in $CBOR_LOCAL_DIR/* $CBOR_RFC_DIR/*; do
		if [ -f $test/skip ]; then
			skip $test $test/skip
			continue
		fi

		in=$test/in
		out=$test/out
		out_json=$test/out.json
		out_test=$test/out.test

		xxd -p -r $in | $NBDIAG -J3 > $out_test

		if [ $? -ne 0 ]; then
			runtime_error $test
		else
			pass $test

			cbordump_vg $in

			if [ -f $out ]; then
				if ! diff -q --ignore-all-space $out $out_test >/dev/null; then
					diff_error $test
					diff --ignore-all-space $out $out_test | sed $'s/^/\t/g'
				else
					pass $test
				fi
			elif [ -f $out_json ]; then
				jq_equal=$(jq --slurpfile a $out_test --slurpfile b $out_json -n "\$a == \$b")

				if [ "$jq_equal" != "true" ]; then
					diff_error $test
				else
					pass $test
				fi
			fi
		fi

		#rm $out_test
	done
}


run_cbor_negative_tests() {
	for test in $CBOR_CORRUPT_DIR/* $CBOR_RANDFILES_DIR/*; do
		in=$test/in

		cbordump_retval $in
		if [ $? -eq 2 ]; then
			pass $test
			cbordump_vg $in
		elif [ $? -eq 0 ]; then
			should_fail $test
			cbordump_vg $in
		else
			runtime_error $test
		fi
	done
}


run_io_buf_echo_tests() {
	for test in $IO_DIR/*; do
		out=$test.out
		../build/test-stream $test $out

		if ! diff -q $test $out >/dev/null; then
			diff_error $test
		else
			pass $test
		fi

		rm $out
	done
}


num_skipped=0
num_errs=0
num_ok=0

make --directory=../build all

setup_test_files
run_cbor_positive_tests
run_cbor_negative_tests
run_io_buf_echo_tests
print_results
