#include "array.h"
#include "cbor.h"
#include "debug.h"
#include "memory.h"
#include "netbufs.h"
#include "strbuf.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>


nb_err_t nb_init(struct netbuf *nb, struct nb_buf *buf)
{
	nb->cs = cbor_stream_new(buf);
	nb->root_ns.type = NB_TYPE_NS;
	nb->root_ns.children = array_new(2, sizeof(*nb->root_ns.children));

	return NB_ERR_OK;
}


static struct nb_node *node_find(struct nb_node *nodes, char *part, size_t len)
{
	size_t j;

	for (j = 0; j < array_size(nodes); j++) {
		if (strlen(nodes[j].part) == len && strncmp(nodes[j].part, part, len) == 0)
			return &nodes[j];
	}
	return NULL;
}


/*
 * TODO introduce some (basic) hashing to avoid comparing strings this much
 */
static struct nb_node *node_new(struct netbuf *nb, char *name, bool create)
{
	struct nb_node *parent = &nb->root_ns;
	struct nb_node *new_parent;
	char *part;
	bool end;
	size_t i;

	part = name;
	while (part[0] != '\0') {
		for (i = 0; part[i] != '\0' && part[i] != '/'; i++) ;
		end = (part[i] == '\0');

		if (i == 0) {
			/* TODO two "//" in a row in the part, error */
			assert(false);
		}

		new_parent = node_find(parent->children, part, i);
		if (!new_parent) {
			if (!create)
				return NULL;

			parent->children = array_push(parent->children, 1);
			new_parent = array_last(parent->children);
			new_parent->name = name;
			new_parent->part = strndup(part, i);
			new_parent->type = NB_TYPE_NS;
			new_parent->children = array_new(2, sizeof(*new_parent->children));
		}

		parent = new_parent;

		if (end)
			return parent;
		part = part + (i + 1);
	}

	return NULL;
}


static struct nb_node *push_node(struct netbuf *nb, size_t offset, size_t size, enum nb_type type, char *name)
{
	struct nb_node *node;

	node = node_new(nb, name, true);
	assert(node != NULL);

	node->type = type;
	node->name = name;
	node->offset = offset;
	node->size = size;
	return node;
}


static struct nb_node *search_node(struct netbuf *nb, char *name)
{
	return node_new(nb, name, false);
}


static struct nb_node *children_of(struct netbuf *nb, char *name)
{
	struct nb_node *parent;
	parent = node_new(nb, name, false);
	if (parent)
		return parent->children;
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
	struct nb_node *members;
	byte_t *bptr;
	size_t i;

	assert(switch_val >= 0);
	members = children_of(nb, unn->name);
	assert(members != NULL);

	/* TODO loop is pointless */
	for (i = 0; i < array_size(members); i++) {
		if (i == switch_val) {
			bptr = ptr + members[i].offset;
			nb_send(nb, bptr, members[i].name);
			break;
		}
	}
}


static void send_struct(struct netbuf *nb, struct nb_node *strct, byte_t *ptr)
{
	struct nb_node *members;
	byte_t *bptr;
	char *nsname;
	int32_t switch_val;
	size_t i;
	size_t len;

	members = children_of(nb, strct->name);
	assert(members != NULL);

	cbor_encode_map_begin_indef(nb->cs);
	for (i = 0; i < array_size(members); i++) {
		assert(members[i].offset >= 0);
		bptr = ptr + members[i].offset;

		/* TODO This conditional is a dirty hack, get rid of it */
		if (members[i].type != NB_TYPE_UNION) {
			nb_send(nb, bptr, members[i].name);
		}
		else {
			struct nb_node *switch_node;
			switch_node = search_node(nb, members[i].switch_name);
			/* assert(switch_node is a sibling in this struct) */
			/* assert(switch_node's type is NB_TYPE_INT32) */
			
			switch_val = *((int32_t *)(ptr + switch_node->offset));
			send_union(nb, &members[i], bptr, switch_val);
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
