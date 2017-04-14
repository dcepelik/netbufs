#include "memory.h"
#include "stack.h"
#include <assert.h>


static bool stack_resize(struct stack *stack, size_t new_size)
{
	void *new_mry;

	new_mry = cbor_realloc(stack->items, new_size * stack->item_size);
	if (!new_mry)
		return false;

	stack->items = new_mry;
	stack->size = new_size;

	return true;
}


bool stack_init(struct stack *stack, size_t init_stack_size, size_t item_size)
{
	stack->items = NULL;
	stack->size = 0;
	stack->item_size = item_size;
	stack->num_items = 0;

	return stack_resize(stack, init_stack_size);
}


void *stack_push(struct stack *stack)
{
	if (stack->num_items >= stack->size)
		if (!stack_resize(stack, 2 * stack->size))
			return NULL;

	assert(stack->num_items < stack->size);
	return stack->items + (stack->num_items++ * stack->item_size);
}


void *stack_pop(struct stack *stack)
{
	assert(stack->num_items > 0);
	return stack->items + ((--stack->num_items) * stack->item_size);
}


void *stack_top(struct stack *stack)
{
	assert(stack->num_items > 0);

	return stack->items + (stack->num_items * stack->item_size);
}


bool stack_is_empty(struct stack *stack)
{
	return (stack->num_items == 0);
}
