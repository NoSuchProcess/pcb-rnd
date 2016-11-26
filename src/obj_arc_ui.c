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
#include <math.h>
#include "obj_arc_ui.h"
#include "obj_arc.h"
#include "box.h"


#include <stdio.h>

void pcb_arc_ui_move_or_copy(pcb_crosshair_t *ch)
{
	int *end_pt = ch->AttachedObject.Ptr3;
	pcb_arc_t *arc = (pcb_arc_t *) pcb_crosshair.AttachedObject.Ptr2;
	pcb_angle_t start = arc->StartAngle, delta = arc->Delta;

	if (end_pt == NULL) {
		fprintf(stderr, "Moving arc endpoint: start point not yet implemented %p\n", end_pt);
	}
	else {
		double new_delta, new_end = atan2(-(ch->Y - arc->Y), (ch->X - arc->X)) * 180.0 / M_PI + 180.0;
		if (delta < 0)
			new_end -= 360.0;
		fprintf(stderr, "delta: %f abs-end: %f new-abs: %f new-delta: %f\n", delta, start+delta, new_end, new_end-start);
		new_delta = new_end-start;
		if (delta > 0) {
			if (new_delta < 0)
				new_delta += 360.0;
		}
		else {
			if (new_delta > 0)
				new_delta -= 360.0;
		}
		delta = new_delta;
	}

	pcb_gui->draw_arc(ch->GC, arc->X, arc->Y, arc->Width, arc->Height, start, delta);
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

