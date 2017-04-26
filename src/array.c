#include "array.h"
#include "memory.h"
#include "util.h"


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


static void *resize(void *arr, size_t new_capacity, size_t item_size)
{
	size_t actual_size;
	struct array_header *header = NULL;

	if (arr)
		header = array_get_header(arr);

	actual_size = sizeof(struct array_header) + new_capacity * item_size;

	header = nb_realloc(header, actual_size);
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


size_t array_size(void *arr)
{
	return array_get_header(arr)->num_items;
}


void array_reset(void *arr)
{
	array_get_header(arr)->num_items = 0;
}


void array_delete(void *arr)
{
	free(array_get_header(arr));
}
