/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996,2004,2006 Thomas Nau
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* attribute lists */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <librnd/config.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/misc_util.h>
#include <librnd/core/rnd_printf.h>
#include "attrib.h"
#include "undo.h"

static const char core_attribute_cookie[] = "core-attribute";

#define NOTIFY(list, name, value) \
do { \
	if (list->post_change != NULL) \
		list->post_change(list, name, value); \
} while(0)

char *pcb_attribute_get(const pcb_attribute_list_t *list, const char *name)
{
	int i;
	for (i = 0; i < list->Number; i++)
		if (strcmp(name, list->List[i].name) == 0)
			return list->List[i].value;
	return NULL;
}

char **pcb_attribute_get_ptr(const pcb_attribute_list_t *list, const char *name)
{
	int i;
	for (i = 0; i < list->Number; i++)
		if (strcmp(name, list->List[i].name) == 0)
			return &list->List[i].value;
	return NULL;
}

int pcb_attribute_get_idx(const pcb_attribute_list_t *list, const char *name)
{
	int i;
	for(i = 0; i < list->Number; i++)
		if (strcmp(name, list->List[i].name) == 0)
			return i;
	return -1;
}

char **pcb_attribute_get_namespace_ptr(const pcb_attribute_list_t *list, const char *plugin, const char *key)
{
	int i, glb = -1, plugin_len = strlen(plugin);

	for (i = 0; i < list->Number; i++) {
		if (strcmp(list->List[i].name, key) == 0)
			glb = i;
		if ((strncmp(list->List[i].name, plugin, plugin_len) == 0) && (list->List[i].name[plugin_len] == ':') && (list->List[i].name[plugin_len+1] == ':')) {
			if (strcmp(list->List[i].name+plugin_len+2, key) == 0)
				return &list->List[i].value; /* found the plugin-specific value, that wins */
		}
	}
	if (glb >= 0)
		return &list->List[glb].value; /* fall back to global if present */
	return NULL; /* nothing */
}

char *pcb_attribute_get_namespace(const pcb_attribute_list_t *list, const char *plugin, const char *key)
{
	char **res = pcb_attribute_get_namespace_ptr(list, plugin, key);
	if (res == NULL)
		return NULL;
	return *res;
}

/* assumes name and value strdup'd by the caller! */
RND_INLINE int pcb_attribute_put_append(pcb_attribute_list_t *list, char *name, char *value)
{
	int i;

	if (list->Number >= list->Max) {
		list->Max += 10;
		list->List = (pcb_attribute_t *) realloc(list->List, list->Max * sizeof(pcb_attribute_t));
	}

	/* Now add the new attribute.  */
	i = list->Number;
	list->List[i].name = name;
	list->List[i].value = value;
	list->List[i].cpb_written = 1;
	NOTIFY(list, list->List[i].name, list->List[i].value);
	list->Number++;
	return 0;
}

int pcb_attribute_put(pcb_attribute_list_t * list, const char *name, const char *value)
{
	int i;

	if ((name == NULL) || (*name == '\0'))
		return -1;

	/* Replace an existing attribute if there is a name match. */
	for (i = 0; i < list->Number; i++) {
		if (strcmp(name, list->List[i].name) == 0) {
			char *old_value = list->List[i].value;
			list->List[i].value = rnd_strdup_null(value);
			NOTIFY(list, list->List[i].name, list->List[i].value);
			free(old_value);
			return 1;
		}
	}

	/* At this point, we're going to need to add a new attribute to the
	   list.  See if there's room.  */
	return pcb_attribute_put_append(list, rnd_strdup_null(name), rnd_strdup_null(value));
}

int pcb_attribute_remove_idx(pcb_attribute_list_t * list, int idx)
{
	int j;
	char *old_name = list->List[idx].name, *old_value = list->List[idx].value;

	for (j = idx; j < list->Number-1; j++)
		list->List[j] = list->List[j + 1];
	list->Number--;

	NOTIFY(list, old_name, NULL);
	free(old_name);
	free(old_value);
	return 0;
}

int pcb_attribute_remove(pcb_attribute_list_t * list, const char *name)
{
	int i, found = 0;
	for (i = 0; i < list->Number; i++)
		if (strcmp(name, list->List[i].name) == 0) {
			found++;
			pcb_attribute_remove_idx(list, i);
		}
	return found;
}

