#include "array.h"
#include "cbor-internal.h"
#include "cbor.h"
#include "debug.h"
#include "netbufs.h"
#include "string.h"


static void send_pid(struct nb *nb, nb_pid_t pid)
{
	cbor_encode_uint64(&nb->cs, pid);
}


static struct nb_attr *group_get_attr(struct nb *nb, struct nb_group *group, nb_lid_t lid)
{
	if (array_size(group->attrs) < lid)
		return NULL;
	return group->attrs[lid];
}


/* TODO deduplicate code */
static void send_ikg(struct nb *nb, char *name, nb_pid_t pid)
{
	//assert(nb->active_group != NULL);
	cbor_encode_text(&nb->cs, name);
	send_pid(nb, pid);
}


static nb_pid_t lid_to_pid(struct nb *nb, struct nb_group *group, nb_lid_t lid)
{
	struct nb_attr *attr;

	attr = group_get_attr(nb, group, lid);

	if (!attr)
		DEBUG_PRINTF("It seems that attribute %i isn't bound to group %s",
			lid, group->name);
	TEMP_ASSERT(attr != NULL);

	if (attr->pid == 0) {
		attr->pid = ++group->max_pid;
		send_ikg(nb, (char *)attr->name, attr->pid);
	}

	assert(attr->pid != 0);	/* pID 0 is reserved */
	return attr->pid;
}


static struct nb_group *get_group(struct nb *nb, nb_lid_t lid)
{
	struct nb_group *group;
	if (array_size(nb->groups) < lid)
		return NULL;
	return nb->groups[lid];
}


void nb_send_id(struct nb *nb, nb_lid_t id)
{
	nb_pid_t pid;

	if (id < 0)
		return; /* TODO */

	TEMP_ASSERT(nb->active_group != NULL); /* TODO make it an assert */

	pid = lid_to_pid(nb, nb->active_group, id);
	send_pid(nb, pid);
}


void nb_send_group(struct nb *nb, nb_lid_t id)
{
	struct nb_group *group;
	nb_pid_t pid;

	group = get_group(nb, id);
	if (!group)
		DEBUG_PRINTF("Cannot get group (lid=%i)", id);
	TEMP_ASSERT(group != NULL);

	cbor_encode_map_begin_indef(&nb->cs);
	nb->active_group = group;
	top_block(&nb->cs)->group = nb->active_group;

	pid = lid_to_pid(nb, &nb->groups_ns, id);

	send_pid(nb, 0);
	send_pid(nb, pid); /* TODO */
}


nb_err_t nb_send_group_end(struct nb *nb)
{
	cbor_encode_map_end(&nb->cs);
	nb->active_group = top_block(&nb->cs)->group;

	return NB_ERR_OK; /* suppress retval */
}


void nb_send_bool(struct nb *nb, nb_lid_t id, bool b)
{
	nb_send_id(nb, id);
	cbor_encode_sval(&nb->cs, b ? CBOR_SVAL_TRUE : CBOR_SVAL_FALSE);
}


void nb_send_i8(struct nb *nb, nb_lid_t id, int8_t i8)
{
	nb_send_id(nb, id);
	cbor_encode_int8(&nb->cs, i8);
}


void nb_send_i16(struct nb *nb, nb_lid_t id, int16_t i16)
{
	nb_send_id(nb, id);
	cbor_encode_int16(&nb->cs, i16);
}


void nb_send_i32(struct nb *nb, nb_lid_t id, int32_t i32)
{
	nb_send_id(nb, id);
	cbor_encode_int32(&nb->cs, i32);
}


void nb_send_i64(struct nb *nb, nb_lid_t id, int64_t i64)
{
	nb_send_id(nb, id);
	cbor_encode_int64(&nb->cs, i64);
}


void nb_send_u8(struct nb *nb, nb_lid_t id, uint8_t u8)
{
	nb_send_id(nb, id);
	cbor_encode_uint8(&nb->cs, u8);
}


void nb_send_u16(struct nb *nb, nb_lid_t id, uint16_t u16)
{
	nb_send_id(nb, id);
	cbor_encode_uint16(&nb->cs, u16);
}


void nb_send_u32(struct nb *nb, nb_lid_t id, uint32_t u32)
{
	nb_send_id(nb, id);
	cbor_encode_uint32(&nb->cs, u32);
}


void nb_send_u64(struct nb *nb, nb_lid_t id, uint64_t u64)
{
	nb_send_id(nb, id);
	cbor_encode_uint64(&nb->cs, u64);
}


void nb_send_string(struct nb *nb, nb_lid_t id, char *str)
{
	nb_send_id(nb, id);
	if (str == NULL)
		str = "";
	cbor_encode_text(&nb->cs, str);
}


void nb_send_array(struct nb *nb, nb_lid_t id, size_t size)
{
	nb_send_id(nb, id);
	cbor_encode_array_begin(&nb->cs, size);
}


void nb_send_array_end(struct nb *nb)
{
	cbor_encode_array_end(&nb->cs);
}
