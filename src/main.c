#include "cbor.h"
#include "stream.h"
#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(void)
{
	struct cbor_stream dummy;
	cbor_stream_init(&dummy);

	struct cbor_encoder *enc = cbor_encoder_new(&dummy);
	struct cbor_decoder *dec = cbor_decoder_new(&dummy);

	//cbor_int16_encode(enc, 1442);
	//cbor_int16_encode(enc, -55);

	//int8_t i;
	//cbor_int8_encode(enc, 11);
	//cbor_int8_decode(dec, &i);

	cbor_err_t err;

	//cbor_array_encode_begin(enc, 3);
	//cbor_array_encode_end(enc);

	//size_t size;
	//err = cbor_array_decode_begin(dec, &size);
	//if (err)
	//	printf("Error: %i\n", err);

	//cbor_array_decode_end(dec);

	//printf("%lu\n", size);

	//unsigned char bytes[4] = { 42, 44, 46, 43 };
	//cbor_bytes_encode(enc, bytes, 4);

	char *str = "Hello World!";
	cbor_text_encode(enc, (unsigned char *)str, strlen(str));

	unsigned char *newstr;
	size_t newstrlen;
	err = cbor_text_decode(dec, &newstr, &newstrlen);

	printf("%i %lu %s %s\n", err, newstrlen, str, newstr);

	//cbor_array_encode_begin(enc, 3);
	//cbor_int8_encode(enc, 1);
	//cbor_array_encode_begin(enc, 2);
	//cbor_int8_encode(enc, 2);
	//cbor_int8_encode(enc, 3);
	//cbor_array_encode_end(enc);
	//cbor_array_encode_begin(enc, 2);
	//cbor_int8_encode(enc, 4);
	//cbor_int8_encode(enc, 5);
	//cbor_array_encode_end(enc);
	//cbor_array_encode_end(enc);

	//cbor_array_encode_begin_indef(enc);
	//cbor_int8_encode(enc, 1);
	//cbor_array_encode_begin(enc, 2);
	//cbor_int8_encode(enc, 2);
	//cbor_int8_encode(enc, 3);
	//cbor_array_encode_end(enc);
	//cbor_array_encode_begin_indef(enc);
	//cbor_int8_encode(enc, 4);
	//cbor_int8_encode(enc, 5);
	//cbor_array_encode_end(enc);
	//cbor_array_encode_end(enc);
	
	/* TODO Test encode-decode an int for various ints */

	return EXIT_SUCCESS;
}
