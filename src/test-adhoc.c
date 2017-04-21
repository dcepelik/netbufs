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


void array_test_encode(struct cbor_encoder *enc)
{
	char *str1 = "Hello World!";
	unsigned char bytes[8] = { 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49 };

	cbor_encode_array_begin(enc, 0);
	cbor_encode_array_end(enc);

	cbor_encode_array_begin(enc, 1);
		cbor_encode_array_begin(enc, 0);
		cbor_encode_array_end(enc);
	cbor_encode_array_end(enc);

	cbor_encode_array_begin_indef(enc);
		cbor_encode_array_begin_indef(enc);
			cbor_encode_array_begin(enc, 1);
				cbor_encode_array_begin_indef(enc);
				cbor_encode_array_end(enc);
			cbor_encode_array_end(enc);
		cbor_encode_array_end(enc);
	cbor_encode_array_end(enc);

	cbor_encode_array_begin(enc, 3);
		cbor_encode_int32(enc, 1);
		cbor_encode_array_begin_indef(enc);
			cbor_encode_int32(enc, 2);
			cbor_encode_int32(enc, 3);
			cbor_encode_array_begin_indef(enc);
				cbor_encode_text(enc, (unsigned char *)str1, strlen(str1));
				cbor_encode_int32(enc, 256);
				cbor_encode_bytes(enc, bytes, ARRAY_SIZE(bytes));
			cbor_encode_array_end(enc);
		cbor_encode_array_end(enc);
		cbor_encode_int32(enc, 4);
	cbor_encode_array_end(enc);
}


void integers_test_encode(struct cbor_encoder *enc)
{
	cbor_encode_uint32(enc, 0);
	cbor_encode_uint32(enc, 1);
	cbor_encode_uint32(enc, 2);
	cbor_encode_uint32(enc, 23);
	cbor_encode_uint32(enc, 24);
	cbor_encode_uint32(enc, 255);
	cbor_encode_uint32(enc, 256);
	cbor_encode_uint32(enc, 16535);
	cbor_encode_uint32(enc, 16536);
	cbor_encode_uint32(enc, 32767);
	cbor_encode_uint32(enc, 32768);

	cbor_encode_int32(enc, 0);
	cbor_encode_int32(enc, -1);
	cbor_encode_int32(enc, -24);
	cbor_encode_int32(enc, -25);
	cbor_encode_int32(enc, -128);
	cbor_encode_int32(enc, -129);
	cbor_encode_int32(enc, -256);
	cbor_encode_int32(enc, -257);
	cbor_encode_int32(enc, -16536);
	cbor_encode_int32(enc, -16537);
	cbor_encode_int32(enc, -32768);
	cbor_encode_int32(enc, -32769);
}


void bytes_test_encode(struct cbor_encoder *enc)
{
	cbor_encode_bytes(enc, uqueen, 0);
	cbor_encode_bytes(enc, uqueen, 1);
	cbor_encode_bytes(enc, uqueen, strlen(queen));

	unsigned char binary[] = {
		0x0, 0x0, 0x1, 0x2, 0x17, 0x0, 0x2, 254,
		255, 255, 0x11, 0x11
	};

	cbor_encode_bytes(enc, binary, ARRAY_SIZE(binary));
}


void text_test_encode(struct cbor_encoder *enc)
{
	size_t i;

	cbor_encode_text(enc, uqueen, 0);
	cbor_encode_text(enc, uqueen, 1);
	cbor_encode_text(enc, uqueen, strlen(queen));

	/*cbor_encode_text_begin_indef(enc);
	for (i = 0; i < ARRAY_SIZE(poem); i++)
		cbor_encode_text(enc, (unsigned char *)poem[i], strlen(poem[i]));
	cbor_encode_text_end(enc);*/
}


void tags_test_encode(struct cbor_encoder *enc)
{
	cbor_encode_tag(enc, 0);
	cbor_encode_int32(enc, 0);

	cbor_encode_tag(enc, 1);
	cbor_encode_tag(enc, 2);
	cbor_encode_int32(enc, 0);
}


