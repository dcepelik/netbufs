#include "cbor.h"
#include "debug.h"
#include "internal.h"
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


void array_test_encode(struct cbor_stream *cs_out)
{
	char *str1 = "Hello World!";
	unsigned char bytes[8] = { 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49 };

	cbor_encode_array_begin(cs_out, 0);
	cbor_encode_array_end(cs_out);

	cbor_encode_array_begin(cs_out, 1);
		cbor_encode_array_begin(cs_out, 0);
		cbor_encode_array_end(cs_out);
	cbor_encode_array_end(cs_out);

	cbor_encode_array_begin_indef(cs_out);
		cbor_encode_array_begin_indef(cs_out);
			cbor_encode_array_begin(cs_out, 1);
				cbor_encode_array_begin_indef(cs_out);
				cbor_encode_array_end(cs_out);
			cbor_encode_array_end(cs_out);
		cbor_encode_array_end(cs_out);
	cbor_encode_array_end(cs_out);

	cbor_encode_array_begin(cs_out, 3);
		cbor_encode_int32(cs_out, 1);
		cbor_encode_array_begin_indef(cs_out);
			cbor_encode_int32(cs_out, 2);
			cbor_encode_int32(cs_out, 3);
			cbor_encode_array_begin_indef(cs_out);
				cbor_encode_text(cs_out, (unsigned char *)str1, strlen(str1));
				cbor_encode_int32(cs_out, 256);
				cbor_encode_bytes(cs_out, bytes, ARRAY_SIZE(bytes));
			cbor_encode_array_end(cs_out);
		cbor_encode_array_end(cs_out);
		cbor_encode_int32(cs_out, 4);
	cbor_encode_array_end(cs_out);
}


void integers_test_encode(struct cbor_stream *cs_out)
{
	cbor_encode_uint32(cs_out, 0);
	cbor_encode_uint32(cs_out, 1);
	cbor_encode_uint32(cs_out, 2);
	cbor_encode_uint32(cs_out, 23);
	cbor_encode_uint32(cs_out, 24);
	cbor_encode_uint32(cs_out, 255);
	cbor_encode_uint32(cs_out, 256);
	cbor_encode_uint32(cs_out, 16535);
	cbor_encode_uint32(cs_out, 16536);
	cbor_encode_uint32(cs_out, 32767);
	cbor_encode_uint32(cs_out, 32768);

	cbor_encode_int32(cs_out, 0);
	cbor_encode_int32(cs_out, -1);
	cbor_encode_int32(cs_out, -24);
	cbor_encode_int32(cs_out, -25);
	cbor_encode_int32(cs_out, -128);
	cbor_encode_int32(cs_out, -129);
	cbor_encode_int32(cs_out, -256);
	cbor_encode_int32(cs_out, -257);
	cbor_encode_int32(cs_out, -16536);
	cbor_encode_int32(cs_out, -16537);
	cbor_encode_int32(cs_out, -32768);
	cbor_encode_int32(cs_out, -32769);
}


void bytes_test_encode(struct cbor_stream *cs_out)
{
	cbor_encode_bytes(cs_out, uqueen, 0);
	cbor_encode_bytes(cs_out, uqueen, 1);
	cbor_encode_bytes(cs_out, uqueen, strlen(queen));

	unsigned char binary[] = {
		0x0, 0x0, 0x1, 0x2, 0x17, 0x0, 0x2, 254,
		255, 255, 0x11, 0x11
	};

	cbor_encode_bytes(cs_out, binary, ARRAY_SIZE(binary));
}


void text_test_encode(struct cbor_stream *cs_out)
{
	size_t i;

	cbor_encode_text(cs_out, uqueen, 0);
	cbor_encode_text(cs_out, uqueen, 1);
	cbor_encode_text(cs_out, uqueen, strlen(queen));

	/*cbor_encode_text_begin_indef(cs_out);
	for (i = 0; i < ARRAY_SIZE(poem); i++)
		cbor_encode_text(cs_out, (unsigned char *)poem[i], strlen(poem[i]));
	cbor_encode_text_end(cs_out);*/
}


void tags_test_encode(struct cbor_stream *cs_out)
{
	cbor_encode_tag(cs_out, 0);
	cbor_encode_int32(cs_out, 0);

	cbor_encode_tag(cs_out, 1);
	cbor_encode_tag(cs_out, 2);
	cbor_encode_int32(cs_out, 0);
}


