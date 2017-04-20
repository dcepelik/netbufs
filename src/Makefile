.PHONY: clean metrics tests all

PROG=playground
LIB=libcbor.a
OBJS=cbor.o encode.o decode.o stream.o util.o memory.o stack.o error.o

CFLAGS+=-c -std=gnu11 -Wall -Werror --pedantic -ggdb3 -DDEBUG -Wno-unused-function -Wno-unused-variable
LDFLAGS+=-Wall

$(LIB): $(OBJS)
	ar rcs $(LIB) $(OBJS)

$(PROG): $(OBJS) main.o
	$(CC) $(LDFALGS) -o $@ $^

%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f -- $(LIB) $(PROG) $(OBJS)

metrics:
	find . -name "*.c" -or -name "*.h" | xargs cat | wc -lc

tests:
	cd ../tests; ./run-tests.sh
