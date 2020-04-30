/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

#include "config.h"
#include "conf_core.h"

#include <stdlib.h>

#include "board.h"
#include "data.h"
#include "draw.h"
#include <librnd/core/error.h>
#include "move.h"
#include "select.h"
#include "undo.h"
#include "event.h"
#include <librnd/core/actions.h>
#include <librnd/core/compat_misc.h>
#include "obj_arc_op.h"
#include "obj_line_op.h"
#include "obj_text_op.h"
#include "obj_subc_op.h"
#include "obj_poly_op.h"
#include "obj_pstk_op.h"
#include "obj_rat_op.h"
#include "obj_gfx_op.h"

pcb_opfunc_t MoveFunctions = {
	NULL, /* common_pre */
	NULL, /* common_post */
	pcb_lineop_move,
	pcb_textop_move,
	pcb_polyop_move,
	pcb_lineop_move_point,
	pcb_polyop_move_point,
	pcb_arcop_move,
	pcb_gfxop_move,
	NULL,
	NULL,
	pcb_subcop_move,
	pcb_pstkop_move,
	0 /* extobj_inhibit_regen */
};

pcb_opfunc_t MoveFunctions_noclip = {
	NULL, /* common_pre */
	NULL, /* common_post */
	pcb_lineop_move_noclip,
	pcb_textop_move_noclip,
	pcb_polyop_move_noclip,
	NULL,
	NULL,
	pcb_arcop_move_noclip,
	pcb_gfxop_move_noclip,
	NULL,
	NULL,
	NULL, /* subc */
	pcb_pstkop_move_noclip,
	0 /* extobj_inhibit_regen */
};

pcb_opfunc_t ClipFunctions = {
	NULL, /* common_pre */
	NULL, /* common_post */
	pcb_lineop_clip,
	pcb_textop_clip,
	pcb_polyop_clip,
	NULL,
	NULL,
	pcb_arcop_clip,
	NULL, /* gfx */
	NULL,
	NULL,
	NULL, /* subc */
	pcb_pstkop_clip,
	1 /* extobj_inhibit_regen */
};

static pcb_opfunc_t MoveToLayerFunctions = {
	NULL, /* common_pre */
	NULL, /* common_post */
	pcb_lineop_move_to_layer,
	pcb_textop_move_to_layer,
	pcb_polyop_move_to_layer,
	NULL,
	NULL,
	pcb_arcop_move_to_layer,
	pcb_gfxop_move_to_layer,
	pcb_ratop_move_to_layer,
	NULL,
	NULL, /* subc */
	NULL, /* padstack */
	0 /* extobj_inhibit_regen */
};

static pcb_opfunc_t CopyFunctions = {
	NULL, /* common_pre */
	NULL, /* common_post */
	pcb_lineop_copy,
	pcb_textop_copy,
	pcb_polyop_copy,
	NULL,
	NULL,
	pcb_arcop_copy,
	pcb_gfxop_copy,
	NULL,
	NULL,
	pcb_subcop_copy,
	pcb_pstkop_copy,
	0 /* extobj_inhibit_regen */
};


