#include "array.h"
#include "cbor-internal.h"
#include "cbor.h"
#include "debug.h"
#include "netbufs-internal.h"
#include "netbufs.h"

#include <string.h>

#define	LID_UNKNOWN	(-1)


/* TODO remove this if possible */
size_t nb_internal_recv_array_size(struct nb *nb)
{
	size_t nitems;
	cbor_decode_array_begin(&nb->cs, &nitems);
	top_block(&nb->cs)->attr = nb->cur_attr;
	return nitems;
}


static void recv_pid(struct nb *nb, nb_pid_t *pid)
{
	cbor_decode_uint64(&nb->cs, pid);
}


static void recv_keys(struct nb *nb, struct nb_group *group)
{
	char *name;
	nb_pid_t pid;
	bool found;
	nb_lid_t lid;
	nb_err_t err;

	cbor_decode_map_begin_indef(&nb->cs);
	cbor_decode_text(&nb->cs, &name);
	recv_pid(nb, &pid);
	cbor_decode_map_end(&nb->cs);

	group->pid_to_lid = array_ensure_index(group->pid_to_lid, pid);

	for (lid = 0, found = false; lid < array_size(group->attrs); lid++)
		if (group->attrs[lid] && strcmp(group->attrs[lid]->name, name) == 0) {
			found = true;
			break;
		}

	TEMP_ASSERT(found); /* TODO allow usage of unknown groups! */
	group->pid_to_lid[pid] = lid;
}


/* TODO undef lids in pid_to_lid shall be LID_UNKNOWN! */
static void recv_id(struct nb *nb, struct nb_group *group, nb_lid_t *id)
{
	nb_err_t err;
	struct cbor_item item;
	nb_pid_t pid;

retry:
	recv_pid(nb, &pid);

	if (pid != 1) {
		if (unlikely(array_size(group->pid_to_lid) < pid || group->pid_to_lid[pid] == LID_UNKNOWN))
			nb_error(nb, NB_ERR_UNDEF_ID, "pID %lu is undefined", pid);

		*id = group->pid_to_lid[pid];
	}
	else { /* pID 1 is reserved and means "insert keys" */
		recv_keys(nb, group);
		goto retry;
	}
}


bool nb_recv_attr(struct nb *nb, nb_lid_t *id)
{
	struct nb_attr *attr;

	if (cbor_is_break(&nb->cs))
		return false;

	if (unlikely(nb->active_group == NULL))
		nb_error(nb, NB_ERR_OPER, "Cannot receive attributes outside of a group, "
			"please call nb_recv_group first");

	recv_id(nb, nb->active_group, id);
	assert(array_size(nb->active_group->attrs) >= *id); /* if not, recv_id would fail */

	attr = nb->active_group->attrs[*id];
	assert(attr != NULL); /* if not, recv_id would fail */
	diag_log_proto(&nb->diag, "%s =", attr->name);
	nb->cur_attr = attr;
	return true;
}


void nb_recv_group(struct nb *nb, nb_lid_t id)
{
	nb_lid_t id_real;
	struct nb_group *group;

	cbor_decode_map_begin_indef(&nb->cs);

	recv_id(nb, &nb->groups_ns, &id_real);
	if (id_real != 0)
		nb_error(nb, NB_ERR_OTHER, "First key sent in a group has to be 0");

	recv_id(nb, &nb->groups_ns, &id_real);
	if (id != id_real) /* TODO msg */
		nb_error(nb, NB_ERR_OTHER, "Expected group `%s', but group `%s' follows",
			NULL, NULL);

	assert(array_size(nb->groups) >= id_real); /* if not, recv_id would fail */
	group = nb->groups[id_real];

	assert(group != NULL); /* if not, recv_id would fail */
	nb->active_group = group;
	top_block(&nb->cs)->group = nb->active_group;

	diag_log_proto(&nb->diag, "(%s) {", nb->active_group->name);
	diag_indent_proto(&nb->diag);
}


/* TODO void nb_recv_group... */
nb_err_t nb_recv_group_end(struct nb *nb)
{
	nb_err_t err;
	struct nb_group *ended_group;

	ended_group = top_block(&nb->cs)->group; /* in Czech: `skončivší', ha ha */
	assert(ended_group != NULL); /* TODO check this */

	//if (top_block(&nb->cs)->num_items > 0)
		//diag_dedent_proto(&nb->diag);
	
	cbor_decode_map_end(&nb->cs);

	nb->active_group = top_block(&nb->cs)->group;

	diag_dedent_proto(&nb->diag);
	diag_log_proto(&nb->diag, "} /* %s */", ended_group->name);

	return NB_ERR_OK;
}


void nb_recv_i8(struct nb *nb, int8_t *i8)
{
	cbor_decode_int8(&nb->cs, i8);
	diag_log_proto(&nb->diag, "%i", *i8);
}


void nb_recv_i16(struct nb *nb, int16_t *i16)
{
	cbor_decode_int16(&nb->cs, i16);
	diag_log_proto(&nb->diag, "%i", *i16);
}


void nb_recv_i32(struct nb *nb, int32_t *i32)
{
	cbor_decode_int32(&nb->cs, i32);
	diag_log_proto(&nb->diag, "%i", *i32);
}


void nb_recv_i64(struct nb *nb, int64_t *i64)
{
	cbor_decode_int64(&nb->cs, i64);
	diag_log_proto(&nb->diag, "%li", *i64);
}


void nb_recv_u8(struct nb *nb, uint8_t *u8)
{
	cbor_decode_uint8(&nb->cs, u8);
	diag_log_proto(&nb->diag, "%u", *u8);
}


void nb_recv_u16(struct nb *nb, uint16_t *u16)
{
	cbor_decode_uint16(&nb->cs, u16);
	diag_log_proto(&nb->diag, "%u", *u16);
}


void nb_recv_u32(struct nb *nb, uint32_t *u32)
{
	cbor_decode_uint32(&nb->cs, u32);
	diag_log_proto(&nb->diag, "%u", *u32);
}


void nb_recv_u64(struct nb *nb, uint64_t *u64)
{
	cbor_decode_uint64(&nb->cs, u64);
	diag_log_proto(&nb->diag, "%lu", *u64);
}


void nb_recv_bool(struct nb *nb, bool *b)
{
	const char *sval_name;
	cbor_decode_bool(&nb->cs, b);
	sval_name = diag_get_sval_name(&nb->diag, *b ? CBOR_SVAL_TRUE : CBOR_SVAL_FALSE);
	assert(sval_name != NULL);
	diag_log_proto(&nb->diag, "%s", sval_name);
}


void nb_recv_string(struct nb *nb, char **str)
{
	cbor_decode_text(&nb->cs, str);
	diag_log_proto(&nb->diag, "\"%s\"", *str);
}


void nb_recv_array_end(struct nb *nb)
{
	struct nb_attr *attr;

	if (cbor_block_stack_empty(&nb->cs))
		nb_error(nb, NB_ERR_OPER, "Cannot end group, there's no group open");

	attr = top_block(&nb->cs)->attr;
	assert(attr != NULL);

	cbor_decode_array_end(&nb->cs);
	diag_dedent_proto(&nb->diag);
	diag_log_proto(&nb->diag, "] /* %s */", attr->name);
}
