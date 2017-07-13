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


void nb_buffer_free(struct nb_buffer *buf)
{
	xfree(buf->buf);
}


void nb_buffer_flush(struct nb_buffer *buf)
{
	buf->flush(buf);
	buf->pos = 0;
	buf->len = 0;
	buf->dirty = false;
}


static bool nb_buffer_fill(struct nb_buffer *buf)
{
	buf->fill(buf);
	buf->pos = 0;
	return buf->len > 0;
}


static void debug_write(unsigned char *bytes, size_t nbytes)
{
	size_t i;

	for (i = 0; i < nbytes; i++) {
		if (isprint(bytes[i])) {
			DEBUG_PRINTF("Writing byte 0x%02X [%c]", bytes[i], bytes[i]);
		}
		else {
			DEBUG_PRINTF("Writing byte 0x%02X", bytes[i]);
		}
	}
	DEBUG_EXPR("%s", "---");
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


size_t nb_buffer_read_len(struct nb_buffer *buf, nb_byte_t *bytes, size_t nbytes)
{
	read_internal(buf, bytes, nbytes);
	return nb_buffer_get_last_read_len(buf);
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


static void nb_buffer_file_close(struct nb_buffer *buf)
{
	close(buf->fd);
}

static size_t nb_buffer_file_tell(struct nb_buffer *buf)
{
	return lseek(buf->fd, 0, SEEK_CUR);
}


static size_t nb_buffer_memory_tell(struct nb_buffer *buf)
{
	return 0;
}


static void nb_buffer_file_fill(struct nb_buffer *buf)
{
	int retval;

	retval = read(buf->fd, buf->buf, buf->bufsize);
	buf->len = MAX(retval, 0);

	/* TODO handle error if retval < 0 */

	buf->eof = (buf->len == 0);
	TEMP_ASSERT(buf->len >= 0);
}


static void nb_buffer_file_flush(struct nb_buffer *buf)
{
	ssize_t written;
	written = write(buf->fd, buf->buf, buf->len);
	TEMP_ASSERT(written == buf->len);
}


static void nb_buffer_memory_close(struct nb_buffer *buf)
{
	xfree(buf->memory);
}


static void nb_buffer_memory_fill(struct nb_buffer *buf)
{
	size_t avail;
	size_t ncpy;
	
	avail = buf->memory_len - buf->memory_pos;
	ncpy = MIN(avail, buf->bufsize);

	memcpy(buf->buf, buf->memory + buf->memory_pos, ncpy);

	buf->len = ncpy;
	buf->memory_pos += ncpy;
}


static void nb_buffer_memory_flush(struct nb_buffer *buf)
{
	size_t new_mry_size;

	if (buf->memory_len + buf->len > buf->memory_size) {
		new_mry_size = MAX(2 * buf->memory_size, buf->bufsize);
		buf->memory = nb_realloc(buf->memory, new_mry_size);
		buf->memory_size = new_mry_size;
	}

	memcpy(buf->memory + buf->memory_len, buf->buf, buf->len);
	buf->memory_len += buf->len;
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
	nb_buffer_free(buf);
	xfree(buf);
}


nb_err_t nb_buffer_open_file(struct nb_buffer *buf, char *filename, int flags, int mode)
{
	if ((buf->fd = open(filename, flags, mode)) == -1)
		return NB_ERR_OPEN;

	buf->filename = filename;
	buf->mode = mode;

	buf->close = nb_buffer_file_close;
	buf->fill = nb_buffer_file_fill;
	buf->flush = nb_buffer_file_flush;
	buf->tell = nb_buffer_file_tell;

	return NB_ERR_OK;
}


static nb_err_t open_fd(struct nb_buffer *buf, int fd)
{
	buf->fd = fd;
	buf->filename = NULL;
	buf->mode = 0;

	buf->close = NULL;
	buf->fill = nb_buffer_file_fill;
	buf->flush = nb_buffer_file_flush;
	buf->tell = nb_buffer_file_tell;

	return NB_ERR_OK;
}


nb_err_t nb_buffer_open_fd(struct nb_buffer *buf, int fd)
{
	return open_fd(buf, fd);
}


nb_err_t nb_buffer_open_stdout(struct nb_buffer *buf)
{
	return open_fd(buf, STDOUT_FILENO);
}


nb_err_t nb_buffer_open_stdin(struct nb_buffer *buf)
{
	return open_fd(buf, STDIN_FILENO);
}


nb_err_t nb_buffer_open_memory(struct nb_buffer *buf)
{
	buf->memory = NULL;
	buf->memory_len = 0;
	buf->memory_size = 0;
	buf->memory_pos = 0;

	buf->close = nb_buffer_memory_close;
	buf->fill = nb_buffer_memory_fill;
	buf->flush = nb_buffer_memory_flush;
	buf->tell = nb_buffer_memory_tell;

	return NB_ERR_OK;
}


void nb_buffer_close(struct nb_buffer *buf)
{
	if (buf->dirty)
		buf->flush(buf);

	if (buf->close)
		buf->close(buf);
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
	return buf->tell(buf);
}
