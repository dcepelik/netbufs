#include "internal.h"
#include "debug.h"
#include <ctype.h>


cbor_err_t cbor_stream_write(struct cbor_stream *stream, unsigned char *bytes,
	size_t count)
{
	size_t i;

	(void) stream;
	(void) bytes;
	(void) count;

	for (i = 0; i < count; i++) {
		if (isprint(bytes[i])) {
			DEBUG_PRINTF("Writing byte 0x%02X [%c]", bytes[i], bytes[i]);
		}
		else {
			DEBUG_PRINTF("Writing byte 0x%02X", bytes[i]);
		}
	}

	return CBOR_ERR_OK;
}


cbor_err_t cbor_stream_read(struct cbor_stream *stream, unsigned char *bytes,
	size_t offset, size_t count)
{
	(void) stream;
	(void) bytes;
	(void) offset;
	(void) count;

	return CBOR_ERR_EOF;
}
