/*
 *
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018,2019 Tibor 'Igor2' Palinkas
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

#include "board.h"
#include "data.h"
#include "layer.h"
#include <librnd/core/rnd_printf.h>

static int idpath_map(pcb_idpath_t *idp, pcb_any_obj_t *obj, int level, int *numlevels)
{
	pcb_data_t *data;
	int n;

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
		case PCB_PARENT_NET:
			return -1;
		case PCB_PARENT_LAYER:
			assert(obj->parent.layer->parent_type = PCB_PARENT_DATA);
			data = obj->parent.layer->parent.data;
			goto recurse;

		case PCB_PARENT_DATA:
			data = obj->parent.data;
			recurse:;
			switch(data->parent_type) {
				case PCB_PARENT_UI:
				case PCB_PARENT_LAYER:
				case PCB_PARENT_DATA:
				case PCB_PARENT_NET:
					return -1;
				case PCB_PARENT_BOARD:
					if (idp == NULL)
						return 0;
					if (data == PCB->Data) {
						idp->data_addr = 1;
						return 0;
					}
				case PCB_PARENT_INVALID:
					if (idp == NULL)
						return 0;
					for(n = 0; n < PCB_MAX_BUFFER; n++) {
						if (data == pcb_buffers[n].Data) {
							idp->data_addr = n+2;
							return 0;
						}
					}
					/* not the board, not any buffer -> unknown */
					return 0;
				case PCB_PARENT_SUBC: /* recurse */
					return idpath_map(idp, (pcb_any_obj_t *)data->parent.subc, level-1, numlevels);
			}
			break;
	}
	return -1;
}

pcb_idpath_t *pcb_idpath_alloc(int len)
{
	pcb_idpath_t *idp;
	idp = calloc(1, sizeof(pcb_idpath_t) + (sizeof(long int) * (len-1)));
	idp->len = len;
	return idp;
}

pcb_idpath_t *pcb_obj2idpath(pcb_any_obj_t *obj)
{
	pcb_idpath_t *idp;
	int len = 0;

	/* determine the length first */
	if (idpath_map(NULL, obj, 0, &len) != 0)
		return NULL;

	idp = pcb_idpath_alloc(len);
	if (idpath_map(idp, obj, len-1, NULL) != 0) {
		free(idp);
		return NULL;
	}

	return idp;
}

pcb_idpath_t *pcb_str2idpath(pcb_board_t *pcb, const char *str)
{
	const char *s, *ps;
	char *next;
	int data_addr, n, len = 1;
	pcb_idpath_t *idp;

	for(ps = str; *ps == '/'; ps++) ;

	data_addr = pcb_data_addr_by_name(pcb, &ps);

	for(s = ps; *s != '\0'; s++) {
		if ((s[0] == '/') && (s[1] != '/') && (s[1] != '\0'))
			len++;
		else if ((!isdigit(*s)) && (*s != '/')) {
			rnd_trace("bad idpath: '%s' in '%s'\n", s, ps);
			return NULL;
		}
	}

	idp = pcb_idpath_alloc(len);
	idp->data_addr = data_addr;

	for(s = ps, n = 0; *s != '\0'; s++,n++) {
		while(*s == '/') s++;
		if (*s == '\0')
			break;
		idp->id[n] = strtol(s, &next, 10);
		s = (const char *)next;
		if (*s == '\0')
			break;
	}
	return idp;
}


void pcb_append_idpath(gds_t *dst, const pcb_idpath_t *idp, rnd_bool relative)
{
	int n;

	if (!relative) {
		size_t end = dst->used;
		gds_enlarge(dst, dst->used + 32);
		dst->used = end;
		pcb_data_name_by_addr(idp->data_addr, dst->array+end);
		dst->used += strlen(dst->array+end);
	}

	for(n = 0; n < idp->len; n++)
		rnd_append_printf(dst, "/%ld", idp->id[n]);
}

char *pcb_idpath2str(const pcb_idpath_t *idp, rnd_bool relative)
{
	gds_t tmp;

	gds_init(&tmp);
	gds_enlarge(&tmp, 32);
	tmp.used = 0;
	pcb_append_idpath(&tmp, idp, relative);
	return tmp.array;
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

pcb_any_obj_t *pcb_idpath2obj_in(pcb_data_t *data, const pcb_idpath_t *path)
{
	return idpath2obj(data, path, 0);
}

pcb_any_obj_t *pcb_idpath2obj(pcb_board_t *pcb, const pcb_idpath_t *path)
{
	pcb_data_t *data;
	if (path->data_addr == 1) {
		if (pcb == NULL)
			return NULL;
		data = pcb->Data;
	}
	else if ((path->data_addr >= 2) && (path->data_addr < PCB_MAX_BUFFER+2))
		data = pcb_buffers[path->data_addr - 2].Data;
	else
		return NULL;
	
	return idpath2obj(data, path, 0);
}

void pcb_idpath_destroy(pcb_idpath_t *path)
{
	pcb_idpath_list_remove(path);
	free(path);
}

void pcb_idpath_list_clear(pcb_idpath_list_t *lst)
{
	pcb_idpath_t *i, *next;
	for(i = pcb_idpath_list_first(lst); i != NULL; i = next) {
		next = pcb_idpath_list_next(i);
		pcb_idpath_destroy(i);
	}
}

pcb_idpath_t *pcb_idpath_dup(const pcb_idpath_t *path)
{
	pcb_idpath_t *res = pcb_idpath_alloc(path->len);
	memcpy(res->id, path->id, (sizeof(long int) * path->len));
	return res;
}


