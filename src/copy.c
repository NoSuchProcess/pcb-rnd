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

/* functions used to copy objects
 * it's necessary to copy data by calling create... since the base pointer
 * may change cause of dynamic memory allocation
 */

#include "config.h"
#include "conf_core.h"

#include "board.h"
#include "data.h"
#include "draw.h"
#include "select.h"
#include "undo.h"
#include "obj_arc_op.h"
#include "obj_line_op.h"
#include "obj_poly_op.h"
#include "obj_text_op.h"
#include "obj_subc_op.h"
#include "obj_pstk_op.h"

static pcb_opfunc_t CopyFunctions = {
	pcb_lineop_copy,
	pcb_textop_copy,
	pcb_polyop_copy,
	NULL,
	NULL,
	pcb_arcop_copy,
	NULL,
	NULL,
	pcb_subcop_copy,
	pcb_pstkop_copy
};

void *pcb_copy_obj(int Type, void *Ptr1, void *Ptr2, void *Ptr3, pcb_coord_t DX, pcb_coord_t DY)
{
	void *ptr;
	pcb_opctx_t ctx;

	ctx.copy.pcb = PCB;
	ctx.copy.DeltaX = DX;
	ctx.copy.DeltaY = DY;
	ctx.copy.from_outside = 0;

	/* the subroutines add the objects to the undo-list */
	ptr = pcb_object_operation(&CopyFunctions, &ctx, Type, Ptr1, Ptr2, Ptr3);
	pcb_undo_inc_serial();
	return ptr;
}
