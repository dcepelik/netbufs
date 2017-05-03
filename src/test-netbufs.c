#include "array.h"
#include "netbufs.h"
#include <stdbool.h>
#include <stdlib.h>


struct toy
{
	char *name;
	bool big;
	bool children_safe;
};


static struct toy *toys;


int main(void)
{
	struct nb_buf *out;
	struct toy *toy0 = NULL;

	toys = array_new(3, sizeof(*toys));
	toys = array_push(toys, 3);
	toys[0] = (struct toy){ .name = "Teddy Bear", .big = false, .children_safe = true };
	toys[1] = (struct toy){ .name = "Big Teddy Bear", .big = true, .children_safe = true };
	toys[2] = (struct toy){ .name = "Bigger Teddy Bear", .big = true, .children_safe = false };

	out = nb_buf_new();
	nb_buf_open_stdout(out);

	struct netbuf nb;
	nb_init(&nb, out);

	nb_bind_struct(&nb, 0, sizeof(struct toy), "example.org/toy");
	nb_bind_string(&nb, (size_t)&toy0->name, "example.org/toy/name");
	nb_bind_bool(&nb, (size_t)&toy0->big, "example.org/toy/big");
	nb_bind_bool(&nb, (size_t)&toy0->children_safe, "example.org/toy/children_safe");

	nb_bind_array(&nb, 0, "example.org/ts", "example.org/toy");

	nb_send(&nb, toys, "example.org/ts");
	nb_buf_flush(out);

	return EXIT_SUCCESS;
}
