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

#include "config.h"

#include "operation.h"
#include "const.h"
#include "board.h"
#include "data.h"
#include "conf_core.h"
#include "undo.h"
#include "rats.h"

/* ----------------------------------------------------------------------
 * performs several operations on the passed object
 */
void *pcb_object_operation(pcb_opfunc_t *F, pcb_opctx_t *ctx, int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	switch (Type) {
	case PCB_TYPE_LINE:
		if (F->Line)
			return (F->Line(ctx, (pcb_layer_t *) Ptr1, (pcb_line_t *) Ptr2));
		break;

	case PCB_TYPE_ARC:
		if (F->Arc)
			return (F->Arc(ctx, (pcb_layer_t *) Ptr1, (pcb_arc_t *) Ptr2));
		break;

	case PCB_TYPE_LINE_POINT:
		if (F->LinePoint)
			return (F->LinePoint(ctx, (pcb_layer_t *) Ptr1, (pcb_line_t *) Ptr2, (pcb_point_t *) Ptr3));
		break;

	case PCB_TYPE_ARC_POINT:
		if (F->ArcPoint)
			return (F->ArcPoint(ctx, (pcb_layer_t *) Ptr1, (pcb_arc_t *) Ptr2, (int *) Ptr3));
		break;

	case PCB_TYPE_TEXT:
		if (F->Text)
			return (F->Text(ctx, (pcb_layer_t *) Ptr1, (pcb_text_t *) Ptr2));
		break;

	case PCB_TYPE_POLYGON:
		if (F->Polygon)
			return (F->Polygon(ctx, (pcb_layer_t *) Ptr1, (pcb_polygon_t *) Ptr2));
		break;

	case PCB_TYPE_POLYGON_POINT:
		if (F->Point)
			return (F->Point(ctx, (pcb_layer_t *) Ptr1, (pcb_polygon_t *) Ptr2, (pcb_point_t *) Ptr3));
		break;

	case PCB_TYPE_VIA:
		if (F->Via)
			return (F->Via(ctx, (pcb_pin_t *) Ptr1));
		break;

	case PCB_TYPE_ELEMENT:
		if (F->Element)
			return (F->Element(ctx, (pcb_element_t *) Ptr1));
		break;

	case PCB_TYPE_SUBC:
		if (F->subc)
			return (F->subc(ctx, (pcb_subc_t *) Ptr1));
		break;

	case PCB_TYPE_PIN:
		if (F->Pin)
			return (F->Pin(ctx, (pcb_element_t *) Ptr1, (pcb_pin_t *) Ptr2));
		break;

	case PCB_TYPE_PAD:
		if (F->Pad)
			return (F->Pad(ctx, (pcb_element_t *) Ptr1, (pcb_pad_t *) Ptr2));
		break;

	case PCB_TYPE_ELEMENT_NAME:
		if (F->ElementName)
			return (F->ElementName(ctx, (pcb_element_t *) Ptr1));
		break;

	case PCB_TYPE_RATLINE:
		if (F->Rat)
			return (F->Rat(ctx, (pcb_rat_t *) Ptr1));
		break;
	}
	return (NULL);
}

/* ----------------------------------------------------------------------
 * performs several operations on selected objects which are also visible
 * The lowlevel procedures are passed together with additional information
 * resets the selected flag if requested
 * returns pcb_true if anything has changed
 */
