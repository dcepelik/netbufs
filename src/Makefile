.PHONY: all clean metrics tests

OBJS = cbor.o encode.o decode.o buf.o util.o memory.o stack.o strbuf.o stream.o buf.o array.o netbufs.o
BENCHMARK_OBJS = serialize-bird.o deserialize-bird.o parser.o\
	serialize-cbor.o deserialize-cbor.o\
	serialize-netbufs.o deserialize-netbufs.o
TESTS = test-stream test-adhoc test-array
BINS = cbordump benchmark

CFLAGS += -c -std=gnu11 -Wall -Werror --pedantic -ggdb3 -DDEBUG -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable
LDFLAGS += -Wall

COMMON_HEADERS = memory.h internal.h error.h util.h
SRCS = $(wildcard *.c)

all: $(LIB) $(TESTS) $(BINS)

%.d: %.c
	gcc -MM -o $@ $<; sed 's/:/ $@:/' $@ > $@.tmp; mv $@.tmp $@

%.o: %.c %.d
	$(CC) $(CFLAGS) -o $@ $<

include $(SRCS:.c=.d)

test-stream: test-stream.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

test-adhoc: test-adhoc.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

test-array: test-array.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

test-netbufs: test-netbufs.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

cbordump: cbordump.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

benchmark: benchmark.o $(OBJS) $(BENCHMARK_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

clean:
	rm -f -- *.o *.d $(TESTS) $(BINS) vgcore.*

metrics:
	find . -name "*.c" -or -name "*.h" | xargs cat | wc -lc

tests: $(TESTS) $(BINS)
	cd ../tests; ./run-tests.sh
