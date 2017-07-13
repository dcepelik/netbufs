/*
 * buffer-file:
 * File Buffer Implementation
 */
 
#include "buffer-internal.h"
#include "buffer.h"
#include "debug.h"
#include "memory.h"
#include "unistd.h"
#include "util.h"


static size_t file_tell(struct nb_buffer *buf);
static void file_delete(struct nb_buffer *buf);
static void file_fill(struct nb_buffer *buf);
static void file_flush(struct nb_buffer *buf);

const struct nb_buffer_ops file_ops = {
	.delete = file_delete,
	.fill = file_fill,
	.flush = file_flush,
	.tell = file_tell,
};

struct nb_buffer_file
{
	struct nb_buffer buf;
	char *filename;
	int fd;
};


struct nb_buffer *nb_buffer_new_file(int fd)
{
	struct nb_buffer_file *file_buf;

	file_buf = nb_malloc(sizeof(*file_buf));
	nb_buffer_init(&file_buf->buf);
	file_buf->buf.ops = &file_ops;
	file_buf->fd = fd;

	return &file_buf->buf;
}



static void file_delete(struct nb_buffer *buf)
{
	xfree((struct nb_buffer_file *)buf);
}


static size_t file_tell(struct nb_buffer *buf)
{
	struct nb_buffer_file *file_buf = (struct nb_buffer_file *)buf;
	return lseek(file_buf->fd, 0, SEEK_CUR);
}


static void file_fill(struct nb_buffer *buf)
{
	struct nb_buffer_file *file_buf = (struct nb_buffer_file *)buf;
	int retval;

	retval = read(file_buf->fd, buf->buf, buf->bufsize);
	buf->len = MAX(retval, 0);

	/* TODO handle error if retval < 0 */

	buf->eof = (buf->len == 0);
	TEMP_ASSERT(buf->len >= 0);
}


static void file_flush(struct nb_buffer *buf)
{
	struct nb_buffer_file *file_buf = (struct nb_buffer_file *)buf;
	ssize_t written;

	written = write(file_buf->fd, buf->buf, buf->len);
	TEMP_ASSERT(written == buf->len);
}
