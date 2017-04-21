#ifndef STACK_H
#define STACK_H

#include "cbor.h"
#include <stdbool.h>
#include <stdlib.h>

struct stack
{
	byte_t *items;
	size_t size;
	size_t item_size;
	size_t num_items;
};

bool stack_init(struct stack *stack, size_t init_stack_size, size_t item_size);
void *stack_push(struct stack *stack);
void *stack_pop(struct stack *stack);
void *stack_top(struct stack *stack);
bool stack_is_empty(struct stack *stack);

#endif
