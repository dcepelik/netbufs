#include "array.h"
#include "cbor-internal.h"
#include "cbor.h"
#include "debug.h"
#include "netbufs.h"


/* TODO remove this if possible */
size_t nb_internal_recv_array_size(struct nb *nb)
{
	size_t nitems;
	cbor_decode_array_begin(nb->cs, &nitems);
	top_block(nb->cs)->attr = nb->cur_attr;
	return nitems;
}


static bool recv_id(struct nb *nb, nb_lid_t *id)
{
	nb_err_t err;
	struct cbor_item item;

	if ((err = cbor_peek(nb->cs, &item)) != NB_ERR_OK) {
		TEMP_ASSERT(err == NB_ERR_BREAK);
		diag_log_raw(&nb->diag, NULL, 0); /* TODO remove, just for debugging */
		return false;
	}

	assert(cbor_decode_int32(nb->cs, id) == NB_ERR_OK);
	return true;
}


bool nb_recv_attr(struct nb *nb, nb_lid_t *id)
{
	bool ret;

	if (top_block(nb->cs)->num_items > 1)
		diag_dedent_proto(&nb->diag);
	ret = recv_id(nb, id);

	/* TODO remove this conditional by always having an active (default) group */
	if (ret && nb->active_group != NULL) {
		if (*id <= array_size(nb->active_group->attrs))
			if (nb->active_group->attrs[*id] != NULL) {
				diag_log_proto(&nb->diag, "%s =",
					nb->active_group->attrs[*id]->name);
				nb->cur_attr = nb->active_group->attrs[*id];
			}
	}

	diag_indent_proto(&nb->diag);
	return ret;
}


void nb_recv_group(struct nb *nb, nb_lid_t id)
{
	nb_lid_t id_real;

	cbor_decode_array_begin_indef(nb->cs);
	TEMP_ASSERT(top_block(nb->cs)->type == CBOR_TYPE_ARRAY);

	recv_id(nb, &id_real);
	TEMP_ASSERT(id <= array_size(nb->groups));
	TEMP_ASSERT(nb->groups[id] != NULL);
	TEMP_ASSERT(id == id_real);

	nb->active_group = top_block(nb->cs)->group = nb->groups[id];
	diag_log_proto(&nb->diag, "(%s) {", nb->active_group->name);
	diag_indent_proto(&nb->diag);
}


nb_err_t nb_recv_group_end(struct nb *nb)
{
	nb_err_t err;
	struct nb_group *ended_group;

	ended_group = top_block(nb->cs)->group;

	if (top_block(nb->cs)->num_items > 0)
		diag_dedent_proto(&nb->diag);
	
	if ((err = cbor_decode_array_end(nb->cs)) != NB_ERR_OK) {
		assert(false);
		return err;
	}

	nb->active_group = top_block(nb->cs)->group;
	//TEMP_ASSERT(nb->active_group != NULL);
	diag_dedent_proto(&nb->diag);
	diag_log_proto(&nb->diag, "} /* %s */",
		ended_group ? ended_group->name : "<none>");

	return NB_ERR_OK;
}


void nb_recv_i8(struct nb *nb, int8_t *i8)
{
	cbor_decode_int8(nb->cs, i8);
	diag_log_proto(&nb->diag, "%i", *i8);
}


void nb_recv_i16(struct nb *nb, int16_t *i16)
{
	cbor_decode_int16(nb->cs, i16);
	diag_log_proto(&nb->diag, "%i", *i16);
}


void nb_recv_i32(struct nb *nb, int32_t *i32)
{
	cbor_decode_int32(nb->cs, i32);
	diag_log_proto(&nb->diag, "%i", *i32);
}


void nb_recv_i64(struct nb *nb, int64_t *i64)
{
	cbor_decode_int64(nb->cs, i64);
	diag_log_proto(&nb->diag, "%li", *i64);
}


void nb_recv_u8(struct nb *nb, uint8_t *u8)
{
	cbor_decode_uint8(nb->cs, u8);
	diag_log_proto(&nb->diag, "%u", *u8);
}


void nb_recv_u16(struct nb *nb, uint16_t *u16)
{
	cbor_decode_uint16(nb->cs, u16);
	diag_log_proto(&nb->diag, "%u", *u16);
}


void nb_recv_u32(struct nb *nb, uint32_t *u32)
{
	cbor_decode_uint32(nb->cs, u32);
	diag_log_proto(&nb->diag, "%u", *u32);
}


void nb_recv_u64(struct nb *nb, uint64_t *u64)
{
	cbor_decode_uint64(nb->cs, u64);
	diag_log_proto(&nb->diag, "%lu", *u64);
}


void nb_recv_bool(struct nb *nb, bool *b)
{
	enum cbor_sval sval;
	cbor_decode_sval(nb->cs, &sval);
	*b = (sval == CBOR_SVAL_TRUE);
	diag_log_proto(&nb->diag, "simple(%u)", sval);
}


void nb_recv_string(struct nb *nb, char **str)
{
	size_t len;
	cbor_decode_text(nb->cs, (nb_byte_t **)str, &len);
	diag_log_proto(&nb->diag, "\"%s\"", *str);
}


void nb_recv_array_end(struct nb *nb)
{
	struct nb_attr *attr;

	attr = top_block(nb->cs)->attr;
	cbor_decode_array_end(nb->cs);
	diag_dedent_proto(&nb->diag);
	diag_log_proto(&nb->diag, "] /* %s */", attr ? attr->name : "?");
}
