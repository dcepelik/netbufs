#ifndef ERROR_H
#define ERROR_H

enum nb_err
{
	NB_ERR_OK,
	NB_ERR_NOMEM,
	NB_ERR_OPEN,
	NB_ERR_READ,
	NB_ERR_EOF,
};

typedef enum nb_err	nb_err_t;

#endif
