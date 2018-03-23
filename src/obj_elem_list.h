/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#ifndef PCB_OBJ_ELEM_LIST_H
#define PCB_OBJ_ELEM_LIST_H

#include "obj_elem.h"

/* List of Elements */
#define TDL(x)      elementlist_ ## x
#define TDL_LIST_T  elementlist_t
#define TDL_ITEM_T  pcb_element_t
#define TDL_FIELD   link
#define TDL_SIZE_T  size_t
#define TDL_FUNC

#define elementlist_foreach(list, iterator, loop_elem) \
	gdl_foreach_((&((list)->lst)), (iterator), (loop_elem))

/* Calculate a hash value using the content of the element. The hash value
   represents the actual content of an element */
unsigned int pcb_element_hash(const pcb_element_t *e);

/* Compare two elements and return 1 if they contain the same objects. */
int pcb_element_eq(const pcb_element_t *e1, const pcb_element_t *e2);



#ifndef LIST_ELEMENT_NOINSTANT
#include <genlist/gentdlist_impl.h>
#include <genlist/gentdlist_undef.h>
#endif

#endif
