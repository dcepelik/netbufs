#ifndef NETBUFS_H
#define NETBUFS_H

#include "buf.h"
#include "error.h"

#include <stdbool.h>
#include <stdint.h>


typedef int nb_lid_t;


struct nb
{
	struct cbor_stream *cs;	/* the CBOR stream processed */
};


struct nb_group
{
	struct nb *nb;
};

nb_err_t nb_init(struct nb *nb, struct nb_buf *buf);
void nb_free(struct nb *nb);

nb_err_t nb_bind(struct nb_group *grp, nb_lid_t id, char *name, bool reqd);
struct nb_group *nb_group(struct nb *nb, nb_lid_t id, char *name);

nb_err_t nb_send_group(struct nb *nb, nb_lid_t id);
nb_err_t nb_send_group_end(struct nb *nb);

nb_err_t nb_send_bool(struct nb *nb, nb_lid_t id, bool b);

nb_err_t nb_send_i8(struct nb *nb, nb_lid_t id, int8_t i8);
nb_err_t nb_send_i16(struct nb *nb, nb_lid_t id, int16_t i16);
nb_err_t nb_send_i32(struct nb *nb, nb_lid_t id, int32_t i32);
nb_err_t nb_send_i64(struct nb *nb, nb_lid_t id, int64_t i64);

nb_err_t nb_send_u8(struct nb *nb, nb_lid_t id, uint8_t u8);
nb_err_t nb_send_u16(struct nb *nb, nb_lid_t id, uint16_t u16);
nb_err_t nb_send_u32(struct nb *nb, nb_lid_t id, uint32_t u32);
nb_err_t nb_send_u64(struct nb *nb, nb_lid_t id, uint64_t u64);

nb_err_t nb_send_string(struct nb *nb, nb_lid_t id, char *str);

nb_err_t nb_send_array(struct nb *nb, nb_lid_t id, size_t nitems);
nb_err_t nb_send_array_end(struct nb *nb);

nb_err_t nb_recv_group(struct nb *nb, nb_lid_t id);
nb_err_t nb_recv_group_end(struct nb *nb);

nb_err_t nb_recv_id(struct nb *nb, nb_lid_t *id);
nb_err_t nb_recv_bool(struct nb *nb, bool *b);

nb_err_t nb_recv_i8(struct nb *nb, int8_t *i8);
nb_err_t nb_recv_i16(struct nb *nb, int16_t *i16);
nb_err_t nb_recv_i32(struct nb *nb, int32_t *i32);
nb_err_t nb_recv_i64(struct nb *nb, int64_t *i64);

nb_err_t nb_recv_u8(struct nb *nb, uint8_t *u8);
nb_err_t nb_recv_u16(struct nb *nb, uint16_t *u16);
nb_err_t nb_recv_u32(struct nb *nb, uint32_t *u32);
nb_err_t nb_recv_u64(struct nb *nb, uint64_t *u64);

nb_err_t nb_recv_string(struct nb *nb, char **str);

nb_err_t nb_recv_array(struct nb *nb, void *arr);
nb_err_t nb_recv_array_end(struct nb *nb);

#endif
