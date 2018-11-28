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

#ifndef PCB_IDPATH_H
#define PCB_IDPATH_H

#include <genlist/gendlist.h>

typedef struct pcb_idpath_s {
	int len;
	gdl_elem_t link;  /* may be part of an idpath list */
	long int id[1];   /* the struct is allocated to be sizeof(long int) * len */
} pcb_idpath_t;

/* List of id paths */
#define TDL(x)      pcb_idpath_list_ ## x
#define TDL_LIST_T  pcb_idpath_list_t
#define TDL_ITEM_T  pcb_idpath_t
#define TDL_FIELD   link
#define TDL_SIZE_T  size_t
#define TDL_FUNC

#define pcb_idpath_list_foreach(list, iterator, loop_elem) \
	gdl_foreach_((&((list)->lst)), (iterator), (loop_elem))

#include <genlist/gentdlist_impl.h>
#include <genlist/gentdlist_undef.h>

#include "obj_common.h"

pcb_idpath_t *pcb_obj2idpath(pcb_any_obj_t *obj);
pcb_any_obj_t *pcb_idpath2obj(pcb_data_t *data, const pcb_idpath_t *path);
void pcb_idpath_destroy(pcb_idpath_t *path);

void pcb_idpath_list_clear(pcb_idpath_list_t *lst);

#endif
