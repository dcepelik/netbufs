#include "cbor.h"
#include "debug.h"
#include "internal.h"
#include "util.h"

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[])
{
	struct cbor_stream *in;
	struct cbor_stream *out;
	struct cbor_decoder *dec;
	struct cbor_encoder *enc;
	struct cbor_item item;
	cbor_err_t err;

	in = cbor_stream_new();
	assert(in);

	assert(cbor_stream_open_file(in, "/home/david/in.cbor", O_RDONLY, 0) == CBOR_ERR_OK);

	//assert(cbor_stream_open_memory(in) == CBOR_ERR_OK);
	//cbor_stream_write(in, (unsigned char[]) { 0x83, 0x01, 0x82, 0x02, 0x03, 0x82, 0x04, 0x05 }, 8);

	dec = cbor_decoder_new(in);
	assert(dec);

	while ((err = cbor_item_decode(dec, &item)) == CBOR_ERR_OK) {
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

	assert(cbor_stream_open_file(out, "/home/david/out.cbor", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR) == CBOR_ERR_OK);

	enc = cbor_encoder_new(out);
	assert(enc);

	char *str1 = "Hello World!";
	unsigned char bytes[8] = { 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49 };
	
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

	cbor_stream_close(in);
	cbor_stream_close(out);

	cbor_decoder_delete(dec);
	cbor_encoder_delete(enc);

	return EXIT_SUCCESS;
}
