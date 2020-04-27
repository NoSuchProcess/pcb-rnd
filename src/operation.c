/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
#include "brave.h"
#include "extobj.h"

/* ----------------------------------------------------------------------
 * performs several operations on the passed object
 */
void *pcb_object_operation(pcb_opfunc_t *F, pcb_opctx_t *ctx, int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	pcb_any_obj_t *res = NULL, *exto = NULL;

	if (F->common_pre != NULL) {
		if (F->common_pre(ctx, Ptr2, Ptr3) == 1)
			return NULL;
	}

	if (!F->extobj_inhibit_regen)
		exto = pcb_extobj_float_pre(Ptr2);

	switch (Type) {
		case PCB_OBJ_LINE:
			if (F->Line)
				res = F->Line(ctx, (pcb_layer_t *)Ptr1, (pcb_line_t *)Ptr2);
			break;

		case PCB_OBJ_ARC:
			if (F->Arc)
				res = F->Arc(ctx, (pcb_layer_t *)Ptr1, (pcb_arc_t *)Ptr2);
			break;

		case PCB_OBJ_GFX:
			if (F->Gfx)
				res = F->Gfx(ctx, (pcb_layer_t *)Ptr1, (pcb_gfx_t *)Ptr2);
			break;

		case PCB_OBJ_LINE_POINT:
			if (F->LinePoint)
				res = F->LinePoint(ctx, (pcb_layer_t *)Ptr1, (pcb_line_t *)Ptr2, (pcb_point_t *)Ptr3);
			break;

		case PCB_OBJ_ARC_POINT:
			if (F->ArcPoint)
				res = F->ArcPoint(ctx, (pcb_layer_t *)Ptr1, (pcb_arc_t *)Ptr2, (int *)Ptr3);
			break;

		case PCB_OBJ_TEXT:
			if (F->Text)
				res = F->Text(ctx, (pcb_layer_t *)Ptr1, (pcb_text_t *)Ptr2);
			break;

		case PCB_OBJ_POLY:
			if (F->Polygon)
				res = F->Polygon(ctx, (pcb_layer_t *)Ptr1, (pcb_poly_t *)Ptr2);
			break;

		case PCB_OBJ_POLY_POINT:
			if (F->Point)
				res = F->Point(ctx, (pcb_layer_t *)Ptr1, (pcb_poly_t *)Ptr2, (pcb_point_t *)Ptr3);
			break;

		case PCB_OBJ_SUBC:
			if (F->subc)
				res = F->subc(ctx, (pcb_subc_t *)Ptr2);
			break;

		case PCB_OBJ_PSTK:
			if (F->padstack)
				res = F->padstack(ctx, (pcb_pstk_t *)Ptr2);
			break;

		case PCB_OBJ_RAT:
			if (F->Rat)
				res = F->Rat(ctx, (pcb_rat_t *)Ptr2);
			break;
	}

	if (exto != NULL)
		pcb_extobj_float_geo(exto);

	if (F->common_post != NULL)
		F->common_post(ctx, Ptr2, Ptr3);

	return res;
}

/* ----------------------------------------------------------------------
 * performs several operations on selected objects which are also visible
 * The lowlevel procedures are passed together with additional information
 * resets the selected flag if requested
 * returns pcb_true if anything has changed
 */
