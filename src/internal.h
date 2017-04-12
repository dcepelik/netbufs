#ifndef INTERNAL_H
#define INTERNAL_H

enum cbor_err
{
	CBOR_ERR_OK,		/* no error */
	CBOR_ERR_NOMEM,		/* unable to allocate memory */
	CBOR_ERR_PARSE,		/* unable to parse input */
	CBOR_ERR_ITEM,		/* unexpected item */
	CBOR_ERR_EOF,		/* unexpected EOF */
	CBOR_ERR_RANGE,		/* value is out of range */
};

typedef enum cbor_err cbor_err_t;

enum cbor_event
{
	CBOR_EVENT_ARRAY_BEGIN,
	CBOR_EVENT_ARRAY_END,
};

/*
 * Major types
 */
enum cbor_type
{
	CBOR_TYPE_UINT,
	CBOR_TYPE_NEGATIVE_INT,
	CBOR_TYPE_BYTES,
	CBOR_TYPE_TEXT,
	CBOR_TYPE_ARRAY,
	CBOR_TYPE_MAP,
	CBOR_TYPE_TAG,
	CBOR_TYPE_OTHER,
};

/*
 * Simple values
 */
enum cbor_simple_value
{
	CBOR_SIMPLE_VALUE_FALSE = 20,
	CBOR_SIMPLE_VALUE_TRUE,
	CBOR_SIMPLE_VALUE_NULL,
	CBOR_SIMPLE_VALUE_UNDEF,
};

struct cbor_item
{
	enum cbor_type type;

	union {
		size_t len;
	};
};

struct cbor_stream
{
	/* buffered I/O */
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
