#ifndef NETBUFS_H
#define NETBUFS_H

#include "buffer.h"
#include "cbor.h"
#include "diag.h"
#include "error.h"
#include "memory.h"

#include <stdbool.h>
#include <stdint.h>

typedef int		nb_lid_t;
typedef	uint64_t	nb_pid_t;

/* TODO hide this */
struct nb_attr
{
	const char *name;
	bool reqd;
	nb_pid_t pid;
};

/* TODO hide this */
struct nb_group
{
	const char *name;
	struct nb_attr **attrs;
	nb_pid_t max_pid;
	nb_lid_t *pid_to_lid;
};

struct nb;

void nb_default_err_handler(struct nb *nb, nb_err_t err, void *arg);
typedef void (nb_err_handler_t)(struct nb *nb, nb_err_t err, void *arg);

/*
 * NetBufs execution context.
 */
struct nb
{
	mempool_t mempool;			/* memory pool for this NetBuf */
	struct cbor_stream cs;			/* CBOR stream do encode/decode */
	struct nb_group **groups;		/* array of defined NB groups */
	struct diag diag;			/* diagnostics buffer */
	struct nb_group groups_ns;		/* groups namespace (bit of a hack) */
	struct nb_group *active_group;		/* (recv) currently active group */
	struct nb_attr *cur_attr;		/* (recv) currently processed attribute */

	nb_err_t err;				/* last error which occured */
	struct strbuf err_msg;			/* error message buffer */
	nb_err_handler_t *err_handler;		/* error handler */
	void *err_arg;				/* extra argument to the error handler */
};

void nb_set_err_handler(struct nb *nb, nb_err_handler_t *handler, void *arg);
char *nb_strerror(struct nb *nb);

#define	nb_recv_array(nb, arr) \
	do { \
		*arr = array_new_size(nb_internal_recv_array_size(nb), sizeof(**arr)); \
		diag_log_proto(&nb->diag, "["); \
		diag_indent_proto(&nb->diag); \
	} while (0);

size_t nb_internal_recv_array_size(struct nb *nb);


void nb_init(struct nb *nb, struct nb_buffer *buf);
void nb_free(struct nb *nb);

void nb_bind(struct nb *nb, struct nb_group *group, nb_lid_t id, const char *name, bool reqd);
struct nb_group *nb_group(struct nb *nb, nb_lid_t id, const char *name);

void nb_send_id(struct nb *nb, nb_lid_t id);

void nb_send_group(struct nb *nb, nb_lid_t id);
nb_err_t nb_send_group_end(struct nb *nb);

void nb_send_bool(struct nb *nb, nb_lid_t id, bool b);

void nb_send_i8(struct nb *nb, nb_lid_t id, int8_t i8);
void nb_send_i16(struct nb *nb, nb_lid_t id, int16_t i16);
void nb_send_i32(struct nb *nb, nb_lid_t id, int32_t i32);
void nb_send_i64(struct nb *nb, nb_lid_t id, int64_t i64);

void nb_send_u8(struct nb *nb, nb_lid_t id, uint8_t u8);
void nb_send_u16(struct nb *nb, nb_lid_t id, uint16_t u16);
void nb_send_u32(struct nb *nb, nb_lid_t id, uint32_t u32);
void nb_send_u64(struct nb *nb, nb_lid_t id, uint64_t u64);

void nb_send_string(struct nb *nb, nb_lid_t id, char *str);

void nb_send_array(struct nb *nb, nb_lid_t id, size_t nitems);
void nb_send_array_end(struct nb *nb);

void nb_recv_group(struct nb *nb, nb_lid_t id);
nb_err_t nb_recv_group_end(struct nb *nb);

bool nb_recv_attr(struct nb *nb, nb_lid_t *id);
void nb_recv_bool(struct nb *nb, bool *b);

void nb_recv_i8(struct nb *nb, int8_t *i8);
void nb_recv_i16(struct nb *nb, int16_t *i16);
void nb_recv_i32(struct nb *nb, int32_t *i32);
void nb_recv_i64(struct nb *nb, int64_t *i64);

void nb_recv_u8(struct nb *nb, uint8_t *u8);
void nb_recv_u16(struct nb *nb, uint16_t *u16);
void nb_recv_u32(struct nb *nb, uint32_t *u32);
void nb_recv_u64(struct nb *nb, uint64_t *u64);

void nb_recv_string(struct nb *nb, char **str);

/* nb_recv_array is a macro defined above */
void nb_recv_array_end(struct nb *nb);

#endif