void *pcb_move_obj(int Type, void *Ptr1, void *Ptr2, void *Ptr3, rnd_coord_t DX, rnd_coord_t DY)
{
	void *result;
	pcb_opctx_t ctx;

	ctx.move.pcb = PCB;
	ctx.move.dx = DX;
	ctx.move.dy = DY;
	pcb_undo_add_obj_to_move(Type, Ptr1, Ptr2, Ptr3, DX, DY);
	result = pcb_object_operation(&MoveFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	return result;
}

void *pcb_move_obj_and_rubberband(int Type, void *Ptr1, void *Ptr2, void *Ptr3, rnd_coord_t DX, rnd_coord_t DY)
{
	pcb_opctx_t ctx;
	void *ptr2;

	if ((DX == 0) && (DY == 0) && (conf_core.editor.move_linepoint_uses_route == 0))
		return NULL;

	ctx.move.pcb = PCB;
	ctx.move.dx = DX;
	ctx.move.dy = DY;

	pcb_data_clip_inhibit_inc(PCB->Data);
	pcb_draw_inhibit_inc();

	switch(Type) {
		case PCB_OBJ_ARC_POINT:
			{
				/* Get the initial arc point positions */
				pcb_arc_t * p_arc = ((pcb_arc_t *)pcb_crosshair.AttachedObject.Ptr2);
				rnd_coord_t ox1,ox2,oy1,oy2;
				rnd_coord_t nx1,nx2,ny1,ny2;

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
				rnd_event(&PCB->hidlib, PCB_EVENT_RUBBER_MOVE, "icccc", 0, nx1-ox1, ny1-oy1, nx2-ox2, ny2-oy2);
				ptr2 = p_arc;
			}
			break;

		case PCB_OBJ_ARC:
			rnd_event(&PCB->hidlib, PCB_EVENT_RUBBER_MOVE, "icccc", 0, DX, DY, DX, DY);
			pcb_undo_add_obj_to_move(Type, Ptr1, Ptr2, Ptr3, DX, DY);
			ptr2 = pcb_object_operation(&MoveFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
			break;

		case PCB_OBJ_LINE_POINT :
			rnd_event(&PCB->hidlib, PCB_EVENT_RUBBER_MOVE, "icc", 0, DX, DY);
			ptr2 = pcb_lineop_move_point_with_route(&ctx, Ptr1, Ptr2, Ptr3);
			break;

		case PCB_OBJ_LINE:
			{
				rnd_coord_t dx1 = DX;
				rnd_coord_t dy1 = DY;
				rnd_coord_t dx2 = DX;
				rnd_coord_t dy2 = DY;
				pcb_line_t *line = (pcb_line_t*) Ptr2;
				int constrained = 0;

				if(conf_core.editor.rubber_band_keep_midlinedir)
					rnd_event(&PCB->hidlib, PCB_EVENT_RUBBER_CONSTRAIN_MAIN_LINE, "pppppp", line, &constrained, &dx1, &dy1, &dx2, &dy2);
				rnd_event(&PCB->hidlib, PCB_EVENT_RUBBER_MOVE, "icccc", constrained, dx1, dy1, dx2, dy2);

				ctx.move.dx = dx1;
				ctx.move.dy = dy1;

				/* If the line ends have moved indpendently then move the individual points */
				if((dx1 != dx2) || (dy1 != dy2)) {
					/* Move point1 form line */
					pcb_undo_add_obj_to_move(PCB_OBJ_LINE_POINT,
							 Ptr1, line, &line->Point1,
							 dx1, dy1);
					pcb_lineop_move_point(&ctx, Ptr1, line, &line->Point1);

					/* Move point2 form line */
					pcb_undo_add_obj_to_move(PCB_OBJ_LINE_POINT,
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

		default:
			rnd_event(&PCB->hidlib, PCB_EVENT_RUBBER_MOVE, "icc", 0, DX, DY);
			pcb_undo_add_obj_to_move(Type, Ptr1, Ptr2, Ptr3, DX, DY);
			ptr2 = pcb_object_operation(&MoveFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
			break;
	}

	pcb_undo_inc_serial();

	pcb_draw_inhibit_dec();
	pcb_data_clip_inhibit_dec(PCB->Data, 0);
	pcb_draw();

	return ptr2;
}

void *pcb_move_obj_to_layer(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_layer_t *Target, rnd_bool enmasse)
{
	void *result;
	pcb_opctx_t ctx;

	ctx.move.pcb = PCB;
	ctx.move.dst_layer = Target;
	ctx.move.more_to_come = enmasse;

	result = pcb_object_operation(&MoveToLayerFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	pcb_undo_inc_serial();
	return result;
}

rnd_bool pcb_move_selected_objs_to_layer(pcb_layer_t *Target)
{
	rnd_bool changed;
	pcb_opctx_t ctx;

	ctx.move.pcb = PCB;
	ctx.move.dst_layer = Target;
	ctx.move.more_to_come = pcb_true;

	changed = pcb_selected_operation(PCB, PCB->Data, &MoveToLayerFunctions, &ctx, pcb_true, PCB_OBJ_ANY, pcb_false);
	/* passing pcb_true to above operation causes Undoserial to auto-increment */
	return changed;
}

void *pcb_copy_obj(int Type, void *Ptr1, void *Ptr2, void *Ptr3, rnd_coord_t DX, rnd_coord_t DY)
{
	void *ptr;
	pcb_opctx_t ctx;

	ctx.copy.pcb = PCB;
	ctx.copy.DeltaX = DX;
	ctx.copy.DeltaY = DY;
	ctx.copy.from_outside = 0;
	ctx.copy.keep_id = 0;

	/* the subroutines add the objects to the undo-list */
	ptr = pcb_object_operation(&CopyFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	pcb_undo_inc_serial();
	return ptr;
}
