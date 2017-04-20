.PHONY: all clean metrics tests

PROG=playground
LIB=libcbor.a
LIBOBJS=cbor.o encode.o decode.o stream.o util.o memory.o stack.o error.o
BINS=test-stream

CFLAGS+=-c -std=gnu11 -Wall -Werror --pedantic -ggdb3 -DDEBUG -Wno-unused-function -Wno-unused-variable
LDFLAGS+=-Wall

all: $(LIB) $(BINS)

$(LIB): $(LIBOBJS)
	ar rcs $(LIB) $(LIBOBJS)

test-stream: test-stream.o $(LIBOBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f -- $(LIB) *.o $(BINS)

metrics:
	find . -name "*.c" -or -name "*.h" | xargs cat | wc -lc

tests: $(BINS)
	./run-tests.sh
