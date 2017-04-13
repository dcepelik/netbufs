#ifndef INTERNAL_H
#define INTERNAL_H

#include "cbor.h"
#include "stream.h"
#include <stdlib.h>

enum cbor_event
{
	CBOR_EVENT_ARRAY_BEGIN,
	CBOR_EVENT_ARRAY_END,
};


struct cbor_encoder
{
	struct cbor_stream stream;
	/* various options */
	/* state encapsulation */
};


struct cbor_decoder
{
	struct cbor_stream stream;
	/* various options */
	/* state encapsulation */
};

struct cbor_document
{
	struct cbor_item root;
};

#endif
