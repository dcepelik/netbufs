#ifndef NB_BUF_H
#define NB_BUF_H

#include "common.h"
#include "error.h"
#include "buffer-internal.h"
#include <string.h>

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

#define BUF_EOF	(-1)

struct nb_buffer;

bool nb_buffer_is_eof(struct nb_buffer *buf);
bool nb_buffer_fill(struct nb_buffer *buf);

struct nb_buffer *nb_buffer_new_file(int fd_in);
struct nb_buffer *nb_buffer_new_memory(void);

void nb_buffer_delete(struct nb_buffer *buf);

ssize_t nb_buffer_write_slow(struct nb_buffer *buf, nb_byte_t *bytes, size_t count);
static inline ssize_t nb_buffer_write(struct nb_buffer *buf, nb_byte_t *bytes, size_t count)
{
	size_t avail;

	assert(buf->mode != BUF_MODE_READING);

	avail = buf->bufsize - buf->len;
	assert(avail >= 0);

	if (__builtin_expect(count <= avail, 1)) {
		memcpy(buf->buf + buf->pos, bytes, count);
		buf->pos += count;
		buf->len = buf->pos;
		buf->written_total += count;
		buf->mode = BUF_MODE_WRITING;
		return count;
	}
	else {
		return nb_buffer_write_slow(buf, bytes, count);
	}
}

ssize_t nb_buffer_read_slow(struct nb_buffer *buf, nb_byte_t *bytes, size_t count);
static inline ssize_t nb_buffer_read(struct nb_buffer *buf, nb_byte_t *bytes, size_t count)
{
	size_t avail;
	assert(buf->mode != BUF_MODE_WRITING);

	avail = buf->len - buf->pos;
	assert(avail >= 0);

	if (__builtin_expect(count <= avail, 1)) {
		memcpy(bytes, buf->buf + buf->pos, count);
		buf->pos += count;
		buf->last_read_len += count;
		buf->mode = BUF_MODE_READING;
		return count;
	}
	else {
		return nb_buffer_read_slow(buf, bytes, count);
	}

	return NB_ERR_OK;
}

static inline int nb_buffer_getc(struct nb_buffer *buf)
{
	if (unlikely(buf->pos >= buf->len))
		if (!nb_buffer_fill(buf))
			return BUF_EOF;
	return buf->buf[buf->pos++];
}

static inline void nb_buffer_ungetc(struct nb_buffer *buf, int c)
{
	assert(buf->pos > 0);
	buf->buf[--buf->pos] = c;
}

static inline int nb_buffer_peek(struct nb_buffer *buf)
{
	int c = nb_buffer_getc(buf);
	if (c != BUF_EOF)
		nb_buffer_ungetc(buf, c);
	return c;
}

size_t nb_buffer_tell(struct nb_buffer *buf);
void nb_buffer_flush(struct nb_buffer *buf);

int nb_buffer_getc(struct nb_buffer *buf);
void nb_buffer_ungetc(struct nb_buffer *buf, int c);
int nb_buffer_peek(struct nb_buffer *buf);

size_t nb_buffer_get_written_total(struct nb_buffer *buf);

#endif
