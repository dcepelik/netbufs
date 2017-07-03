#ifndef STRBUF_H
#define STRBUF_H

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

struct strbuf
{
	char *str;		/* the buffer itself */
	size_t size;	/* current size of the buffer */
	size_t len;		/* length of the string */
};

void strbuf_init(struct strbuf *buf, size_t init_size);
void strbuf_free(struct strbuf *buf);
void strbuf_reset(struct strbuf *buf);
size_t strbuf_putc(struct strbuf *buf, char c);
size_t strbuf_puts(struct strbuf *buf, char *str);

void strbuf_prepare_write(struct strbuf *buf, size_t count);

size_t strbuf_strlen(struct strbuf *buf);
char *strbuf_get_string(struct strbuf *buf);
char *strbuf_strcpy(struct strbuf *buf);

size_t strbuf_printf(struct strbuf *buf, char *fmt, ...);
size_t strbuf_vprintf(struct strbuf *buf, char *fmt, va_list args);
size_t strbuf_vprintf_at(struct strbuf *buf, size_t offset, char *fmt, va_list args);

void strbuf_fill(struct strbuf *buf, size_t offset, size_t count, char c);
void strbuf_trim(struct strbuf *buf, ssize_t len);

#endif
