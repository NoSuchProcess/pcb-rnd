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
#include "set.h"
#include "undo.h"
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
void RotatePointLowLevel(pcb_point_t *Point, pcb_coord_t X, pcb_coord_t Y, unsigned Number)
{
	ROTATE(Point->X, Point->Y, X, Y, Number);
}

/* ---------------------------------------------------------------------------
 * rotates a box in 90 degree steps
 */
void RotateBoxLowLevel(pcb_box_t *Box, pcb_coord_t X, pcb_coord_t Y, unsigned Number)
{
	pcb_coord_t x1 = Box->X1, y1 = Box->Y1, x2 = Box->X2, y2 = Box->Y2;

	ROTATE(x1, y1, X, Y, Number);
	ROTATE(x2, y2, X, Y, Number);
	Box->X1 = MIN(x1, x2);
	Box->Y1 = MIN(y1, y2);
	Box->X2 = MAX(x1, x2);
	Box->Y2 = MAX(y1, y2);
}

/* ----------------------------------------------------------------------
 * rotates an objects at the cursor position as identified by its ID
 * the center of rotation is determined by the current cursor location
 */
void *RotateObject(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t X, pcb_coord_t Y, unsigned Steps)
{
	pcb_rubberband_t *ptr;
	void *ptr2;
	pcb_bool changed = pcb_false;
	pcb_opctx_t ctx;

	/* setup default  global identifiers */
	ctx.rotate.pcb = PCB;
	ctx.rotate.number = Steps;
	ctx.rotate.center_x = X;
	ctx.rotate.center_y = Y;

	/* move all the rubberband lines... and reset the counter */
	ptr = Crosshair.AttachedObject.Rubberband;
	while (Crosshair.AttachedObject.RubberbandN) {
		changed = pcb_true;
		CLEAR_FLAG(PCB_FLAG_RUBBEREND, ptr->Line);
		AddObjectToRotateUndoList(PCB_TYPE_LINE_POINT, ptr->Layer, ptr->Line, ptr->MovedPoint, ctx.rotate.center_x, ctx.rotate.center_y, Steps);
		EraseLine(ptr->Line);
		if (ptr->Layer) {
			RestoreToPolygon(PCB->Data, PCB_TYPE_LINE, ptr->Layer, ptr->Line);
			r_delete_entry(ptr->Layer->line_tree, (pcb_box_t *) ptr->Line);
		}
		else
			r_delete_entry(PCB->Data->rat_tree, (pcb_box_t *) ptr->Line);
		RotatePointLowLevel(ptr->MovedPoint, ctx.rotate.center_x, ctx.rotate.center_y, Steps);
		SetLineBoundingBox(ptr->Line);
		if (ptr->Layer) {
			r_insert_entry(ptr->Layer->line_tree, (pcb_box_t *) ptr->Line, 0);
			ClearFromPolygon(PCB->Data, PCB_TYPE_LINE, ptr->Layer, ptr->Line);
			DrawLine(ptr->Layer, ptr->Line);
		}
		else {
			r_insert_entry(PCB->Data->rat_tree, (pcb_box_t *) ptr->Line, 0);
			DrawRat((pcb_rat_t *) ptr->Line);
		}
		Crosshair.AttachedObject.RubberbandN--;
		ptr++;
	}
	AddObjectToRotateUndoList(Type, Ptr1, Ptr2, Ptr3, ctx.rotate.center_x, ctx.rotate.center_y, ctx.rotate.number);
	ptr2 = ObjectOperation(&RotateFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	changed |= (ptr2 != NULL);
	if (changed) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (ptr2);
}

void RotateScreenObject(pcb_coord_t X, pcb_coord_t Y, unsigned Steps)
{
	int type;
	void *ptr1, *ptr2, *ptr3;
	if ((type = SearchScreen(X, Y, ROTATE_TYPES, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE) {
		if (TEST_FLAG(PCB_FLAG_LOCK, (pcb_arc_t *) ptr2)) {
			Message(PCB_MSG_DEFAULT, _("Sorry, the object is locked\n"));
			return;
		}
		Crosshair.AttachedObject.RubberbandN = 0;
		if (conf_core.editor.rubber_band_mode)
			LookupRubberbandLines(type, ptr1, ptr2, ptr3);
		if (type == PCB_TYPE_ELEMENT)
			LookupRatLines(type, ptr1, ptr2, ptr3);
		RotateObject(type, ptr1, ptr2, ptr3, X, Y, Steps);
		SetChangedFlag(pcb_true);
	}
}
