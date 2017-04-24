#ifndef INTERNAL_H
#define INTERNAL_H

#include "cbor.h"
#include "stack.h"
#include "strbuf.h"
#include <stdlib.h>

#define CBOR_BLOCK_STACK_INIT_SIZE	4

typedef cbor_err_t (filter_t)(struct buf *buf, byte_t *bytes, size_t offset, size_t nbytes);

struct buf
{
	byte_t *buf;	/* data buffer */
	size_t bufsize;	/* size of the buffer */
	size_t pos;	/* current read/write position within the buf */
	size_t len;	/* number of valid bytes in the buffer (for reading) */
	size_t last_read_len;
	bool dirty;	/* do we have data to be written? */
	bool eof;	/* did we hit EOF during last filling? */

	/* for file streams */
	char *filename;	/* filename of currently open file */
	int fd;		/* file descriptor */
	int mode;	/* file flags (@see man 3p open) */

	/* for in-memory streams */
	byte_t *memory;	/* the memory */
	size_t memory_size;	/* memory size */
	size_t memory_len;	/* number of valid bytes in memory */
	size_t memory_pos;	/* position in memory (for reading/writing) */

	void (*flush)(struct buf *buf);
	void (*fill)(struct buf *buf);
	void (*close)(struct buf *buf);
	filter_t *filter;
};

cbor_err_t buf_write(struct buf *buf, byte_t *bytes, size_t count);
cbor_err_t buf_read(struct buf *buf, byte_t *bytes, size_t offset, size_t count);
void buf_set_filter(struct buf *buf, filter_t *filter);

/* XXX This is a temporary tests helper */
size_t buf_read_len(struct buf *buf, byte_t *bytes, size_t nbytes);

/* XXX Yuk! */
static inline cbor_err_t buf_get_last_read_len(struct buf *buf)
{
	return buf->last_read_len;
}

cbor_err_t buf_hex_filter(struct buf *buf, byte_t *bytes, size_t offset, size_t nbytes);

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


struct block
{
	enum major major;	/* Major Type for which this block has been open */
	bool indefinite;	/* is indefinite-lenght encoding used? */
	uint64_t len;		/* intended length of the block when !indefinite */
	size_t num_items;	/* actual number of items encoded */
};

struct cbor_stream
{
	struct buf *buf;
	struct stack blocks;
	cbor_err_t err;
	struct strbuf err_buf;

	/* TODO various encoding/decoding options to come as needed */
	bool fail_on_error;
};


static inline bool major_allows_indefinite(enum major major)
{
	return major == CBOR_MAJOR_TEXT
		|| major == CBOR_MAJOR_BYTES
		|| major == CBOR_MAJOR_ARRAY
		|| major == CBOR_MAJOR_MAP;
}


cbor_err_t error(struct cbor_stream *cs, cbor_err_t err, char *str, ...);
cbor_err_t push_block(struct cbor_stream *cs, enum major major, bool indefinite, uint64_t len);
struct block *top_block(struct cbor_stream *cs);

#endif
