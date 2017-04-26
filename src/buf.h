#ifndef BUF_H
#define BUF_H

#include "cbor.h"
#include <stdbool.h>
#include <stdlib.h>

typedef cbor_err_t (filter_t)(struct buf *buf, byte_t *bytes, size_t nbytes);

struct buf
{
	byte_t *buf;	/* data buffer */
	size_t bufsize;	/* size of the buffer */
	size_t pos;	/* current read/write position within the buf */
	size_t len;	/* number of valid bytes in the buffer (for reading) */
	size_t last_read_len;
	bool dirty;	/* do we have data to be written? */
	bool eof;	/* did we hit EOF during last filling? */

	union {
		struct { /* for file streams */
			char *filename;	/* filename of currently open file */
			int fd;		/* file descriptor */
			int mode;	/* file flags (@see man 3p open) */
		};

		struct { /* for in-memory streams */
			byte_t *memory;	/* the memory */
			size_t memory_size;	/* memory size */
			size_t memory_len;	/* number of valid bytes in memory */
			size_t memory_pos;	/* position in memory (for reading/writing) */
		};
	};

	void (*flush)(struct buf *buf);
	void (*fill)(struct buf *buf);
	void (*close)(struct buf *buf);

	filter_t *read_filter;
	filter_t *write_filter;
};


struct buf *buf_new(void);
void buf_delete(struct buf *buf);

bool buf_is_eof(struct buf *buf);

cbor_err_t buf_open_file(struct buf *buf, char *filename, int flags, int mode);
cbor_err_t buf_open_stdin(struct buf *buf);
cbor_err_t buf_open_stdout(struct buf *buf);
cbor_err_t buf_open_memory(struct buf *buf);
void buf_close(struct buf *buf);

cbor_err_t buf_write(struct buf *buf, byte_t *bytes, size_t count);
cbor_err_t buf_read(struct buf *buf, byte_t *bytes, size_t count);

void buf_set_read_filter(struct buf *buf, filter_t *filter);
void buf_set_write_filter(struct buf *buf, filter_t *filter);

/* XXX This is a temporary tests helper */
size_t buf_read_len(struct buf *buf, byte_t *bytes, size_t nbytes);

/* XXX Yuk! */
static inline cbor_err_t buf_get_last_read_len(struct buf *buf)
{
	return buf->last_read_len;
}

cbor_err_t buf_hex_read_filter(struct buf *buf, byte_t *bytes, size_t nbytes);
cbor_err_t buf_hex_write_filter(struct buf *buf, byte_t *bytes, size_t nbytes);

#endif
