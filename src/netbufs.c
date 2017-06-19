#include "cbor.h"
#include "debug.h"
#include "netbufs.h"
#include <assert.h>
#include <string.h>


nb_err_t nb_init(struct netbuf *nb, struct nb_buf *buf)
{
	if ((nb->cs = cbor_stream_new(buf)) == NULL)
		return NB_ERR_NOMEM;

	nb->cur_key_valid = false;

	return NB_ERR_OK;
}


void nb_free(struct netbuf *nb)
{
	cbor_stream_delete(nb->cs);
}


static nb_err_t send_keypair(struct netbuf *nb, char *name, int key)
{
}


nb_err_t nb_bind(struct netbuf *nb, char *name, int key)
{
	return send_keypair(nb, name, key);
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


static nb_err_t peek_key(struct netbuf *nb, nb_key_t *key)
{
	nb_err_t err;

	if (!nb->cur_key_valid) {
		if ((err = cbor_decode_tag(nb->cs, &nb->cur_key)) != NB_ERR_OK)
			return err;
		nb->cur_key_valid = true;
	}

	assert(nb->cur_key_valid);

	*key = nb->cur_key;
	return NB_ERR_OK;
}


/* TODO this has to fail for read-across-group */
static nb_err_t recv_key(struct netbuf *nb, nb_key_t *key)
{
	nb_err_t err;
	err = peek_key(nb, key);
	nb->cur_key_valid = false;
	return err;
}


static nb_err_t seek_item(struct netbuf *nb, nb_key_t key)
{
	nb_key_t cur_key;
	nb_err_t err;

	while ((err = recv_key(nb, &cur_key)) == NB_ERR_OK)
		if (cur_key == key)
			break;

	return err;
}


nb_err_t nb_recv_bool(struct netbuf *nb, nb_key_t key, bool *b)
{
	nb_err_t err;
	enum cbor_sval sval;

	if ((err = seek_item(nb, key)) != NB_ERR_OK)
		return err;

	if ((err = cbor_decode_sval(nb->cs, &sval)) != NB_ERR_OK)
		return err;

	*b = (sval == CBOR_SVAL_TRUE);
	return NB_ERR_OK;
}


nb_err_t nb_recv_u32(struct netbuf *nb, nb_key_t key, uint32_t *u32)
{
	nb_err_t err;
	if ((err = seek_item(nb, key)) == NB_ERR_OK)
		return cbor_decode_uint32(nb->cs, u32);
	return err;
}


nb_err_t nb_recv_i32(struct netbuf *nb, nb_key_t key, int32_t *i32)
{
	nb_err_t err;
	if ((err = seek_item(nb, key)) == NB_ERR_OK)
		return cbor_decode_int32(nb->cs, i32);
	return err;
}


nb_err_t nb_recv_string(struct netbuf *nb, nb_key_t key, char **str)
{
	nb_err_t err;
	size_t foo;
	if ((err = seek_item(nb, key)) == NB_ERR_OK)
		cbor_decode_text(nb->cs, (byte_t **)str, &foo);
	return err;
}


nb_err_t nb_recv_array_begin(struct netbuf *nb, nb_key_t key, size_t *size)
{
	nb_err_t err;
	if ((err = seek_item(nb, key)) == NB_ERR_OK)
		return cbor_decode_array_begin(nb->cs, size);
	return err;
}


nb_err_t nb_recv_array_end(struct netbuf *nb)
{
	/* TODO take care of extra items */
	return cbor_decode_array_end(nb->cs);
}


nb_err_t nb_recv_map_begin(struct netbuf *nb, nb_key_t key, size_t *size)
{
	nb_err_t err;
	if ((err = seek_item(nb, key)) == NB_ERR_OK)
		return cbor_decode_map_begin(nb->cs, size);
	return err;
}


nb_err_t nb_recv_map_end(struct netbuf *nb)
{
	/* TODO take care of extra items */
	return cbor_decode_map_end(nb->cs);
}


nb_err_t nb_recv_group_begin(struct netbuf *nb, nb_key_t key)
{
	nb_err_t err;
	if ((err = seek_item(nb, key)) == NB_ERR_OK)
		return cbor_decode_array_begin_indef(nb->cs);
	return err;
}


nb_err_t nb_recv_group_end(struct netbuf *nb)
{
	/* TODO skip extra items */
	return cbor_decode_array_end(nb->cs);
}
