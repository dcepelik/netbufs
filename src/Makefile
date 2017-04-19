.PHONY: clean metrics

PROG=run-tests
LIB=libcbor.a
OBJS=main.o cbor.o encode.o decode.o stream.o util.o memory.o stack.o

CFLAGS+=-c -std=gnu11 -Wall -Werror --pedantic -ggdb3 -DDEBUG
LDFLAGS+=-Wall


$(LIB): $(OBJS)
	ar rcs $(LIB) $(OBJS)

$(PROG): $(LIB)
	$(CC) $(LDFALGS) -o $@ $(LIB)

%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f -- $(BINS) *.o

metrics:
	find . -name "*.c" -or -name "*.h" | xargs cat | wc -lc
