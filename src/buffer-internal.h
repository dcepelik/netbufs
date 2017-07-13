#ifndef BUFFER_INTERNAL_H
#define BUFFER_INTERNAL_H

#include "common.h"
#include <stdbool.h>
#include <stddef.h>

struct nb_buffer;

struct nb_buffer_ops
{
	void (*delete)(struct nb_buffer *buf);
	void (*fill)(struct nb_buffer *buf);
	void (*flush)(struct nb_buffer *buf);
	size_t (*tell)(struct nb_buffer *buf);
};

struct nb_buffer
{
	const struct nb_buffer_ops *ops;
	nb_byte_t *buf;		/* data buffer */
	size_t bufsize;		/* size of the buffer */
	size_t pos;		/* current read/write position within the buf */
	size_t len;		/* number of valid bytes in the buffer (for reading) */
	size_t last_read_len;
	bool dirty;		/* do we have data to be written? */
	bool eof;		/* did we hit EOF during last filling? */
	int ungetc;		/* character to be returned by next getc()-call */
};


void nb_buffer_init(struct nb_buffer *buf);

/*
 * This is a test helper.
 */
size_t nb_buffer_read_len(struct nb_buffer *buf, nb_byte_t *bytes, size_t nbytes);

#endif
