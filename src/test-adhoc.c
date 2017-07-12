/* TODO update this or delete this */

#include "cbor.h"
#include "debug.h"
#include "util.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *queen = "Nothing really matters, anyone can see: nothing really matters, nothing really matters to me.";
unsigned char *uqueen;

char *poem[] = {
	"Zdravím tě hnilobo, jak ti tleje?",
	"Z té sklínky tě nikdo už neumeje",
	"Snad ti kafe přijde k chuti",
	"Kafe to já tuze rád!",
	"Zdravím tě hnilobo, jak ti tleje?",
	"Jsem tvůj postižený kamarád.",
};


void array_test_encode(struct cbor_stream *cs)
{
	char *str1 = "Hello World!";
	unsigned char bytes[8] = { 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49 };

	cbor_encode_array_begin(cs, 0);
	cbor_encode_array_end(cs);

	cbor_encode_array_begin(cs, 1);
		cbor_encode_array_begin(cs, 0);
		cbor_encode_array_end(cs);
	cbor_encode_array_end(cs);

	cbor_encode_array_begin_indef(cs);
		cbor_encode_array_begin_indef(cs);
			cbor_encode_array_begin(cs, 1);
				cbor_encode_array_begin_indef(cs);
				cbor_encode_array_end(cs);
			cbor_encode_array_end(cs);
		cbor_encode_array_end(cs);
	cbor_encode_array_end(cs);

	cbor_encode_array_begin(cs, 3);
		cbor_encode_int32(cs, 1);
		cbor_encode_array_begin_indef(cs);
			cbor_encode_int32(cs, 2);
			cbor_encode_int32(cs, 3);
			cbor_encode_array_begin_indef(cs);
				cbor_encode_text(cs, (unsigned char *)str1, strlen(str1));
				cbor_encode_int32(cs, 256);
				cbor_encode_bytes(cs, bytes, ARRAY_SIZE(bytes));
			cbor_encode_array_end(cs);
		cbor_encode_array_end(cs);
		cbor_encode_int32(cs, 4);
	cbor_encode_array_end(cs);
}


void integers_test_encode(struct cbor_stream *cs)
{
	cbor_encode_uint32(cs, 0);
	cbor_encode_uint32(cs, 1);
	cbor_encode_uint32(cs, 2);
	cbor_encode_uint32(cs, 23);
	cbor_encode_uint32(cs, 24);
	cbor_encode_uint32(cs, 255);
	cbor_encode_uint32(cs, 256);
	cbor_encode_uint32(cs, 16535);
	cbor_encode_uint32(cs, 16536);
	cbor_encode_uint32(cs, 32767);
	cbor_encode_uint32(cs, 32768);

	cbor_encode_int32(cs, 0);
	cbor_encode_int32(cs, -1);
	cbor_encode_int32(cs, -24);
	cbor_encode_int32(cs, -25);
	cbor_encode_int32(cs, -128);
	cbor_encode_int32(cs, -129);
	cbor_encode_int32(cs, -256);
	cbor_encode_int32(cs, -257);
	cbor_encode_int32(cs, -16536);
	cbor_encode_int32(cs, -16537);
	cbor_encode_int32(cs, -32768);
	cbor_encode_int32(cs, -32769);
}


void bytes_test_encode(struct cbor_stream *cs)
{
	cbor_encode_bytes(cs, uqueen, 0);
	cbor_encode_bytes(cs, uqueen, 1);
	cbor_encode_bytes(cs, uqueen, strlen(queen));

	unsigned char binary[] = {
		0x0, 0x0, 0x1, 0x2, 0x17, 0x0, 0x2, 254,
		255, 255, 0x11, 0x11
	};

	cbor_encode_bytes(cs, binary, ARRAY_SIZE(binary));
}


void text_test_encode(struct cbor_stream *cs)
{
	size_t i;

	cbor_encode_text(cs, uqueen, 0);
	cbor_encode_text(cs, uqueen, 1);
	cbor_encode_text(cs, uqueen, strlen(queen));

	/*cbor_encode_text_begin_indef(cs);
	for (i = 0; i < ARRAY_SIZE(poem); i++)
		cbor_encode_text(cs, (unsigned char *)poem[i], strlen(poem[i]));
	cbor_encode_text_end(cs);*/
}


void tags_test_encode(struct cbor_stream *cs)
{
	cbor_encode_tag(cs, 0);
	cbor_encode_int32(cs, 0);

	cbor_encode_tag(cs, 1);
	cbor_encode_tag(cs, 2);
	cbor_encode_int32(cs, 0);
}


