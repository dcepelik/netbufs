#include "array.h"
#include "cbor.h"
#include "debug.h"
#include "memory.h"
#include "netbufs.h"
#include "strbuf.h"
#include <assert.h>
#include <string.h>


nb_err_t nb_init(struct netbuf *nb, struct nb_buf *buf)
{
	nb->nodes = array_new(4, sizeof(*nb->nodes));
	nb->cs = cbor_stream_new(buf);
	return NB_ERR_OK;
}


static struct nb_node *push_node(struct netbuf *nb, size_t offset, size_t size, enum nb_type type, char *name)
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


static struct nb_node *search_node(struct netbuf *nb, char *name)
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


void nb_bind_int32(struct netbuf *nb, size_t offset, char *name)
{
	push_node(nb, offset, 4, NB_TYPE_INT32, name);
}


void nb_bind_uint32(struct netbuf *nb, size_t offset, char *name)
{
	push_node(nb, offset, 4, NB_TYPE_UINT32, name);
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


void nb_bind_union(struct netbuf *nb, size_t offset, size_t size, char *name, char *switch_name)
{
	struct nb_node *node;

	node = push_node(nb, offset, size, NB_TYPE_UNION, name);
	node->switch_name = switch_name;
}


static void send_array(struct netbuf *nb, struct nb_node *node, byte_t *ptr)
{
	size_t i;
	struct nb_node *item_node;
	byte_t **bptr = (byte_t **)ptr;

	item_node = search_node(nb, node->item_name);
	assert(item_node != NULL);

	cbor_encode_array_begin_indef(nb->cs);
	for (i = 0; i < array_size(*bptr); i++)
		nb_send(nb, *bptr + i * item_node->size, node->item_name);
	cbor_encode_array_end(nb->cs);
}


static char *get_ns_name(char *name)
{
	char *nsname;
	size_t len;

	len = strlen(name);
	nsname = nb_malloc(len + 2);
	strncpy(nsname, name, len);
	nsname[len] = '/';
	nsname[len + 1] = '\0';
	return nsname;
}


static void send_union(struct netbuf *nb, struct nb_node *unn, byte_t *ptr, int32_t switch_val)
{
	struct nb_node *target_node;
	char *nsname;
	byte_t *bptr;
	size_t i;
	size_t ord;
	size_t len;

	assert(switch_val >= 0);
	nsname = get_ns_name(unn->name);
	len = strlen(nsname);

	for (i = 0, ord = 0; i < array_size(nb->nodes); i++) {
		if (nb->nodes[i].name != unn->name && strncmp(nsname, nb->nodes[i].name, len) == 0) {
			if (&nb->nodes[i] == unn || strchr(nb->nodes[i].name + len, '/') != NULL)
				continue;

			if (ord == switch_val) {
				bptr = ptr + nb->nodes[i].offset;
				DEBUG_EXPR("%i", *((int32_t *)bptr));
				nb_send(nb, bptr, nb->nodes[i].name);
				break;
			}
			ord++;
		}
	}
}


static void send_struct(struct netbuf *nb, struct nb_node *strct, byte_t *ptr)
{
	byte_t *bptr;
	char *nsname;
	int32_t switch_val;
	size_t i;
	size_t len;

	nsname = get_ns_name(strct->name);
	len = strlen(nsname);

	cbor_encode_map_begin_indef(nb->cs);
	for (i = 0; i < array_size(nb->nodes); i++) {
		if (strncmp(nsname, nb->nodes[i].name, len) == 0) {
			/* avoid loops when struct name ends in '/', only send direct children of this struct */
			if (&nb->nodes[i] == strct || strchr(nb->nodes[i].name + len, '/') != NULL)
				continue;

			assert(nb->nodes[i].offset >= 0);
			bptr = ptr + nb->nodes[i].offset;

			/* TODO This conditional is a dirty hack, get rid of it */
			if (nb->nodes[i].type != NB_TYPE_UNION) {
				nb_send(nb, bptr, nb->nodes[i].name);
			}
			else {
				struct nb_node *switch_node;
				switch_node = search_node(nb, nb->nodes[i].switch_name);
				/* assert(switch_node is a sibling in this struct) */
				/* assert(switch_node's type is NB_TYPE_INT32) */
				
				switch_val = *((int32_t *)(ptr + switch_node->offset));
				send_union(nb, &nb->nodes[i], bptr, switch_val);
			}
		}
	}
	cbor_encode_map_end(nb->cs);
}


void nb_send(struct netbuf *nb, void *ptr, char *name)
{
	struct nb_node *node;
	byte_t *bptr = (byte_t *)ptr;

	node = search_node(nb, name);
	assert(node != NULL);

	cbor_encode_text(nb->cs, (byte_t *)name, strlen(name));

	switch (node->type) {
	case NB_TYPE_STRING:
		cbor_encode_text(nb->cs, *(byte_t **)bptr, strlen(*(char **)bptr));
		break;
	case NB_TYPE_BOOL:
		cbor_encode_sval(nb->cs, *bptr ? CBOR_SVAL_TRUE : CBOR_SVAL_FALSE);
		break;
	case NB_TYPE_INT32:
		//DEBUG_PRINTF("INT32 %s", name);
		//DEBUG_EXPR("%i", *(int32_t *)bptr);
		cbor_encode_int32(nb->cs, *(int32_t *)bptr);
		break;
	case NB_TYPE_UINT32:
		cbor_encode_uint32(nb->cs, *(uint32_t *)bptr);
		break;
	case NB_TYPE_ARRAY:
		send_array(nb, node, bptr);
		break;
	case NB_TYPE_STRUCT:
		send_struct(nb, node, bptr);
		break;
	case NB_TYPE_UNION:
		assert(0);
		break;
	default:
		assert(0);
	}
}