void map_test_encode(struct cbor_stream *cs_out)
{
	char *str1 = "Hello World!";
	unsigned char bytes[8] = { 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49 };

	cbor_encode_map_begin(cs_out, 0);
	cbor_encode_map_end(cs_out);

	cbor_encode_map_begin(cs_out, 1);
		cbor_encode_map_begin(cs_out, 0);
		cbor_encode_map_end(cs_out);

		cbor_encode_map_begin(cs_out, 0);
		cbor_encode_map_end(cs_out);
	cbor_encode_map_end(cs_out);

	cbor_encode_map_begin_indef(cs_out);
		cbor_encode_int32(cs_out, 1);

		cbor_encode_map_begin_indef(cs_out);
			cbor_encode_int32(cs_out, 2);
			cbor_encode_map_begin(cs_out, 1);
				cbor_encode_int32(cs_out, 3);
				cbor_encode_map_begin_indef(cs_out);
				cbor_encode_map_end(cs_out);
			cbor_encode_map_end(cs_out);
		cbor_encode_map_end(cs_out);
	cbor_encode_map_end(cs_out);

	cbor_encode_map_begin(cs_out, 2);
		cbor_encode_int32(cs_out, 4);
		cbor_encode_map_begin_indef(cs_out);
			cbor_encode_int32(cs_out, 5);
			cbor_encode_map_begin_indef(cs_out);
				cbor_encode_text(cs_out, (unsigned char *)str1, strlen(str1));
				cbor_encode_int32(cs_out, 256);

				cbor_encode_int32(cs_out, 6);
				cbor_encode_bytes(cs_out, bytes, ARRAY_SIZE(bytes));
			cbor_encode_map_end(cs_out);
		cbor_encode_map_end(cs_out);

		cbor_encode_int32(cs_out, 7);
		cbor_encode_int32(cs_out, 8);
	cbor_encode_map_end(cs_out);
}


void bigmap_test_encode(struct cbor_stream *cs_out)
{
	size_t i;
	size_t a_lot = 1024;
	size_t a_lot_more = 32 * a_lot;

	cbor_encode_map_begin_indef(cs_out);
	for (i = 0; i < a_lot; i++) {
		cbor_encode_int32(cs_out, i);
		cbor_encode_int32(cs_out, i);
	}
	cbor_encode_map_end(cs_out);

	cbor_encode_map_begin(cs_out, a_lot_more);
	for (i = 0; i < a_lot_more; i++) {
		cbor_encode_int32(cs_out, i);
		cbor_encode_int32(cs_out, i);
	}
	cbor_encode_map_end(cs_out);
}


typedef cbor_err_t (action_t)(struct cbor_stream *cs_out);


void nesting_test_encode(struct cbor_stream *cs_out)
{
	size_t num_nests = 4096;
	size_t i;

	for (i = 0; i < num_nests; i++)
		cbor_encode_array_begin_indef(cs_out);

	for (i = 0; i < num_nests; i++)
		cbor_encode_array_end(cs_out);
}


int main(int argc, char *argv[])
{
	struct buf *in;
	struct buf *out;
	struct cbor_stream *cs_in;
	struct cbor_stream *cs_out;
	struct cbor_item item;
	cbor_err_t err;

	uqueen = (unsigned char *)queen;

	in = buf_new();
	assert(in);

	assert(buf_open_file(in, "/home/david/in.cbor", O_RDONLY, 0) == CBOR_ERR_OK);

	cs_in = cbor_stream_new(in);
	assert(cs_in);

	while ((err = cbor_decode_item(cs_in, &item)) == CBOR_ERR_OK) {
		DEBUG_TRACE;
		cbor_item_dump(&item, stderr);
	}

	if (err != CBOR_ERR_EOF) {
		fprintf(stderr, "An error occured while decoding the buf: %s\n",
			cbor_err_to_string(err));
		return EXIT_FAILURE;
	}

	out = buf_new();
	assert(out);

	assert(buf_open_file(out, "/home/david/sw/netbufs/tests/libcbor/nested.cbor", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR) == CBOR_ERR_OK);

	cs_out = cbor_stream_new(out);
	assert(cs_out);

	//array_test_encode(cs_out);
	//integers_test_encode(cs_out);
	//bytes_test_encode(cs_out);
	//text_test_encode(cs_out);
	//tags_test_encode(cs_out);
	//map_test_encode(cs_out);
	//bigmap_test_encode(cs_out);
	nesting_test_encode(cs_out);

	buf_close(in);
	buf_close(out);

	cbor_stream_delete(cs_in);
	cbor_stream_delete(cs_out);

	return EXIT_SUCCESS;
}