void map_test_encode(struct cbor_stream *cs)
{
	char *str1 = "Hello World!";
	unsigned char bytes[8] = { 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49 };

	cbor_encode_map_begin(cs, 0);
	cbor_encode_map_end(cs);

	cbor_encode_map_begin(cs, 1);
		cbor_encode_map_begin(cs, 0);
		cbor_encode_map_end(cs);

		cbor_encode_map_begin(cs, 0);
		cbor_encode_map_end(cs);
	cbor_encode_map_end(cs);

	cbor_encode_map_begin_indef(cs);
		cbor_encode_int32(cs, 1);

		cbor_encode_map_begin_indef(cs);
			cbor_encode_int32(cs, 2);
			cbor_encode_map_begin(cs, 1);
				cbor_encode_int32(cs, 3);
				cbor_encode_map_begin_indef(cs);
				cbor_encode_map_end(cs);
			cbor_encode_map_end(cs);
		cbor_encode_map_end(cs);
	cbor_encode_map_end(cs);

	cbor_encode_map_begin(cs, 2);
		cbor_encode_int32(cs, 4);
		cbor_encode_map_begin_indef(cs);
			cbor_encode_int32(cs, 5);
			cbor_encode_map_begin_indef(cs);
				cbor_encode_text(cs, (unsigned char *)str1, strlen(str1));
				cbor_encode_int32(cs, 256);

				cbor_encode_int32(cs, 6);
				cbor_encode_bytes(cs, bytes, ARRAY_SIZE(bytes));
			cbor_encode_map_end(cs);
		cbor_encode_map_end(cs);

		cbor_encode_int32(cs, 7);
		cbor_encode_int32(cs, 8);
	cbor_encode_map_end(cs);
}


void bigmap_test_encode(struct cbor_stream *cs)
{
	size_t i;
	size_t a_lot = 1024;
	size_t a_lot_more = 32 * a_lot;

	cbor_encode_map_begin_indef(cs);
	for (i = 0; i < a_lot; i++) {
		cbor_encode_int32(cs, i);
		cbor_encode_int32(cs, i);
	}
	cbor_encode_map_end(cs);

	cbor_encode_map_begin(cs, a_lot_more);
	for (i = 0; i < a_lot_more; i++) {
		cbor_encode_int32(cs, i);
		cbor_encode_int32(cs, i);
	}
	cbor_encode_map_end(cs);
}


void arraysnmaps_test_encode(struct cbor_stream *cs)
{
	size_t i;
	char *str1 = "First Key";
	char *str2 = "Some Integers";

	cbor_encode_array_begin_indef(cs);
		for (i = 0; i < 4; i++)
			cbor_encode_int32(cs, i + 1);
	cbor_encode_array_end(cs);

	cbor_encode_array_begin_indef(cs);
		cbor_encode_map_begin(cs, 2);
			cbor_encode_int32(cs, 1);
			cbor_encode_text(cs, (byte_t *)str1, strlen(str1));

			cbor_encode_int32(cs, 2);
			cbor_encode_array_begin(cs, 2);
				cbor_encode_int32(cs, 3);
				cbor_encode_int32(cs, 4);
			cbor_encode_array_end(cs);
		cbor_encode_map_end(cs);

		cbor_encode_map_begin_indef(cs);
			cbor_encode_text(cs, (byte_t *)str2, strlen(str2));
			cbor_encode_array_begin(cs, 8);
				for (i = 0; i < 8; i++)
					cbor_encode_int32(cs, i + 1);
			cbor_encode_array_end(cs);
		cbor_encode_map_end(cs);
	cbor_encode_array_end(cs);
}


typedef nb_err_t (action_t)(struct cbor_stream *cs);


void nesting_test_encode(struct cbor_stream *cs)
{
	size_t num_nests = 4096;
	size_t i;

	for (i = 0; i < num_nests; i++)
		cbor_encode_array_begin_indef(cs);

	for (i = 0; i < num_nests; i++)
		cbor_encode_array_end(cs);
}


int main(int argc, char *argv[])
{
	struct nb_buf *out;
	struct cbor_stream cs;
	nb_err_t err;
	int mode;
	int oflags;

	out = nb_buf_new();
	assert(out);

	mode = S_IRUSR | S_IWUSR;
	oflags = O_RDWR | O_CREAT | O_TRUNC;

	assert(nb_buf_open_file(out, "../tests/libcbor/arrays+maps.cbor", oflags, mode) == NB_ERR_OK);

	cbor_stream_new(&cs, out);

	uqueen = (unsigned char *)queen;
	//array_test_encode(&cs);
	//integers_test_encode(&cs);
	//bytes_test_encode(&cs);
	//text_test_encode(&cs);
	//tags_test_encode(&cs);
	//map_test_encode(&cs);
	//bigmap_test_encode(&cs);
	//nesting_test_encode(&cs);
	arraysnmaps_test_encode(&cs);

	if (cs->err != NB_ERR_OK) {
		fprintf(stderr, "Error while encoding the stream: %s\n",
			cbor_stream_strerror(&cs));
		return EXIT_FAILURE;
	}

	nb_buf_close(out);
	cbor_stream_free(&cs);

	return EXIT_SUCCESS;
}
