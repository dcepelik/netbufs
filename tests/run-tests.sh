#/usr/bin/env sh

make -Bs --no-print-directory --directory=../src libcbor.a

bins=""
for file in *.c; do
	bin=$(basename $file .c)
	gcc -std=c11 -Wall -Werror --pedantic -DDEBUG -I ../src -o $bin $file ../src/libcbor.a
	bins="$bins $bin"
done

random_sizes="1 1k 8k 1M 16M"
for size in $random_sizes; do
	if [ ! -f examples/random-$size ]; then
		dd if=/dev/urandom of=examples/random-$size bs=$size count=1
	fi
done

echo "stream-echo:"
for file in examples/random-*; do
	outfile=$(basename $file).out
	./streams-echo $file $outfile

	if ! diff $file $outfile; then
		echo -e "\tFAILED $file"
	else
		echo -e "\tOK"
	fi

	rm -f $outfile
done
#
#echo "decode-encode:"
#for file in examples/*.cbor; do
#	outfile=$(basename $file .cbor)
#	./decode-encode $file $outfile
#
#	if ! diff $file $outfile; then
#		echo -e "\tFAILED $file"
#	else
#		echo -e "\tOK"
#	fi
#
#	rm -f $outfile
#done
#
##rm -f -- $bins
