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
#include "event.h"
#include "hid_actions.h"
#include "compat_misc.h"
#include "obj_all_op.h"

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
pcb_opfunc_t MoveFunctions = {
	pcb_lineop_move,
	MoveText,
	pcb_polyop_move,
	pcb_viaop_move,
	pcb_elemop_move,
	pcb_elemop_move_name,
	NULL,
	NULL,
	pcb_lineop_move_point,
	pcb_polyop_move_point,
	pcb_arcop_move,
	NULL,
	NULL,
	MoveSubc
};

static pcb_opfunc_t MoveToLayerFunctions = {
	pcb_lineop_move_to_layer,
	MoveTextToLayer,
	pcb_polyop_move_to_layer,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	pcb_arcop_move_to_layer,
	pcb_ratop_move_to_layer,
	NULL,
	NULL
};

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
	pcb_undo_add_obj_to_move(Type, Ptr1, Ptr2, Ptr3, DX, DY);
	result = pcb_object_operation(&MoveFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	return (result);
}

/* ---------------------------------------------------------------------------
 * moves the object identified by its data pointers and the type
 * as well as all attached rubberband lines
 */
void *pcb_move_obj_and_rubberband(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t DX, pcb_coord_t DY)
{
	pcb_opctx_t ctx1;
	pcb_opctx_t ctx2;
	void *ptr2;

	ctx1.move.pcb = ctx2.move.pcb = PCB;
	ctx1.move.dx = ctx2.move.dx = DX;
	ctx1.move.dy = ctx2.move.dy = DY;

	pcb_event(PCB_EVENT_RUBBER_MOVE, "ipp", Type, &ctx1, &ctx2);

	if (DX == 0 && DY == 0)
		return NULL;

	pcb_draw_inhibit_inc();

	if (Type == PCB_TYPE_ARC_POINT) {
		/* moving the endpoint of an arc is not really a move, but a change of arc properties */
		if (pcb_crosshair.AttachedObject.radius == 0)
			pcb_arc_set_angles((pcb_layer_t *)Ptr1, (pcb_arc_t *)Ptr2,
					   pcb_crosshair.AttachedObject.start_angle,
					   pcb_crosshair.AttachedObject.delta_angle);
		else
			pcb_arc_set_radii((pcb_layer_t *)Ptr1, (pcb_arc_t *)Ptr2,
					  pcb_crosshair.AttachedObject.radius,
					  pcb_crosshair.AttachedObject.radius);
		pcb_crosshair.AttachedObject.radius = 0;
	}
	else {
		/* If rubberband has adapted movement, move accordingly */
		if (Type == PCB_TYPE_LINE &&
		    (ctx1.move.dx != ctx2.move.dx || ctx1.move.dy != ctx2.move.dy)) {
		    	pcb_line_t *line = (pcb_line_t*) Ptr2;

		    	/* Move point1 form line */
			pcb_undo_add_obj_to_move(PCB_TYPE_LINE_POINT,
						 Ptr1, line, &line->Point1,
						 ctx1.move.dx, ctx1.move.dy);
			pcb_lineop_move_point(&ctx1, Ptr1, line, &line->Point1);

			/* Move point2 form line */
			pcb_undo_add_obj_to_move(PCB_TYPE_LINE_POINT,
						 Ptr1, line, &line->Point2,
						 ctx2.move.dx, ctx2.move.dy);
			ptr2 = pcb_lineop_move_point(&ctx2, Ptr1, line, &line->Point2);
		}
		/* Otherwise make a normal move */
		else if(Type == PCB_TYPE_LINE_POINT) {
			ptr2 = pcb_lineop_move_point_with_route(&ctx1, Ptr1, Ptr2, Ptr3);
		}
		else {
			pcb_undo_add_obj_to_move(Type, Ptr1, Ptr2, Ptr3, DX, DY);
			ptr2 = pcb_object_operation(&MoveFunctions, &ctx1, Type, Ptr1, Ptr2, Ptr3);
		}
	}
	pcb_undo_inc_serial();

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

	result = pcb_object_operation(&MoveToLayerFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	pcb_undo_inc_serial();
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

	changed = pcb_selected_operation(PCB, &MoveToLayerFunctions, &ctx, pcb_true, PCB_TYPEMASK_ALL);
	/* passing pcb_true to above operation causes Undoserial to auto-increment */
	return (changed);
}
