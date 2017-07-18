#include "array.h"
#include "debug.h"
#include "netbufs-internal.h"
#include "netbufs.h"
#include "string.h"

#include <assert.h>
#include <stdarg.h>


#define NB_GROUPS_INIT_SIZE	4
#define NB_ATTRS_INIT_SIZE	4
#define NB_ERR_MSG_INIT_LEN	4
#define NB_MEMPOOL_BLOCK_SIZE	(16 * sizeof(struct nb_group))


static void init_group(struct nb *nb, struct nb_group *group, const char *name)
{
	group->name = name; /* TODO strdup if no longer const */
	group->attrs = array_new(NB_ATTRS_INIT_SIZE, sizeof(*group->attrs));
	group->max_pid = 1;
	group->pid_to_lid = array_new(NB_GROUPS_INIT_SIZE, sizeof(*group->pid_to_lid));
	group->pid_to_lid[0] = 0;
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


void nb_init(struct nb *nb, struct nb_buffer *buf)
{
	nb->mempool = mempool_new(NB_MEMPOOL_BLOCK_SIZE);

	cbor_stream_init(&nb->cs, buf);
	cbor_stream_set_error_handler(&nb->cs, handle_cbor_error, nb);

	/* TODO clean-up */
	diag_init(&nb->diag, stderr);
	diag_enable_col(&nb->diag, DIAG_COL_RAW);
	diag_enable_col(&nb->diag, DIAG_COL_ITEMS);
	diag_enable_col(&nb->diag, DIAG_COL_CBOR);
	diag_enable_col(&nb->diag, DIAG_COL_PROTO);
	nb->diag.enabled = true;

	nb->active_group = NULL;
	cbor_stream_set_diag(&nb->cs, &nb->diag);

	init_group(nb, &nb->groups_ns, NULL);
	strbuf_init(&nb->err_msg, NB_ERR_MSG_INIT_LEN);
	nb->err = NB_ERR_OK;
	nb->err_handler = nb_default_err_handler;
	nb->err_arg = NULL;

	nb->groups = array_new(NB_GROUPS_INIT_SIZE, sizeof(*nb->groups));
}


void nb_free(struct nb *nb)
{
	cbor_stream_free(&nb->cs);
	array_delete(nb->groups);
	mempool_delete(nb->mempool);
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


void nb_default_err_handler(struct nb *nb, nb_err_t err, void *arg)
{
	(void) arg;
	fprintf(stderr, "%s: NetBufs error: #%i: %s.\n", __func__, err,
		nb_strerror(nb));
	assert(false);
	exit(EXIT_FAILURE);
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


struct nb_group *nb_group(struct nb *nb, nb_lid_t id, const char *name)
{
	assert(id >= 0);

	struct nb_group *group;
	group = mempool_malloc(nb->mempool, sizeof(*group));
	init_group(nb, group, name);

	nb->groups = array_ensure_index(nb->groups, id);
	nb_bind(nb, &nb->groups_ns, id, name, false);
	return nb->groups[id] = group;
}


void nb_bind(struct nb *nb, struct nb_group *group, nb_lid_t id, const char *name, bool reqd)
{
	assert(id >= 0);

	struct nb_attr *attr;

	attr = mempool_malloc(nb->mempool, sizeof(*attr));
	attr->name = name;
	attr->reqd = reqd;
	attr->pid = 0;

	group->attrs = array_ensure_index(group->attrs, id);
	group->attrs[id] = attr;
}
