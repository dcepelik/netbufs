/*
 * TODO This should be linked with the die()-ing version of memory allocators.
 *	(we're not checking for NULL return values from _new() functions).
 */
#include "buf.h"
#include "benchmark.h"

#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


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

    clock_t start;
	clock_t end;
	double cpu_time_used;

	argv0 = (const char *)basename(argv[0]);

	if (argc != 4) {
		fprintf(stderr, "Usage: %s <method> <input-file> <output-file>\n", argv0);
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

	printf("Using bird_deserialize() to load RT from '%s'\n", fn_in);

	rt = deserialize_bird(in);

	printf("RT loaded with %lu entries\n", array_size(rt->entries));
	printf("Transferring data using method '%s'\n", method);

	start = clock();
	if (strcmp(method, "bird") == 0) {
		serialize_bird(rt, mry);
		nb_buf_flush(mry);
		rt2 = deserialize_bird(mry);

	}
	else if (strcmp(method, "cbor") == 0) {
		serialize_cbor(rt, mry);
		nb_buf_flush(mry);
		rt2 = deserialize_cbor(mry);
	}
	else if (strcmp(method, "netbufs") == 0) {
		serialize_netbufs(rt, out);
		nb_buf_flush(out);
		//rt2 = deserialize_netbufs(mry);
	}
	else {
		fprintf(stderr, "%s: unknown method: '%s'\n", argv0, method);
		exit(EXIT_FAILURE);
	}
	end = clock();

	printf("Using bird_serialize() to write RT back to '%s'\n", fn_out);
	//serialize_bird(rt2, out);
	//nb_buf_flush(out);

	cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	printf("%.3f\n", cpu_time_used);
	nb_buf_delete(in);
	nb_buf_delete(mry);
	nb_buf_delete(out);
	return EXIT_SUCCESS;
}
