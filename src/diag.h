#ifndef DIAG_H
#define DIAG_H

void cbor_item_dump(struct cbor_item *item, FILE *file);
nb_err_t cbor_stream_dump(struct cbor_stream *cs, FILE *file);

#endif
