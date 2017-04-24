.PHONY: all clean metrics tests

PROG=playground
LIB=libcbor.a
LIBOBJS=cbor.o encode.o decode.o buf.o util.o memory.o stack.o error.o strbuf.o stream.o buf.o
TESTS=test-stream test-decenc test-adhoc test-corrupt
BINS=cbordump

CFLAGS+=-c -std=gnu11 -Wall -Werror --pedantic -ggdb3 -DDEBUG -Wno-unused-function -Wno-unused-variable
LDFLAGS+=-Wall

all: $(LIB) $(TESTS) $(BINS)

$(LIB): $(LIBOBJS)
	ar rcs $(LIB) $(LIBOBJS)

test-stream: test-stream.o $(LIBOBJS)
	$(CC) $(LDFLAGS) -o $@ $^

test-decenc: test-decenc.o $(LIBOBJS)
	$(CC) $(LDFLAGS) -o $@ $^

test-adhoc: test-adhoc.o $(LIBOBJS)
	$(CC) $(LDFLAGS) -o $@ $^

test-corrupt: test-corrupt.o $(LIBOBJS)
	$(CC) $(LDFLAGS) -o $@ $^

cbordump: cbordump.o $(LIBOBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f -- $(LIB) *.o $(TESTS) $(BINS) vgcore.*

metrics:
	find . -name "*.c" -or -name "*.h" | xargs cat | wc -lc

tests: $(TESTS)
	cd ../tests; ./genfiles.sh
	./run-tests.sh
