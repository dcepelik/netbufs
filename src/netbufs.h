#ifndef NETBUFS_H
#define NETBUFS_H

#include "error.h"
#include "internal.h"

#include <stdbool.h>
#include <stdint.h>


typedef uint64_t nb_key_t;


struct netbuf
{
	struct cbor_stream *cs;
	struct cbor_item **items;	/* unprocessed data items */
};


nb_err_t nb_init(struct netbuf *nb, struct nb_buf *buf);
void nb_free(struct netbuf *nb);

nb_err_t nb_send_bool(struct netbuf *nb, nb_key_t key, bool b);

nb_err_t nb_send_int(struct netbuf *nb, nb_key_t key, int64_t i64);
nb_err_t nb_send_uint(struct netbuf *nb, nb_key_t key, uint64_t u64);

nb_err_t nb_send_string(struct netbuf *nb, nb_key_t key, char *str);

nb_err_t nb_send_array_begin(struct netbuf *nb, nb_key_t key, uint64_t size);
nb_err_t nb_send_array_end(struct netbuf *nb);

nb_err_t nb_send_map_begin(struct netbuf *nb, nb_key_t key, size_t size);
nb_err_t nb_send_map_end(struct netbuf *nb);

nb_err_t nb_send_group_begin(struct netbuf *nb, nb_key_t key);
nb_err_t nb_send_group_end(struct netbuf *nb);

nb_err_t nb_recv_bool(struct netbuf *nb, nb_key_t key, bool *b);

nb_err_t nb_recv_i8(struct netbuf *nb, nb_key_t key, int8_t *i8);
nb_err_t nb_recv_i16(struct netbuf *nb, nb_key_t key, int16_t *i16);
nb_err_t nb_recv_i32(struct netbuf *nb, nb_key_t key, int32_t *i32);
nb_err_t nb_recv_i64(struct netbuf *nb, nb_key_t key, int64_t *i64);

nb_err_t nb_recv_u8(struct netbuf *nb, nb_key_t key, uint8_t *u8);
nb_err_t nb_recv_u16(struct netbuf *nb, nb_key_t key, uint16_t *u16);
nb_err_t nb_recv_u32(struct netbuf *nb, nb_key_t key, uint32_t *u32);
nb_err_t nb_recv_u64(struct netbuf *nb, nb_key_t key, uint64_t *u64);

nb_err_t nb_recv_string(struct netbuf *nb, nb_key_t key, char **str);

nb_err_t nb_recv_array_begin(struct netbuf *nb, nb_key_t key, uint64_t *size);
nb_err_t nb_recv_array_end(struct netbuf *nb);

nb_err_t nb_recv_map_begin(struct netbuf *nb, nb_key_t key, size_t *size);
nb_err_t nb_recv_map_end(struct netbuf *nb);

nb_err_t nb_recv_group_begin(struct netbuf *nb, nb_key_t key);
nb_err_t nb_recv_group_end(struct netbuf *nb);

#endif
