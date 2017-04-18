#ifndef INTERNAL_H
#define INTERNAL_H

#include "cbor.h"
#include "stack.h"
#include "stream.h"
#include <stdlib.h>

typedef unsigned char		cbor_extra_t;

#define CBOR_MAJOR_MASK		0xE0
#define CBOR_EXTRA_MASK		0x1F

#define CBOR_BREAK		((CBOR_MAJOR_OTHER << 5) + 31)

#define CBOR_BLOCK_STACK_INIT_SIZE	4

struct errlist
{
	int i;
};

struct block
{
	struct cbor_type type;
	size_t num_items;
};

cbor_err_t stack_block_begin(struct stack *stack, enum cbor_major type, bool indef, uint64_t len);
cbor_err_t stack_block_end(struct stack *stack, enum cbor_major type, bool indef, uint64_t len);

struct cbor_encoder
{
	struct cbor_stream *stream;
	struct errlist errors;
	struct stack blocks;
	/* state encapsulation */
	/* various options */
};

struct cbor_decoder
{
	struct cbor_stream *stream;
	struct errlist errors;
	struct stack blocks;
	/* state encapsulation */
	/* various options */
	/* state encapsulation */
};

struct cbor_document
{
	struct cbor_item root;
};

#endif
