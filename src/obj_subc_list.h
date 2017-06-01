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

#ifndef PCB_OBJ_SUBC_LIST_H
#define PCB_OBJ_SUBC_LIST_H

#include "obj_subc.h"

/* List of Elements */
#define TDL(x)      pcb_subclist_ ## x
#define TDL_LIST_T  pcb_subclist_t
#define TDL_ITEM_T  pcb_subc_t
#define TDL_FIELD   link
#define TDL_SIZE_T  size_t
#define TDL_FUNC

#define subclist_foreach(list, iterator, loop_elem) \
	gdl_foreach_((&((list)->lst)), (iterator), (loop_elem))

#include "ht_subc.h"
#include <genht/hash.h>

/* Calculate a hash value using the content of the subc. The hash value
   represents the actual content of an subc */
unsigned int pcb_subc_hash(const pcb_subc_t *e);

/* Compare two subcs and return 1 if they contain the same objects. */
int pcb_subc_eq(const pcb_subc_t *e1, const pcb_subc_t *e2);

/* Create a new local variable to be used for deduplication */
#define pcb_subclist_dedup_initializer(state) htep_t *state = NULL;

/* Do a "continue" if an subc matching loop_elem has been seen already;
   Typically this is invoked as the first statement of an subclist_foreach()
   loop. */
#define pcb_subclist_dedup_skip(state, loop_elem) \
switch(1) { \
	case 1: { \
		if (state == NULL) \
			state = htep_alloc(pcb_subc_hash, pcb_subc_eq); \
		if (htep_has(state, loop_elem)) \
			continue; \
		htep_set(state, loop_elem, 1); \
	} \
}

/* use this after the loop to free all memory used by state */
#define pcb_subclist_dedup_free(state) \
	do { \
		if (state != NULL) { \
			htep_free(state); \
			state = NULL; \
		} \
	} while(0)


#ifndef LIST_SUBCENT_NOINSTANT
#include <genlist/gentdlist_impl.h>
#include <genlist/gentdlist_undef.h>
#endif

#endif