rnd_bool pcb_selected_operation(pcb_board_t *pcb, pcb_data_t *data, pcb_opfunc_t *F, pcb_opctx_t *ctx, rnd_bool Reset, int type, rnd_bool on_locked_too)
{
	rnd_bool changed = pcb_false;
	pcb_any_obj_t *exto;

	if (!(pcb_brave & PCB_BRAVE_NOCLIPBATCH) && (data != NULL))
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
			if (F->common_pre != NULL)
				if (F->common_pre(ctx, (pcb_any_obj_t *)line, NULL) == 1) continue;
			if (F->extobj_inhibit_regen)
				exto = NULL;
			else
				exto = pcb_extobj_float_pre((pcb_any_obj_t *)line);
			F->Line(ctx, layer, line);
			if (exto != NULL) pcb_extobj_float_geo(exto);
			if (F->common_post != NULL) F->common_post(ctx, (pcb_any_obj_t *)line, NULL);
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
			if (F->common_pre != NULL)
				if (F->common_pre(ctx, (pcb_any_obj_t *)arc, NULL) == 1) continue;
			if (F->extobj_inhibit_regen)
				exto = NULL;
			else
				exto = pcb_extobj_float_pre((pcb_any_obj_t *)arc);
			F->Arc(ctx, layer, arc);
			if (exto != NULL) pcb_extobj_float_geo(exto);
			if (F->common_post != NULL) F->common_post(ctx, (pcb_any_obj_t *)arc, NULL);
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
			if (F->common_pre != NULL)
				if (F->common_pre(ctx, (pcb_any_obj_t *)text, NULL) == 1) continue;
			if (F->extobj_inhibit_regen)
				exto = NULL;
			else
				exto = pcb_extobj_float_pre((pcb_any_obj_t *)text);
			F->Text(ctx, layer, text);
			if (exto != NULL) pcb_extobj_float_geo(exto);
			if (F->common_post != NULL) F->common_post(ctx, (pcb_any_obj_t *)text, NULL);
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
			if (F->common_pre != NULL)
				if (F->common_pre(ctx, (pcb_any_obj_t *)polygon, NULL) == 1) continue;
			if (F->extobj_inhibit_regen)
				exto = NULL;
			else
				exto = pcb_extobj_float_pre((pcb_any_obj_t *)polygon);
			F->Polygon(ctx, layer, polygon);
			if (exto != NULL) pcb_extobj_float_geo(exto);
			if (F->common_post != NULL) F->common_post(ctx, (pcb_any_obj_t *)polygon, NULL);
			changed = pcb_true;
		}
		PCB_ENDALL_LOOP;
	}

	/* check gfx */
	if (type & PCB_OBJ_GFX && F->Gfx) {
		PCB_GFX_VISIBLE_LOOP(data);
		{
			if (!PCB_FLAG_TEST(PCB_FLAG_SELECTED, gfx))
				continue;
			if (!on_locked_too && PCB_FLAG_TEST(PCB_FLAG_LOCK, gfx))
				continue;
			if (Reset) {
				pcb_undo_add_obj_to_flag(gfx);
				PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, gfx);
			}
			if (F->common_pre != NULL)
				if (F->common_pre(ctx, (pcb_any_obj_t *)gfx, NULL) == 1) continue;
			if (F->extobj_inhibit_regen)
				exto = NULL;
			else
				exto = pcb_extobj_float_pre((pcb_any_obj_t *)gfx);
			F->Gfx(ctx, layer, gfx);
			if (exto != NULL) pcb_extobj_float_geo(exto);
			if (F->common_post != NULL) F->common_post(ctx, (pcb_any_obj_t *)gfx, NULL);
			changed = pcb_true;
		}
		PCB_ENDALL_LOOP;
	}

	if ((type & (PCB_OBJ_SUBC | PCB_OBJ_SUBC_PART)) && F->subc) {
		PCB_SUBC_LOOP(data);
		{
			if (!on_locked_too && PCB_FLAG_TEST(PCB_FLAG_LOCK, subc) && (subc->extobj == NULL)) /* extobj: even locked means floaters can be operated on */
				continue;
			if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc)) {
				if (Reset) {
					pcb_undo_add_obj_to_flag(subc);
					PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, subc);
				}
				if (F->common_pre != NULL)
					if (F->common_pre(ctx, (pcb_any_obj_t *)subc, NULL) == 1) continue;
				if (F->extobj_inhibit_regen)
					exto = NULL;
				else
					exto = pcb_extobj_float_pre((pcb_any_obj_t *)subc);
				F->subc(ctx, subc);
				if (exto != NULL) pcb_extobj_float_geo(exto);
				if (F->common_post != NULL) F->common_post(ctx, (pcb_any_obj_t *)subc, NULL);
				changed = pcb_true;
			}
			else if ((pcb->loose_subc) || (type & PCB_OBJ_SUBC_PART) || (subc->extobj != NULL)) {
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
			if (F->common_pre != NULL)
				if (F->common_pre(ctx, (pcb_any_obj_t *)padstack, NULL) == 1) continue;
			if (F->extobj_inhibit_regen)
				exto = NULL;
			else
				exto = pcb_extobj_float_pre((pcb_any_obj_t *)padstack);
			F->padstack(ctx, padstack);
			if (exto != NULL) pcb_extobj_float_geo(exto);
			if (F->common_post != NULL) F->common_post(ctx, (pcb_any_obj_t *)padstack, NULL);
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
			if (F->common_pre != NULL)
				if (F->common_pre(ctx, (pcb_any_obj_t *)line, NULL) == 1) continue;
			F->Rat(ctx, line);
			if (F->common_post != NULL) F->common_post(ctx, (pcb_any_obj_t *)line, NULL);
			changed = pcb_true;
		}
		PCB_END_LOOP;
	}

	if (Reset && changed)
		pcb_undo_inc_serial();

	if (!(pcb_brave & PCB_BRAVE_NOCLIPBATCH) && (data != NULL))
		pcb_data_clip_inhibit_dec(data, 0);

	return changed;
}
