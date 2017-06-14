#ifndef ARRAY_H
#define ARRAY_H

#include <stdlib.h>

void *array_new(size_t num_items, size_t item_size);
void *array_push(void *arr, size_t num_items);
size_t array_size(void *arr);
void *array_ensure_size(void *arr, size_t size);
void array_reset(void *arr);
void array_delete(void *arr);
void *array_last(void *arr);

#endif
