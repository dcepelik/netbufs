#ifndef NETBUFS_H
#define NETBUFS_H

#include "error.h"
#include "internal.h"

#include <stdbool.h>
#include <stdint.h>


typedef uint64_t nb_key_t;


struct netbuf
{
	struct cbor_stream *cs;	/* the CBOR stream processed */
	nb_key_t cur_key;		/* current processed key (internal) */
	bool cur_key_valid;		/* is cur_key valid? */
	const char **names;		/* ekey to name */
	nb_key_t *ikey;			/* ekey to ikey map */
	int *ekey;				/* ikey to ekey map */
	int ikey_max;			/* ikey counter */
	bool disable_keys;		/* peek_key recursion breaker */
};


nb_err_t nb_init(struct netbuf *nb, struct nb_buf *buf);
void nb_free(struct netbuf *nb);

nb_err_t nb_recv_item(struct netbuf *nb, nb_key_t *key, struct cbor_item *item);
nb_err_t nb_peek(struct netbuf *nb, nb_key_t *key);

nb_err_t nb_bind(struct netbuf *nb, const char *name, int key);

nb_err_t nb_send_bool(struct netbuf *nb, int key, bool b);

nb_err_t nb_send_int(struct netbuf *nb, int key, int64_t i64);
nb_err_t nb_send_uint(struct netbuf *nb, int key, uint64_t u64);

nb_err_t nb_send_string(struct netbuf *nb, int key, char *str);

nb_err_t nb_send_array_begin(struct netbuf *nb, int key, uint64_t size);
nb_err_t nb_send_array_end(struct netbuf *nb);

nb_err_t nb_send_map_begin(struct netbuf *nb, int key, size_t size);
nb_err_t nb_send_map_end(struct netbuf *nb);

nb_err_t nb_send_group_begin(struct netbuf *nb, int key);
nb_err_t nb_send_group_end(struct netbuf *nb);

nb_err_t nb_recv_bool(struct netbuf *nb, int key, bool *b);

nb_err_t nb_recv_i8(struct netbuf *nb, int key, int8_t *i8);
nb_err_t nb_recv_i16(struct netbuf *nb, int key, int16_t *i16);
nb_err_t nb_recv_i32(struct netbuf *nb, int key, int32_t *i32);
nb_err_t nb_recv_i64(struct netbuf *nb, int key, int64_t *i64);

nb_err_t nb_recv_u8(struct netbuf *nb, int key, uint8_t *u8);
nb_err_t nb_recv_u16(struct netbuf *nb, int key, uint16_t *u16);
nb_err_t nb_recv_u32(struct netbuf *nb, int key, uint32_t *u32);
nb_err_t nb_recv_u64(struct netbuf *nb, int key, uint64_t *u64);

nb_err_t nb_recv_string(struct netbuf *nb, int key, char **str);

nb_err_t nb_recv_array_begin(struct netbuf *nb, int key, uint64_t *size);
nb_err_t nb_recv_array_end(struct netbuf *nb);

nb_err_t nb_recv_map_begin(struct netbuf *nb, int key, size_t *size);
nb_err_t nb_recv_map_end(struct netbuf *nb);

nb_err_t nb_recv_group_begin(struct netbuf *nb, int key);
nb_err_t nb_recv_group_end(struct netbuf *nb);

#endif
