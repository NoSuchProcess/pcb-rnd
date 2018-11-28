/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "config.h"

#define TDL_DONT_UNDEF
#include "idpath.h"
#include <genlist/gentdlist_impl.c>
#undef TDL_DONT_UNDEF
#include <genlist/gentdlist_undef.h>

#include <assert.h>

#include "data.h"
#include "layer.h"

static int idpath_map(pcb_idpath_t *idp, pcb_any_obj_t *obj, int level, int *numlevels)
{
	pcb_data_t *data;

	if (numlevels != 0)
		(*numlevels)++;

	if (idp != NULL) {
		if (level < 0)
			return -1;
		idp->id[level] = obj->ID;
	}

	switch(obj->parent_type) {
		case PCB_PARENT_INVALID:
		case PCB_PARENT_UI:
		case PCB_PARENT_SUBC:
		case PCB_PARENT_BOARD:
			return -1;
		case PCB_PARENT_LAYER:
			assert(obj->parent.layer->parent_type = PCB_PARENT_DATA);
			data = obj->parent.layer->parent.data;
			goto recurse;

		case PCB_PARENT_DATA:
			data = obj->parent.data;
			recurse:;
			switch(data->parent_type) {
				case PCB_PARENT_INVALID:
				case PCB_PARENT_UI:
				case PCB_PARENT_LAYER:
				case PCB_PARENT_DATA:
					return -1;
				case PCB_PARENT_BOARD:
					return 0;
				case PCB_PARENT_SUBC: /* recurse */
					return idpath_map(idp, (pcb_any_obj_t *)data->parent.subc, level-1, numlevels);
			}
			break;
	}
	return -1;
}

pcb_idpath_t *pcb_obj2idpath(pcb_any_obj_t *obj)
{
	pcb_idpath_t *idp;
	int len = 0;

	/* determine the length first */
	if (idpath_map(NULL, obj, 0, &len) != 0)
		return NULL;

	idp = malloc(sizeof(pcb_idpath_t) + (sizeof(long int) * (len-1)));
	if (idpath_map(idp, obj, len-1, NULL) != 0) {
		free(idp);
		return NULL;
	}

	idp->len = len;
	return idp;
}

static pcb_any_obj_t *idpath2obj(pcb_data_t *data, const pcb_idpath_t *path, int level)
{

	for(;;) {
		pcb_any_obj_t *obj = htip_get(&data->id2obj, path->id[level]);
		pcb_subc_t *sc = (pcb_subc_t *)obj;

		if (obj == NULL)
			return NULL;

		level++;
		if (level == path->len)
			return obj;

		if (obj->type != PCB_OBJ_SUBC) /* can descend only in subcircuits */
			return NULL;

		data = sc->data;
	}
}

pcb_any_obj_t *pcb_idpath2obj(pcb_data_t *data, const pcb_idpath_t *path)
{
	return idpath2obj(data, path, 0);
}

void pcb_idpath_destroy(pcb_idpath_t *path)
{
	pcb_idpath_list_remove(path);
	free(path);
}

void pcb_idpath_list_clear(pcb_idpath_list_t *lst)
{
	pcb_idpath_t *i;
	for(i = pcb_idpath_list_first(lst); i != NULL; i = pcb_drc_list_next(i))
		pcb_idpath_destroy(i);
}

