#include "error.h"
#include <stdlib.h>


const char *cbor_err_to_string(cbor_err_t err)
{
	switch (err)
	{
	case CBOR_ERR_OK:
		return "Success.";
	case CBOR_ERR_NOMEM:
		return "Out of memory.";
	case CBOR_ERR_PARSE:
		return "Parse error.";
	case CBOR_ERR_ITEM:
		return "An item was decoded that was unexpected here.";
	case CBOR_ERR_EOF:
		return "End-of-file was reached.";
	case CBOR_ERR_RANGE:
		return "Value is out of range.";
	case CBOR_ERR_OPER:
		return "Operation is invalid (at the moment).";
	case CBOR_ERR_NOFILE:
		return "File not found or open() failed.";
	case CBOR_ERR_INDEF:
		return "Indefinite length item is not allowed here.";
	case CBOR_ERR_READ:
		return "An error occured during read().";
	case CBOR_ERR_UNSUP:
		return "Operation not supported or feature not implemented.";
	case CBOR_ERR_NOMORE:
		return "No more items.";
	}

	return NULL;
}
