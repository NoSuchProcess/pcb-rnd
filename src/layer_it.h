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
 *
 */

/* Layer and layer group iterators (static inline functions) */

#ifndef PCB_LAYER_IT_H
#define PCB_LAYER_IT_H

#include "layer_grp.h"

typedef struct pcb_layer_it_s pcb_layer_it_t;

/* Start an iteration matching exact flags or any of the flags; returns -1 if over */
RND_INLINE pcb_layer_id_t pcb_layer_first(pcb_layer_stack_t *stack, pcb_layer_it_t *it, unsigned int exact_mask);
RND_INLINE pcb_layer_id_t pcb_layer_first_any(pcb_layer_stack_t *stack, pcb_layer_it_t *it, unsigned int any_mask);

/* next iteration; returns -1 if over */
RND_INLINE pcb_layer_id_t pcb_layer_next(pcb_layer_it_t *it);


/*************** inline implementation *************************/
struct pcb_layer_it_s {
	pcb_layer_stack_t *stack;
	pcb_layergrp_id_t gid;
	rnd_cardinal_t lidx;
	unsigned int mask;
	unsigned int exact:1;
	unsigned int global:1;
};

RND_INLINE pcb_layer_id_t pcb_layer_next(pcb_layer_it_t *it)
{
	if (it->global) {
		/* over all layers, random order, without any checks - go the cheap way, bypassing groups */
		if (it->lidx < pcb_max_layer(PCB))
			return it->lidx++;
		return -1;
	}
	else for(;;) {
		pcb_layergrp_t *g = &(it->stack->grp[it->gid]);
		pcb_layer_id_t lid;
		unsigned int hit;
		if (it->lidx >= g->len) { /* layer list over in this group */
			it->gid++;
			if (it->gid >= PCB_MAX_LAYERGRP) /* last group */
				return -1;
			it->lidx = 0;
			continue; /* skip to next group */
		}
		/* TODO: check group flags against mask here for more efficiency */
		lid = g->lid[it->lidx];
		it->lidx++;
		hit = pcb_layer_flags(PCB, lid) & it->mask;
		if (it->exact) {
			if (hit == it->mask)
				return lid;
		}
		else {
			if (hit)
				return lid;
		}
		/* skip to next group */
	}
}

RND_INLINE pcb_layer_id_t pcb_layer_first(pcb_layer_stack_t *stack, pcb_layer_it_t *it, unsigned int exact_mask)
{
	it->stack = stack;
	it->mask = exact_mask;
	it->gid = 0;
	it->lidx = 0;
	it->exact = 1;
	it->global = 0;
	return pcb_layer_next(it);
}

RND_INLINE pcb_layer_id_t pcb_layer_first_any(pcb_layer_stack_t *stack, pcb_layer_it_t *it, unsigned int any_mask)
{
	it->stack = stack;
	it->mask = any_mask;
	it->gid = 0;
	it->lidx = 0;
	it->exact = 0;
	it->global = 0;
	return pcb_layer_next(it);
}

RND_INLINE pcb_layer_id_t pcb_layer_first_all(pcb_layer_stack_t *stack, pcb_layer_it_t *it)
{
	it->stack = stack;
	it->lidx = 0;
	it->global = 1;
	return pcb_layer_next(it);
}


#endif
