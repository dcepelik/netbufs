#ifndef STREAM_H
#define STREAM_H

#include "error.h"
#include <stdlib.h>
#include <stdbool.h>

enum cbor_stream_mode
{
	CBOR_STREAM_MODE_READ,
	CBOR_STREAM_MODE_WRITE,
};

void cbor_stream_init(struct cbor_stream *stream);
cbor_err_t cbor_stream_write(struct cbor_stream *stream, unsigned char *bytes, size_t count);
size_t cbor_stream_read(struct cbor_stream *stream, unsigned char *bytes, size_t offset, size_t count);

#endif
