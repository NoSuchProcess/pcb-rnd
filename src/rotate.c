/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
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
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */


/* functions used to rotate pins, elements ...
 */

#include "config.h"

#include <stdlib.h>

#include "board.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "polygon.h"
#include "rotate.h"
#include "search.h"
#include "select.h"
#include "undo.h"
#include "event.h"
#include "conf_core.h"
#include "compat_nls.h"
#include "obj_all_op.h"

#include "obj_line_draw.h"
#include "obj_rat_draw.h"

/* ----------------------------------------------------------------------
 * some local identifiers
 */
static pcb_opfunc_t RotateFunctions = {
	NULL,
	RotateText,
	NULL,
	NULL,
	RotateElement,
	RotateElementName,
	NULL,
	NULL,
	RotateLinePoint,
	NULL,
	RotateArc,
	NULL
};

/* ---------------------------------------------------------------------------
 * rotates a point in 90 degree steps
 */
void pcb_point_rotate90(pcb_point_t *Point, pcb_coord_t X, pcb_coord_t Y, unsigned Number)
{
	PCB_COORD_ROTATE90(Point->X, Point->Y, X, Y, Number);
}

/* ----------------------------------------------------------------------
 * rotates an objects at the cursor position as identified by its ID
 * the center of rotation is determined by the current cursor location
 */
void *pcb_obj_rotate90(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t X, pcb_coord_t Y, unsigned Steps)
{
	void *ptr2;
	int changed = 0;
	pcb_opctx_t ctx;

	/* setup default  global identifiers */
	ctx.rotate.pcb = PCB;
	ctx.rotate.number = Steps;
	ctx.rotate.center_x = X;
	ctx.rotate.center_y = Y;

	pcb_event(PCB_EVENT_RUBBER_ROTATE90, "ipppccip", Type, Ptr1, Ptr2, Ptr2, ctx.rotate.center_x, ctx.rotate.center_y, ctx.rotate.number, &changed);

	pcb_undo_add_obj_to_rotate(Type, Ptr1, Ptr2, Ptr3, ctx.rotate.center_x, ctx.rotate.center_y, ctx.rotate.number);
	ptr2 = pcb_object_operation(&RotateFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	changed |= (ptr2 != NULL);
	if (changed) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	return (ptr2);
}

void pcb_screen_obj_rotate90(pcb_coord_t X, pcb_coord_t Y, unsigned Steps)
{
	int type;
	void *ptr1, *ptr2, *ptr3;
	if ((type = pcb_search_screen(X, Y, PCB_ROTATE_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE) {
		if (PCB_FLAG_TEST(PCB_FLAG_LOCK, (pcb_arc_t *) ptr2)) {
			pcb_message(PCB_MSG_DEFAULT, _("Sorry, the object is locked\n"));
			return;
		}
		pcb_event(PCB_EVENT_RUBBER_RESET, NULL);
		if (conf_core.editor.rubber_band_mode)
			pcb_rubber_band_lookup_lines(type, ptr1, ptr2, ptr3);
		if (type == PCB_TYPE_ELEMENT)
			pcb_rubber_band_lookup_rat_lines(type, ptr1, ptr2, ptr3);
		pcb_obj_rotate90(type, ptr1, ptr2, ptr3, X, Y, Steps);
		pcb_board_set_changed_flag(pcb_true);
	}
}
