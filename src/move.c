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
	pcb_textop_move,
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
	pcb_subcop_move
};

pcb_opfunc_t MoveFunctions_noclip = {
	pcb_lineop_move_noclip,
	pcb_textop_move_noclip,
	pcb_polyop_move_noclip,
	pcb_viaop_move_noclip,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	pcb_arcop_move_noclip,
	NULL,
	NULL,
	NULL
};

static pcb_opfunc_t MoveToLayerFunctions = {
	pcb_lineop_move_to_layer,
	pcb_textop_move_to_layer,
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
	pcb_opctx_t ctx;
	void *ptr2;

	if ((DX == 0) && (DY == 0) && (conf_core.editor.move_linepoint_uses_route == 0))
		return NULL;

	ctx.move.dx = DX;
	ctx.move.dy = DY;

	pcb_draw_inhibit_inc();

	switch(Type) {
		case PCB_TYPE_ARC_POINT :
			{
				/* Get the initial arc point positions */
				pcb_arc_t * p_arc = ((pcb_arc_t *)pcb_crosshair.AttachedObject.Ptr2);
				pcb_coord_t ox1,ox2,oy1,oy2;
				pcb_coord_t nx1,nx2,ny1,ny2;

				pcb_arc_get_end(p_arc,0, &ox1, &oy1);
				pcb_arc_get_end(p_arc,1, &ox2, &oy2);

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

				/* Get the new arc point positions so that we can calculate the position deltas */
				pcb_arc_get_end(p_arc,0, &nx1, &ny1);
				pcb_arc_get_end(p_arc,1, &nx2, &ny2);
				pcb_event(PCB_EVENT_RUBBER_MOVE, "icccc", 0, nx1-ox1, ny1-oy1, nx2-ox2, ny2-oy2);
			}
			break;

		case PCB_TYPE_ARC :
			pcb_event(PCB_EVENT_RUBBER_MOVE, "icccc", 0, DX, DY, DX, DY);
			pcb_undo_add_obj_to_move(Type, Ptr1, Ptr2, Ptr3, DX, DY);
			ptr2 = pcb_object_operation(&MoveFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
			break;

		case PCB_TYPE_LINE_POINT :
			pcb_event(PCB_EVENT_RUBBER_MOVE, "icc", 0, DX, DY);
			ptr2 = pcb_lineop_move_point_with_route(&ctx, Ptr1, Ptr2, Ptr3);
			break;

		case PCB_TYPE_LINE :
			{
				pcb_coord_t dx1 = DX;
				pcb_coord_t dy1 = DY;
				pcb_coord_t dx2 = DX;
				pcb_coord_t dy2 = DY;
				pcb_line_t *line = (pcb_line_t*) Ptr2;
				int constrained = 0;

				if(conf_core.editor.rubber_band_keep_midlinedir)
					pcb_event(PCB_EVENT_RUBBER_CONSTRAIN_MAIN_LINE, "pppppp", line, &constrained, &dx1, &dy1, &dx2, &dy2);
				pcb_event(PCB_EVENT_RUBBER_MOVE, "icccc", constrained, dx1, dy1, dx2, dy2);

				ctx.move.dx = dx1;
				ctx.move.dy = dy1;

				/* If the line ends have moved indpendently then move the individual points */
				if((dx1 != dx2) || (dy1 != dy2)) {
					/* Move point1 form line */
					pcb_undo_add_obj_to_move(PCB_TYPE_LINE_POINT,
							 Ptr1, line, &line->Point1,
							 dx1, dy1);
					pcb_lineop_move_point(&ctx, Ptr1, line, &line->Point1);

					/* Move point2 form line */
					pcb_undo_add_obj_to_move(PCB_TYPE_LINE_POINT,
								 Ptr1, line, &line->Point2,
								 dx2, dy2);
					ctx.move.dx = dx2;
					ctx.move.dy = dy2;
					ptr2 = pcb_lineop_move_point(&ctx, Ptr1, line, &line->Point2);
				}
				/* Otherwise make a normal move */
				else {
					pcb_undo_add_obj_to_move(Type, Ptr1, Ptr2, Ptr3, dx1, dy1);
					ptr2 = pcb_object_operation(&MoveFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
				}
			}	
			break;

		default :	
			pcb_event(PCB_EVENT_RUBBER_MOVE, "icc", 0, DX, DY);
			pcb_undo_add_obj_to_move(Type, Ptr1, Ptr2, Ptr3, DX, DY);
			ptr2 = pcb_object_operation(&MoveFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
			break;
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
