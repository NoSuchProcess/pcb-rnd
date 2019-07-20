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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* Arc UI logics */

#include "config.h"
#include <math.h>
#include "obj_arc_ui.h"
#include "obj_arc.h"
#include "box.h"


#include <stdio.h>

static void draw_mark(pcb_hid_gc_t gc, const pcb_arc_t *arc)
{
	const pcb_coord_t mark = PCB_MM_TO_COORD(0.2);
	pcb_gui->draw_line(gc, arc->X-mark, arc->Y, arc->X+mark, arc->Y);
	pcb_gui->draw_line(gc, arc->X, arc->Y-mark, arc->X, arc->Y+mark);
}

static void pcb_arc_ui_move_or_copy_angle(pcb_crosshair_t *ch)
{
	int *end_pt = ch->AttachedObject.Ptr3;
	pcb_arc_t *arc = (pcb_arc_t *) pcb_crosshair.AttachedObject.Ptr2;
	pcb_angle_t start = arc->StartAngle, delta = arc->Delta;

	if (end_pt == pcb_arc_start_ptr) {
		double end2, new_delta, new_start = atan2(-(ch->Y - arc->Y), (ch->X - arc->X)) * 180.0 / M_PI + 180.0;

		end2 = start + delta;
		new_delta = end2 - new_start;
		if (new_delta > 360.0)
			new_delta -= 360.0;
		if (new_delta < -360.0)
			new_delta += 360.0;

/*		fprintf(stderr, "start: %f new_start: %f delta=%f new delta=%f\n", start, new_start, delta, new_delta);*/

		if (delta > 0) {
			if (new_delta < 0)
				new_delta += 360.0;
		}
		else {
			if (new_delta > 0)
				new_delta -= 360.0;
		}

		start = new_start;
		delta = new_delta;
	}
	else {
		double new_delta, new_end = atan2(-(ch->Y - arc->Y), (ch->X - arc->X)) * 180.0 / M_PI + 180.0;
		if (delta < 0)
			new_end -= 360.0;
/*		fprintf(stderr, "delta: %f abs-end: %f new-abs: %f new-delta: %f\n", delta, start+delta, new_end, new_end-start);*/
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

	/* remember the result of the calculation so the actual move code can reuse them */
	ch->AttachedObject.start_angle = start;
	ch->AttachedObject.delta_angle = delta;
	ch->AttachedObject.radius = 0;

	pcb_gui->draw_arc(ch->GC, arc->X, arc->Y, arc->Width, arc->Height, start, delta);
	draw_mark(ch->GC, arc);
}

void pcb_arc_ui_move_or_copy_endp(pcb_crosshair_t *ch)
{
	int *end_pt = ch->AttachedObject.Ptr3;
	pcb_arc_t arc2, *arc = (pcb_arc_t *) pcb_crosshair.AttachedObject.Ptr2;
	pcb_coord_t ex, ey;
	double dx, dy, d;

	pcb_arc_get_end(arc, (end_pt != NULL), &ex, &ey);

	dx = (arc->X - ch->X);
	dy = (arc->Y - ch->Y);
	d = sqrt(dx*dx+dy*dy);

	ch->AttachedObject.radius = d;

	pcb_gui->draw_arc(ch->GC, arc->X, arc->Y, ch->AttachedObject.radius, ch->AttachedObject.radius, arc->StartAngle, arc->Delta);

	draw_mark(ch->GC, &arc2);
}


void pcb_arc_ui_move_or_copy(pcb_crosshair_t *ch)
{
	if (pcb_gui->shift_is_pressed(pcb_gui) || (ch->AttachedObject.radius != 0))
		pcb_arc_ui_move_or_copy_endp(ch);
	else
		pcb_arc_ui_move_or_copy_angle(ch);
}


int pcb_obj_ui_arc_point_bbox(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_box_t *res)
{
	pcb_arc_t *arc = Ptr2;
	int *end_pt = Ptr3;
	pcb_coord_t ex, ey;
	pcb_arc_get_end(arc, (end_pt != pcb_arc_start_ptr), &ex, &ey);
	*res = pcb_point_box(ex, ey);
	return 0;
}

