/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 *
 *  This module, rats.c, was written and is Copyright (C) 1997 by harry eaton
 *  this module is also subject to the GNU GPL as described below
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

/* Arc UI logics */

#include "config.h"
#include "obj_arc_ui.h"
#include "obj_arc.h"
#include "box.h"

#include <stdio.h>

void pcb_arc_ui_move_or_copy(pcb_crosshair_t *ch)
{
	fprintf(stderr, "Moving arc endpoint: not yet implemented\n");
}

int pcb_obj_ui_arc_point_bbox(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_box_t *res)
{
	pcb_arc_t *arc = Ptr2;
	int *end_pt = Ptr3;
	pcb_box_t *ends = pcb_arc_get_ends(arc);

	if (end_pt == NULL)
		*res = pcb_point_box(ends->X1, ends->Y1);
	else
		*res = pcb_point_box(ends->X2, ends->Y2);

	return 0;
}

