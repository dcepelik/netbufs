#include "array.h"
#include "debug.h"
#include <stdlib.h>

#define NUM_WRITES	2048

struct foo
{
	int i1;
	int i2;
};

int main(void)
{
	int *arr1;
	struct foo *arr2;
	size_t i;

	arr1 = array_new(4, sizeof(int));
	arr2 = array_new(16, sizeof(struct foo));

	for (i = 0; i < NUM_WRITES; i++) {
		arr1 = array_push(arr1, 1);
		arr1[i] = i;
	}

	for (i = 0; i < NUM_WRITES; i++)
		assert(arr1[i] == i);

	arr2 = array_push(arr2, NUM_WRITES);
	for (i = 0; i < NUM_WRITES; i++) {
		arr2[i].i1 = i;
		arr2[i].i2 = -i;
	}

	for (i = 0; i < NUM_WRITES; i++) {
		assert(arr2[i].i1 == i);
		assert(arr2[i].i2 == -i);
	}

	array_delete(arr1);
	array_delete(arr2);

	return EXIT_SUCCESS;
}
