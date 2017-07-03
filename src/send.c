#include "cbor.h"
#include "debug.h"
#include "netbufs.h"
#include "string.h"


void nb_send_id(struct nb *nb, nb_lid_t id)
{
	/* TODO */
	if (id >= 0)
		cbor_encode_int32(nb->cs, id);
}


void nb_send_group(struct nb *nb, nb_lid_t id)
{
	cbor_encode_array_begin_indef(nb->cs);
	nb_send_id(nb, id);
}


nb_err_t nb_send_group_end(struct nb *nb)
{
	return cbor_encode_array_end(nb->cs);
}


void nb_send_bool(struct nb *nb, nb_lid_t id, bool b)
{
	nb_send_id(nb, id);
	cbor_encode_sval(nb->cs, b ? CBOR_SVAL_TRUE : CBOR_SVAL_FALSE);
}

void nb_send_i8(struct nb *nb, nb_lid_t id, int8_t i8)
{
	nb_send_id(nb, id);
	cbor_encode_int8(nb->cs, i8);
}


void nb_send_i16(struct nb *nb, nb_lid_t id, int16_t i16)
{
	nb_send_id(nb, id);
	cbor_encode_int16(nb->cs, i16);
}


void nb_send_i32(struct nb *nb, nb_lid_t id, int32_t i32)
{
	nb_send_id(nb, id);
	cbor_encode_int32(nb->cs, i32);
}


void nb_send_i64(struct nb *nb, nb_lid_t id, int64_t i64)
{
	nb_send_id(nb, id);
	cbor_encode_int64(nb->cs, i64);
}


void nb_send_u8(struct nb *nb, nb_lid_t id, uint8_t u8)
{
	nb_send_id(nb, id);
	cbor_encode_uint8(nb->cs, u8);
}


void nb_send_u16(struct nb *nb, nb_lid_t id, uint16_t u16)
{
	nb_send_id(nb, id);
	cbor_encode_uint16(nb->cs, u16);
}


void nb_send_u32(struct nb *nb, nb_lid_t id, uint32_t u32)
{
	nb_send_id(nb, id);
	cbor_encode_uint32(nb->cs, u32);
}


void nb_send_u64(struct nb *nb, nb_lid_t id, uint64_t u64)
{
	nb_send_id(nb, id);
	cbor_encode_uint64(nb->cs, u64);
}


void nb_send_string(struct nb *nb, nb_lid_t id, char *str)
{
	nb_send_id(nb, id);
	if (str == NULL)
		str = "";
	cbor_encode_text(nb->cs, (nb_byte_t *)str, strlen(str));
}


void nb_send_array(struct nb *nb, nb_lid_t id, size_t size)
{
	nb_send_id(nb, id);
	cbor_encode_array_begin(nb->cs, size);
}


void nb_send_array_end(struct nb *nb)
{
	cbor_encode_array_end(nb->cs);
}
