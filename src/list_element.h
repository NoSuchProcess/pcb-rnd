/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef LIST_ELEMENT_H
#define LIST_ELEMENT_H

#include <genht/hash.h>
#include <genht/htip.h>

/* List of Lines */
#define TDL(x)      elementlist_ ## x
#define TDL_LIST_T  elementlist_t
#define TDL_ITEM_T  ElementType
#define TDL_FIELD   link
#define TDL_SIZE_T  size_t
#define TDL_FUNC

#define elementlist_foreach(list, iterator, loop_elem) \
	gdl_foreach_((&((list)->lst)), (iterator), (loop_elem))

/* Calculate a hash value using the content of the element. The hash value
   represents the actual content of an element */
long pcb_element_hash(const ElementType *e);

#define elementlist_dedup_initializer(state) htip_t *state = NULL;

#define elementlist_dedup_skip(state, loop_elem) \
switch(1) { \
	case 1: { \
		long element_hash; \
		if (state == NULL) \
			state = htip_alloc(longhash, longkeyeq); \
		element_hash = pcb_element_hash(loop_elem); \
		if (htip_has(state, element_hash)) \
			continue; \
		htip_set(state, element_hash, loop_elem); \
	} \
}

#define elementlist_dedup_free(state) \
	do { \
		if (state != NULL) { \
			htip_free(state); \
			state = NULL; \
		} \
	} while(0)


#ifndef LIST_ELEMENT_NOINSTANT
#include <genlist/gentdlist_impl.h>
#include <genlist/gentdlist_undef.h>
#endif

#endif
