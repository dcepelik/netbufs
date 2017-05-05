/*
 * TODO Sending items without keys (do we need it?)
 */

#ifndef NETBUFS_H
#define NETBUFS_H

#include "error.h"
#include "internal.h"
#include <stdbool.h>
#include <stdint.h>

struct netbuf;

nb_err_t nb_init(struct netbuf *nb);

nb_err_t nb_send_u8(struct netbuf *nb, int key, uint8_t u8);
nb_err_t nb_send_u16(struct netbuf *nb, int key, uint16_t u16);
nb_err_t nb_send_u32(struct netbuf *nb, int key, uint32_t u32);
nb_err_t nb_send_u64(struct netbuf *nb, int key, uint64_t u64);
nb_err_t nb_send_uint(struct netbuf *nb, int key, unsigned int u);

nb_err_t nb_send_i8(struct netbuf *nb, int key, int8_t i8);
nb_err_t nb_send_i16(struct netbuf *nb, int key, int16_t i16);
nb_err_t nb_send_i32(struct netbuf *nb, int key, int32_t i32);
nb_err_t nb_send_i64(struct netbuf *nb, int key, int64_t i64);
nb_err_t nb_send_int(struct netbuf *nb, int key, int i);

nb_err_t nb_send_bool(struct netbuf *nb, int key, bool b);
nb_err_t nb_send_string(struct netbuf *nb, int key, char *str);

nb_err_t nb_array_begin(struct netbuf *nb, int key, uint64_t nitems);
nb_err_t nb_array_end(struct netbuf *nb);

nb_err_t nb_map_begin(struct netbuf *nb, int key, uint64_t npairs);
nb_err_t nb_map_end(struct netbuf *nb);

nb_err_t nb_send_ipv4(struct netbuf *nb, int key, ipv4_t ip);

#endif
