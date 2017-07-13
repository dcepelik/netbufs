#ifndef NB_BUF_H
#define NB_BUF_H

#include "common.h"
#include "error.h"
#include "buffer-internal.h"

#include <stdbool.h>
#include <stdlib.h>

#define BUF_EOF	(-1)

struct nb_buffer;

bool nb_buffer_is_eof(struct nb_buffer *buf);

struct nb_buffer *nb_buffer_new_file(int fd_in);
struct nb_buffer *nb_buffer_new_memory(void);

void nb_buffer_delete(struct nb_buffer *buf);

nb_err_t nb_buffer_write(struct nb_buffer *buf, nb_byte_t *bytes, size_t count);
nb_err_t nb_buffer_read(struct nb_buffer *buf, nb_byte_t *bytes, size_t count);
size_t nb_buffer_tell(struct nb_buffer *buf);
void nb_buffer_flush(struct nb_buffer *buf);

int nb_buffer_getc(struct nb_buffer *buf);
void nb_buffer_ungetc(struct nb_buffer *buf, int c);
int nb_buffer_peek(struct nb_buffer *buf);

#endif
