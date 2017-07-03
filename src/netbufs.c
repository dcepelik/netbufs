#include "array.h"
#include "memory.h"
#include "netbufs.h"
#include "string.h"
#include <assert.h>


#define NB_GROUPS_INIT_SIZE	4
#define NB_ATTRS_INIT_SIZE	4


typedef uint64_t nb_pid_t;


void nb_init(struct nb *nb, struct nb_buf *buf)
{
	nb->cs = cbor_stream_new(buf);

	/* TODO clean-up */
	diag_init(&nb->diag, nb->cs, stderr);
	cbor_stream_set_diag(nb->cs, &nb->diag);

	nb->groups = array_new(NB_GROUPS_INIT_SIZE, sizeof(*nb->groups));
}


void nb_free(struct nb *nb)
{
	cbor_stream_delete(nb->cs);
	array_delete(nb->groups);
}


static void init_group(struct nb_group *group, const char *name)
{
}


struct nb_group *nb_group(struct nb *nb, nb_lid_t id, const char *name)
{
	assert(id >= 0);

	struct nb_group *group;

	group = nb_malloc(sizeof(*group));
	group->name = name; /* TODO strdup if no longer const */
	group->attrs = array_new(NB_ATTRS_INIT_SIZE, sizeof(*group->attrs));

	nb->groups = array_ensure_index(nb->groups, id);
	return nb->groups[id] = group;
}


void nb_bind(struct nb_group *group, nb_lid_t id, const char *name, bool reqd)
{
	assert(id >= 0);

	struct nb_attr *attr;

	attr = nb_malloc(sizeof(*attr));
	attr->name = name;
	attr->reqd = reqd;

	group->attrs = array_ensure_index(group->attrs, id);
	group->attrs[id] = attr;
}
