/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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


/* functions used to insert points into objects */

#include "config.h"
#include "conf_core.h"

#include "board.h"
#include "data.h"
#include "select.h"
#include "undo.h"

#include "obj_line_op.h"
#include "obj_arc_op.h"
#include "obj_rat_op.h"
#include "obj_poly_op.h"

static pcb_opfunc_t InsertFunctions = {
	NULL, /* common_pre */
	NULL, /* common_post */
	pcb_lineop_insert_point,
	NULL,
	pcb_polyop_insert_point,
	NULL,
	NULL,
	pcb_arc_insert_point,
	NULL, /* gfx */
	pcb_ratop_insert_point,
	NULL,
	NULL, /* subc */
	NULL,  /* padstack */
	0 /* extobj_inhibit_regen */
};

void *pcb_insert_point_in_object(int Type, void *Ptr1, void *Ptr2, rnd_cardinal_t *Ptr3, rnd_coord_t DX, rnd_coord_t DY, rnd_bool Force, rnd_bool insert_last)
{
	void *ptr;
	pcb_opctx_t ctx;

	ctx.insert.pcb = PCB;
	ctx.insert.x = DX;
	ctx.insert.y = DY;
	ctx.insert.idx = *Ptr3;
	ctx.insert.last = insert_last;
	ctx.insert.forcible = Force;

	/* the operation insert the points to the undo-list */
	ptr = pcb_object_operation(&InsertFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	if (ptr != NULL)
		pcb_undo_inc_serial();
	return ptr;
}

rnd_point_t *pcb_adjust_insert_point(void)
{
	static rnd_point_t InsertedPoint;

	InsertedPoint.X = pcb_crosshair.X;
	InsertedPoint.Y = pcb_crosshair.Y;
	return &InsertedPoint;
}
