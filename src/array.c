#include "array.h"
#include "common.h"
#include "debug.h"
#include "memory.h"
#include "util.h"

#include <assert.h>
#include <string.h>

struct array_header *array_get_header(void *arr);
extern size_t array_size(void *arr);

/*
 * TODO Alignment 8-multiples
 */
static void *resize(void *arr, size_t new_capacity, size_t item_size)
{
	size_t actual_size;
	struct array_header *header = NULL;
	nb_byte_t *new_mem_start;
	size_t old_capacity = 0;

	if (arr) {
		header = array_get_header(arr);
		old_capacity = header->capacity;
	}

	assert(new_capacity >= old_capacity);

	actual_size = sizeof(struct array_header) + new_capacity * item_size;

	header = nb_realloc(header, actual_size);
	new_mem_start = (nb_byte_t *)(header + 1) + old_capacity * item_size;

	memset(new_mem_start, 0, (new_capacity - old_capacity) * item_size);

	header->item_size = item_size;
	header->capacity = new_capacity; 
	return header + 1;
}


void *array_new(size_t init_capacity, size_t item_size)
{
	void *arr = resize(NULL, init_capacity, item_size);
	array_get_header(arr)->num_items = 0;
	return arr;
}


void *array_new_size(size_t num_items, size_t item_size)
{
	return array_push(array_new(num_items, item_size), num_items);
}


void *array_push(void *arr, size_t num_items)
{
	struct array_header *header;
	size_t capacity_reqd;

	header = array_get_header(arr);
	capacity_reqd = header->num_items + num_items;
	if (capacity_reqd > header->capacity) {
		arr = resize(arr, MAX(capacity_reqd, 2 * header->capacity), header->item_size);
		header = array_get_header(arr);
	}

	header->num_items += num_items;
	return arr;
}


void *array_ensure_index(void *arr, size_t index)
{
	struct array_header *hdr = array_get_header(arr);
	size_t nitems_after;

	nitems_after = index + 1;
	hdr->num_items = MAX(hdr->num_items, nitems_after);

	if (hdr->capacity >= nitems_after)
		return arr;
	return resize(arr, MAX(2 * hdr->capacity, nitems_after), hdr->item_size);
}


void array_reset(void *arr)
{
	array_get_header(arr)->num_items = 0;
}


void array_delete(void *arr)
{
	free(array_get_header(arr));
}


void *array_last(void *arr)
{
	struct array_header *hdr = array_get_header(arr);
	assert(hdr->num_items > 0);
	return ((nb_byte_t *)arr) + (hdr->num_items - 1) * hdr->item_size;
}
