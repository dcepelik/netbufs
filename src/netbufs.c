#include "cbor.h"
#include "netbufs.h"
#include <inttypes.h>
#include <string.h>


void nb_init(struct netbuf *nb, struct nb_buf *buf)
{
	nb->cs = cbor_stream_new(buf);
}


static void nb_send_key(struct netbuf *nb, int key)
{
	cbor_encode_int64(nb->cs, key);
}


void nb_send_uint(struct netbuf *nb, int key, uint64_t u64)
{
	nb_send_key(nb, key);
	cbor_encode_uint64(nb->cs, u64);
}


void nb_send_int(struct netbuf *nb, int key, int64_t i64)
{
	nb_send_key(nb, key);
	cbor_encode_int64(nb->cs, i64);
}


void nb_map_begin(struct netbuf *nb, int key)
{
	nb_send_key(nb, key);
	cbor_encode_map_begin_indef(nb->cs);
}


void nb_map_end(struct netbuf *nb)
{
	cbor_encode_map_end(nb->cs);
}


void nb_array_begin(struct netbuf *nb, int key)
{
	nb_send_key(nb, key);
	cbor_encode_array_begin_indef(nb->cs);
}


void nb_array_end(struct netbuf *nb)
{
	cbor_encode_array_end(nb->cs);
}


void nb_send_bool(struct netbuf *nb, int key, bool b)
{
	nb_send_key(nb, key);
	cbor_encode_sval(nb->cs, b ? CBOR_SVAL_TRUE : CBOR_SVAL_FALSE);
}


void nb_send_string(struct netbuf *nb, int key, char *str)
{
	nb_send_key(nb, key);
	cbor_encode_text(nb->cs, (byte_t *)str, strlen(str));
}


void nb_send_ipv4(struct netbuf *nb, int key, ipv4_t ip)
{
	nb_send_uint(nb, key, ip);
}
