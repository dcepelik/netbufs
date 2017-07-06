#!/bin/sh
#
#   run-benchmarks:
#   Automated benchmark runner
#

BUILD_DIR=../build
METHODS="binary bird cbor netbufs"
BENCHMARK_TOOL=$BUILD_DIR/benchmark
INPUT_TEST=data/rt4

make --directory=$BUILD_DIR

for method in $METHODS; do
	echo $method
	$BENCHMARK_TOOL $method $INPUT_TEST /tmp/out
done