pcb_bool pcb_selected_operation(pcb_board_t *pcb, pcb_opfunc_t *F, pcb_opctx_t *ctx, pcb_bool Reset, int type)
{
	pcb_bool changed = pcb_false;

	/* check lines */
	if (type & PCB_TYPE_LINE && F->Line)
		PCB_LINE_VISIBLE_LOOP(pcb->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, line)) {
			if (Reset) {
				pcb_undo_add_obj_to_flag(PCB_TYPE_LINE, layer, line, line);
				PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, line);
			}
			F->Line(ctx, layer, line);
			changed = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;

	/* check arcs */
	if (type & PCB_TYPE_ARC && F->Arc)
		PCB_ARC_VISIBLE_LOOP(pcb->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, arc)) {
			if (Reset) {
				pcb_undo_add_obj_to_flag(PCB_TYPE_ARC, layer, arc, arc);
				PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, arc);
			}
			F->Arc(ctx, layer, arc);
			changed = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;

	/* check text */
	if (type & PCB_TYPE_TEXT && F->Text)
		PCB_TEXT_ALL_LOOP(pcb->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, text) && pcb_text_is_visible(PCB, layer, text)) {
			if (Reset) {
				pcb_undo_add_obj_to_flag(PCB_TYPE_TEXT, layer, text, text);
				PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, text);
			}
			F->Text(ctx, layer, text);
			changed = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;

	/* check polygons */
	if (type & PCB_TYPE_POLYGON && F->Polygon)
		PCB_POLY_VISIBLE_LOOP(pcb->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, polygon)) {
			if (Reset) {
				pcb_undo_add_obj_to_flag(PCB_TYPE_POLYGON, layer, polygon, polygon);
				PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, polygon);
			}
			F->Polygon(ctx, layer, polygon);
			changed = pcb_true;
		}
	}
	PCB_ENDALL_LOOP;

	/* elements silkscreen */
	if (type & PCB_TYPE_ELEMENT && pcb_silk_on(pcb) && F->Element)
		PCB_ELEMENT_LOOP(pcb->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, element)) {
			if (Reset) {
				pcb_undo_add_obj_to_flag(PCB_TYPE_ELEMENT, element, element, element);
				PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, element);
			}
			F->Element(ctx, element);
			changed = pcb_true;
		}
	}
	PCB_END_LOOP;
	if (type & PCB_TYPE_ELEMENT_NAME && pcb_silk_on(pcb) && F->ElementName)
		PCB_ELEMENT_LOOP(pcb->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, &PCB_ELEM_TEXT_VISIBLE(PCB, element))) {
			if (Reset) {
				pcb_undo_add_obj_to_flag(PCB_TYPE_ELEMENT_NAME, element, &PCB_ELEM_TEXT_VISIBLE(PCB, element), &PCB_ELEM_TEXT_VISIBLE(PCB, element));
				PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, &PCB_ELEM_TEXT_VISIBLE(PCB, element));
			}
			F->ElementName(ctx, element);
			changed = pcb_true;
		}
	}
	PCB_END_LOOP;

	if (type & PCB_TYPE_PIN && pcb->PinOn && F->Pin)
		PCB_ELEMENT_LOOP(pcb->Data);
	{
		PCB_PIN_LOOP(element);
		{
			if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, pin)) {
				if (Reset) {
					pcb_undo_add_obj_to_flag(PCB_TYPE_PIN, element, pin, pin);
					PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, pin);
				}
				F->Pin(ctx, element, pin);
				changed = pcb_true;
			}
		}
		PCB_END_LOOP;
	}
	PCB_END_LOOP;

	if (type & PCB_TYPE_PAD && pcb->PinOn && F->Pad)
		PCB_ELEMENT_LOOP(pcb->Data);
	{
		PCB_PAD_LOOP(element);
		{
			if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, pad)) {
				if (Reset) {
					pcb_undo_add_obj_to_flag(PCB_TYPE_PAD, element, pad, pad);
					PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, pad);
				}
				F->Pad(ctx, element, pad);
				changed = pcb_true;
			}
		}
		PCB_END_LOOP;
	}
	PCB_END_LOOP;

	/* process vias */
	if (type & PCB_TYPE_VIA && pcb->ViaOn && F->Via)
		PCB_VIA_LOOP(pcb->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, via)) {
			if (Reset) {
				pcb_undo_add_obj_to_flag(PCB_TYPE_VIA, via, via, via);
				PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, via);
			}
			F->Via(ctx, via);
			changed = pcb_true;
		}
	}
	PCB_END_LOOP;
	/* and rat-lines */
	if (type & PCB_TYPE_RATLINE && pcb->RatOn && F->Rat)
		PCB_RAT_LOOP(pcb->Data);
	{
		if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, line)) {
			if (Reset) {
				pcb_undo_add_obj_to_flag(PCB_TYPE_RATLINE, line, line, line);
				PCB_FLAG_CLEAR(PCB_FLAG_SELECTED, line);
			}
			F->Rat(ctx, line);
			changed = pcb_true;
		}
	}
	PCB_END_LOOP;
	if (Reset && changed)
		pcb_undo_inc_serial();
	return (changed);
}
