#include "array.h"
#include "cbor.h"
#include "debug.h"
#include "netbufs.h"
#include "strbuf.h"
#include <string.h>
#include <assert.h>


nb_err_t nb_init(struct netbuf *nb, struct nb_buf *buf)
{
	nb->nodes = array_new(4, sizeof(*nb->nodes));
	nb->cs = cbor_stream_new(buf);
	return NB_ERR_OK;
}


struct nb_node *push_node(struct netbuf *nb, size_t offset, size_t size, enum nb_type type, char *name)
{
	struct nb_node *node;
	size_t len;

	len = array_size(nb->nodes);
	nb->nodes = array_push(nb->nodes, 1);

	node = &nb->nodes[len];
	node->type = type;
	node->name = name;
	node->offset = offset;
	node->size = size;

	return node;
}


struct nb_node *search_node(struct netbuf *nb, char *name)
{
	size_t i;

	for (i = 0; i < array_size(nb->nodes); i++)
		if (strcmp(nb->nodes[i].name, name) == 0)
			return &nb->nodes[i];

	return NULL;
}


void nb_bind_string(struct netbuf *nb, size_t offset, char *name)
{
	push_node(nb, offset, 8, NB_TYPE_STRING, name);
}


void nb_bind_bool(struct netbuf *nb, size_t offset, char *name)
{
	push_node(nb, offset, 1, NB_TYPE_BOOL, name);
}


void nb_bind_struct(struct netbuf *nb, size_t offset, size_t size, char *name)
{
	push_node(nb, offset, size, NB_TYPE_STRUCT, name);
}


void nb_bind_array(struct netbuf *nb, size_t offset, char *name, char *item_name)
{
	struct nb_node *node;

	node = push_node(nb, offset, 0, NB_TYPE_ARRAY, name);
	node->item_name = item_name;
}


void send_array(struct netbuf *nb, struct nb_node *node, byte_t *ptr)
{
	size_t i;
	struct nb_node *item_node;

	item_node = search_node(nb, node->item_name);
	assert(item_node != NULL);

	for (i = 0; i < array_size(ptr); i++) {
		nb_send(nb, ptr + i * item_node->size, node->item_name);
	}
}


void send_struct(struct netbuf *nb, struct nb_node *strct, byte_t *ptr)
{
	size_t i;
	for (i = 0; i < array_size(nb->nodes); i++) {
		if (strncmp(strct->name, nb->nodes[i].name, strlen(strct->name)) == 0) {
			if (&nb->nodes[i] != strct)
				nb_send(nb, ptr, nb->nodes[i].name);
		}
	}
}


void nb_send(struct netbuf *nb, void *ptr, char *name)
{
	struct nb_node *node;
	byte_t *bptr;

	node = search_node(nb, name);
	assert(node != NULL);

	bptr = (byte_t *)ptr + node->offset;

	switch (node->type) {
	case NB_TYPE_STRING:
		cbor_encode_text(nb->cs, *(byte_t **)bptr, strlen(*(char **)bptr));
		break;
	case NB_TYPE_BOOL:
		cbor_encode_sval(nb->cs, *bptr ? CBOR_SVAL_TRUE : CBOR_SVAL_FALSE);
		break;
	case NB_TYPE_ARRAY:
		send_array(nb, node, bptr);
		break;
	case NB_TYPE_STRUCT:
		send_struct(nb, node, bptr);
		break;
	default:
		assert(0);
	}
}
