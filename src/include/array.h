#ifndef ARRAY_H
#define ARRAY_H

#include <stdlib.h>

struct array_header
{
	size_t item_size;
	size_t num_items;
	size_t capacity;
};


static inline struct array_header *array_get_header(void *arr)
{
	return (struct array_header *)((unsigned char *)arr - sizeof(struct array_header));
}


static inline size_t array_size(void *arr)
{
	return array_get_header(arr)->num_items;
}


void *array_new(size_t init_capacity, size_t item_size);
void *array_new_size(size_t num_items, size_t item_size);
void *array_push(void *arr, size_t num_items);
size_t array_size(void *arr);
void *array_ensure_index(void *arr, size_t index);
void array_reset(void *arr);
void array_delete(void *arr);
void *array_last(void *arr);

#endif
