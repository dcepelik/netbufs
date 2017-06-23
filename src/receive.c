#include "array.h"
#include "cbor.h"
#include "debug.h"
#include "netbufs.h"


bool nb_recv_id(struct nb *nb, nb_lid_t *id)
{
	nb_err_t err;

	if ((err = cbor_decode_int32(nb->cs, id)) == NB_ERR_OK)
		return true;
	TEMP_ASSERT(err == NB_ERR_BREAK);
	return false;
}


void nb_recv_group(struct nb *nb, nb_lid_t id_expected)
{
	nb_lid_t id_real;
	nb_recv_id(nb, &id_real);
	TEMP_ASSERT(id_expected == id_real);
	cbor_decode_array_begin_indef(nb->cs);
}


void nb_recv_i8(struct nb *nb, int8_t *i8)
{
	cbor_decode_int8(nb->cs, i8);
}


void nb_recv_i16(struct nb *nb, int16_t *i16)
{
	cbor_decode_int16(nb->cs, i16);
}


void nb_recv_i32(struct nb *nb, int32_t *i32)
{
	cbor_decode_int32(nb->cs, i32);
}


void nb_recv_i64(struct nb *nb, int64_t *i64)
{
	cbor_decode_int64(nb->cs, i64);
}


void nb_recv_u8(struct nb *nb, uint8_t *u8)
{
	cbor_decode_uint8(nb->cs, u8);
}


void nb_recv_u16(struct nb *nb, uint16_t *u16)
{
	cbor_decode_uint16(nb->cs, u16);
}


void nb_recv_u32(struct nb *nb, uint32_t *u32)
{
	cbor_decode_uint32(nb->cs, u32);
}


void nb_recv_u64(struct nb *nb, uint64_t *u64)
{
	cbor_decode_uint64(nb->cs, u64);
}


void nb_recv_bool(struct nb *nb, bool *b)
{
	enum cbor_sval sval;
	cbor_decode_sval(nb->cs, &sval);
	*b = (sval == CBOR_SVAL_TRUE);
}


void nb_recv_string(struct nb *nb, char **str)
{
	size_t len;
	cbor_decode_text(nb->cs, (byte_t **)str, &len);
}


void nb_recv_array_end(struct nb *nb)
{
	cbor_decode_array_end(nb->cs);
}


nb_err_t nb_recv_group_end(struct nb *nb)
{
	return cbor_decode_array_end(nb->cs);
}
