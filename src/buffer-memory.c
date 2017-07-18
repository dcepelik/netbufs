/*
 * buffer-memory:
 * Memory Buffer Implementation
 */

#include "buffer-internal.h"
#include "buffer.h"
#include "debug.h"
#include "memory.h"
#include "string.h"
#include "util.h"

#define NB_DEBUG_THIS	1


static size_t mem_tell(struct nb_buffer *buf);
static void mem_delete(struct nb_buffer *buf);
static void mem_fill(struct nb_buffer *buf);
static void mem_flush(struct nb_buffer *buf);

const struct nb_buffer_ops mem_ops = {
	.free = mem_delete,
	.fill = mem_fill,
	.flush = mem_flush,
	.tell = mem_tell,
};

struct nb_buffer_memory
{
	struct nb_buffer buf;
	nb_byte_t *memory;
	size_t memory_size;
	size_t memory_len;
	size_t memory_pos;
};


struct nb_buffer *nb_buffer_new_memory(void)
{
	struct nb_buffer_memory *mem_buf;

	mem_buf = nb_malloc(sizeof(*mem_buf));
	nb_buffer_init(&mem_buf->buf);
	mem_buf->buf.ops = &mem_ops;
	mem_buf->memory = NULL;
	mem_buf->memory_len = 0;
	mem_buf->memory_size = 0;
	mem_buf->memory_pos = 0;

	return &mem_buf->buf;
}


static size_t mem_tell(struct nb_buffer *buf)
{
	(void) buf;
	return 0;
}


static void mem_delete(struct nb_buffer *buf)
{
	struct nb_buffer_memory *mem_buf = (struct nb_buffer_memory *)buf;
	xfree(mem_buf->memory);
	xfree(mem_buf);
}


static void mem_fill(struct nb_buffer *buf)
{
	struct nb_buffer_memory *mem_buf = (struct nb_buffer_memory *)buf;
	size_t avail;
	size_t ncpy;
	
	avail = mem_buf->memory_len - mem_buf->memory_pos;
	ncpy = MIN(avail, buf->bufsize);

	memcpy(buf->buf, mem_buf->memory + mem_buf->memory_pos, ncpy);

	buf->len = ncpy;
	mem_buf->memory_pos += ncpy;
}


static void mem_flush(struct nb_buffer *buf)
{
	struct nb_buffer_memory *mem_buf = (struct nb_buffer_memory *)buf;
	size_t new_mry_size;

	if (mem_buf->memory_len + buf->len > mem_buf->memory_size) {
		new_mry_size = MAX(2 * mem_buf->memory_size, buf->bufsize);
		mem_buf->memory = nb_realloc(mem_buf->memory, new_mry_size);
		mem_buf->memory_size = new_mry_size;
	}

	memcpy(mem_buf->memory + mem_buf->memory_len, buf->buf, buf->len);
	mem_buf->memory_len += buf->len;
}
