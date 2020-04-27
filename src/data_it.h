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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Flat, non-recursive iterator on pcb_data_t children objects */

#ifndef PCB_DATA_IT_H
#define PCB_DATA_IT_H

#include "obj_common.h"
#include "data.h"

typedef struct pcb_data_it_s {
	/* config */
	pcb_data_t *data;
	pcb_objtype_t mask; /* caller wants to see these types */

	/* current state */
	pcb_objtype_t remaining;
	pcb_objtype_t type;
	rnd_cardinal_t ln;
	pcb_any_obj_t *last;
} pcb_data_it_t;

#define PCB_DATA_IT_TYPES \
	(PCB_OBJ_LINE | PCB_OBJ_TEXT | PCB_OBJ_POLY | PCB_OBJ_ARC | PCB_OBJ_GFX | \
	PCB_OBJ_PSTK | PCB_OBJ_SUBC | PCB_OBJ_RAT)

/* Start an iteration on data, looking for any object type listed in mask;
   returns NULL if nothing is found. The iteration is non-recursive to subcircuits */
RND_INLINE pcb_any_obj_t *pcb_data_first(pcb_data_it_t *it, pcb_data_t *data, pcb_objtype_t mask);

/* Return the next object or NULL on end of iteration */
RND_INLINE pcb_any_obj_t *pcb_data_next(pcb_data_it_t *it);

/**** implementation (inlines) ****/

/* Skip to the next type; returns 0 on success, non-0 at the end of types */
RND_INLINE int pcb_data_it_next_type(pcb_data_it_t *it)
{
	if (it->remaining == 0)
		return 1;

	if (it->type == 0)
		it->type = 1;
	else
		it->type <<= 1;

	/* find and remove the next bit */
	while((it->remaining & it->type) == 0) it->type <<= 1;
	it->remaining &= ~it->type;

	return 0;
}

/* Take the next layer object, skip layer if there's no more */
#define PCB_DATA_IT_LOBJ(it, otype, ltype, list) \
	do { \
		if (it->last == NULL) \
			it->last = (pcb_any_obj_t *)ltype ## _first(&(it->data->Layer[it->ln].list)); \
		else \
			it->last = (pcb_any_obj_t *)ltype ## _next((otype *)it->last); \
		if (it->last == NULL) \
			goto next_layer; \
	} while(0)

/* Take the next global object, skip type if there's no more */
#define PCB_DATA_IT_GOBJ(it, otype, ltype, list) \
	do { \
		if (it->last == NULL) \
			it->last = (pcb_any_obj_t *)ltype ## _first(&(it->data->list)); \
		else \
			it->last = (pcb_any_obj_t *)ltype ## _next((otype *)it->last); \
		if (it->last == NULL) \
			goto next_type; \
	} while(0)


RND_INLINE pcb_any_obj_t *pcb_data_next(pcb_data_it_t *it)
{
	retry:;

	switch(it->type) {
		case PCB_OBJ_LINE: PCB_DATA_IT_LOBJ(it, pcb_line_t, linelist, Line); break;
		case PCB_OBJ_TEXT: PCB_DATA_IT_LOBJ(it, pcb_text_t, textlist, Text); break;
		case PCB_OBJ_POLY: PCB_DATA_IT_LOBJ(it, pcb_poly_t, polylist, Polygon); break;
		case PCB_OBJ_ARC:  PCB_DATA_IT_LOBJ(it, pcb_arc_t,  arclist,  Arc); break;
		case PCB_OBJ_PSTK: PCB_DATA_IT_GOBJ(it, pcb_pstk_t, padstacklist, padstack); break;
		case PCB_OBJ_SUBC: PCB_DATA_IT_GOBJ(it, pcb_subc_t, pcb_subclist, subc); break;
		case PCB_OBJ_RAT:  PCB_DATA_IT_GOBJ(it, pcb_rat_t,  ratlist, Rat); break;
		case PCB_OBJ_GFX:  PCB_DATA_IT_LOBJ(it, pcb_gfx_t,  gfxlist, Gfx); break;
		default:
			assert(!"iterating on invalid type");
			return NULL;
	}
	return it->last;

	/* skip to the next layer and retry unless ran out of types */
	next_layer:;
	it->ln++;
	if (it->ln >= it->data->LayerN) { /* checked all layers for this type, skip to next type */
		it->ln = 0;
		next_type:;
		if (pcb_data_it_next_type(it))
			return NULL;
	}

	goto retry; /* ... on the new layer */
}

RND_INLINE pcb_any_obj_t *pcb_data_first(pcb_data_it_t *it, pcb_data_t *data, pcb_objtype_t mask)
{
	it->data = data;
	it->mask = it->remaining = mask & PCB_DATA_IT_TYPES;
	it->type = 0;
	it->ln = 0;
	it->last = NULL;
	if (pcb_data_it_next_type(it))
		return NULL;
	return pcb_data_next(it);
}

#endif
