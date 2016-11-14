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


/* functions used to move pins, elements ...
 */

#include "config.h"
#include "conf_core.h"

#include <stdlib.h>

#include "board.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "move.h"
#include "select.h"
#include "undo.h"
#include "hid_actions.h"
#include "compat_misc.h"
#include "obj_all_op.h"

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static pcb_opfunc_t MoveFunctions = {
	MoveLine,
	MoveText,
	MovePolygon,
	MoveVia,
	MoveElement,
	MoveElementName,
	NULL,
	NULL,
	MoveLinePoint,
	MovePolygonPoint,
	MoveArc,
	NULL
}, MoveToLayerFunctions = {
MoveLineToLayer, MoveTextToLayer, MovePolygonToLayer, NULL, NULL, NULL, NULL, NULL, NULL, NULL, MoveArcToLayer, MoveRatToLayer};

/* ---------------------------------------------------------------------------
 * moves the object identified by its data pointers and the type
 * not we don't bump the undo serial number
 */
void *pcb_move_obj(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t DX, pcb_coord_t DY)
{
	void *result;
	pcb_opctx_t ctx;

	ctx.move.pcb = PCB;
	ctx.move.dx = DX;
	ctx.move.dy = DY;
	AddObjectToMoveUndoList(Type, Ptr1, Ptr2, Ptr3, DX, DY);
	result = ObjectOperation(&MoveFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	return (result);
}

/* ---------------------------------------------------------------------------
 * moves the object identified by its data pointers and the type
 * as well as all attached rubberband lines
 */
void *pcb_move_obj_and_rubberband(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t DX, pcb_coord_t DY)
{
	pcb_rubberband_t *ptr;
	pcb_opctx_t ctx;
	void *ptr2;

	pcb_draw_inhibit_inc();

	ctx.move.pcb = PCB;
	ctx.move.dx = DX;
	ctx.move.dy = DY;

	if (DX == 0 && DY == 0) {
		int n;

		/* first clear any marks that we made in the line flags */
		for(n = 0, ptr = Crosshair.AttachedObject.Rubberband; n < Crosshair.AttachedObject.RubberbandN; n++, ptr++)
			PCB_FLAG_CLEAR(PCB_FLAG_RUBBEREND, ptr->Line);

		return (NULL);
	}

	/* move all the lines... and reset the counter */
	ptr = Crosshair.AttachedObject.Rubberband;
	while (Crosshair.AttachedObject.RubberbandN) {
		/* first clear any marks that we made in the line flags */
		PCB_FLAG_CLEAR(PCB_FLAG_RUBBEREND, ptr->Line);
		AddObjectToMoveUndoList(PCB_TYPE_LINE_POINT, ptr->Layer, ptr->Line, ptr->MovedPoint, DX, DY);
		MoveLinePoint(&ctx, ptr->Layer, ptr->Line, ptr->MovedPoint);
		Crosshair.AttachedObject.RubberbandN--;
		ptr++;
	}

	AddObjectToMoveUndoList(Type, Ptr1, Ptr2, Ptr3, DX, DY);
	ptr2 = ObjectOperation(&MoveFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	IncrementUndoSerialNumber();

	pcb_draw_inhibit_dec();
	pcb_draw();

	return (ptr2);
}

/* ---------------------------------------------------------------------------
 * moves the object identified by its data pointers and the type
 * to a new layer without changing it's position
 */
void *pcb_move_obj_to_layer(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_layer_t *Target, pcb_bool enmasse)
{
	void *result;
	pcb_opctx_t ctx;

	ctx.move.pcb = PCB;
	ctx.move.dst_layer = Target;
	ctx.move.more_to_come = enmasse;

	result = ObjectOperation(&MoveToLayerFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	IncrementUndoSerialNumber();
	return (result);
}

/* ---------------------------------------------------------------------------
 * moves the selected objects to a new layer without changing their
 * positions
 */
pcb_bool pcb_move_selected_objs_to_layer(pcb_layer_t *Target)
{
	pcb_bool changed;
	pcb_opctx_t ctx;

	ctx.move.pcb = PCB;
	ctx.move.dst_layer = Target;
	ctx.move.more_to_come = pcb_true;

	changed = SelectedOperation(&MoveToLayerFunctions, &ctx, pcb_true, PCB_TYPEMASK_ALL);
	/* passing pcb_true to above operation causes Undoserial to auto-increment */
	return (changed);
}