void pcb_attribute_free(pcb_attribute_list_t *list)
{
	int i;

	/* first notify for all removals while the values are still available */
	for (i = 0; i < list->Number; i++)
		NOTIFY(list, list->List[i].name, NULL);

	for (i = 0; i < list->Number; i++) {
		free(list->List[i].name);
		free(list->List[i].value);
	}
	free(list->List);
	list->List = NULL;
	list->Max = 0;
}

void pcb_attribute_copy_all(pcb_attribute_list_t *dest, const pcb_attribute_list_t *src)
{
	int i;

	for (i = 0; i < src->Number; i++)
		pcb_attribute_put(dest, src->List[i].name, src->List[i].value);
}


void pcb_attribute_copyback_begin(pcb_attribute_list_t *dst)
{
	int i;

	for (i = 0; i < dst->Number; i++)
		dst->List[i].cpb_written = 0;
}

void pcb_attribute_copyback(pcb_attribute_list_t *dst, const char *name, const char *value)
{
	int i;
	for (i = 0; i < dst->Number; i++) {
		if (strcmp(name, dst->List[i].name) == 0) {
			dst->List[i].cpb_written = 1;
			if (strcmp(value, dst->List[i].value) != 0) {
				char *old_value = dst->List[i].value;
				dst->List[i].value = rnd_strdup(value);
				NOTIFY(dst, dst->List[i].name, dst->List[i].value);
				free(old_value);
			}
			return;
		}
	}
	pcb_attribute_put(dst, name, value);
}

void pcb_attribute_copyback_end(pcb_attribute_list_t *dst)
{
	int i;
	for (i = 0; i < dst->Number; i++)
		if (!dst->List[i].cpb_written)
			pcb_attribute_remove_idx(dst, i);
}


void pcb_attrib_compat_set_intconn(pcb_attribute_list_t *dst, int intconn)
{
	char buff[32];

	if (intconn <= 0)
		return;

	sprintf(buff, "%d", intconn & 0xFF);
	pcb_attribute_put(dst, "intconn", buff);
}



/*** undoable attribute set/del */
typedef struct {
	pcb_attribute_list_t *list; /* it is safe to save the object pointer because it is persistent (through the removed object list) */
	char *name;
	char *value;
} undo_attribute_set_t;

static int undo_attribute_set_swap(void *udata)
{
	undo_attribute_set_t *g = udata;
	int idx = pcb_attribute_get_idx(g->list, g->name);
	char **slot = NULL;

	if (idx >= 0)
		slot = &g->list->List[idx].value;

	/* want to delete */
	if (g->value == NULL) {
		if (slot == NULL) /* but it does not exist */
			return 0;

		/* save old value and remove */
		g->value = *slot;
		*slot = NULL;
		pcb_attribute_remove_idx(g->list, idx);
		return 0;
	}

	/* old value doesn't exist: need to create it */
	if (slot == NULL) {
		pcb_attribute_put_append(g->list, rnd_strdup(g->name), g->value);
		g->value = NULL;
		return 0;
	}

	/* replace the existing value */
	rnd_swap(char *, *slot, g->value);
	NOTIFY(g->list, g->name, *slot);
	return 0;
}

static void undo_attribute_set_print(void *udata, char *dst, size_t dst_len)
{
	undo_attribute_set_t *g = udata;
	rnd_snprintf(dst, dst_len, "attribute set %s to %s", g->name, g->value == NULL ? "<remove>" : g->value);
}

static void undo_attribute_set_free(void *udata)
{
	undo_attribute_set_t *g = udata;
	free(g->name);
	free(g->value);
}

static const uundo_oper_t undo_attribute_set = {
	core_attribute_cookie,
	undo_attribute_set_free,
	undo_attribute_set_swap,
	undo_attribute_set_swap,
	undo_attribute_set_print
};



void pcb_attribute_set(pcb_board_t *pcb, pcb_attribute_list_t *list, const char *name, const char *value, rnd_bool undoable)
{
	undo_attribute_set_t gtmp, *g = &gtmp;

	if (undoable) g = pcb_undo_alloc(pcb, &undo_attribute_set, sizeof(undo_attribute_set_t));

	g->list = list;
	g->name = rnd_strdup_null(name);
	g->value = rnd_strdup_null(value);

	undo_attribute_set_swap(g);
	if (undoable) pcb_undo_inc_serial();
}

