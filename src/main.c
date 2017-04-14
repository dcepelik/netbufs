#include "cbor.h"
#include <stdlib.h>
#include <string.h>


int main(void)
{
	struct cbor_encoder *enc = cbor_encoder_new();

	//cbor_int16_encode(enc, 1442);
	//cbor_int16_encode(enc, -55);

	//unsigned char bytes[4] = { 42, 44, 46, 43 };
	//cbor_bytes_encode(enc, bytes, 4);

	//char *str = "Hello World!";
	//cbor_text_encode(enc, str, strlen(str));

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

	cbor_array_encode_begin_indef(enc);
	cbor_int8_encode(enc, 1);
	cbor_array_encode_begin(enc, 2);
	cbor_int8_encode(enc, 2);
	cbor_int8_encode(enc, 3);
	cbor_array_encode_end(enc);
	cbor_array_encode_begin_indef(enc);
	cbor_int8_encode(enc, 4);
	cbor_int8_encode(enc, 5);
	cbor_array_encode_end(enc);
	cbor_array_encode_end(enc);
	
	/* TODO Test encode-decode an int for various ints */

	return EXIT_SUCCESS;
}
