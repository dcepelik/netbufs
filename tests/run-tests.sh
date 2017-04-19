#/usr/bin/env sh

make -B --no-print-directory --directory=../src libcbor.a

bins=""
for file in *.c; do
	bin=$(basename $file .c)
	gcc -std=c11 -Wall -Werror --pedantic -I ../src -o $bin $file ../src/libcbor.a
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
	outfile=$(basename $file)
	./streams-echo $file $outfile
	if ! diff $file $outfile; then
		echo -e "\tFAILED $file"
	else
		echo -e "\tOK"
		rm $outfile
	fi
done

rm -f -- $bins
