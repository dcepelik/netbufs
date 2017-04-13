#include "internal.h"
#include "debug.h"


cbor_err_t cbor_stream_write(struct cbor_stream *stream, unsigned char *bytes,
	size_t count)
{
	size_t i;

	(void) stream;
	(void) bytes;
	(void) count;

	for (i = 0; i < count; i++)
		DEBUG_PRINTF("Writing byte 0x%02X", bytes[i]);

	return CBOR_ERR_OK;
}