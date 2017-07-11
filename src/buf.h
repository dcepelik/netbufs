#ifndef NB_BUF_H
#define NB_BUF_H

#include "common.h"
#include "error.h"

#include <stdbool.h>
#include <stdlib.h>

#define BUF_EOF	(-1)

struct nb_buf
{
	nb_byte_t *buf;		/* data buffer */
	size_t bufsize;		/* size of the buffer */
	size_t pos;			/* current read/write position within the buf */
	size_t len;			/* number of valid bytes in the buffer (for reading) */
	size_t last_read_len;
	bool dirty;			/* do we have data to be written? */
	bool eof;			/* did we hit EOF during last filling? */
	int ungetc;			/* character to be returned by next getc()-call */

	union {
		struct { /* for file streams */
			char *filename;	/* filename of currently open file */
			int fd;			/* file descriptor */
			int mode;		/* file flags (@see man 3p open) */
		};

		struct { /* for in-memory streams */
			nb_byte_t *memory;		/* the memory */
			size_t memory_size;	/* memory size */
			size_t memory_len;	/* number of valid bytes in memory */
			size_t memory_pos;	/* position in memory (for reading/writing) */
		};
	};

	void (*flush)(struct nb_buf *buf);
	void (*fill)(struct nb_buf *buf);
	void (*close)(struct nb_buf *buf);
	size_t (*tell)(struct nb_buf *buf);
};


struct nb_buf *nb_buf_new(void);
void nb_buf_delete(struct nb_buf *buf);

bool nb_buf_is_eof(struct nb_buf *buf);

nb_err_t nb_buf_open_file(struct nb_buf *buf, char *filename, int oflags, int mode);
nb_err_t nb_buf_open_stdin(struct nb_buf *buf);
nb_err_t nb_buf_open_stdout(struct nb_buf *buf);

nb_err_t nb_buf_open_fd(struct nb_buf *buf, int fd);

nb_err_t nb_buf_open_memory(struct nb_buf *buf);

void nb_buf_close(struct nb_buf *buf);

nb_err_t nb_buf_write(struct nb_buf *buf, nb_byte_t *bytes, size_t count);
nb_err_t nb_buf_read(struct nb_buf *buf, nb_byte_t *bytes, size_t count);
void nb_buf_flush(struct nb_buf *buf);

int nb_buf_getc(struct nb_buf *buf);
void nb_buf_ungetc(struct nb_buf *buf, int c);
int nb_buf_peek(struct nb_buf *buf);

/* XXX This is a temporary tests helper */
size_t nb_buf_read_len(struct nb_buf *buf, nb_byte_t *bytes, size_t nbytes);

/* XXX Yuk! */
static inline nb_err_t nb_buf_get_last_read_len(struct nb_buf *buf)
{
	return buf->last_read_len;
}

nb_err_t nb_buf_printf(struct nb_buf *buf, char *msg, ...);
size_t nb_buf_tell(struct nb_buf *buf);

#endif
