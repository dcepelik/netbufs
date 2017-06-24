#ifndef DIAG_H
#define DIAG_H

#include "strbuf.h"

struct diag
{
	struct cbor_stream *cs;
	struct strbuf strbuf;
	unsigned level;
	bool indent_next;

	/* TODO more options to come as needed */
	size_t str_dump_maxlen;
	size_t bytes_dump_maxlen;
};

void diag_init(struct diag *diag, struct cbor_stream *cs);
nb_err_t diag_dump(struct diag *diag, FILE *out);
void diag_free(struct diag *diag);

#endif
