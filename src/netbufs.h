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
};


void nb_init(struct netbuf *nb, struct nb_buf *buf);

void nb_send_int(struct netbuf *nb, nb_key_t key, int64_t i64);
void nb_send_uint(struct netbuf *nb, nb_key_t key, uint64_t u64);
void nb_send_bool(struct netbuf *nb, nb_key_t key, bool b);
void nb_send_string(struct netbuf *nb, nb_key_t key, char *str);

nb_err_t nb_recv_u32(struct netbuf *nb, nb_key_t key, uint32_t *i64);

void nb_send_array_begin(struct netbuf *nb, nb_key_t key, uint64_t size);
void nb_send_array_end(struct netbuf *nb);

nb_err_t nb_recv_array_begin(struct netbuf *nb, nb_key_t key, uint64_t *size);
nb_err_t nb_recv_array_end(struct netbuf *nb);

void nb_map_begin(struct netbuf *nb, nb_key_t key);
void nb_map_end(struct netbuf *nb);

void nb_send_ipv4(struct netbuf *nb, nb_key_t key, ipv4_t ip);
nb_err_t nb_recv_ipv4(struct netbuf *nb, nb_key_t key, ipv4_t *ip);
nb_err_t nb_recv_string(struct netbuf *nb, nb_key_t key, char **str);

void nb_begin_group(struct netbuf *nb);

nb_err_t nb_send_group_begin(struct netbuf *nb, nb_key_t key);
nb_err_t nb_recv_group_begin(struct netbuf *nb, nb_key_t key);
nb_err_t nb_send_group_end(struct netbuf *nb);
nb_err_t nb_recv_group_end(struct netbuf *nb);

#endif
