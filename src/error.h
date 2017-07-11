#ifndef ERROR_H
#define ERROR_H

/*
 * CBOR operation result flags
 */
enum nb_err
{
	NB_ERR_OK,		/* no error */
	NB_ERR_NOMEM,		/* unable to allocate memory */
	NB_ERR_PARSE,		/* unable to parse input */
	NB_ERR_ITEM,		/* unexpected item */
	NB_ERR_EOF,		/* unexpected EOF */
	NB_ERR_RANGE,		/* value is out of range */
	NB_ERR_OPER,		/* invalid operation */
	NB_ERR_NOFILE,		/* file not found */
	NB_ERR_INDEF,		/* (in)definite-length item was unexpected */
	NB_ERR_READ,		/* read()-related error */
	NB_ERR_UNSUP,		/* operation not supported */
	NB_ERR_NOMORE,		/* no more items */
	NB_ERR_BREAK,		/* break was hit */
	NB_ERR_NITEMS,		/* invalid number of items */
	NB_ERR_OPEN,		/* open()-related error */
	NB_ERR_UNDEF_ID,	/* an ID was used prior to being defined */
	NB_ERR_OTHER,		/* other error occured */
};

typedef enum nb_err nb_err_t;

#endif
