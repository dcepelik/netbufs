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

struct method
{
	char *name;
	void (*serialize)(struct rt *rt, struct nb_buffer *buf);
	struct rt *(*deserialize)(struct nb_buffer *buf);
};


static struct method methods[] = {
	{ .name = "binary", .serialize = serialize_binary, .deserialize = deserialize_binary },
	{ .name = "bird", .serialize = serialize_bird, .deserialize = deserialize_bird },
	{ .name = "cbor", .serialize = serialize_cbor, .deserialize = deserialize_cbor },
	{ .name = "netbufs", .serialize = serialize_netbufs, .deserialize = deserialize_netbufs },
	{ .name = "protobuf", .serialize = serialize_pb, .deserialize = deserialize_pb },
	{ .name = "xml", .serialize = serialize_xml, .deserialize = deserialize_xml },

	{ .name = "bc-ex1", .serialize = serialize_bc_ex1, .deserialize = NULL },
};


int main(int argc, char *argv[])
{
	char *method_name;
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
	pid_t parent_pid;
	pid_t child_pid;
	char pid_str[16];
	size_t i;

    	clock_t start_s;
	clock_t end_s;
	clock_t start_d;
	clock_t end_d;
	int dev_null_fd;

	struct method *method;

	argv0 = (const char *)basename(argv[0]);

	if (argc != 4) {
		fprintf(stderr, "Usage: %s METHOD INPUT-FILE OUTPUT-FILE\n", argv0);
		return EXIT_FAILURE;
	}

	method_name = argv[1];
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

	for (i = 0, method = NULL; i < ARRAY_SIZE(methods); i++)
		if (strcmp(method_name, methods[i].name) == 0) {
			method = &methods[i];
			break;
		}

	if (!method) {
		fprintf(stderr, "%s: unknown method: '%s'\n", argv0, method_name);
		exit(EXIT_FAILURE);
	}

	rt = deserialize_bird(in);

	parent_pid = getpid();
	child_pid = fork();
	snprintf(pid_str, sizeof(pid_str), "%i", parent_pid);

	if (child_pid == 0) {
		dev_null_fd = open("/dev/null", O_RDWR);
		assert(dev_null_fd != -1);
		dup2(dev_null_fd, STDOUT_FILENO);
		dup2(dev_null_fd, STDERR_FILENO);
		exit(execl("/usr/bin/perf",
			"perf",
			"record",
			"--freq", "8000",
			"-o", "perf.data",
			"-p", pid_str,
			NULL));
	}

	start_s = clock();
	methods[i].serialize(rt, mry);
	end_s = clock();

	nb_buffer_flush(mry);

	start_d = end_d = 0;
	rt2 = NULL;
	if (methods[i].deserialize) {
		start_d = clock();
		rt2 = methods[i].deserialize(mry);
		end_d = clock();
	}

	printf("\t%.3f %.3f %lu\n",
		time_diff(start_s, end_s),
		time_diff(start_d, end_d),
		nb_buffer_get_written_total(mry));

	kill(child_pid, SIGINT);
	waitpid(child_pid, NULL, 0);

	if (methods[i].deserialize && rt2 != NULL) {
		serialize_bird(rt2, out);
		nb_buffer_flush(out);
	}

	close(fd_out);

	nb_buffer_delete(in);
	nb_buffer_delete(mry);
	nb_buffer_delete(out);
	return EXIT_SUCCESS;
}
