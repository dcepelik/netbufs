#include "debug.h"
#include "internal.h"
#include "memory.h"
#include "util.h"

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static nb_err_t write_internal(struct nb_buf *buf, byte_t *bytes, size_t nbytes);


void nb_buf_init(struct nb_buf *buf)
{
	size_t size = 1024;

	buf->buf = nb_malloc(size);
	TEMP_ASSERT(buf->buf);

	buf->bufsize = size;
	buf->pos = 0;
	buf->len = 0;
	buf->dirty = false;
	buf->eof = false;
	buf->read_filter = NULL;
	buf->write_filter = NULL;
	buf->ungetc = -1;
}


void nb_buf_set_read_filter(struct nb_buf *buf, filter_t *filter)
{
	buf->read_filter = filter;
}


void nb_buf_set_write_filter(struct nb_buf *buf, filter_t *filter)
{
	buf->write_filter = filter;
}


void nb_buf_free(struct nb_buf *buf)
{
	nb_free(buf->buf);
}


void nb_buf_flush(struct nb_buf *buf)
{
	buf->flush(buf);
	buf->pos = 0;
	buf->len = 0;
	buf->dirty = false;
	DEBUG_MSG("Flush");
}


static bool nb_buf_fill(struct nb_buf *buf)
{
	if (buf->dirty)
		nb_buf_flush(buf);

	buf->fill(buf);
	buf->pos = 0;
	return buf->len > 0;
}


/* TODO Check that: once EOF is returned for the first time, all successive calls return EOF as well */
static nb_err_t read_internal(struct nb_buf *buf, byte_t *bytes, size_t nbytes)
{
	DEBUG_MSG("Read");
	size_t avail;
	size_t ncpy;

	if (buf->dirty)
		nb_buf_flush(buf);

	buf->last_read_len = 0;

	while (nbytes > 0) {
		avail = buf->len - buf->pos;
		assert(avail >= 0);

		if (!avail) {
			nb_buf_fill(buf);
			avail = buf->len;

			if (avail == 0)
				return NB_ERR_EOF;
		}

		ncpy = MIN(avail, nbytes);
		memcpy(bytes, buf->buf + buf->pos, ncpy);
		bytes += ncpy;
		buf->pos += ncpy;
		nbytes -= ncpy;
		buf->last_read_len += ncpy;
	}

	if (nbytes > 0)
		return NB_ERR_READ; /* TODO save strerror(errno) somewhere */

	return NB_ERR_OK;
}


static inline byte_t hexval(char c)
{
	assert(isxdigit(c));

	if (isdigit(c))
		return c - '0';
	else if (c >= 'a' && c <= 'f')
		return 10 + (c - 'a');
	else
		return 10 + (c - 'A');
}


static inline char hexchar(byte_t val)
{
	assert(val >= 0 && val <= 15);

	if (val >= 0 && val <= 9)
		return '0' + val;
	else
		return 'A' + (val - 10);
}


nb_err_t nb_buf_hex_read_filter(struct nb_buf *buf, byte_t *bytes, size_t nbytes)
{
	size_t nnbytes;
	byte_t *tmpbuf;
	size_t i;
	size_t j;
	size_t read_len;
	nb_err_t err;

	nnbytes = 2 * nbytes;
	tmpbuf = malloc(nnbytes);
	TEMP_ASSERT(tmpbuf);

	if ((err = read_internal(buf, tmpbuf, nnbytes)) != NB_ERR_OK)
		return err;

	TEMP_ASSERT(nb_buf_get_last_read_len(buf) % 2 == 0);
	read_len = nb_buf_get_last_read_len(buf) / 2;

	for (i = 0, j = 0; i < read_len; i++, j += 2) {
		bytes[i] = 16 * hexval(tmpbuf[j]) + hexval(tmpbuf[j + 1]);
	}

	free(tmpbuf);
	return NB_ERR_OK;
}


nb_err_t nb_buf_hex_write_filter(struct nb_buf *buf, byte_t *bytes, size_t nbytes)
{
	size_t nnbytes;
	byte_t *tmpbuf;
	size_t i;
	size_t j;
	nb_err_t err;

	nnbytes = 2 * nbytes;
	tmpbuf = malloc(nnbytes);
	TEMP_ASSERT(tmpbuf);

	for (i = 0, j = 0; i < nbytes; i++, j += 2) {
		tmpbuf[j] = hexchar(bytes[i] >> 4);
		tmpbuf[j + 1] = hexchar(bytes[i] & 0x0F);
	}

	err = write_internal(buf, tmpbuf, nnbytes);
	free(tmpbuf);

	return err;
}


nb_err_t nb_buf_read(struct nb_buf *buf, byte_t *bytes, size_t nbytes)
{
	if (buf->read_filter)
		return buf->read_filter(buf, bytes, nbytes);
	return read_internal(buf, bytes, nbytes);
}


size_t nb_buf_read_len(struct nb_buf *buf, byte_t *bytes, size_t nbytes)
{
	read_internal(buf, bytes, nbytes);
	return nb_buf_get_last_read_len(buf);
}


nb_err_t nb_buf_write(struct nb_buf *buf, byte_t *bytes, size_t nbytes)
{
	if (buf->write_filter)
		return buf->write_filter(buf, bytes, nbytes);
	return write_internal(buf, bytes, nbytes);
}


