.PHONY: clean

PROG=benchmark
OBJS=parse.o main.o util.o

CFLAGS+=-c -std=gnu11 -Wno-unused-function -Wall -Werror --pedantic -ggdb3 -DDEBUG
LDFLAGS+=-Wall

$(PROG): $(OBJS)
	$(CC) $(LDFALGS) -o $@ $^


%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f -- $(BINS) *.o
