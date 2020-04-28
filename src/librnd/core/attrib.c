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
#include <librnd/core/attrib.h>

#define NOTIFY(list, name, value) \
do { \
	if (list->post_change != NULL) \
		list->post_change(list, name, value); \
} while(0)

char *rnd_attribute_get(const rnd_attribute_list_t *list, const char *name)
{
	int i;
	for (i = 0; i < list->Number; i++)
		if (strcmp(name, list->List[i].name) == 0)
			return list->List[i].value;
	return NULL;
}

char **rnd_attribute_get_ptr(const rnd_attribute_list_t *list, const char *name)
{
	int i;
	for (i = 0; i < list->Number; i++)
		if (strcmp(name, list->List[i].name) == 0)
			return &list->List[i].value;
	return NULL;
}

char **rnd_attribute_get_namespace_ptr(const rnd_attribute_list_t *list, const char *plugin, const char *key)
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

char *rnd_attribute_get_namespace(const rnd_attribute_list_t *list, const char *plugin, const char *key)
{
	char **res = rnd_attribute_get_namespace_ptr(list, plugin, key);
	if (res == NULL)
		return NULL;
	return *res;
}

int rnd_attribute_put(rnd_attribute_list_t * list, const char *name, const char *value)
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
	if (list->Number >= list->Max) {
		list->Max += 10;
		list->List = (rnd_attribute_t *) realloc(list->List, list->Max * sizeof(rnd_attribute_t));
	}

	/* Now add the new attribute.  */
	i = list->Number;
	list->List[i].name = rnd_strdup_null(name);
	list->List[i].value = rnd_strdup_null(value);
	list->List[i].cpb_written = 1;
	NOTIFY(list, list->List[i].name, list->List[i].value);
	list->Number++;
	return 0;
}

int rnd_attribute_remove_idx(rnd_attribute_list_t * list, int idx)
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

int rnd_attribute_remove(rnd_attribute_list_t * list, const char *name)
{
	int i, found = 0;
	for (i = 0; i < list->Number; i++)
		if (strcmp(name, list->List[i].name) == 0) {
			found++;
			rnd_attribute_remove_idx(list, i);
		}
	return found;
}

void rnd_attribute_free(rnd_attribute_list_t *list)
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

void rnd_attribute_copy_all(rnd_attribute_list_t *dest, const rnd_attribute_list_t *src)
{
	int i;

	for (i = 0; i < src->Number; i++)
		rnd_attribute_put(dest, src->List[i].name, src->List[i].value);
}


void rnd_attribute_copyback_begin(rnd_attribute_list_t *dst)
{
	int i;

	for (i = 0; i < dst->Number; i++)
		dst->List[i].cpb_written = 0;
}

void rnd_attribute_copyback(rnd_attribute_list_t *dst, const char *name, const char *value)
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
	rnd_attribute_put(dst, name, value);
}

void rnd_attribute_copyback_end(rnd_attribute_list_t *dst)
{
	int i;
	for (i = 0; i < dst->Number; i++)
		if (!dst->List[i].cpb_written)
			rnd_attribute_remove_idx(dst, i);
}


void rnd_attrib_compat_set_intconn(rnd_attribute_list_t *dst, int intconn)
{
	char buff[32];

	if (intconn <= 0)
		return;

	sprintf(buff, "%d", intconn & 0xFF);
	rnd_attribute_put(dst, "intconn", buff);
}

