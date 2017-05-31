.PHONY: clean all

ROOT_DIR = ..
SRC_DIR = $(ROOT_DIR)/src
BENCH_SRC_DIR = $(ROOT_DIR)/benchmark/src

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(subst $(SRC_DIR)/, objs/netbufs/, $(patsubst %.c, %.o, $(SRCS)))
DEPS = $(subst $(SRC_DIR)/, deps/netbufs/, $(patsubst %.c, %.deps, $(SRCS)))
BINS = benchmark cbordump test-array test-stream

BENCH_SRCS = $(wildcard $(BENCH_SRC_DIR)/*.c)
BENCH_OBJS = $(subst $(BENCH_SRC_DIR), objs/benchmark, $(patsubst %.c, %.o, $(BENCH_SRCS)))
BENCH_DEPS = $(subst $(BENCH_SRC_DIR), deps/benchmark, $(patsubst %.c, %.deps, $(BENCH_SRCS)))

MAINS = $(addprefix objs/netbufs/, cbordump.o test-array.o test-stream.o test-adhoc.o)

CFLAGS += -c -std=gnu11 -Wall -Werror --pedantic \
	-Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable \
	-I $(SRC_DIR) \
	-ggdb3 -DDEBUG

LDFLAGS += -Wall

all: $(BINS)

cbordump: $(filter-out $(MAINS), $(OBJS)) objs/netbufs/cbordump.o
	$(CC) $(LDFLAGS) -o $@ $^

benchmark: $(BENCH_OBJS) $(filter-out $(MAINS), $(OBJS))
	$(CC) $(LDFLAGS) -o $@ $^

test-stream: $(addprefix objs/netbufs/, test-stream.o buf.o memory.o)
	$(CC) $(LDFLAGS) -o $@ $^

test-array: $(addprefix objs/netbufs/, test-array.o array.o memory.o)
	$(CC) $(LDFLAGS) -o $@ $^

objs/netbufs/%.o: $(SRC_DIR)/%.c deps/netbufs/%.deps
	$(CC) $(CFLAGS) -o $@ $<

deps/netbufs/%.deps: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -MM -MT objs/$*.o -o $@ $<; sed 's%:% $@:%' $@ > $@.tmp; mv $@.tmp $@

objs/benchmark/%.o: $(BENCH_SRC_DIR)/%.c deps/benchmark/%.deps
	$(CC) $(CFLAGS) -o $@ $<

deps/benchmark/%.deps: $(BENCH_SRC_DIR)/%.c
	$(CC) $(CFLAGS) -MM -MT objs/$*.o -o $@ $<; sed 's%:% $@:%' $@ > $@.tmp; mv $@.tmp $@

include $(DEPS) $(BENCH_DEPS)

clean:
	rm -f -- $(BINS) $(OBJS) $(BENCH_OBJS) $(DEPS) $(BENCH_DEPS)
