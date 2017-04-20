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

	cbor_array_encode_begin(enc, 0);
	cbor_array_encode_end(enc);

	cbor_array_encode_begin(enc, 1);
		cbor_array_encode_begin(enc, 0);
		cbor_array_encode_end(enc);
	cbor_array_encode_end(enc);

	cbor_array_encode_begin_indef(enc);
		cbor_array_encode_begin_indef(enc);
			cbor_array_encode_begin(enc, 1);
				cbor_array_encode_begin_indef(enc);
				cbor_array_encode_end(enc);
			cbor_array_encode_end(enc);
		cbor_array_encode_end(enc);
	cbor_array_encode_end(enc);

	cbor_array_encode_begin(enc, 3);
		cbor_int32_encode(enc, 1);
		cbor_array_encode_begin_indef(enc);
			cbor_int32_encode(enc, 2);
			cbor_int32_encode(enc, 3);
			cbor_array_encode_begin_indef(enc);
				cbor_text_encode(enc, (unsigned char *)str1, strlen(str1));
				cbor_int32_encode(enc, 256);
				cbor_bytes_encode(enc, bytes, ARRAY_SIZE(bytes));
			cbor_array_encode_end(enc);
		cbor_array_encode_end(enc);
		cbor_int32_encode(enc, 4);
	cbor_array_encode_end(enc);
}


void integers_test_encode(struct cbor_encoder *enc)
{
	cbor_uint32_encode(enc, 0);
	cbor_uint32_encode(enc, 1);
	cbor_uint32_encode(enc, 2);
	cbor_uint32_encode(enc, 23);
	cbor_uint32_encode(enc, 24);
	cbor_uint32_encode(enc, 255);
	cbor_uint32_encode(enc, 256);
	cbor_uint32_encode(enc, 16535);
	cbor_uint32_encode(enc, 16536);
	cbor_uint32_encode(enc, 32767);
	cbor_uint32_encode(enc, 32768);

	cbor_int32_encode(enc, 0);
	cbor_int32_encode(enc, -1);
	cbor_int32_encode(enc, -24);
	cbor_int32_encode(enc, -25);
	cbor_int32_encode(enc, -128);
	cbor_int32_encode(enc, -129);
	cbor_int32_encode(enc, -256);
	cbor_int32_encode(enc, -257);
	cbor_int32_encode(enc, -16536);
	cbor_int32_encode(enc, -16537);
	cbor_int32_encode(enc, -32768);
	cbor_int32_encode(enc, -32769);
}


void bytes_test_encode(struct cbor_encoder *enc)
{
	cbor_bytes_encode(enc, uqueen, 0);
	cbor_bytes_encode(enc, uqueen, 1);
	cbor_bytes_encode(enc, uqueen, strlen(queen));

	unsigned char binary[] = {
		0x0, 0x0, 0x1, 0x2, 0x17, 0x0, 0x2, 254,
		255, 255, 0x11, 0x11
	};

	cbor_bytes_encode(enc, binary, ARRAY_SIZE(binary));
}


void text_test_encode(struct cbor_encoder *enc)
{
	size_t i;

	cbor_text_encode(enc, uqueen, 0);
	cbor_text_encode(enc, uqueen, 1);
	cbor_text_encode(enc, uqueen, strlen(queen));

	cbor_text_encode_begin_indef(enc);
	for (i = 0; i < ARRAY_SIZE(poem); i++)
		cbor_text_encode(enc, (unsigned char *)poem[i], strlen(poem[i]));
	cbor_text_encode_end(enc);
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

	assert(cbor_stream_open_file(out, "/home/david/sw/netbufs/tests/libcbor/text.cbor", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR) == CBOR_ERR_OK);

	enc = cbor_encoder_new(out);
	assert(enc);

	//array_test_encode(enc);
	//integers_test_encode(enc);
	//bytes_test_encode(enc);
	//text_test_encode(enc);

	cbor_stream_close(in);
	cbor_stream_close(out);

	cbor_decoder_delete(dec);
	cbor_encoder_delete(enc);

	return EXIT_SUCCESS;
}
