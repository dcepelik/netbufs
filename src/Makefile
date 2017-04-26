.PHONY: all clean metrics tests

LIB=libcbor.a
OBJS=cbor.o encode.o decode.o buf.o util.o memory.o stack.o strbuf.o stream.o buf.o array.o
BENCHMARK_OBJS=serialize-bird.o parse.o
TESTS=test-stream test-adhoc test-array
BINS=cbordump benchmark

CFLAGS+=-c -std=gnu11 -Wall -Werror --pedantic -ggdb3 -DDEBUG -Wno-unused-function -Wno-unused-variable
LDFLAGS+=-Wall

all: $(LIB) $(TESTS) $(BINS)

$(LIB): $(OBJS)
	ar rcs $(LIB) $(OBJS)

test-stream: test-stream.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

test-adhoc: test-adhoc.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

test-array: test-array.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

cbordump: cbordump.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

benchmark: benchmark.o $(OBJS) $(BENCHMARK_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f -- $(LIB) *.o $(TESTS) $(BINS) vgcore.*

metrics:
	find . -name "*.c" -or -name "*.h" | xargs cat | wc -lc

tests: $(TESTS)
	cd ../tests; ./run-tests.sh
