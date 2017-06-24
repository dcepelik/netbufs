#include "cbor.h"


const char *cbor_type_string(enum cbor_type type)
{
	switch (type) {
	case CBOR_TYPE_UINT:
		return "unsigned integer";
	case CBOR_TYPE_INT:
		return "integer";
	case CBOR_TYPE_BYTES:
		return "byte string";
	case CBOR_TYPE_TEXT:
		return "text string";
	case CBOR_TYPE_ARRAY:
		return "array";
	case CBOR_TYPE_MAP:
		return "map";
	case CBOR_TYPE_TAG:
		return "tag";
	case CBOR_TYPE_SVAL:
		return "simple value";
	case CBOR_TYPE_FLOAT16:
		return "16-bit float";
	case CBOR_TYPE_FLOAT32:
		return "32-bit float";
	case CBOR_TYPE_FLOAT64:
		return "64-bit float";
	}

	return NULL;
}
