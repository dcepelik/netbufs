#include "cbor.h"
#include "debug.h"
#include "internal.h"
#include <stdio.h>


const char *cbor_major_to_string(enum cbor_major major)
{
	switch (major) {
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


void cbor_item_dump(struct cbor_item *item)
{
	size_t i;

	switch (item->hdr.major) {
	case CBOR_MAJOR_UINT:
		fprintf(stderr, "%lu", item->hdr.u64);
		break;

	case CBOR_MAJOR_NEGATIVE_INT:
		fprintf(stderr, "%li", item->i64);
		break;

	case CBOR_MAJOR_BYTES:
		fputs("b\'", stderr);
		for (i = 0; i < item->len; i++)
			fprintf(stderr, "%02X", item->bytes[i]);
		fputc('\'', stderr);
		break;

	case CBOR_MAJOR_TEXT:
		fprintf(stderr, "\"%s\"", item->str);
		break;

	case CBOR_MAJOR_ARRAY:
		fprintf(stderr, "[");
		for (i = 0; i < item->len; i++) {
			if (i > 0)
				fputs(", ", stderr);
			cbor_item_dump(&item->items[i]);
		}
		fprintf(stderr, "]");
		break;

	case CBOR_MAJOR_MAP:
		fprintf(stderr, "{");
		for (i = 0; i < item->len; i++) {
			if (i > 0)
				fputs(", ", stderr);

			cbor_item_dump(&item->pairs[i].key);
			fputs(": ", stderr);
			cbor_item_dump(&item->pairs[i].value);
		}
		fprintf(stderr, "}");
		break;

	default:
		fprintf(stderr, "hdr: %s\n", cbor_major_to_string(item->hdr.major));
		TEMP_ASSERT(false);
	}
}
