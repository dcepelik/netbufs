/*
 * TODO This should be linked with the die()-ing version of memory allocators.
 *      (we're not checking for NULL return values from _new() functions).
 */
#include "buf.h"
#include "benchmark.h"
#include "util.h"

#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


static struct {
	char *name;
	void (*serialize)(struct rt *rt, struct nb_buf *buf);
	struct rt *(*deserialize)(struct nb_buf *buf);
} methods[] = {
	{ .name = "netbufs", .serialize = serialize_netbufs, .deserialize = deserialize_netbufs },
	{ .name = "cbor", .serialize = serialize_cbor, .deserialize = deserialize_cbor },
	{ .name = "binary", .serialize = serialize_binary, .deserialize = NULL },
	{ .name = "bird", .serialize = serialize_bird, .deserialize = deserialize_bird },
	{ .name = "xml", .serialize = NULL, .deserialize = NULL },
	{ .name = "protobufs", .serialize = NULL, .deserialize = NULL },
};


int main(int argc, char *argv[])
{
	char *method;
	char *fn_in;
	char *fn_out;
	struct rt *rt;
	struct rt *rt2;
	struct nb_buf *in;
	struct nb_buf *out;
	struct nb_buf *mry;
	const char *argv0;
	bool found;
	size_t i;

    clock_t start;
	clock_t end;
	double time_serialize;
	double time_deserialize;

	argv0 = (const char *)basename(argv[0]);

	if (argc != 4) {
		fprintf(stderr, "Usage: %s METHOD INPUT-FILE OUTPUT-FILE\n", argv0);
		return EXIT_FAILURE;
	}

	method = argv[1];
	fn_in = argv[2];
	fn_out = argv[3];

	mry = nb_buf_new();
	nb_buf_open_memory(mry);

	in = nb_buf_new();
	out = nb_buf_new();

	if (nb_buf_open_file(in, fn_in, O_RDONLY, 0) != NB_ERR_OK) {
		fprintf(stderr, "%s: cannot open file '%s'\n", argv0, fn_in);
		return EXIT_FAILURE;
	}
	if (nb_buf_open_file(out, fn_out, O_RDWR | O_CREAT | O_TRUNC, 0666) != NB_ERR_OK) {
		fprintf(stderr, "%s: cannot open file '%s'\n", argv0, fn_out);
		return EXIT_FAILURE;
	}

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

	start = clock();
	methods[i].serialize(rt, mry);
	end = clock();
	time_serialize = ((double) (end - start)) / CLOCKS_PER_SEC;

	nb_buf_flush(mry);

	start = clock();
	rt2 = methods[i].deserialize(mry);
	end = clock();
	time_deserialize = ((double) (end - start)) / CLOCKS_PER_SEC;

	serialize_bird(rt2, out);
	nb_buf_flush(out);

	printf("%lu %.3f %.3f\n", array_size(rt->entries), time_serialize, time_deserialize);
	nb_buf_delete(in);
	nb_buf_delete(mry);
	nb_buf_delete(out);
	return EXIT_SUCCESS;
}
