#include "cbor.h"
#include "netbufs.h"
#include <assert.h>
#include <inttypes.h>
#include <string.h>


void nb_init(struct netbuf *nb, struct nb_buf *buf)
{
	nb->cs = cbor_stream_new(buf);
}


static nb_err_t nb_send_key(struct netbuf *nb, nb_key_t key)
{
	return (nb_err_t)cbor_encode_tag(nb->cs, key);
}


static nb_err_t recv_key(struct netbuf *nb, uint64_t *key)
{
	return (nb_err_t)cbor_decode_tag(nb->cs, key);
}


static nb_err_t check_key(struct netbuf *nb, nb_key_t key)
{
	uint64_t actual_key;
	assert(recv_key(nb, &actual_key) == NB_ERR_OK);
	assert(actual_key == key);
	return NB_ERR_OK;
}


void nb_send_uint(struct netbuf *nb, nb_key_t key, uint64_t u64)
{
	nb_send_key(nb, key);
	cbor_encode_uint64(nb->cs, u64);
}


void nb_send_int(struct netbuf *nb, nb_key_t key, int64_t i64)
{
	nb_send_key(nb, key);
	cbor_encode_int64(nb->cs, i64);
}


nb_err_t nb_recv_u32(struct netbuf *nb, nb_key_t key, uint32_t *u32)
{
	check_key(nb, key);
	cbor_decode_uint32(nb->cs, u32);
	return NB_ERR_OK;
}


void nb_map_begin(struct netbuf *nb, nb_key_t key)
{
	nb_send_key(nb, key);
	cbor_encode_map_begin_indef(nb->cs);
}


void nb_map_end(struct netbuf *nb)
{
	cbor_encode_map_end(nb->cs);
}


void nb_send_array_begin(struct netbuf *nb, nb_key_t key, uint64_t size)
{
	nb_send_key(nb, key);
	cbor_encode_array_begin(nb->cs, size);
}


void nb_send_array_end(struct netbuf *nb)
{
	cbor_encode_array_end(nb->cs);
}


void nb_send_bool(struct netbuf *nb, nb_key_t key, bool b)
{
	nb_send_key(nb, key);
	cbor_encode_sval(nb->cs, b ? CBOR_SVAL_TRUE : CBOR_SVAL_FALSE);
}


void nb_send_string(struct netbuf *nb, nb_key_t key, char *str)
{
	nb_send_key(nb, key);
	cbor_encode_text(nb->cs, (byte_t *)str, strlen(str));
}


void nb_send_ipv4(struct netbuf *nb, nb_key_t key, ipv4_t ip)
{
	nb_send_uint(nb, key, ip);
}


bool nb_end(struct netbuf *nb)
{
	return true;
}


nb_err_t nb_recv_ipv4(struct netbuf *nb, nb_key_t key, ipv4_t *ip)
{
	check_key(nb, key);
	nb_recv_u32(nb, key, (uint32_t *)ip);
	return NB_ERR_OK;
}


nb_err_t nb_recv_string(struct netbuf *nb, nb_key_t key, char **str)
{
	size_t tmp;
	check_key(nb, key);
	cbor_decode_text(nb->cs, (byte_t **)str, &tmp);
	return NB_ERR_OK;
}


nb_err_t nb_send_group_begin(struct netbuf *nb, nb_key_t key)
{
	cbor_err_t err;
	if ((err = cbor_encode_tag(nb->cs, key)) == CBOR_ERR_OK)
		return cbor_encode_array_begin_indef(nb->cs);
	return NB_ERR_OK;
}


nb_err_t nb_recv_group_begin(struct netbuf *nb, nb_key_t key)
{
	check_key(nb, key);
	cbor_decode_array_begin_indef(nb->cs);
	return NB_ERR_OK;
}


nb_err_t nb_send_group_end(struct netbuf *nb)
{
	cbor_encode_array_end(nb->cs);
	return NB_ERR_OK;
}


nb_err_t nb_recv_group_end(struct netbuf *nb)
{
	cbor_decode_array_end(nb->cs);
	return NB_ERR_OK;
}


nb_err_t nb_recv_array_begin(struct netbuf *nb, nb_key_t key, uint64_t *size)
{
	check_key(nb, key);
	cbor_decode_array_begin(nb->cs, size);
	return NB_ERR_OK;
}


nb_err_t nb_recv_array_end(struct netbuf *nb)
{
	(void) nb;
	/* TODO nothing to do here? */
	return NB_ERR_OK;
}
