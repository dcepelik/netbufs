#ifndef STACK_H
#define STACK_H

#include "common.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

struct stack
{
	nb_byte_t *items;
	size_t size;
	size_t item_size;
	size_t num_items;
};

#define stack_foreach(stack, item) \
	for (item = stack_top(stack); (nb_byte_t *)item >= (stack)->items; item--)


static inline void *stack_top(struct stack *stack)
{
	assert(stack->num_items > 0);
	return stack->items + ((stack->num_items - 1) * stack->item_size);
}


bool stack_init(struct stack *stack, size_t init_stack_size, size_t item_size);
void *stack_push(struct stack *stack);
void *stack_pop(struct stack *stack);
bool stack_is_empty(struct stack *stack);
void stack_free(struct stack *stack);

#endif
