#!/bin/sh
#
#   run-benchmarks:
#   Automated benchmark runner
#

BUILD_DIR=../build
METHODS="binary bird cbor netbufs xml"
BENCHMARK_TOOL=$BUILD_DIR/benchmark
INPUT_TEST=data/rt4.bigger

make --directory=$BUILD_DIR

for method in $METHODS; do
	echo $method
	$BENCHMARK_TOOL $method $INPUT_TEST /tmp/out
done
