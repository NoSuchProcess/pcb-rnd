/*
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Follow galvanic connections through overlapping objects */

#ifndef PCB_FIND2_H
#define PCB_FIND2_H

#include <genvector/vtp0.h>
#include "flag.h"

typedef struct pcb_find_s pcb_find_t;
struct pcb_find_s {
	/* public config - all-zero uses the original method, except for flag set */
	unsigned stay_layergrp:1;       /* do not leave the layer (no padstack hop) */
	unsigned allow_noncopper:1;     /* also run on non-copper objects */
	unsigned list_found:1;          /* allow adding objects in the ->found vector */
	unsigned ignore_intconn:1;      /* do not jump terminals on subc intconn */
	unsigned consider_rats:1;       /* don't ignore rat lines, don't consider physical objects only */
	unsigned flag_set_undoable:1;   /* when set, and flag_set is non-zero, put all found objects on the flag-undo */
	unsigned long flag_set;         /* when non-zero, set the static flag bits on objects found */

	/* if non-NULL, call after an object is found; if returns non-zero,
	   set ->aborted and stop the search. When search started from an object,
	   it is called for the starting object as well. All object data and ctx
	   fields are updated for obj before the call. */
	int (*found_cb)(pcb_find_t *ctx, pcb_any_obj_t *obj);

	/* public state/result */
	vtp0_t found;                   /* objects found, when list_found is 1 - of (pcb_any_obj_t *) */
	void *user_data;                /* filled in by caller, not read or modified by find code */

	/* private */
	vtp0_t open;                    /* objects already found but need checking for conns of (pcb_any_obj_t *) */
	pcb_data_t *data;
	pcb_board_t *pcb;
	pcb_layergrp_t *start_layergrp;
	pcb_dynf_t mark;
	unsigned long nfound;
	unsigned in_use:1;
	unsigned aborted:1;
};


unsigned long pcb_find_from_obj(pcb_find_t *ctx, pcb_data_t *data, pcb_any_obj_t *from);
unsigned long pcb_find_from_xy(pcb_find_t *ctx, pcb_data_t *data, pcb_coord_t x, pcb_coord_t y);

void pcb_find_free(pcb_find_t *ctx);

#endif
