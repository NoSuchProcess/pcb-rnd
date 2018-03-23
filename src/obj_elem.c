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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* Drawing primitive: elements */

#include "config.h"

#include "board.h"
#include "data.h"
#include "list_common.h"
#include "plug_io.h"
#include "conf_core.h"
#include "compat_nls.h"
#include "compat_misc.h"
#include "remove.h"
#include "polygon.h"
#include "undo.h"
#include "obj_pinvia_op.h"
#include "obj_pad_op.h"

#include "obj_pinvia_draw.h"
#include "obj_pad_draw.h"
#include "obj_line_draw.h"
#include "obj_arc_draw.h"

#include "obj_elem.h"
#include "obj_elem_op.h"
#include "obj_subc_list.h"

/* TODO: remove this: */
#include "draw.h"
#include "obj_text_draw.h"

/*** utility ***/

void *pcb_element_remove(pcb_element_t *Element)
{
	void *res;
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.destroy_target = NULL;
	PCB_CLEAR_PARENT(Element);

	res = pcb_elemop_remove(&ctx, Element);
	pcb_draw();
	return res;
}

/*** ops ***/

/* removes an element */
void *pcb_elemop_remove(pcb_opctx_t *ctx, pcb_element_t *Element)
{
	pcb_undo_move_obj_to_remove(PCB_TYPE_ELEMENT, Element, Element, Element);
	return NULL;
}
