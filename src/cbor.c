#include "cbor.h"
#include "internal.h"


const char *cbor_type_to_string(struct cbor_type type)
{
	switch (type.major) {
	case CBOR_MAJOR_UINT:
		return "unsigned int";
	case CBOR_MAJOR_NEGATIVE_INT:
		return "negative int";
	case CBOR_MAJOR_BYTES:
		return "byte string";
	case CBOR_MAJOR_TEXT:
		return "text string";
	case CBOR_MAJOR_MAP:
		return "map";
	case CBOR_MAJOR_ARRAY:
		return "array";
	case CBOR_MAJOR_TAG:
		return "tag";
	case CBOR_MAJOR_OTHER:
		return "other";
	default:
		return "<invalid>";
	}
}
