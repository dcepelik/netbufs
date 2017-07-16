/*
 * TODO This should be linked with the die()-ing version of memory allocators.
 *      (we're not checking for NULL return values from _new() functions).
 */
#include "buffer.h"
#include "benchmark.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>


static struct {
	char *name;
	void (*serialize)(struct rt *rt, struct nb_buffer *buf);
	struct rt *(*deserialize)(struct nb_buffer *buf);
} methods[] = {
	{ .name = "binary", .serialize = serialize_binary, .deserialize = deserialize_binary },
	{ .name = "bird", .serialize = serialize_bird, .deserialize = deserialize_bird },
	{ .name = "cbor", .serialize = serialize_cbor, .deserialize = NULL }, //deserialize_cbor },
	{ .name = "netbufs", .serialize = serialize_netbufs, .deserialize = NULL }, //deserialize_netbufs },
	{ .name = "protobuf", .serialize = serialize_pb, .deserialize = NULL },
	{ .name = "xml", .serialize = serialize_xml, .deserialize = deserialize_xml },

	{ .name = "bc-ex1", .serialize = serialize_bc_ex1, .deserialize = NULL },
};


int main(int argc, char *argv[])
{
	char *method;
	char *fn_in;
	char *fn_out;
	int fd_in;
	int fd_out;
	struct rt *rt;
	struct rt *rt2;
	struct nb_buffer *in;
	struct nb_buffer *out;
	struct nb_buffer *mry;
	const char *argv0;
	bool found;
	size_t i;

    	clock_t start_s;
	clock_t end_s;
	double time_serialize;
	double time_deserialize = 0;

	argv0 = (const char *)basename(argv[0]);

	if (argc != 4) {
		fprintf(stderr, "Usage: %s METHOD INPUT-FILE OUTPUT-FILE\n", argv0);
		return EXIT_FAILURE;
	}

	method = argv[1];
	fn_in = argv[2];
	fn_out = argv[3];

	if ((fd_in = open(fn_in, O_RDONLY, 0)) == -1) {
		fprintf(stderr, "%s: cannot open input file '%s' for reading: %s\n",
			argv0, fn_in, strerror(errno));
		return EXIT_FAILURE;
	}
	if ((fd_out = open(fn_out, O_RDWR | O_CREAT | O_TRUNC, 0666)) == -1) {
		fprintf(stderr, "%s: cannot open output file '%s': %s\n",
			argv0, fn_out, strerror(errno));
		return EXIT_FAILURE;
	}

	in = nb_buffer_new_file(fd_in);
	out = nb_buffer_new_file(fd_out);
	mry = nb_buffer_new_memory();

	rt = deserialize_bird(in);

	for (i = 0, found = false; i < ARRAY_SIZE(methods); i++)
		if (strcmp(method, methods[i].name) == 0) {
			found = true;
			break;
		}

	if (!found) {
		fprintf(stderr, "%s: unknown method: '%s'\n", argv0, method);
		exit(EXIT_FAILURE);
	}

	pid_t pid;
	pid_t child;
	pid = getpid();
	child = fork();
	char pid_str[16];
	snprintf(pid_str, sizeof(pid_str), "%i", pid);
	if (child == 0) {
		int dev_null = open("/dev/null", O_RDWR);
		assert(dev_null != -1);
		dup2(dev_null, STDOUT_FILENO);
		dup2(dev_null, STDERR_FILENO);
		exit(execl("/usr/bin/perf", "perf", "record", "-o", "perf.data",
			"-p", pid_str, NULL));
	}

	if (methods[i].deserialize) {
		start_s = clock();
		methods[i].serialize(rt, mry);
		end_s = clock();
		time_serialize = ((double) (end_s - start_s)) / CLOCKS_PER_SEC;

		nb_buffer_flush(mry);

		start_s = clock();
		rt2 = methods[i].deserialize(mry);
		end_s = clock();
		time_deserialize = ((double) (end_s - start_s)) / CLOCKS_PER_SEC;

		printf("%lu %.3f %.3f\n", array_size(rt->entries), time_serialize, time_deserialize);
	}
	else {
		start_s = clock();
		methods[i].serialize(rt, out);
		nb_buffer_flush(out);
		end_s = clock();
		time_serialize = ((double) (end_s - start_s)) / CLOCKS_PER_SEC;
		printf("%lu %.3f\n", array_size(rt->entries), time_serialize);
	}

	kill(child, SIGINT);
	waitpid(child, NULL, 0);

	if (methods[i].deserialize) {
		serialize_bird(rt2, out);
		nb_buffer_flush(out);
	}

	close(fd_out);

	nb_buffer_delete(in);
	nb_buffer_delete(mry);
	nb_buffer_delete(out);
	return EXIT_SUCCESS;
}