void map_test_encode(struct cbor_encoder *enc)
{
	char *str1 = "Hello World!";
	unsigned char bytes[8] = { 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49 };

	cbor_encode_map_begin(enc, 0);
	cbor_encode_map_end(enc);

	cbor_encode_map_begin(enc, 1);
		cbor_encode_map_begin(enc, 0);
		cbor_encode_map_end(enc);

		cbor_encode_map_begin(enc, 0);
		cbor_encode_map_end(enc);
	cbor_encode_map_end(enc);

	cbor_encode_map_begin_indef(enc);
		cbor_encode_int32(enc, 1);

		cbor_encode_map_begin_indef(enc);
			cbor_encode_int32(enc, 2);
			cbor_encode_map_begin(enc, 1);
				cbor_encode_int32(enc, 3);
				cbor_encode_map_begin_indef(enc);
				cbor_encode_map_end(enc);
			cbor_encode_map_end(enc);
		cbor_encode_map_end(enc);
	cbor_encode_map_end(enc);

	cbor_encode_map_begin(enc, 2);
		cbor_encode_int32(enc, 4);
		cbor_encode_map_begin_indef(enc);
			cbor_encode_int32(enc, 5);
			cbor_encode_map_begin_indef(enc);
				cbor_encode_text(enc, (unsigned char *)str1, strlen(str1));
				cbor_encode_int32(enc, 256);

				cbor_encode_int32(enc, 6);
				cbor_encode_bytes(enc, bytes, ARRAY_SIZE(bytes));
			cbor_encode_map_end(enc);
		cbor_encode_map_end(enc);

		cbor_encode_int32(enc, 7);
		cbor_encode_int32(enc, 8);
	cbor_encode_map_end(enc);
}


void bigmap_test_encode(struct cbor_encoder *enc)
{
	size_t i;
	size_t a_lot = 1024;
	size_t a_lot_more = 32 * a_lot;

	cbor_encode_map_begin_indef(enc);
	for (i = 0; i < a_lot; i++) {
		cbor_encode_int32(enc, i);
		cbor_encode_int32(enc, i);
	}
	cbor_encode_map_end(enc);

	cbor_encode_map_begin(enc, a_lot_more);
	for (i = 0; i < a_lot_more; i++) {
		cbor_encode_int32(enc, i);
		cbor_encode_int32(enc, i);
	}
	cbor_encode_map_end(enc);
}


typedef cbor_err_t (action_t)(struct cbor_encoder *enc);


void nesting_test_encode(struct cbor_encoder *enc)
{
	size_t num_nests = 4096;
	size_t i;

	for (i = 0; i < num_nests; i++)
		cbor_encode_array_begin_indef(enc);

	for (i = 0; i < num_nests; i++)
		cbor_encode_array_end(enc);
}


int main(int argc, char *argv[])
{
	struct cbor_stream *in;
	struct cbor_stream *out;
	struct cbor_decoder *dec;
	struct cbor_encoder *enc;
	struct cbor_item item;
	cbor_err_t err;

	uqueen = (unsigned char *)queen;

	in = cbor_stream_new();
	assert(in);

	assert(cbor_stream_open_file(in, "/home/david/in.cbor", O_RDONLY, 0) == CBOR_ERR_OK);

	dec = cbor_decoder_new(in);
	assert(dec);

	while ((err = cbor_decode_item(dec, &item)) == CBOR_ERR_OK) {
		DEBUG_TRACE;
		cbor_item_dump(&item);
	}

	if (err != CBOR_ERR_EOF) {
		fprintf(stderr, "An error occured while decoding the stream: %s\n",
			cbor_err_to_string(err));
		return EXIT_FAILURE;
	}

	out = cbor_stream_new();
	assert(out);

	assert(cbor_stream_open_file(out, "/home/david/sw/netbufs/tests/libcbor/nested.cbor", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR) == CBOR_ERR_OK);

	enc = cbor_encoder_new(out);
	assert(enc);

	//array_test_encode(enc);
	//integers_test_encode(enc);
	//bytes_test_encode(enc);
	//text_test_encode(enc);
	//tags_test_encode(enc);
	//map_test_encode(enc);
	//bigmap_test_encode(enc);
	nesting_test_encode(enc);

	cbor_stream_close(in);
	cbor_stream_close(out);

	cbor_decoder_delete(dec);
	cbor_encoder_delete(enc);

	return EXIT_SUCCESS;
}
