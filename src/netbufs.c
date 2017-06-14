#include "cbor.h"
#include "debug.h"
#include "netbufs.h"
#include <assert.h>
#include <string.h>


nb_err_t nb_init(struct netbuf *nb, struct nb_buf *buf)
{
	nb->cs = cbor_stream_new(buf);
	if (!nb->cs)
		return NB_ERR_NOMEM;

	return NB_ERR_OK;
}


void nb_free(struct netbuf *nb)
{
	cbor_stream_delete(nb->cs);
}


static inline nb_err_t send_key(struct netbuf *nb, nb_key_t key)
{
	return cbor_encode_tag(nb->cs, key);
}


static inline nb_err_t send_item(struct netbuf *nb, nb_key_t key, struct cbor_item item)
{
	nb_err_t err;
	if ((err = send_key(nb, key)) == NB_ERR_OK)
		return cbor_encode_item(nb->cs, &item);
	return err;
}


nb_err_t nb_send_bool(struct netbuf *nb, nb_key_t key, bool b)
{
	return send_item(nb, key, (struct cbor_item) {
		.type = CBOR_TYPE_SVAL,
		.sval = b ? CBOR_SVAL_TRUE : CBOR_SVAL_FALSE
	});
}


nb_err_t nb_send_int(struct netbuf *nb, nb_key_t key, int64_t i64)
{
	return send_item(nb, key, (struct cbor_item) {
		.type = CBOR_TYPE_INT,
		.i64 = i64
	});
}


nb_err_t nb_send_uint(struct netbuf *nb, nb_key_t key, uint64_t u64)
{
	return send_item(nb, key, (struct cbor_item) {
		.type = CBOR_TYPE_UINT,
		.u64 = u64
	});
}


nb_err_t nb_send_string(struct netbuf *nb, nb_key_t key, char *str)
{
	return send_item(nb, key, (struct cbor_item) {
		.type = CBOR_TYPE_TEXT,
		.len = str != NULL ? strlen(str) : 0,
		.str = str
	});
}


nb_err_t nb_send_array_begin(struct netbuf *nb, nb_key_t key, size_t size)
{
	nb_err_t err;
	if ((err = send_key(nb, key)) == NB_ERR_OK)
		return cbor_encode_array_begin(nb->cs, size);
	return err;
}


nb_err_t nb_send_array_end(struct netbuf *nb)
{
	return cbor_encode_array_end(nb->cs);
}


nb_err_t nb_send_map_begin(struct netbuf *nb, nb_key_t key, size_t size)
{
	nb_err_t err;
	if ((err = send_key(nb, key)) == NB_ERR_OK)
		return cbor_encode_map_begin(nb->cs, size);
	return err;
}


nb_err_t nb_send_map_end(struct netbuf *nb)
{
	return cbor_encode_map_end(nb->cs);
}


nb_err_t nb_send_group_begin(struct netbuf *nb, nb_key_t key)
{
	nb_err_t err;
	if ((err = send_key(nb, key)) == NB_ERR_OK)
		return cbor_encode_array_begin_indef(nb->cs);
	return err;
}


nb_err_t nb_send_group_end(struct netbuf *nb)
{
	return cbor_encode_array_end(nb->cs);
}


static nb_err_t recv_item(struct netbuf *nb, nb_key_t key, struct cbor_item *item)
{
	uint64_t tag_no;
	nb_err_t err;

	if ((err = cbor_decode_tag(nb->cs, &tag_no)) != NB_ERR_OK)
		return err;

	TEMP_ASSERT(tag_no == key);

	err = cbor_decode_item(nb->cs, item);
	assert(item->type != CBOR_TYPE_TAG);
	return err;
}


static uint64_t recv_uint(struct netbuf *nb, nb_key_t key, uint64_t max, nb_err_t *err)
{
	struct cbor_item item;

	if ((*err = recv_item(nb, key, &item)) != NB_ERR_OK)
		return 0;

	if (item.type != CBOR_TYPE_UINT) {
		*err = NB_ERR_ITEM; /* TODO this error? */
		return 0;
	}

	if (item.u64 > max) {
		*err = NB_ERR_RANGE;
		return 0;
	}

	return item.u64;
}


