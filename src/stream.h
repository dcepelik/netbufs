#ifndef STREAM_H
#define STREAM_H

#include "error.h"
#include <stdlib.h>

enum cbor_stream_mode
{
	CBOR_STREAM_MODE_READ,
	CBOR_STREAM_MODE_WRITE,
};

struct cbor_stream
{
	char *buf;
	size_t buf_size;
	size_t len;
	size_t pos;
	enum cbor_stream_mode mode;
};

void cbor_stream_init(struct cbor_stream *stream);
cbor_err_t cbor_stream_write(struct cbor_stream *stream, unsigned char *bytes, size_t count);
cbor_err_t cbor_stream_read(struct cbor_stream *stream, unsigned char *bytes, size_t offset, size_t count);

#endif
