#ifndef ERROR_H
#define ERROR_H

enum cbor_err
{
	CBOR_ERR_OK,		/* no error */
	CBOR_ERR_NOMEM,		/* unable to allocate memory */
	CBOR_ERR_PARSE,		/* unable to parse input */
	CBOR_ERR_ITEM,		/* unexpected item */
	CBOR_ERR_EOF,		/* unexpected EOF */
	CBOR_ERR_RANGE,		/* value is out of range */
};

typedef enum cbor_err cbor_err_t;

#endif
