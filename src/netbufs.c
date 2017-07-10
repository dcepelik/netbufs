#include "array.h"
#include "debug.h"
#include "memory.h"
#include "netbufs.h"
#include "string.h"

#include <assert.h>


#define NB_GROUPS_INIT_SIZE	4
#define NB_ATTRS_INIT_SIZE	4


static void init_group(struct nb *nb, struct nb_group *group, const char *name)
{
	group->name = name; /* TODO strdup if no longer const */
	group->attrs = array_new(NB_ATTRS_INIT_SIZE, sizeof(*group->attrs));
	group->max_pid = 0;
	group->pid_to_lid = array_new(NB_GROUPS_INIT_SIZE, sizeof(*group->pid_to_lid));
}


void nb_init(struct nb *nb, struct nb_buf *buf)
{
	nb->cs = cbor_stream_new(buf);

	/* TODO clean-up */
	diag_init(&nb->diag, nb->cs, stderr);
	nb->diag.enabled = true;
	nb->active_group = NULL;
	cbor_stream_set_diag(nb->cs, &nb->diag);
	init_group(nb, &nb->groups_ns, NULL);

	nb->groups = array_new(NB_GROUPS_INIT_SIZE, sizeof(*nb->groups));
}


void nb_free(struct nb *nb)
{
	cbor_stream_delete(nb->cs);
	array_delete(nb->groups);
}


struct nb_group *nb_group(struct nb *nb, nb_lid_t id, const char *name)
{
	assert(id >= 0);

	struct nb_group *group;
	group = nb_malloc(sizeof(*group));
	init_group(nb, group, name);

	nb->groups = array_ensure_index(nb->groups, id);
	nb_bind(&nb->groups_ns, id, name, false);
	return nb->groups[id] = group;
}


void nb_bind(struct nb_group *group, nb_lid_t id, const char *name, bool reqd)
{
	assert(id >= 0);

	struct nb_attr *attr;

	attr = nb_malloc(sizeof(*attr));
	attr->name = name;
	attr->reqd = reqd;
	attr->pid = 0;

	group->attrs = array_ensure_index(group->attrs, id);
	group->attrs[id] = attr;
}
