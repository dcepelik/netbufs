#!/bin/sh
#
#   run-benchmarks:
#   Automated benchmark runner
#

BUILD_DIR=../build
METHODS="binary cbor netbufs protobuf"
BENCHMARK_TOOL=$BUILD_DIR/benchmark
INPUT_TEST=data/rt4

make --directory=$BUILD_DIR

for method in $METHODS; do
	result=$($BENCHMARK_TOOL $method $INPUT_TEST /tmp/out)
	send=$(echo $result | cut -d' ' -f1)
	recv=$(echo $result | cut -d' ' -f2)
	size=$(echo $result | cut -d' ' -f3)
	hsize=$(numfmt --to=iec-i --suffix=B --format="%.3f" $size)

	echo "\tt $method & $send s & $recv s & $hsize\crl"
done
