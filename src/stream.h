#ifndef STREAM_H
#define STREAM_H

#include "error.h"
#include <stdlib.h>

struct cbor_stream
{
	unsigned char *buf;
};

cbor_err_t cbor_stream_write(struct cbor_stream *stream, unsigned char *bytes,
	size_t count);
cbor_err_t cbor_stream_read(struct cbor_stream *stream, unsigned char *bytes,
	size_t offset, size_t count);

#endif
