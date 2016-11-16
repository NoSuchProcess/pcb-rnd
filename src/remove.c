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

/* functions used to remove vias, pins ... */

#include "config.h"
#include "conf_core.h"

#include "board.h"
#include "draw.h"
#include "remove.h"
#include "select.h"
#include "undo.h"
#include "obj_all_op.h"

/* ---------------------------------------------------------------------------
 * some local types
 */
static pcb_opfunc_t RemoveFunctions = {
	RemoveLine_op,
	RemoveText_op,
	RemovePolygon_op,
	RemoveVia,
	RemoveElement_op,
	NULL,
	NULL,
	NULL,
	RemoveLinePoint,
	RemovePolygonPoint,
	RemoveArc_op,
	RemoveRat
};

static pcb_opfunc_t DestroyFunctions = {
	DestroyLine,
	DestroyText,
	DestroyPolygon,
	DestroyVia,
	DestroyElement,
	NULL,
	NULL,
	NULL,
	NULL,
	DestroyPolygonPoint,
	DestroyArc,
	DestroyRat
};

/* ----------------------------------------------------------------------
 * removes all selected and visible objects
 * returns pcb_true if any objects have been removed
 */
pcb_bool pcb_remove_selected(void)
{
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.bulk = pcb_true;
	ctx.remove.destroy_target = NULL;

	if (pcb_selected_operation(&RemoveFunctions, &ctx, pcb_false, PCB_TYPEMASK_ALL)) {
		IncrementUndoSerialNumber();
		pcb_draw();
		return (pcb_true);
	}
	return (pcb_false);
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
	ctx.remove.bulk = pcb_false;
	ctx.remove.destroy_target = NULL;

	ptr = pcb_object_operation(&RemoveFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	return (ptr);
}

/* ---------------------------------------------------------------------------
 * remove object as referred by pointers and type
 * allocated memory is destroyed assumed to already be erased
 */
void *pcb_destroy_object(pcb_data_t *Target, int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.bulk = pcb_false;
	ctx.remove.destroy_target = Target;

	return (pcb_object_operation(&DestroyFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3));
}