static nb_err_t write_internal(struct nb_buf *buf, byte_t *bytes, size_t nbytes)
{
	size_t avail;
	size_t ncpy;

	DEBUG_MSG("Write");

	if (!buf->dirty) {
		buf->pos = 0;
	}

	while (nbytes > 0) {
		avail = buf->bufsize - buf->len;
		assert(avail >= 0);

		if (!avail) {
			nb_buf_flush(buf);
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


int nb_buf_getc(struct nb_buf *buf)
{
	char c;

	if (buf->ungetc != BUF_EOF) {
		c = buf->ungetc;
		buf->ungetc = BUF_EOF;
		return c;
	}

	if (buf->pos >= buf->len)
		if (!nb_buf_fill(buf))
			return BUF_EOF;

	return buf->buf[buf->pos++];
}


void nb_buf_ungetc(struct nb_buf *buf, int c)
{
	assert(buf->ungetc == BUF_EOF); /* don't override ungetc */
	buf->ungetc = c;
}


int nb_buf_peek(struct nb_buf *buf)
{
	char c;
	c = nb_buf_getc(buf);
	nb_buf_ungetc(buf, c);
	return c;
}


static void nb_buf_file_close(struct nb_buf *buf)
{
	close(buf->fd);
}

static void nb_buf_file_fill(struct nb_buf *buf)
{
	buf->len = read(buf->fd, buf->buf, buf->bufsize);
	buf->eof = (buf->len == 0);
	TEMP_ASSERT(buf->len >= 0);
}


static void nb_buf_file_flush(struct nb_buf *buf)
{
	ssize_t written;

	written = write(buf->fd, buf->buf, buf->len);
	TEMP_ASSERT(written == buf->len);
}


static void nb_buf_memory_close(struct nb_buf *buf)
{
	nb_free(buf->memory);
}


static void nb_buf_memory_fill(struct nb_buf *buf)
{
	size_t avail;
	size_t ncpy;
	
	avail = buf->memory_len - buf->memory_pos;
	ncpy = MIN(avail, buf->bufsize);

	memcpy(buf->buf, buf->memory + buf->memory_pos, ncpy);

	buf->len = ncpy;
	buf->memory_pos += ncpy;
}


static void nb_buf_memory_flush(struct nb_buf *buf)
{
	size_t new_mry_size;

	if (buf->memory_len + buf->len > buf->memory_size) {
		new_mry_size = MAX(2 * buf->memory_size, buf->bufsize);
		buf->memory = nb_realloc(buf->memory, new_mry_size);
		TEMP_ASSERT(buf->memory);
		buf->memory_size = new_mry_size;
		DEBUG_EXPR("%lu", buf->memory_size);
	}

	memcpy(buf->memory + buf->memory_len, buf->buf, buf->len);
	buf->memory_len += buf->len;

	DEBUG_EXPR("%lu", buf->memory_len);
}


struct nb_buf *nb_buf_new(void)
{
	struct nb_buf *buf;
	
	buf = nb_malloc(sizeof(*buf));
	if (buf)
		nb_buf_init(buf);

	return buf;
}


void nb_buf_delete(struct nb_buf *buf)
{
	nb_buf_free(buf);
	nb_free(buf);
}


nb_err_t nb_buf_open_file(struct nb_buf *buf, char *filename, int flags, int mode)
{
	if ((buf->fd = open(filename, flags, mode)) == -1)
		return NB_ERR_OPEN;

	buf->filename = filename;
	buf->mode = mode;

	buf->close = nb_buf_file_close;
	buf->fill = nb_buf_file_fill;
	buf->flush = nb_buf_file_flush;

	return NB_ERR_OK;
}


static nb_err_t open_fd(struct nb_buf *buf, int fd)
{
	buf->fd = fd;
	buf->filename = NULL;
	buf->mode = 0;

	buf->close = NULL;
	buf->fill = nb_buf_file_fill;
	buf->flush = nb_buf_file_flush;

	return NB_ERR_OK;
}


/* TODO DRY */
nb_err_t nb_buf_open_stdout(struct nb_buf *buf)
{
	return open_fd(buf, STDOUT_FILENO);
}


nb_err_t nb_buf_open_stdin(struct nb_buf *buf)
{
	return open_fd(buf, STDIN_FILENO);
}


nb_err_t nb_buf_open_memory(struct nb_buf *buf)
{
	buf->memory = NULL;
	buf->memory_len = 0;
	buf->memory_size = 0;
	buf->memory_pos = 0;

	buf->close = nb_buf_memory_close;
	buf->fill = nb_buf_memory_fill;
	buf->flush = nb_buf_memory_flush;

	return NB_ERR_OK;
}


void nb_buf_close(struct nb_buf *buf)
{
	if (buf->dirty)
		buf->flush(buf);

	if (buf->close)
		buf->close(buf);
}


bool nb_buf_is_eof(struct nb_buf *buf)
{
	assert(!buf->dirty);

	size_t avail;

	avail = buf->len - buf->pos;
	if (!avail) {
		buf->fill(buf);
		return buf->len == 0;
	}

	return false;
}
