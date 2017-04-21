#ifndef INTERNAL_H
#define INTERNAL_H

#include "cbor.h"
#include "stack.h"
#include "strbuf.h"
#include <stdlib.h>

typedef unsigned char		cbor_extra_t;
typedef unsigned char		byte;

#define CBOR_MAJOR_MASK		0xE0
#define CBOR_EXTRA_MASK		0x1F

#define CBOR_BREAK		((CBOR_MAJOR_OTHER << 5) + 31)

#define CBOR_BLOCK_STACK_INIT_SIZE	4


struct cbor_stream
{
	unsigned char *buf;	/* data buffer */
	size_t bufsize;	/* size of the buffer */
	size_t pos;	/* current read/write position within the stream */
	size_t len;	/* number of valid bytes in the buffer (for reading) */
	size_t last_read_len;
	bool dirty;	/* do we have data to be written? */
	bool eof;	/* did we hit EOF during last filling? */

	/* for file streams */
	char *filename;	/* filename of currently open file */
	int fd;		/* file descriptor */
	int mode;	/* file flags (@see man 3p open) */

	/* for in-memory streams */
	unsigned char *memory;	/* the memory */
	size_t memory_size;	/* memory size */
	size_t memory_len;	/* number of valid bytes in memory */
	size_t memory_pos;	/* position in memory (for reading/writing) */

	void (*flush)(struct cbor_stream *stream);
	void (*fill)(struct cbor_stream *stream);
	void (*close)(struct cbor_stream *stream);
};

cbor_err_t cbor_stream_write(struct cbor_stream *stream, unsigned char *bytes, size_t count);
cbor_err_t cbor_stream_read(struct cbor_stream *stream, unsigned char *bytes, size_t offset, size_t count);

/* XXX This is a temporary tests helper */
size_t cbor_stream_read_len(struct cbor_stream *stream, unsigned char *bytes, size_t nbytes);

static inline cbor_err_t cbor_stream_get_last_read_len(struct cbor_stream *stream)
{
	return stream->last_read_len;
}


enum cbor_ebits
{
	CBOR_EBITS_1B = 24,
	CBOR_EBITS_2B,
	CBOR_EBITS_4B,
	CBOR_EBITS_8B,
	CBOR_EBITS_INDEF = 31,
};

struct errlist
{
	int i;
};

struct block
{
	struct cbor_hdr hdr;
	size_t num_items;
};

cbor_err_t stack_block_begin(struct stack *stack, enum cbor_major hdr, bool indef, uint64_t len);
cbor_err_t stack_block_end(struct stack *stack, enum cbor_major hdr, bool indef, uint64_t len);

struct cbor_encoder
{
	struct cbor_stream *stream;
	struct stack blocks;
	cbor_err_t err;
	struct strbuf err_buf;

	/* various options */
};

struct cbor_decoder
{
	struct cbor_stream *stream;
	struct stack blocks;
	cbor_err_t err;
	struct strbuf err_buf;

	/* various options */
};

struct cbor_document
{
	struct cbor_item root;
};


static inline bool major_allows_indef(enum cbor_major major)
{
	return major == CBOR_MAJOR_TEXT
		|| major == CBOR_MAJOR_BYTES
		|| major == CBOR_MAJOR_ARRAY
		|| major == CBOR_MAJOR_MAP;
}


static inline bool is_break(struct cbor_hdr *hdr)
{
	return (hdr->major == CBOR_MAJOR_OTHER && hdr->minor == CBOR_MINOR_BREAK);
}

#endif
