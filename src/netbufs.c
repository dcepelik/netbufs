#include "cbor.h"
#include "debug.h"
#include "netbufs.h"
#include <assert.h>
#include <string.h>

/* remove this include */
#include "../benchmark/src/serialize-netbufs.h"


nb_err_t nb_init(struct netbuf *nb, struct nb_buf *buf)
{
	if ((nb->cs = cbor_stream_new(buf)) == NULL)
		return NB_ERR_NOMEM;

	nb->cur_key_valid = false;

	nb->names = calloc(64, sizeof(*nb->names));
	nb->ikey = calloc(64, sizeof(*nb->ikey));
	nb->ekey = calloc(64, sizeof(*nb->ekey));
	nb->ikey_max = 0;
	nb->disable_keys = true;

	return NB_ERR_OK;
}


void nb_free(struct netbuf *nb)
{
	cbor_stream_delete(nb->cs);
}


static nb_err_t send_keypair(struct netbuf *nb, char *name, int key)
{
	nb_send_group_begin(nb, NB_KEY);
	nb_send_string(nb, NB_KEY_NAME, name);
	nb_send_uint(nb, NB_KEY_ID, key);
	return nb_send_group_end(nb);
}


nb_err_t nb_bind(struct netbuf *nb, const char *name, int ekey)
{
	nb->names[ekey] = name;
	assert(ekey < 64);
	return NB_ERR_OK;
}


static inline bool known_ekey(struct netbuf *nb, int ekey)
{
	return nb->ikey[ekey] != 0;
}


static inline bool known_ikey(struct netbuf *nb, nb_err_t ikey)
{
	return known_ekey(nb, nb->ekey[ikey]);
}


static inline nb_err_t introduce_ikey(struct netbuf *nb, int ekey)
{
	nb_key_t ikey;
	nb_err_t err;

	assert(nb->names[ekey]);

	ikey = ++nb->ikey_max;
	TEMP_ASSERT(ikey < 64);

	nb->ikey[ekey] = ikey;
	nb->ekey[ikey] = ekey;

	nb_send_group_begin(nb, NB_KEY);
	nb_send_string(nb, NB_KEY_NAME, (char *)nb->names[ekey]);
	nb_send_uint(nb, NB_KEY_ID, ikey);
	return nb_send_group_end(nb);
}


static inline nb_err_t recv_ikey(struct netbuf *nb)
{
	char *name;
	nb_key_t ikey;
	int ekey;
	bool found;
	nb_err_t err;

	assert(nb->cur_key == NB_KEY);

	nb_recv_group_begin(nb, NB_KEY);
	nb_recv_string(nb, NB_KEY_NAME, &name);
	nb_recv_u64(nb, NB_KEY_ID, &ikey);

	if ((err = nb_recv_group_end(nb)) != NB_ERR_OK)
		return err;

	for (ekey = 0; ekey < 64; ekey++) {
		if (nb->names[ekey] && strcmp(nb->names[ekey], name) == 0) {
			found = true;
			break;
		}
	}

	if (found) {
		assert(ekey < 64 && ikey < 64);
		nb->ikey[ekey] = ikey;
		nb->ekey[ikey] = ekey;
	}

	return NB_ERR_OK;
}


static inline nb_err_t send_key(struct netbuf *nb, int ekey)
{
	nb_err_t err;
	int ikey;

	if (!known_ekey(nb, ekey))
		if ((err = introduce_ikey(nb, ekey)) != NB_ERR_OK)
			return err;

	ikey = nb->ikey[ekey];
	return cbor_encode_tag(nb->cs, ikey);
}


static inline nb_err_t send_item(struct netbuf *nb, int key, struct cbor_item item)
{
	nb_err_t err;
	if ((err = send_key(nb, key)) == NB_ERR_OK)
		return cbor_encode_item(nb->cs, &item);
	return err;
}


