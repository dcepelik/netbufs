#include "memory.h"
#include "strbuf.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>


#define MAX(a, b) ((a) >= (b) ? (a) : (b))


static void strbuf_resize(struct strbuf *buf, size_t new_size)
{
	assert(new_size > 0);

	buf->str = cbor_realloc(buf->str, new_size);

	buf->size = new_size;
	if (buf->len > buf->size - 1)
		buf->len = buf->size - 1;
}


void strbuf_init(struct strbuf *buf, size_t init_size)
{
	assert(init_size > 0);

	buf->str = NULL;
	buf->len = 0;
	buf->size = 0;

	strbuf_resize(buf, init_size);
}


void strbuf_prepare_write(struct strbuf *buf, size_t count)
{
	if (buf->len + count >= buf->size) /* >= because of the '\0' */
		strbuf_resize(buf, MAX(count, 2 * buf->size));
}


size_t strbuf_putc(struct strbuf *buf, char c)
{
	strbuf_prepare_write(buf, 1);
	buf->str[buf->len++] = c;
	return 1;
}


size_t strbuf_puts(struct strbuf *buf, char *str)
{
	strbuf_printf(buf, "%s", str);
	return strlen(str);
}


char *strbuf_get_string(struct strbuf *buf)
{
	buf->str[buf->len] = '\0'; /* there's room for the NUL */
	return buf->str;
}


size_t strbuf_strlen(struct strbuf *buf)
{
	return buf->len;
}


char *strbuf_strcpy(struct strbuf *buf)
{
	char *copy;

	copy = cbor_malloc(buf->len + 1);
	strncpy(copy, buf->str, buf->len + 1);

	return copy;
}


void strbuf_reset(struct strbuf *buf)
{
	buf->len = 0;
}


void strbuf_free(struct strbuf *buf)
{
	free(buf->str);
}


size_t strbuf_vprintf_at(struct strbuf *buf, size_t offset, char *fmt, va_list args)
{
	va_list args2;
	size_t num_written;
	size_t size_needed;

	va_copy(args2, args);

	num_written = vsnprintf(NULL, 0, fmt, args2);
	if (num_written < 0)
		return -1;

	size_needed = offset + num_written + 1;
	if (size_needed > buf->size)
		strbuf_resize(buf, MAX(2 * buf->size, size_needed));

	vsnprintf(buf->str + offset, (num_written + 1), fmt, args);
	buf->len = MAX(buf->len, offset + num_written);

	va_end(args2);
	va_end(args);

	return num_written;
}


size_t strbuf_printf(struct strbuf *buf, char *fmt, ...)
{
	size_t num_written;

	va_list args;
	va_start(args, fmt);
	num_written = strbuf_vprintf_at(buf, buf->len, fmt, args);
	va_end(args);

	return num_written;
}
