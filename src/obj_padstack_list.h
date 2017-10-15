/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
 */

#ifndef PCB_OBJ_PADSTACK_LIST_H
#define PCB_OBJ_PADSTACK_LIST_H

#define PCB_PADSTACK_MAX_SHAPES 31

#include "obj_common.h"

/* The actual padstack is just a reference to a padstack proto within the same data */
struct pcb_padstack_s {
	PCB_ANYOBJECTFIELDS;
	pcb_cardinal_t proto;          /* reference to a pcb_padstack_proto_t within pcb_data_t */
	pcb_coord_t x, y;
	struct {
		unsigned long used;
		char *shape;                 /* indexed by layer ID */
	} thermal;
	gdl_elem_t link;               /* a padstack is in a list in pcb_data_t as a global object */
};


/* List of padstatcks */
#define TDL(x)      padstacklist_ ## x
#define TDL_LIST_T  padstacklist_t
#define TDL_ITEM_T  pcb_padstack_t
#define TDL_FIELD   link
#define TDL_SIZE_T  size_t
#define TDL_FUNC

#define padstacklist_foreach(list, iterator, loop_elem) \
	gdl_foreach_((&((list)->lst)), (iterator), (loop_elem))


#include <genlist/gentdlist_impl.h>
#include <genlist/gentdlist_undef.h>

#endif
