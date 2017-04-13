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

	char *str = "Hello World!";
	cbor_text_encode(enc, str, strlen(str));
	
	/* TODO Test encode-decode an int for various ints */

	return EXIT_SUCCESS;
}
