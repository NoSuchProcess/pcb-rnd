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

#include "board.h"
#include "draw.h"
#include "remove.h"
#include "select.h"
#include "undo.h"
#include "obj_arc_op.h"
#include "obj_line_op.h"
#include "obj_poly_op.h"
#include "obj_text_op.h"
#include "obj_rat_op.h"
#include "obj_subc_op.h"
#include "obj_pstk_op.h"

static pcb_opfunc_t RemoveFunctions = {
	pcb_lineop_remove,
	pcb_textop_remove,
	pcb_polyop_remove,
	pcb_lineop_remove_point,
	pcb_polyop_remove_point,
	pcb_arcop_remove,
	pcb_ratop_remove,
	pcb_arcop_remove_point,
	pcb_subcop_remove,
	pcb_pstkop_remove
};

static pcb_opfunc_t DestroyFunctions = {
	pcb_lineop_destroy,
	pcb_textop_destroy,
	pcb_polyop_destroy,
	NULL,
	pcb_polyop_destroy_point,
	pcb_arcop_destroy,
	pcb_ratop_destroy,
	NULL,
	pcb_subcop_destroy,
	pcb_pstkop_destroy,
};

/* ----------------------------------------------------------------------
 * removes all selected and visible objects
 * returns pcb_true if any objects have been removed
 */
pcb_bool pcb_remove_selected(pcb_bool locked_too)
{
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.destroy_target = NULL;

	if (pcb_selected_operation(PCB, PCB->Data, &RemoveFunctions, &ctx, pcb_false, PCB_OBJ_ANY & (~PCB_OBJ_SUBC_PART), locked_too)) {
		pcb_undo_inc_serial();
		pcb_draw();
		return pcb_true;
	}
	return pcb_false;
}

/* ---------------------------------------------------------------------------
 * remove object as referred by pointers and type,
 * allocated memory is passed to the 'remove undo' list
 */
void *pcb_remove_object(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;
	void *ptr;

	ctx.remove.pcb = PCB;
	ctx.remove.destroy_target = NULL;

	ptr = pcb_object_operation(&RemoveFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	pcb_draw();
	return ptr;
}

void *pcb_destroy_object(pcb_data_t *Target, pcb_objtype_t Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	void *res;
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.destroy_target = Target;

	res = pcb_object_operation(&DestroyFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	pcb_draw();
	return res;
}
