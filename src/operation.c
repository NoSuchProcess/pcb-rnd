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

#include "operation.h"
#include "board.h"
#include "data.h"
#include "conf_core.h"
#include "undo.h"
#include "rats.h"
#include "brave.h"

/* ----------------------------------------------------------------------
 * performs several operations on the passed object
 */
void *pcb_object_operation(pcb_opfunc_t *F, pcb_opctx_t *ctx, int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	switch (Type) {
	case PCB_OBJ_LINE:
		if (F->Line)
			return (F->Line(ctx, (pcb_layer_t *) Ptr1, (pcb_line_t *) Ptr2));
		break;

	case PCB_OBJ_ARC:
		if (F->Arc)
			return (F->Arc(ctx, (pcb_layer_t *) Ptr1, (pcb_arc_t *) Ptr2));
		break;

	case PCB_OBJ_LINE_POINT:
		if (F->LinePoint)
			return (F->LinePoint(ctx, (pcb_layer_t *) Ptr1, (pcb_line_t *) Ptr2, (pcb_point_t *) Ptr3));
		break;

	case PCB_OBJ_ARC_POINT:
		if (F->ArcPoint)
			return (F->ArcPoint(ctx, (pcb_layer_t *) Ptr1, (pcb_arc_t *) Ptr2, (int *) Ptr3));
		break;

	case PCB_OBJ_TEXT:
		if (F->Text)
			return (F->Text(ctx, (pcb_layer_t *) Ptr1, (pcb_text_t *) Ptr2));
		break;

	case PCB_OBJ_POLY:
		if (F->Polygon)
			return (F->Polygon(ctx, (pcb_layer_t *) Ptr1, (pcb_poly_t *) Ptr2));
		break;

	case PCB_OBJ_POLY_POINT:
		if (F->Point)
			return (F->Point(ctx, (pcb_layer_t *) Ptr1, (pcb_poly_t *) Ptr2, (pcb_point_t *) Ptr3));
		break;

	case PCB_OBJ_SUBC:
		if (F->subc)
			return (F->subc(ctx, (pcb_subc_t *) Ptr2));
		break;

	case PCB_OBJ_PSTK:
		if (F->padstack)
			return (F->padstack(ctx, (pcb_pstk_t *)Ptr2));
		break;

	case PCB_OBJ_RAT:
		if (F->Rat)
			return (F->Rat(ctx, (pcb_rat_t *) Ptr2));
		break;
	}
	return NULL;
}

/* ----------------------------------------------------------------------
 * performs several operations on selected objects which are also visible
 * The lowlevel procedures are passed together with additional information
 * resets the selected flag if requested
 * returns pcb_true if anything has changed
 */
pcb_bool pcb_selected_operation(pcb_board_t *pcb, pcb_data_t *data, pcb_opfunc_t *F, pcb_opctx_t *ctx, pcb_bool Reset, int type, pcb_bool on_locked_too)
{
	pcb_bool changed = pcb_false;

	if ((pcb_brave & PCB_BRAVE_CLIPBATCH) && (data != NULL))
		pcb_data_clip_inhibit_inc(data);

	/* check lines */
	if (type & PCB_OBJ_LINE && F->Line) {
		PCB_LINE_VISIBLE_LOOP(data);
		{
			if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, line))
				continue;
			if (!on_locked_too && PCB_FLAG_TEST(PCB_FLAG_LOCK, line))
				continue;
			if (Reset) {
				pcb_undo_add_obj_to_flag(line);
				PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, line);
			}
			F->Line(ctx, layer, line);
			changed = pcb_true;
		}
		PCB_ENDALL_LOOP;
	}

	/* check arcs */
	if (type & PCB_OBJ_ARC && F->Arc) {
		PCB_ARC_VISIBLE_LOOP(data);
		{
			if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, arc))
				continue;
			if (!on_locked_too && PCB_FLAG_TEST(PCB_FLAG_LOCK, arc))
				continue;
			if (Reset) {
				pcb_undo_add_obj_to_flag(arc);
				PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, arc);
			}
			F->Arc(ctx, layer, arc);
			changed = pcb_true;
		}
		PCB_ENDALL_LOOP;
	}

	/* check text */
	if (type & PCB_OBJ_TEXT && F->Text) {
		PCB_TEXT_ALL_LOOP(data);
		{
			if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, text) || !pcb_text_is_visible(PCB, layer, text))
				continue;
			if (!on_locked_too && PCB_FLAG_TEST(PCB_FLAG_LOCK, text))
				continue;
			if (Reset) {
				pcb_undo_add_obj_to_flag(text);
				PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, text);
			}
			F->Text(ctx, layer, text);
			changed = pcb_true;
		}
		PCB_ENDALL_LOOP;
	}

	/* check polygons */
	if (type & PCB_OBJ_POLY && F->Polygon) {
		PCB_POLY_VISIBLE_LOOP(data);
		{
			if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, polygon))
				continue;
			if (!on_locked_too && PCB_FLAG_TEST(PCB_FLAG_LOCK, polygon))
				continue;
			if (Reset) {
				pcb_undo_add_obj_to_flag(polygon);
				PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, polygon);
			}
			F->Polygon(ctx, layer, polygon);
			changed = pcb_true;
		}
		PCB_ENDALL_LOOP;
	}

	if ((type & (PCB_OBJ_SUBC | PCB_OBJ_SUBC_PART)) && F->subc) {
		PCB_SUBC_LOOP(data);
		{
			if (!on_locked_too && PCB_FLAG_TEST(PCB_FLAG_LOCK, subc))
				continue;
			if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc)) {
				if (Reset) {
					pcb_undo_add_obj_to_flag(subc);
					PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, subc);
				}
				F->subc(ctx, subc);
				changed = pcb_true;
			}
			else if ((pcb->loose_subc) || (type & PCB_OBJ_SUBC_PART)) {
				if (pcb_selected_operation(pcb, subc->data, F, ctx, Reset, type, on_locked_too))
					changed = pcb_true;
			}
		}
		PCB_END_LOOP;
	}

	/* process padstacks */
	if (type & PCB_OBJ_PSTK && pcb->pstk_on && F->padstack) {
		PCB_PADSTACK_LOOP(data);
		{
			if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, padstack))
				continue;
			if (!on_locked_too && PCB_FLAG_TEST(PCB_FLAG_LOCK, padstack))
				continue;
			if (Reset) {
				pcb_undo_add_obj_to_flag(padstack);
				PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, padstack);
			}
			F->padstack(ctx, padstack);
			changed = pcb_true;
		}
		PCB_END_LOOP;
	}

	/* and rat-lines */
	if (type & PCB_OBJ_RAT && pcb->RatOn && F->Rat) {
		PCB_RAT_LOOP(data);
		{
			if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, line))
				continue;
			if (!on_locked_too && PCB_FLAG_TEST(PCB_FLAG_LOCK, line))
				continue;
			if (Reset) {
				pcb_undo_add_obj_to_flag(line);
				PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, line);
			}
			F->Rat(ctx, line);
			changed = pcb_true;
		}
		PCB_END_LOOP;
	}

	if (Reset && changed)
		pcb_undo_inc_serial();

	if ((pcb_brave & PCB_BRAVE_CLIPBATCH) && (data != NULL))
		pcb_data_clip_inhibit_dec(data, 0);

	return changed;
}
