#include "buf.h"
#include "error.h"
#include <stdlib.h>

#ifndef NETBUF_H
#define NETBUF_H

struct ns;

enum nb_type
{
	NB_TYPE_STRING = 1,
	NB_TYPE_BOOL,
	NB_TYPE_INT32,
	NB_TYPE_UINT32,
	NB_TYPE_ARRAY,
	NB_TYPE_STRUCT,
	NB_TYPE_UNION,
	NB_TYPE_NS,
};

struct nb_node
{
	char *name;
	char *part;
	enum nb_type type;
	size_t offset;		/* offset within a struct or union */
	void *item_name;	/* arrays only: the name of the contained type */
	void *switch_name;	/* TODO */
	size_t size;		/* struct size */
	struct nb_node *children;
};

struct netbuf
{
	struct cbor_stream *cs;
	struct nb_node root_ns;	/* TODO maybe I could get rid of this */
};

nb_err_t nb_init(struct netbuf *nb, struct nb_buf *buf);
//void nb_free(struct netbuf *nb);

void nb_bind_string(struct netbuf *nb, size_t offset, char *name);
void nb_bind_bool(struct netbuf *nb, size_t offset, char *name);
void nb_bind_int32(struct netbuf *nb, size_t offset, char *name);
void nb_bind_uint32(struct netbuf *nb, size_t offset, char *name);
void nb_bind_struct(struct netbuf *nb, size_t offset, size_t size, char *name);
void nb_bind_array(struct netbuf *nb, size_t offset, char *name, char *item_name);
void nb_bind_union(struct netbuf *nb, size_t offset, size_t size, char *name, char *switch_name);

void nb_send(struct netbuf *nb, void *displace, char *name);

#endif
