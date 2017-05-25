#ifndef NETBUFS_H
#define NETBUFS_H

#include "error.h"
#include "internal.h"
#include <stdbool.h>
#include <stdint.h>

struct netbuf
{
	struct cbor_stream *cs;
};


void nb_init(struct netbuf *nb, struct nb_buf *buf);

void nb_send_int(struct netbuf *nb, int key, int64_t i64);
void nb_send_uint(struct netbuf *nb, int key, uint64_t u64);
void nb_send_bool(struct netbuf *nb, int key, bool b);
void nb_send_string(struct netbuf *nb, int key, char *str);

void nb_array_begin(struct netbuf *nb, int key);
void nb_array_end(struct netbuf *nb);

void nb_map_begin(struct netbuf *nb, int key);
void nb_map_end(struct netbuf *nb);

void nb_send_ipv4(struct netbuf *nb, int key, ipv4_t ip);

#endif
