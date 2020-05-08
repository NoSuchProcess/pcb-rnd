/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
 *  Copyright (C) 2017,2019,2020 Tibor 'Igor2' Palinkas
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

#include "config.h"

#include "board.h"
#include "crosshair.h"
#include "insert.h"
#include "polygon.h"
#include "search.h"
#include <librnd/core/tool.h>


static struct {
	pcb_poly_t *poly;
	pcb_line_t line;
} fake;

static rnd_point_t InsertedPoint;
static rnd_cardinal_t polyIndex = 0;


void pcb_tool_insert_uninit(void)
{
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_false);
	pcb_crosshair.AttachedObject.Type = PCB_OBJ_VOID;
	pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
	pcb_crosshair.extobj_edit = NULL;
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_true);
}

void pcb_tool_insert_notify_mode(rnd_hidlib_t *hl)
{
	switch (pcb_crosshair.AttachedObject.State) {
		/* first notify, lookup object */
	case PCB_CH_STATE_FIRST:
		pcb_crosshair.AttachedObject.Type =
			pcb_search_screen(hl->tool_x, hl->tool_y, PCB_INSERT_TYPES,
									 &pcb_crosshair.AttachedObject.Ptr1, &pcb_crosshair.AttachedObject.Ptr2, &pcb_crosshair.AttachedObject.Ptr3);

		if (pcb_crosshair.AttachedObject.Type != PCB_OBJ_VOID) {
			pcb_any_obj_t *obj = (pcb_any_obj_t *)pcb_crosshair.AttachedObject.Ptr2;
			if (PCB_FLAG_TEST(PCB_FLAG_LOCK, obj)) {
				rnd_message(RND_MSG_WARNING, "Sorry, %s object is locked\n", pcb_obj_type_name(obj->type));
				pcb_crosshair.AttachedObject.Type = PCB_OBJ_VOID;
				pcb_crosshair.extobj_edit = NULL;
				break;
			}
			else {
				/* get starting point of nearest segment */
				if (pcb_crosshair.AttachedObject.Type == PCB_OBJ_POLY) {
					fake.poly = (pcb_poly_t *) pcb_crosshair.AttachedObject.Ptr2;
					polyIndex = pcb_poly_get_lowest_distance_point(fake.poly, hl->tool_x, hl->tool_y);
					fake.line.Point1 = fake.poly->Points[polyIndex];
					fake.line.Point2 = fake.poly->Points[pcb_poly_contour_prev_point(fake.poly, polyIndex)];
					pcb_crosshair.AttachedObject.Ptr2 = &fake.line;

				}
				{
					InsertedPoint = *pcb_adjust_insert_point();
					pcb_crosshair_set_local_ref(InsertedPoint.X, InsertedPoint.Y, rnd_true);
					if (pcb_crosshair.AttachedObject.Type == PCB_OBJ_POLY)
						pcb_insert_point_in_object(PCB_OBJ_POLY,
																	pcb_crosshair.AttachedObject.Ptr1, fake.poly,
																	&polyIndex, InsertedPoint.X, InsertedPoint.Y, rnd_false, rnd_false);
					else
						pcb_insert_point_in_object(pcb_crosshair.AttachedObject.Type,
																	pcb_crosshair.AttachedObject.Ptr1,
																	pcb_crosshair.AttachedObject.Ptr2, &polyIndex, InsertedPoint.X, InsertedPoint.Y, rnd_false, rnd_false);
					pcb_board_set_changed_flag(rnd_true);

					pcb_crosshair.AttachedObject.Type = PCB_OBJ_VOID;
					pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
					pcb_crosshair.extobj_edit = NULL;
				}
			}
		}
		break;

		/* second notify, insert new point into object */
	case PCB_CH_STATE_SECOND:
		if (pcb_crosshair.AttachedObject.Type == PCB_OBJ_POLY)
			pcb_insert_point_in_object(PCB_OBJ_POLY,
														pcb_crosshair.AttachedObject.Ptr1, fake.poly,
														&polyIndex, InsertedPoint.X, InsertedPoint.Y, rnd_false, rnd_false);
		else
			pcb_insert_point_in_object(pcb_crosshair.AttachedObject.Type,
														pcb_crosshair.AttachedObject.Ptr1,
														pcb_crosshair.AttachedObject.Ptr2, &polyIndex, InsertedPoint.X, InsertedPoint.Y, rnd_false, rnd_false);
		pcb_board_set_changed_flag(rnd_true);

		/* reset identifiers */
		pcb_crosshair.AttachedObject.Type = PCB_OBJ_VOID;
		pcb_crosshair.AttachedObject.State = PCB_CH_STATE_FIRST;
		pcb_crosshair.extobj_edit = NULL;
		break;
	}
}

void pcb_tool_insert_adjust_attached_objects(rnd_hidlib_t *hl)
{
	rnd_point_t *pnt;
	pnt = pcb_adjust_insert_point();
	if (pnt)
		InsertedPoint = *pnt;
}

void pcb_tool_insert_draw_attached(rnd_hidlib_t *hl)
{
	pcb_xordraw_insert_pt_obj();
}

rnd_bool pcb_tool_insert_undo_act(rnd_hidlib_t *hl)
{
	/* don't allow undo in the middle of an operation */
	if (pcb_crosshair.AttachedObject.State != PCB_CH_STATE_FIRST)
		return rnd_false;
	return rnd_true;
}

/* XPM */
static const char *ins_icon[] = {
/* columns rows colors chars-per-pixel */
"21 21 3 1",
"  c #000000",
". c #6EA5D7",
"o c None",
/* pixels */
"oooooo...oooooooooooo",
"ooooo.ooo.ooooooooooo",
"ooooo.o.o.ooooooooooo",
"oooooo....ooooooooooo",
"ooooooooooooooooooooo",
"oooo  ooooo  oooooooo",
"ooooooooooooooooooooo",
"oo...ooooooooooo...oo",
"o.oo..ooooooooo.ooo.o",
"o.o o...........o o.o",
"o.ooo.ooooooooo.ooo.o",
"oo...ooooooooooo...oo",
"ooooooooooooooooooooo",
"ooo   o ooo oo    ooo",
"oooo oo ooo o ooooooo",
"oooo oo  oo o ooooooo",
"oooo oo o o oo   oooo",
"oooo oo oo  ooooo ooo",
"oooo oo ooo ooooo ooo",
"oooo oo ooo ooooo ooo",
"ooo   o ooo o    oooo"
};

rnd_tool_t pcb_tool_insert = {
	"insert", NULL, NULL, 100, ins_icon, RND_TOOL_CURSOR_NAMED("dotbox"), 0,
	NULL,
	pcb_tool_insert_uninit,
	pcb_tool_insert_notify_mode,
	NULL,
	pcb_tool_insert_adjust_attached_objects,
	pcb_tool_insert_draw_attached,
	pcb_tool_insert_undo_act,
	NULL,
	NULL, /* escape */
	
	0
};