static int64_t recv_int(struct netbuf *nb, nb_key_t key, int64_t min, int64_t max, nb_err_t *err)
{
	struct cbor_item item;
	int64_t i64;

	if ((*err = recv_item(nb, key, &item)) != NB_ERR_OK)
		return 0;

	if (item.type == CBOR_TYPE_INT) {
		i64 = item.i64;
	}
	else if (item.type == CBOR_TYPE_UINT) {
		if (item.u64 > INT64_MAX) {
			*err = NB_ERR_RANGE;
			return 0;
		}

		i64 = (int64_t)item.u64;
	}
	else {
		*err = NB_ERR_ITEM;
		return 0;
	}

	if (i64 >= min && i64 <= max)
		return i64;

	*err = NB_ERR_RANGE;
	return 0;
}


nb_err_t nb_recv_bool(struct netbuf *nb, nb_key_t key, bool *b)
{
	nb_err_t err;
	struct cbor_item item;
	if ((err = recv_item(nb, key, &item)) == NB_ERR_OK)
		*b = (item.sval == CBOR_SVAL_TRUE);
	return err;
}


nb_err_t nb_recv_u32(struct netbuf *nb, nb_key_t key, uint32_t *u32)
{
	nb_err_t err;
	*u32 = (uint32_t)recv_uint(nb, key, UINT32_MAX, &err);
	return err;
}


nb_err_t nb_recv_i32(struct netbuf *nb, nb_key_t key, int32_t *i32)
{
	nb_err_t err;
	*i32 = (int32_t)recv_int(nb, key, INT32_MIN, INT32_MAX, &err);
	return err;
}


nb_err_t nb_recv_string(struct netbuf *nb, nb_key_t key, char **str)
{
	struct cbor_item item;
	nb_err_t err;
	
	if ((err = recv_item(nb, key, &item)) == NB_ERR_OK)
		*str = item.str;
	return err;
}


nb_err_t nb_recv_array_begin(struct netbuf *nb, nb_key_t key, size_t *size)
{
	uint64_t tag_no;
	nb_err_t err;

	if ((err = cbor_decode_tag(nb->cs, &tag_no)) != NB_ERR_OK)
		return err;

	TEMP_ASSERT(tag_no == key);
	return cbor_decode_array_begin(nb->cs, size);
}


nb_err_t nb_recv_array_end(struct netbuf *nb)
{
	return cbor_decode_array_end(nb->cs);
}

nb_err_t nb_recv_map_begin(struct netbuf *nb, nb_key_t key, size_t *size)
{
	uint64_t tag_no;
	nb_err_t err;

	if ((err = cbor_decode_tag(nb->cs, &tag_no)) != NB_ERR_OK)
		return err;

	TEMP_ASSERT(tag_no == key);
	return cbor_decode_map_begin(nb->cs, size);
}


nb_err_t nb_recv_map_end(struct netbuf *nb)
{
	return cbor_decode_map_end(nb->cs);
}


/*
 * TODO This has to work in a very different way.
 */
nb_err_t nb_recv_group_begin(struct netbuf *nb, nb_key_t key)
{
	uint64_t tag_no;
	nb_err_t err;

	if ((err = cbor_decode_tag(nb->cs, &tag_no)) != NB_ERR_OK)
		return err;

	TEMP_ASSERT(tag_no == key);
	err = cbor_decode_array_begin_indef(nb->cs);
	assert(top_block(nb->cs)->indefinite);
	return err;
}


/*
 * TODO This too has to be expanded.
 */
nb_err_t nb_recv_group_end(struct netbuf *nb)
{
	assert(top_block(nb->cs)->indefinite);
	return cbor_decode_array_end(nb->cs);
}
