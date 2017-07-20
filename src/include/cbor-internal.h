#ifndef CBOR_INTERNAL_H
#define CBOR_INTERNAL_H

#include "cbor.h"
#include <stdbool.h>

#define CBOR_BLOCK_STACK_INIT_SIZE	4

/*
 * CBOR Major Types
 * @see RFC 7049, section 2.1.
 */
enum major
{
	CBOR_MAJOR_UINT,
	CBOR_MAJOR_NEGINT,
	CBOR_MAJOR_BYTES,
	CBOR_MAJOR_TEXT,
	CBOR_MAJOR_ARRAY,
	CBOR_MAJOR_MAP,
	CBOR_MAJOR_TAG,
	CBOR_MAJOR_7,
};

/* Additional Information for Major Types 1..6
 * @see TODO
 */
enum lbits
{
	/* 0, ..., 23: small unsigned integer */
	LBITS_1B = 24,
	LBITS_2B,
	LBITS_4B,
	LBITS_8B,
	/* 28, 29, 30: unassigned */
	LBITS_INDEFINITE = 31,
};

/*
 * Additional Information for Major Type 7
 * @see RFC 7049, section 2.3., Table 1.
 */
enum minor
{
	/* 0, ..., 23: simple value (direct) */
	CBOR_MINOR_SVAL = 24,
	CBOR_MINOR_FLOAT16,
	CBOR_MINOR_FLOAT32,
	CBOR_MINOR_FLOAT64,
	/* 28, 29, 30: unassigned */
	CBOR_MINOR_BREAK = 31,
};

static inline bool major_allows_indefinite(enum major major)
{
	switch (major) {
	case CBOR_MAJOR_TEXT:
	case CBOR_MAJOR_BYTES:
	case CBOR_MAJOR_ARRAY:
	case CBOR_MAJOR_MAP:
		return true;
	default:
		return false;
	}
}

nb_err_t error(struct cbor_stream *cs, nb_err_t err, char *str, ...);
nb_err_t push_block(struct cbor_stream *cs, enum cbor_type type, bool indefinite, uint64_t len);
struct block *top_block(struct cbor_stream *cs);
bool is_indefinite(struct cbor_item *item);

#endif
