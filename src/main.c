#include "cbor.h"
#include <stdlib.h>


int main(void)
{
	struct cbor_encoder *enc = cbor_encoder_new();

	cbor_int16_encode(enc, 1442);
	cbor_int16_encode(enc, -55);
	
	/* TODO Test encode-decode an int for various ints */

	return EXIT_SUCCESS;
}
