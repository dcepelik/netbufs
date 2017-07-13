#include "buffer.h"
#include "common.h"
#include "debug.h"
#include "memory.h"
#include "util.h"

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEBUG_THIS	0

static nb_err_t write_internal(struct nb_buffer *buf, nb_byte_t *bytes, size_t nbytes);


void nb_buffer_init(struct nb_buffer *buf)
{
	size_t size = 4096;

	buf->buf = nb_malloc(size);
	TEMP_ASSERT(buf->buf);

	buf->bufsize = size;
	buf->pos = 0;
	buf->len = 0;
	buf->dirty = false;
	buf->eof = false;
	buf->ungetc = -1;
}


void nb_buffer_flush(struct nb_buffer *buf)
{
	buf->ops->flush(buf);
	buf->pos = 0;
	buf->len = 0;
	buf->dirty = false;
}


static bool nb_buffer_fill(struct nb_buffer *buf)
{
	buf->ops->fill(buf);
	buf->pos = 0;
	return buf->len > 0;
}


/* TODO Check that: once EOF is returned for the first time, all successive calls return EOF as well */
static nb_err_t read_internal(struct nb_buffer *buf, nb_byte_t *bytes, size_t nbytes)
{
	size_t avail;
	size_t ncpy;

	assert(!buf->dirty);
	buf->last_read_len = 0;

	while (nbytes > 0) {
		avail = buf->len - buf->pos;
		assert(avail >= 0);

		if (!avail) {
			nb_buffer_fill(buf);
			avail = buf->len;

			if (avail == 0)
				return NB_ERR_EOF;
		}

		//DEBUG_EXPR("%lu", buf->pos);
		//DEBUG_EXPR("%lu", buf->len);

		ncpy = MIN(avail, nbytes);
		memcpy(bytes, buf->buf + buf->pos, ncpy);
		bytes += ncpy;
		buf->pos += ncpy;
		nbytes -= ncpy;
		buf->last_read_len += ncpy;
	}

	assert(nbytes == 0);
	return NB_ERR_OK;
}


static inline nb_byte_t hexval(char c)
{
	assert(isxdigit(c));

	if (isdigit(c))
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return 10 + (c - 'a');
	else
		return 10 + (c - 'A');
}


static inline char hexchar(nb_byte_t val)
{
	assert(val >= 0 && val <= 15);

	if (val >= 0 && val <= 9)
		return '0' + val;
	else
		return 'A' + (val - 10);
}


nb_err_t nb_buffer_read(struct nb_buffer *buf, nb_byte_t *bytes, size_t nbytes)
{
	return read_internal(buf, bytes, nbytes);
}


nb_err_t nb_buffer_write(struct nb_buffer *buf, nb_byte_t *bytes, size_t nbytes)
{
	return write_internal(buf, bytes, nbytes);
}


static nb_err_t write_internal(struct nb_buffer *buf, nb_byte_t *bytes, size_t nbytes)
{
	size_t avail;
	size_t ncpy;

	while (nbytes > 0) {
		avail = buf->bufsize - buf->len;
		assert(avail >= 0);

		if (!avail) {
			nb_buffer_flush(buf);
			avail = buf->bufsize;
		}

		ncpy = MIN(avail, nbytes);
		memcpy(buf->buf + buf->pos, bytes, ncpy);
		bytes += ncpy;
		buf->pos += ncpy;
		buf->len = buf->pos;
		nbytes -= ncpy;
		buf->dirty = true;
	}

	assert(nbytes == 0);

	return NB_ERR_OK;
}


int nb_buffer_getc(struct nb_buffer *buf)
{
	char c;

	if (buf->ungetc != BUF_EOF) {
		c = buf->ungetc;
		buf->ungetc = BUF_EOF;
		return c;
	}

	if (buf->pos >= buf->len)
		if (!nb_buffer_fill(buf))
			return BUF_EOF;

	return buf->buf[buf->pos++];
}


void nb_buffer_ungetc(struct nb_buffer *buf, int c)
{
	assert(buf->ungetc == BUF_EOF); /* don't override ungetc */
	buf->ungetc = c;
}


int nb_buffer_peek(struct nb_buffer *buf)
{
	char c;
	c = nb_buffer_getc(buf);
	nb_buffer_ungetc(buf, c);
	return c;
}


struct nb_buffer *nb_buffer_new(void)
{
	struct nb_buffer *buf;
	
	buf = nb_malloc(sizeof(*buf));
	if (buf)
		nb_buffer_init(buf);

	return buf;
}


void nb_buffer_delete(struct nb_buffer *buf)
{
	if (buf->dirty)
		buf->ops->flush(buf);

	xfree(buf->buf);
	buf->ops->delete(buf);
}


bool nb_buffer_is_eof(struct nb_buffer *buf)
{
	assert(!buf->dirty);

	size_t avail;

	avail = buf->len - buf->pos;
	if (!avail) {
		nb_buffer_fill(buf);
		return buf->len == 0;
	}

	return false;
}


size_t nb_buffer_tell(struct nb_buffer *buf)
{
	return buf->ops->tell(buf);
}


size_t nb_buffer_read_len(struct nb_buffer *buf, nb_byte_t *bytes, size_t nbytes)
{
	read_internal(buf, bytes, nbytes);
	return buf->last_read_len;
}
