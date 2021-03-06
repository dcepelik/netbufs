#
# Makefile for Network Buffers
#

.PHONY: all clean metrics

ROOT_DIR = ..
SRC_DIR = $(ROOT_DIR)/src
BENCH_SRC_DIR = $(ROOT_DIR)/benchmark/src

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(subst $(SRC_DIR)/, objs/netbufs/, $(patsubst %.c, %.o, $(SRCS)))
DEPS = $(subst $(SRC_DIR)/, deps/netbufs/, $(patsubst %.c, %.deps, $(SRCS)))
BINS = benchmark test-array test-stream

BENCH_SRCS = $(wildcard $(BENCH_SRC_DIR)/*.c)
BENCH_OBJS = $(subst $(BENCH_SRC_DIR), objs/benchmark, $(patsubst %.c, %.o, $(BENCH_SRCS)))
BENCH_OBJS += $(addprefix objs/benchmark/, pb.o serialize-pb.o deserialize-pb.o)
BENCH_DEPS = $(subst $(BENCH_SRC_DIR), deps/benchmark, $(patsubst %.c, %.deps, $(BENCH_SRCS)))

MAINS = $(addprefix objs/netbufs/, nbdiag.o test-array.o test-stream.o test-adhoc.o)

CFLAGS += -c -std=gnu11 \
	-Wall -Werror --pedantic \
	-O2 \
	-Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable \
	-I $(SRC_DIR)/include \
	-I /usr/include/libxml2 \
	-ggdb3 -DNB_DEBUG

CXXFLAGS += -c \
	-Wall -Werror --pedantic \
	-O2 \
	-Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable \
	-I $(SRC_DIR)/include \
	-ggdb3
LDFLAGS += -Wall -lxml2 -flto

all: $(BINS) nbdiag

nbdiag: $(filter-out $(addprefix ../src/, test-array.c test-stream.c test-adhoc.c benchmark.c), $(SRCS))
	$(CC) -Wall -Werror --pedantic -Wno-unused-function -Wno-unused-variable \
		-Wno-unused-but-set-variable -I$(SRC_DIR)/include -ggdb3 -DNB_DEBUG -DDIAG_ENABLE -o $@ $^

benchmark: $(BENCH_OBJS) $(filter-out $(MAINS),$(OBJS))
	$(CXX) $(LDFLAGS) -o $@ $^ -lprotobuf -pthread

test-stream: $(addprefix objs/netbufs/, test-stream.o buffer.o buffer-file.o memory.o util.o)
	$(CC) $(LDFLAGS) -o $@ $^

test-array: $(addprefix objs/netbufs/, test-array.o array.o memory.o util.o)
	$(CC) $(LDFLAGS) -o $@ $^

objs/netbufs/%.o: $(SRC_DIR)/%.c deps/netbufs/%.deps
	$(CC) $(CFLAGS) -o $@ $<

objs/benchmark/serialize-pb.o: $(BENCH_SRC_DIR)/serialize-pb.cc
	$(CXX) $(CXXFLAGS) -o $@ $<

objs/benchmark/deserialize-pb.o: $(BENCH_SRC_DIR)/deserialize-pb.cc
	$(CXX) $(CXXFLAGS) -o $@ $<

objs/benchmark/pb.o: $(BENCH_SRC_DIR)/gen/rt.pb.cc $(BENCH_SRC_DIR)/gen/rt.pb.h
	$(CXX) $(CXXFLAGS) -o $@ $<

gen: $(BENCH_SRC_DIR)/rt.proto
	 protoc -I$(BENCH_SRC_DIR) --cpp_out=$(BENCH_SRC_DIR)/gen $<

deps/netbufs/%.deps: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -MM -MT objs/$*.o -o $@ $<; sed 's%:% $@:%' $@ > $@.tmp; mv $@.tmp $@

objs/benchmark/%.o: $(BENCH_SRC_DIR)/%.c deps/benchmark/%.deps
	$(CC) $(CFLAGS) -o $@ $<

deps/benchmark/%.deps: $(BENCH_SRC_DIR)/%.c
	$(CC) $(CFLAGS) -MM -MT objs/$*.o -o $@ $<; sed 's%:% $@:%' $@ > $@.tmp; mv $@.tmp $@

include $(DEPS) $(BENCH_DEPS)

clean:
	rm -f -- $(BINS) $(OBJS) $(BENCH_OBJS) $(DEPS) $(BENCH_DEPS) vgcore.*

metrics:
	for src_dir in $(SRC_DIR) $(BENCH_SRC_DIR) $(ROOT_DIR); do \
		echo $$src_dir:; \
		find $$src_dir -name "*.c" -or -name "*.h" | xargs cat | wc -cl; \
	done
