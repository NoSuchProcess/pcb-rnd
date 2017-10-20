/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

#include "config.h"

#include "action_helper.h"
#include "board.h"
#include "compat_nls.h"
#include "crosshair.h"
#include "insert.h"
#include "polygon.h"
#include "search.h"
#include "tool.h"


static struct {
	pcb_poly_t *poly;
	pcb_line_t line;
} fake;

static pcb_point_t InsertedPoint;
static pcb_cardinal_t polyIndex = 0;


void pcb_tool_insert_notify_mode(void)
{
	switch (pcb_crosshair.AttachedObject.State) {
		/* first notify, lookup object */
	case PCB_CH_STATE_FIRST:
		pcb_crosshair.AttachedObject.Type =
			pcb_search_screen(Note.X, Note.Y, PCB_INSERT_TYPES,
									 &pcb_crosshair.AttachedObject.Ptr1, &pcb_crosshair.AttachedObject.Ptr2, &pcb_crosshair.AttachedObject.Ptr3);

		if (pcb_crosshair.AttachedObject.Type != PCB_TYPE_NONE) {
			if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_poly_t *)
										pcb_crosshair.AttachedObject.Ptr2)) {
				pcb_message(PCB_MSG_WARNING, _("Sorry, the object is locked\n"));
				pcb_crosshair.AttachedObject.Type = PCB_TYPE_NONE;
				break;
			}
			else {
				/* get starting point of nearest segment */
				if (pcb_crosshair.AttachedObject.Type == PCB_TYPE_POLYGON) {
					fake.poly = (pcb_poly_t *) pcb_crosshair.AttachedObject.Ptr2;
					polyIndex = pcb_poly_get_lowest_distance_point(fake.poly, Note.X, Note.Y);
					fake.line.Point1 = fake.poly->Points[polyIndex];
					fake.line.Point2 = fake.poly->Points[pcb_poly_contour_prev_point(fake.poly, polyIndex)];
					pcb_crosshair.AttachedObject.Ptr2 = &fake.line;

				}
				pcb_crosshair.AttachedObject.State = PCB_CH_STATE_SECOND;
				InsertedPoint = *pcb_adjust_insert_point();
			}
		}
		break;

		/* second notify, insert new point into object */
	case PCB_CH_STATE_SECOND:
		if (pcb_crosshair.AttachedObject.Type == PCB_TYPE_POLYGON)
			pcb_insert_point_in_object(PCB_TYPE_POLYGON,
														pcb_crosshair.AttachedObject.Ptr1, fake.poly,
														&polyIndex, InsertedPoint.X, InsertedPoint.Y, pcb_false, pcb_false);
		else
			pcb_insert_point_in_object(pcb_crosshair.AttachedObject.Type,
														pcb_crosshair.AttachedObject.Ptr1,
														pcb_crosshair.AttachedObject.Ptr2, &polyIndex, InsertedPoint.X, InsertedPoint.Y, pcb_false, pcb_false);
		pcb_board_set_changed_flag(pcb_true);

		/* reset identifiers */
		pcb_crosshair.AttachedObject.Type = PCB_TYPE_NONE;
		pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
		break;
	}
}

void pcb_tool_insert_adjust_attached_objects(void)
{
	pcb_point_t *pnt;
	pnt = pcb_adjust_insert_point();
	if (pnt)
		InsertedPoint = *pnt;
}

pcb_tool_t pcb_tool_insert = {
	"insert", NULL, 100,
	pcb_tool_insert_notify_mode,
	pcb_tool_insert_adjust_attached_objects
};