nb_err_t nb_send_bool(struct netbuf *nb, int key, bool b)
{
	return send_item(nb, key, (struct cbor_item) {
		.type = CBOR_TYPE_SVAL,
		.sval = b ? CBOR_SVAL_TRUE : CBOR_SVAL_FALSE
	});
}


nb_err_t nb_send_int(struct netbuf *nb, int key, int64_t i64)
{
	return send_item(nb, key, (struct cbor_item) {
		.type = CBOR_TYPE_INT,
		.i64 = i64
	});
}


nb_err_t nb_send_uint(struct netbuf *nb, int key, uint64_t u64)
{
	return send_item(nb, key, (struct cbor_item) {
		.type = CBOR_TYPE_UINT,
		.u64 = u64
	});
}


nb_err_t nb_send_string(struct netbuf *nb, int key, char *str)
{
	return send_item(nb, key, (struct cbor_item) {
		.type = CBOR_TYPE_TEXT,
		.len = str != NULL ? strlen(str) : 0,
		.str = str
	});
}


nb_err_t nb_send_array_begin(struct netbuf *nb, int key, size_t size)
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


nb_err_t nb_send_map_begin(struct netbuf *nb, int key, size_t size)
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


nb_err_t nb_send_group_begin(struct netbuf *nb, int key)
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

again:
	if (!nb->cur_key_valid) {
		if ((err = cbor_decode_tag(nb->cs, &nb->cur_key)) != NB_ERR_OK)
			return err;
		nb->cur_key_valid = true;
	}

	assert(nb->cur_key_valid);

	*key = nb->cur_key;

	/* TODO this may not be the best place to do this (but it works) */
	if (!nb->disable_keys && *key == NB_KEY) {
		nb->disable_keys = true;
		recv_ikey(nb);
		nb->disable_keys = false;
		goto again;
	}

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


static nb_err_t seek_item(struct netbuf *nb, int key)
{
	nb_key_t cur_key;
	nb_err_t err;
	struct cbor_item foo;

	while ((err = recv_key(nb, &cur_key)) == NB_ERR_OK) {
		if (cur_key == key)
			break;

		if (!known_ikey(nb, cur_key))
			cbor_decode_item(nb->cs, &foo);
	}

	return err;
}

nb_err_t nb_recv_item(struct netbuf *nb, nb_key_t *key, struct cbor_item *item)
{
	nb_err_t err;

	if ((err = recv_key(nb, key)) != NB_ERR_OK)
		return err;

	return cbor_decode_item(nb->cs, item);
}


nb_err_t nb_peek(struct netbuf *nb, nb_key_t *key)
{
	return peek_key(nb, key);
}


nb_err_t nb_recv_bool(struct netbuf *nb, int key, bool *b)
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


nb_err_t nb_recv_u32(struct netbuf *nb, int key, uint32_t *u32)
{
	nb_err_t err;
	if ((err = seek_item(nb, key)) == NB_ERR_OK)
		return cbor_decode_uint32(nb->cs, u32);
	return err;
}


nb_err_t nb_recv_u64(struct netbuf *nb, int key, uint64_t *u64)
{
	nb_err_t err;
	if ((err = seek_item(nb, key)) == NB_ERR_OK)
		return cbor_decode_uint64(nb->cs, u64);
	return err;
}


nb_err_t nb_recv_i32(struct netbuf *nb, int key, int32_t *i32)
{
	nb_err_t err;
	if ((err = seek_item(nb, key)) == NB_ERR_OK)
		return cbor_decode_int32(nb->cs, i32);
	return err;
}


nb_err_t nb_recv_string(struct netbuf *nb, int key, char **str)
{
	nb_err_t err;
	size_t foo;
	if ((err = seek_item(nb, key)) == NB_ERR_OK)
		cbor_decode_text(nb->cs, (byte_t **)str, &foo);
	return err;
}


nb_err_t nb_recv_array_begin(struct netbuf *nb, int key, size_t *size)
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


nb_err_t nb_recv_map_begin(struct netbuf *nb, int key, size_t *size)
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


nb_err_t nb_recv_group_begin(struct netbuf *nb, int key)
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
