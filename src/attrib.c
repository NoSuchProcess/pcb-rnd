/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996,2004,2006 Thomas Nau
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */

/* attribute lists */
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "compat_misc.h"
#include "attrib.h"

#define NOTIFY(list, name, value) \
do { \
	if (list->post_change != NULL) \
		list->post_change(list, name, value); \
} while(0)

char *pcb_attribute_get(pcb_attribute_list_t * list, const char *name)
{
	int i;
	for (i = 0; i < list->Number; i++)
		if (strcmp(name, list->List[i].name) == 0)
			return list->List[i].value;
	return NULL;
}

int pcb_attribute_put(pcb_attribute_list_t * list, const char *name, const char *value, int replace)
{
	int i;

	if ((name == NULL) || (*name == '\0'))
		return -1;

	/* If we're allowed to replace an existing attribute, see if we
	   can.  */
	if (replace) {
		for (i = 0; i < list->Number; i++)
			if (strcmp(name, list->List[i].name) == 0) {
				free(list->List[i].value);
				list->List[i].value = pcb_strdup_null(value);
				NOTIFY(list, list->List[i].name, list->List[i].value);
				return 1;
			}
	}

	/* At this point, we're going to need to add a new attribute to the
	   list.  See if there's room.  */
	if (list->Number >= list->Max) {
		list->Max += 10;
		list->List = (pcb_attribute_t *) realloc(list->List, list->Max * sizeof(pcb_attribute_t));
	}

	/* Now add the new attribute.  */
	i = list->Number;
	list->List[i].name = pcb_strdup_null(name);
	list->List[i].value = pcb_strdup_null(value);
	NOTIFY(list, list->List[i].name, list->List[i].value);
	list->Number++;
	return 0;
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

	for (i = 0; i < list->Number; i++) {
		free(list->List[i].name);
		free(list->List[i].value);
	}
	free(list->List);
	list->List = NULL;
	list->Max = 0;
}

void pcb_attribute_copy_all(pcb_attribute_list_t *dest, const pcb_attribute_list_t *src, int replace)
{
	int i;

	for (i = 0; i < src->Number; i++)
		pcb_attribute_put(dest, src->List[i].name, src->List[i].value, replace);
}

