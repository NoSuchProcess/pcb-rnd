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
#include "obj_elem_list.h"
#include "obj_elem_op.h"
#include "obj_subc_list.h"

/* TODO: remove this: */
#include "draw.h"
#include "obj_text_draw.h"
#include "obj_elem_draw.h"

/*** utility ***/

#warning subc TODO: move this to obj_subc.c
/* loads footprint data from file/library into buffer (as subcircuit)
 * returns pcb_false on error
 * if successful, update some other stuff and reposition the pastebuffer */
pcb_bool pcb_element_load_to_buffer(pcb_buffer_t *Buffer, const char *Name, const char *fmt)
{
	pcb_buffer_clear(PCB, Buffer);
	if (!pcb_parse_element(Buffer->Data, Name, fmt)) {
		if (conf_core.editor.show_solder_side)
			pcb_buffer_flip_side(PCB, Buffer);
		pcb_set_buffer_bbox(Buffer);

		Buffer->X = 0;
		Buffer->Y = 0;
		Buffer->from_outside = 1;

		if (pcb_subclist_length(&Buffer->Data->subc)) {
			pcb_subc_t *subc = pcb_subclist_first(&Buffer->Data->subc);
			pcb_subc_get_origin(subc, &Buffer->X, &Buffer->Y);
		}
		return pcb_true;
	}

	/* release memory which might have been acquired */
	pcb_buffer_clear(PCB, Buffer);
	return pcb_false;
}

#warning subc TODO: is this function needed?
/* Searches for the given element by "footprint" name, and loads it
   into the buffer. Returns zero on success, non-zero on error.  */
int pcb_element_load_footprint_by_name(pcb_buffer_t *Buffer, const char *Footprint)
{
	return !pcb_element_load_to_buffer(Buffer, Footprint, NULL);
}

/* see if a polygon is a rectangle.  If so, canonicalize it. */
static int polygon_is_rectangle(pcb_poly_t *poly)
{
	int i, best;
	pcb_point_t temp[4];
	if (poly->PointN != 4 || poly->HoleIndexN != 0)
		return 0;
	best = 0;
	for (i = 1; i < 4; i++)
		if (poly->Points[i].X < poly->Points[best].X || poly->Points[i].Y < poly->Points[best].Y)
			best = i;
	for (i = 0; i < 4; i++)
		temp[i] = poly->Points[(i + best) % 4];
	if (temp[0].X == temp[1].X)
		memcpy(poly->Points, temp, sizeof(temp));
	else {
		/* reverse them */
		poly->Points[0] = temp[0];
		poly->Points[1] = temp[3];
		poly->Points[2] = temp[2];
		poly->Points[3] = temp[1];
	}
	if (poly->Points[0].X == poly->Points[1].X
			&& poly->Points[1].Y == poly->Points[2].Y
			&& poly->Points[2].X == poly->Points[3].X && poly->Points[3].Y == poly->Points[0].Y)
		return 1;
	return 0;
}

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
	/* erase from screen */
	if ((pcb_silk_on(PCB) || PCB->PinOn) && (PCB_FRONT(Element) || PCB->InvisibleObjectsOn))
		pcb_elem_invalidate_erase(Element);
	pcb_undo_move_obj_to_remove(PCB_TYPE_ELEMENT, Element, Element, Element);
	return NULL;
}

/*** draw ***/

void pcb_elem_invalidate_erase(pcb_element_t *Element)
{
	PCB_ELEMENT_PCB_LINE_LOOP(Element);
	{
		pcb_line_invalidate_erase(line);
	}
	PCB_END_LOOP;
	PCB_ARC_LOOP(Element);
	{
		pcb_arc_invalidate_erase(arc);
	}
	PCB_END_LOOP;
	pcb_elem_name_invalidate_erase(Element);
	pcb_flag_erase(&Element->Flags);
}

void pcb_elem_name_invalidate_erase(pcb_element_t *Element)
{
	if (PCB_FLAG_TEST(PCB_FLAG_HIDENAME, Element)) {
		return;
	}
	pcb_text_invalidate_draw(NULL, &PCB_ELEM_TEXT_VISIBLE(PCB, Element));
}

void pcb_elem_invalidate_draw(pcb_element_t *Element)
{
	pcb_elem_name_invalidate_draw(Element);
}

void pcb_elem_name_invalidate_draw(pcb_element_t *Element)
{
	if (PCB_FLAG_TEST(PCB_FLAG_HIDENAME, Element))
		return;
	pcb_text_invalidate_draw(NULL, &PCB_ELEM_TEXT_VISIBLE(PCB, Element));
}
