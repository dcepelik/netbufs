#include "internal.h"
#include "memory.h"


static cbor_err_t cbor_uint32_encode(struct cbor_encoder *enc, uint32_t val)
{
	if (val <= 23) {
		return cbor_stream_write(&enc->stream, (2 << 5) + val);
	}

	if (val <= 255) {
		return 
	}
}


cbor_err_t cbor_int_encode(struct cbor_encoder *enc, int val)
{
	if (val >= 0)
		return cbor_uint_encode(enc, val);
	else
		return cbor_negative_int_encode(enc, val);
}
