#include "array.h"
#include "debug.h"
#include "memory.h"
#include "netbufs-internal.h"
#include "netbufs.h"
#include "string.h"

#include <assert.h>
#include <stdarg.h>


#define NB_GROUPS_INIT_SIZE	4
#define NB_ATTRS_INIT_SIZE	4
#define NB_ERR_MSG_INIT_LEN	4


static void init_group(struct nb *nb, struct nb_group *group, const char *name)
{
	group->name = name; /* TODO strdup if no longer const */
	group->attrs = array_new(NB_ATTRS_INIT_SIZE, sizeof(*group->attrs));
	group->max_pid = 0;
	group->pid_to_lid = array_new(NB_GROUPS_INIT_SIZE, sizeof(*group->pid_to_lid));
}


static void handle_cbor_error(struct cbor_stream *cs, nb_err_t err, void *arg)
{
	struct nb *nb = (struct nb *)arg;
	nb_error(nb, err, "error while encoding or decoding CBOR: %s",
		cbor_stream_strerror(cs));

	/*
	 * nb_error calls NetBufs' error handler, which should take care
	 * that the stream isn't processed any further; effectively, this
	 * function shall never return
	 */
}


void nb_init(struct nb *nb, struct nb_buf *buf)
{
	nb->cs = cbor_stream_new(buf);
	cbor_stream_set_error_handler(nb->cs, handle_cbor_error, nb);

	/* TODO clean-up */
	diag_init(&nb->diag, stderr);
	nb->diag.enabled = true;
	nb->active_group = NULL;
	cbor_stream_set_diag(nb->cs, &nb->diag);

	init_group(nb, &nb->groups_ns, NULL);
	strbuf_init(&nb->err_msg, NB_ERR_MSG_INIT_LEN);
	nb->err = NB_ERR_OK;
	nb->err_handler = nb_default_err_handler;
	nb->err_arg = NULL;

	nb->groups = array_new(NB_GROUPS_INIT_SIZE, sizeof(*nb->groups));
}


void nb_set_err_handler(struct nb *nb, nb_err_handler_t *handler, void *arg)
{
	nb->err_handler = handler;
	nb->err_arg = arg;
}


char *nb_strerror(struct nb *nb)
{
	return strbuf_get_string(&nb->err_msg);
}


void nb_default_err_handler(struct nb *nb, nb_err_t err, void *arg)
{
	(void) arg;
	fprintf(stderr, "%s: NetBufs error: #%i: %s.\n", __func__, err,
		nb_strerror(nb));
	exit(EXIT_FAILURE);
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


nb_err_t nb_error(struct nb *nb, nb_err_t err, char *msg, ...)
{
	assert(err != NB_ERR_OK);

	va_list args;
	va_start(args, msg);
	strbuf_reset(&nb->err_msg);
	strbuf_vprintf_at(&nb->err_msg, 0, msg, args);
	va_end(args);

	nb->err_handler(nb, err, nb->err_arg);

	return err;
}
