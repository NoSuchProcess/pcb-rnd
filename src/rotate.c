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

#include <stdlib.h>

#include "board.h"
#include "data.h"
#include "draw.h"
#include <librnd/core/error.h>
#include "polygon.h"
#include "rotate.h"
#include "search.h"
#include "select.h"
#include "undo.h"
#include "event.h"
#include "data.h"
#include "conf_core.h"
#include "obj_arc_op.h"
#include "obj_line_op.h"
#include "obj_poly_op.h"
#include "obj_text_op.h"
#include "obj_subc_op.h"
#include "obj_pstk_op.h"
#include "obj_gfx_op.h"

#include "obj_line_draw.h"
#include "obj_rat_draw.h"

pcb_opfunc_t Rotate90Functions = {
	NULL, /* common_pre */
	NULL, /* common_post */
	pcb_lineop_rotate90,
	pcb_textop_rotate90,
	pcb_polyop_rotate90,
	pcb_lineop_rotate90_point,
	NULL,
	pcb_arcop_rotate90,
	pcb_gfxop_rotate90,
	NULL,
	NULL,
	pcb_subcop_rotate90,
	pcb_pstkop_rotate90,
	0 /* extobj_inhibit_regen */
};

pcb_opfunc_t RotateFunctions = {
	NULL, /* common_pre */
	NULL, /* common_post */
	pcb_lineop_rotate,
	pcb_textop_rotate,
	pcb_polyop_rotate,
	NULL,
	NULL,
	pcb_arcop_rotate,
	pcb_gfxop_rotate,
	NULL,
	NULL,
	pcb_subcop_rotate,
	pcb_pstkop_rotate,
	0 /* extobj_inhibit_regen */
};

/* rotates a point in 90 degree steps */
void pcb_point_rotate90(rnd_point_t *Point, rnd_coord_t X, rnd_coord_t Y, unsigned Number)
{
	RND_COORD_ROTATE90(Point->X, Point->Y, X, Y, Number);
}

pcb_any_obj_t *pcb_obj_rotate90(pcb_board_t *pcb, pcb_any_obj_t *obj, rnd_coord_t X, rnd_coord_t Y, unsigned Steps)
{
	int changed = 0;
	pcb_opctx_t ctx;

	pcb_data_clip_inhibit_inc(pcb->Data);

	/* setup default  global identifiers */
	ctx.rotate.pcb = pcb;
	ctx.rotate.number = Steps;
	ctx.rotate.center_x = X;
	ctx.rotate.center_y = Y;

	rnd_event(&pcb->hidlib, PCB_EVENT_RUBBER_ROTATE90, "ipppccip", obj->type, obj->parent.any, obj, obj, ctx.rotate.center_x, ctx.rotate.center_y, ctx.rotate.number, &changed);

	if (obj->type != PCB_OBJ_PSTK) /* padstack has its own way doing the rotation-undo */
		pcb_undo_add_obj_to_rotate90(obj->type, obj->parent.any, obj, obj, ctx.rotate.center_x, ctx.rotate.center_y, ctx.rotate.number);
	obj = pcb_object_operation(&Rotate90Functions, &ctx, obj->type, obj->parent.any, obj, obj);
	changed |= (obj != NULL);
	if (changed) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	pcb_data_clip_inhibit_dec(pcb->Data, 0);
	return obj;
}

pcb_any_obj_t *pcb_obj_rotate(pcb_board_t *pcb, pcb_any_obj_t *obj, rnd_coord_t X, rnd_coord_t Y, rnd_angle_t angle)
{
	int changed = 0;
	pcb_opctx_t ctx;

	pcb_data_clip_inhibit_inc(pcb->Data);

	/* setup default  global identifiers */
	ctx.rotate.pcb = pcb;
	ctx.rotate.angle = angle;
	ctx.rotate.center_x = X;
	ctx.rotate.center_y = Y;

	rnd_event(&pcb->hidlib, PCB_EVENT_RUBBER_ROTATE, "ipppccip", obj->type, obj->parent.any, obj, obj, ctx.rotate.center_x, ctx.rotate.center_y, ctx.rotate.angle, &changed);

	if (obj->type != PCB_OBJ_PSTK) /* padstack has its own way doing the rotation-undo */
		pcb_undo_add_obj_to_rotate(obj->type, obj->parent.any, obj, obj, ctx.rotate.center_x, ctx.rotate.center_y, ctx.rotate.angle);
	obj = pcb_object_operation(&RotateFunctions, &ctx, obj->type, obj->parent.any, obj, obj);
	changed |= (obj != NULL);
	if (changed) {
		pcb_draw();
		pcb_undo_inc_serial();
	}
	pcb_data_clip_inhibit_dec(pcb->Data, 0);
	return obj;
}

void pcb_screen_obj_rotate90(pcb_board_t *pcb, rnd_coord_t X, rnd_coord_t Y, unsigned Steps)
{
	int type;
	void *ptr1, *ptr2, *ptr3;
	if ((type = pcb_search_screen(X, Y, PCB_ROTATE_TYPES | PCB_LOOSE_SUBC(PCB), &ptr1, &ptr2, &ptr3)) != PCB_OBJ_VOID) {
		pcb_any_obj_t *obj = (pcb_any_obj_t *)ptr2;
		if (PCB_FLAG_TEST(PCB_FLAG_LOCK, obj)) {
			rnd_message(RND_MSG_WARNING, "Sorry, %s object is locked\n", pcb_obj_type_name(obj->type));
			return;
		}
		rnd_event(&pcb->hidlib, PCB_EVENT_RUBBER_RESET, NULL);
		if (conf_core.editor.rubber_band_mode)
			rnd_event(&pcb->hidlib, PCB_EVENT_RUBBER_LOOKUP_LINES, "ippp", type, ptr1, ptr2, ptr3);
		if (type == PCB_OBJ_SUBC)
			rnd_event(&pcb->hidlib, PCB_EVENT_RUBBER_LOOKUP_RATS, "ippp", type, ptr1, ptr2, ptr3);
		pcb_obj_rotate90(pcb, obj, X, Y, Steps);
		pcb_board_set_changed_flag(pcb, rnd_true);
	}
}

void pcb_screen_obj_rotate(pcb_board_t *pcb, rnd_coord_t X, rnd_coord_t Y, rnd_angle_t angle)
{
	int type;
	void *ptr1, *ptr2, *ptr3;
	if ((type = pcb_search_screen(X, Y, PCB_ROTATE_TYPES | PCB_LOOSE_SUBC(PCB), &ptr1, &ptr2, &ptr3)) != PCB_OBJ_VOID) {
		pcb_any_obj_t *obj = (pcb_any_obj_t *)ptr2;
		if (PCB_FLAG_TEST(PCB_FLAG_LOCK, obj)) {
			rnd_message(RND_MSG_WARNING, "Sorry, %s object is locked\n", pcb_obj_type_name(obj->type));
			return;
		}
		rnd_event(&pcb->hidlib, PCB_EVENT_RUBBER_RESET, NULL);
		if (conf_core.editor.rubber_band_mode)
			rnd_event(&pcb->hidlib, PCB_EVENT_RUBBER_LOOKUP_LINES, "ippp", type, ptr1, ptr2, ptr3);
		if (type == PCB_OBJ_SUBC)
			rnd_event(&pcb->hidlib, PCB_EVENT_RUBBER_LOOKUP_RATS, "ippp", type, ptr1, ptr2, ptr3);
		pcb_obj_rotate(PCB, obj, X, Y, angle);
		pcb_board_set_changed_flag(PCB, rnd_true);
	}
}
