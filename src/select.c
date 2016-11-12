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


/* select routines
 */

#include "config.h"
#include "conf_core.h"

#include "board.h"
#include "data.h"
#include "draw.h"
#include "error.h"
#include "search.h"
#include "select.h"
#include "undo.h"
#include "find.h"
#include "compat_misc.h"
#include "compat_nls.h"

#include "obj_elem_draw.h"
#include "obj_pad_draw.h"
#include "obj_arc_draw.h"
#include "obj_pinvia_draw.h"
#include "obj_line_draw.h"
#include "obj_poly_draw.h"
#include "obj_text_draw.h"
#include "obj_rat_draw.h"

#include <genregex/regex_sei.h>

void pcb_select_element(ElementType *element, pcb_change_flag_t how, int redraw)
{
	/* select all pins and names of the element */
	PIN_LOOP(element);
	{
		AddObjectToFlagUndoList(PCB_TYPE_PIN, element, pin, pin);
		CHANGE_FLAG(how, PCB_FLAG_SELECTED, pin);
	}
	END_LOOP;
	PAD_LOOP(element);
	{
		AddObjectToFlagUndoList(PCB_TYPE_PAD, element, pad, pad);
		CHANGE_FLAG(how, PCB_FLAG_SELECTED, pad);
	}
	END_LOOP;
	ELEMENTTEXT_LOOP(element);
	{
		AddObjectToFlagUndoList(PCB_TYPE_ELEMENT_NAME, element, text, text);
		CHANGE_FLAG(how, PCB_FLAG_SELECTED, text);
	}
	END_LOOP;
	AddObjectToFlagUndoList(PCB_TYPE_ELEMENT, element, element, element);
	CHANGE_FLAG(how, PCB_FLAG_SELECTED, element);

	if (redraw) {
		if (PCB->ElementOn && ((TEST_FLAG(PCB_FLAG_ONSOLDER, element) != 0) == SWAP_IDENT || PCB->InvisibleObjectsOn))
			if (PCB->ElementOn) {
				DrawElementName(element);
				DrawElementPackage(element);
			}
		if (PCB->PinOn)
			DrawElementPinsAndPads(element);
	}
}

void pcb_select_element_name(ElementType *element, pcb_change_flag_t how, int redraw)
{
	/* select all names of the element */
	ELEMENTTEXT_LOOP(element);
	{
		AddObjectToFlagUndoList(PCB_TYPE_ELEMENT_NAME, element, text, text);
		CHANGE_FLAG(how, PCB_FLAG_SELECTED, text);
	}
	END_LOOP;

	if (redraw)
		DrawElementName(element);
}


/* ---------------------------------------------------------------------------
 * toggles the selection of any kind of object
 * the different types are defined by search.h
 */
pcb_bool SelectObject(void)
{
	void *ptr1, *ptr2, *ptr3;
	pcb_layer_t *layer;
	int type;

	pcb_bool changed = pcb_true;

	type = SearchScreen(Crosshair.X, Crosshair.Y, SELECT_TYPES, &ptr1, &ptr2, &ptr3);
	if (type == PCB_TYPE_NONE || TEST_FLAG(PCB_FLAG_LOCK, (PinTypePtr) ptr2))
		return (pcb_false);
	switch (type) {
	case PCB_TYPE_VIA:
		AddObjectToFlagUndoList(PCB_TYPE_VIA, ptr1, ptr1, ptr1);
		TOGGLE_FLAG(PCB_FLAG_SELECTED, (PinTypePtr) ptr1);
		DrawVia((PinTypePtr) ptr1);
		break;

	case PCB_TYPE_LINE:
		{
			pcb_line_t *line = (pcb_line_t *) ptr2;

			layer = (pcb_layer_t *) ptr1;
			AddObjectToFlagUndoList(PCB_TYPE_LINE, ptr1, ptr2, ptr2);
			TOGGLE_FLAG(PCB_FLAG_SELECTED, line);
			DrawLine(layer, line);
			break;
		}

	case PCB_TYPE_RATLINE:
		{
			RatTypePtr rat = (RatTypePtr) ptr2;

			AddObjectToFlagUndoList(PCB_TYPE_RATLINE, ptr1, ptr1, ptr1);
			TOGGLE_FLAG(PCB_FLAG_SELECTED, rat);
			DrawRat(rat);
			break;
		}

	case PCB_TYPE_ARC:
		{
			ArcType *arc = (ArcTypePtr) ptr2;

			layer = (pcb_layer_t *) ptr1;
			AddObjectToFlagUndoList(PCB_TYPE_ARC, ptr1, ptr2, ptr2);
			TOGGLE_FLAG(PCB_FLAG_SELECTED, arc);
			DrawArc(layer, arc);
			break;
		}

	case PCB_TYPE_TEXT:
		{
			TextType *text = (TextTypePtr) ptr2;

			layer = (pcb_layer_t *) ptr1;
			AddObjectToFlagUndoList(PCB_TYPE_TEXT, ptr1, ptr2, ptr2);
			TOGGLE_FLAG(PCB_FLAG_SELECTED, text);
			DrawText(layer, text);
			break;
		}

	case PCB_TYPE_POLYGON:
		{
			PolygonType *poly = (PolygonTypePtr) ptr2;

			layer = (pcb_layer_t *) ptr1;
			AddObjectToFlagUndoList(PCB_TYPE_POLYGON, ptr1, ptr2, ptr2);
			TOGGLE_FLAG(PCB_FLAG_SELECTED, poly);
			DrawPolygon(layer, poly);
			/* changing memory order no longer effects draw order */
			break;
		}

	case PCB_TYPE_PIN:
		AddObjectToFlagUndoList(PCB_TYPE_PIN, ptr1, ptr2, ptr2);
		TOGGLE_FLAG(PCB_FLAG_SELECTED, (PinTypePtr) ptr2);
		DrawPin((PinTypePtr) ptr2);
		break;

	case PCB_TYPE_PAD:
		AddObjectToFlagUndoList(PCB_TYPE_PAD, ptr1, ptr2, ptr2);
		TOGGLE_FLAG(PCB_FLAG_SELECTED, (PadTypePtr) ptr2);
		DrawPad((PadTypePtr) ptr2);
		break;

	case PCB_TYPE_ELEMENT_NAME:
		pcb_select_element_name((ElementType *) ptr1, PCB_CHGFLG_TOGGLE, 1);
		break;

	case PCB_TYPE_ELEMENT:
		pcb_select_element((ElementType *) ptr1, PCB_CHGFLG_TOGGLE, 1);
		break;
	}
	Draw();
	IncrementUndoSerialNumber();
	return (changed);
}

/* ----------------------------------------------------------------------
 * selects/unselects or lists visible objects within the passed box
 * If len is NULL:
 *  Flag determines if the block is to be selected or unselected
 *  returns non-NULL if the state of any object has changed
 * if len is non-NULL:
 *  returns a list of object IDs matched the search and loads len with the
 *  length of the list. Returns NULL on no match.
 */
static long int *ListBlock_(pcb_box_t *Box, pcb_bool Flag, int *len)
{
	int changed = 0;
	int used = 0, alloced = 0;
	long int *list = NULL;

#define swap(a,b) \
do { \
	Coord tmp; \
	tmp = a; \
	a = b; \
	b = tmp; \
} while(0)

	/* If board view is flipped, box coords need to be flipped too to reflect
	   the on-screen direction of draw */
	if (conf_core.editor.view.flip_x)
		swap(Box->X1, Box->X2);
	if (conf_core.editor.view.flip_y)
		swap(Box->Y1, Box->Y2);

	if (conf_core.editor.selection.disable_negative) {
		if (Box->X1 > Box->X2)
			swap(Box->X1, Box->X2);
		if (Box->Y1 > Box->Y2)
			swap(Box->Y1, Box->Y2);
	}
	else {
		if (conf_core.editor.selection.symmetric_negative) {
			if (Box->Y1 > Box->Y2)
				swap(Box->Y1, Box->Y2);
		}
		/* Make sure our negative box is canonical: always from bottom-right to top-left
		   This limits all possible box coordinate orders to two, one for positive and
		   one for negative. */
		if (IS_BOX_NEGATIVE(Box)) {
			if (Box->X1 < Box->X2)
				swap(Box->X1, Box->X2);
			if (Box->Y1 < Box->Y2)
				swap(Box->Y1, Box->Y2);
		}
	}
#undef swap

/*pcb_printf("box: %mm %mm - %mm %mm    [ %d ] %d %d\n", Box->X1, Box->Y1, Box->X2, Box->Y2, IS_BOX_NEGATIVE(Box), conf_core.editor.view.flip_x, conf_core.editor.view.flip_y);*/

/* append an object to the return list OR set the flag if there's no list */
#define append(undo_type, p1, obj) \
do { \
	if (len == NULL) { \
		AddObjectToFlagUndoList (undo_type, p1, obj, obj); \
		ASSIGN_FLAG (PCB_FLAG_SELECTED, Flag, obj); \
	} \
	else { \
		if (used >= alloced) { \
			alloced += 64; \
			list = realloc(list, sizeof(*list) * alloced); \
		} \
		list[used] = obj->ID; \
		used++; \
	} \
	changed = 1; \
} while(0)

	if (IS_BOX_NEGATIVE(Box) && ((Box->X1 == Box->X2) || (Box->Y2 == Box->Y1))) {
		if (len != NULL)
			*len = 0;
		return NULL;
	}

	if (PCB->RatOn || !Flag)
		RAT_LOOP(PCB->Data);
	{
		if (LINE_NEAR_BOX((pcb_line_t *) line, Box) && !TEST_FLAG(PCB_FLAG_LOCK, line) && TEST_FLAG(PCB_FLAG_SELECTED, line) != Flag) {
			append(PCB_TYPE_RATLINE, line, line);
			if (PCB->RatOn)
				DrawRat(line);
		}
	}
	END_LOOP;

	/* check layers */
	LAYER_LOOP(PCB->Data, max_copper_layer + 2);
	{
		if (layer == &PCB->Data->SILKLAYER) {
			if (!(PCB->ElementOn || !Flag))
				continue;
		}
		else if (layer == &PCB->Data->BACKSILKLAYER) {
			if (!(PCB->InvisibleObjectsOn || !Flag))
				continue;
		}
		else if (!(layer->On || !Flag))
			continue;

		LINE_LOOP(layer);
		{
			if (LINE_NEAR_BOX(line, Box)
					&& !TEST_FLAG(PCB_FLAG_LOCK, line)
					&& TEST_FLAG(PCB_FLAG_SELECTED, line) != Flag) {
				append(PCB_TYPE_LINE, layer, line);
				if (layer->On)
					DrawLine(layer, line);
			}
		}
		END_LOOP;
		ARC_LOOP(layer);
		{
			if (ARC_NEAR_BOX(arc, Box)
					&& !TEST_FLAG(PCB_FLAG_LOCK, arc)
					&& TEST_FLAG(PCB_FLAG_SELECTED, arc) != Flag) {
				append(PCB_TYPE_ARC, layer, arc);
				if (layer->On)
					DrawArc(layer, arc);
			}
		}
		END_LOOP;
		TEXT_LOOP(layer);
		{
			if (!Flag || TEXT_IS_VISIBLE(PCB, layer, text)) {
				if (TEXT_NEAR_BOX(text, Box)
						&& !TEST_FLAG(PCB_FLAG_LOCK, text)
						&& TEST_FLAG(PCB_FLAG_SELECTED, text) != Flag) {
					append(PCB_TYPE_TEXT, layer, text);
					if (TEXT_IS_VISIBLE(PCB, layer, text))
						DrawText(layer, text);
				}
			}
		}
		END_LOOP;
		POLYGON_LOOP(layer);
		{
			if (POLYGON_NEAR_BOX(polygon, Box)
					&& !TEST_FLAG(PCB_FLAG_LOCK, polygon)
					&& TEST_FLAG(PCB_FLAG_SELECTED, polygon) != Flag) {
				append(PCB_TYPE_POLYGON, layer, polygon);
				if (layer->On)
					DrawPolygon(layer, polygon);
			}
		}
		END_LOOP;
	}
	END_LOOP;

	/* elements */
	ELEMENT_LOOP(PCB->Data);
	{
		{
			pcb_bool gotElement = pcb_false;
			if ((PCB->ElementOn || !Flag)
					&& !TEST_FLAG(PCB_FLAG_LOCK, element)
					&& ((TEST_FLAG(PCB_FLAG_ONSOLDER, element) != 0) == SWAP_IDENT || PCB->InvisibleObjectsOn)) {
				if (BOX_NEAR_BOX(&ELEMENT_TEXT(PCB, element).BoundingBox, Box)
						&& !TEST_FLAG(PCB_FLAG_LOCK, &ELEMENT_TEXT(PCB, element))
						&& TEST_FLAG(PCB_FLAG_SELECTED, &ELEMENT_TEXT(PCB, element)) != Flag) {
					/* select all names of element */
					ELEMENTTEXT_LOOP(element);
					{
						append(PCB_TYPE_ELEMENT_NAME, element, text);
					}
					END_LOOP;
					if (PCB->ElementOn)
						DrawElementName(element);
				}
				if ((PCB->PinOn || !Flag) && ELEMENT_NEAR_BOX(element, Box))
					if (TEST_FLAG(PCB_FLAG_SELECTED, element) != Flag) {
						append(PCB_TYPE_ELEMENT, element, element);
						PIN_LOOP(element);
						{
							if (TEST_FLAG(PCB_FLAG_SELECTED, pin) != Flag) {
								append(PCB_TYPE_PIN, element, pin);
								if (PCB->PinOn)
									DrawPin(pin);
							}
						}
						END_LOOP;
						PAD_LOOP(element);
						{
							if (TEST_FLAG(PCB_FLAG_SELECTED, pad) != Flag) {
								append(PCB_TYPE_PAD, element, pad);
								if (PCB->PinOn)
									DrawPad(pad);
							}
						}
						END_LOOP;
						if (PCB->PinOn)
							DrawElement(element);
						gotElement = pcb_true;
					}
			}
			if ((PCB->PinOn || !Flag) && !TEST_FLAG(PCB_FLAG_LOCK, element) && !gotElement) {
				PIN_LOOP(element);
				{
					if ((VIA_OR_PIN_NEAR_BOX(pin, Box)
							 && TEST_FLAG(PCB_FLAG_SELECTED, pin) != Flag)) {
						append(PCB_TYPE_PIN, element, pin);
						if (PCB->PinOn)
							DrawPin(pin);
					}
				}
				END_LOOP;
				PAD_LOOP(element);
				{
					if (PAD_NEAR_BOX(pad, Box)
							&& TEST_FLAG(PCB_FLAG_SELECTED, pad) != Flag
							&& (TEST_FLAG(PCB_FLAG_ONSOLDER, pad) == SWAP_IDENT || PCB->InvisibleObjectsOn || !Flag)) {
						append(PCB_TYPE_PAD, element, pad);
						if (PCB->PinOn)
							DrawPad(pad);
					}
				}
				END_LOOP;
			}
		}
	}
	END_LOOP;
	/* end with vias */
	if (PCB->ViaOn || !Flag)
		VIA_LOOP(PCB->Data);
	{
		if (VIA_OR_PIN_NEAR_BOX(via, Box)
				&& !TEST_FLAG(PCB_FLAG_LOCK, via)
				&& TEST_FLAG(PCB_FLAG_SELECTED, via) != Flag) {
			append(PCB_TYPE_VIA, via, via);
			if (PCB->ViaOn)
				DrawVia(via);
		}
	}
	END_LOOP;

	if (changed) {
		Draw();
		IncrementUndoSerialNumber();
	}

	if (len == NULL) {
		static long int non_zero;
		return (changed ? &non_zero : NULL);
	}
	else {
		*len = used;
		return list;
	}
}

#undef append

/* ----------------------------------------------------------------------
 * selects/unselects all visible objects within the passed box
 * Flag determines if the block is to be selected or unselected
 * returns pcb_true if the state of any object has changed
 */
pcb_bool SelectBlock(pcb_box_t *Box, pcb_bool Flag)
{
	/* do not list, set flag */
	return (ListBlock_(Box, Flag, NULL) == NULL) ? pcb_false : pcb_true;
}

/* ----------------------------------------------------------------------
 * List all visible objects within the passed box
 */
long int *ListBlock(pcb_box_t *Box, int *len)
{
	return ListBlock_(Box, 1, len);
}

/* ----------------------------------------------------------------------
 * performs several operations on the passed object
 */
#warning TODO: maybe move this to operation.c
void *ObjectOperation(pcb_opfunc_t *F, pcb_opctx_t *ctx, int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	switch (Type) {
	case PCB_TYPE_LINE:
		if (F->Line)
			return (F->Line(ctx, (pcb_layer_t *) Ptr1, (pcb_line_t *) Ptr2));
		break;

	case PCB_TYPE_ARC:
		if (F->Arc)
			return (F->Arc(ctx, (pcb_layer_t *) Ptr1, (ArcTypePtr) Ptr2));
		break;

	case PCB_TYPE_LINE_POINT:
		if (F->LinePoint)
			return (F->LinePoint(ctx, (pcb_layer_t *) Ptr1, (pcb_line_t *) Ptr2, (PointTypePtr) Ptr3));
		break;

	case PCB_TYPE_TEXT:
		if (F->Text)
			return (F->Text(ctx, (pcb_layer_t *) Ptr1, (TextTypePtr) Ptr2));
		break;

	case PCB_TYPE_POLYGON:
		if (F->Polygon)
			return (F->Polygon(ctx, (pcb_layer_t *) Ptr1, (PolygonTypePtr) Ptr2));
		break;

	case PCB_TYPE_POLYGON_POINT:
		if (F->Point)
			return (F->Point(ctx, (pcb_layer_t *) Ptr1, (PolygonTypePtr) Ptr2, (PointTypePtr) Ptr3));
		break;

	case PCB_TYPE_VIA:
		if (F->Via)
			return (F->Via(ctx, (PinTypePtr) Ptr1));
		break;

	case PCB_TYPE_ELEMENT:
		if (F->Element)
			return (F->Element(ctx, (ElementTypePtr) Ptr1));
		break;

	case PCB_TYPE_PIN:
		if (F->Pin)
			return (F->Pin(ctx, (ElementTypePtr) Ptr1, (PinTypePtr) Ptr2));
		break;

	case PCB_TYPE_PAD:
		if (F->Pad)
			return (F->Pad(ctx, (ElementTypePtr) Ptr1, (PadTypePtr) Ptr2));
		break;

	case PCB_TYPE_ELEMENT_NAME:
		if (F->ElementName)
			return (F->ElementName(ctx, (ElementTypePtr) Ptr1));
		break;

	case PCB_TYPE_RATLINE:
		if (F->Rat)
			return (F->Rat(ctx, (RatTypePtr) Ptr1));
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
pcb_bool SelectedOperation(pcb_opfunc_t *F, pcb_opctx_t *ctx, pcb_bool Reset, int type)
{
	pcb_bool changed = pcb_false;

	/* check lines */
	if (type & PCB_TYPE_LINE && F->Line)
		VISIBLELINE_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_SELECTED, line)) {
			if (Reset) {
				AddObjectToFlagUndoList(PCB_TYPE_LINE, layer, line, line);
				CLEAR_FLAG(PCB_FLAG_SELECTED, line);
			}
			F->Line(ctx, layer, line);
			changed = pcb_true;
		}
	}
	ENDALL_LOOP;

	/* check arcs */
	if (type & PCB_TYPE_ARC && F->Arc)
		VISIBLEARC_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_SELECTED, arc)) {
			if (Reset) {
				AddObjectToFlagUndoList(PCB_TYPE_ARC, layer, arc, arc);
				CLEAR_FLAG(PCB_FLAG_SELECTED, arc);
			}
			F->Arc(ctx, layer, arc);
			changed = pcb_true;
		}
	}
	ENDALL_LOOP;

	/* check text */
	if (type & PCB_TYPE_TEXT && F->Text)
		ALLTEXT_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_SELECTED, text) && TEXT_IS_VISIBLE(PCB, layer, text)) {
			if (Reset) {
				AddObjectToFlagUndoList(PCB_TYPE_TEXT, layer, text, text);
				CLEAR_FLAG(PCB_FLAG_SELECTED, text);
			}
			F->Text(ctx, layer, text);
			changed = pcb_true;
		}
	}
	ENDALL_LOOP;

	/* check polygons */
	if (type & PCB_TYPE_POLYGON && F->Polygon)
		VISIBLEPOLYGON_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_SELECTED, polygon)) {
			if (Reset) {
				AddObjectToFlagUndoList(PCB_TYPE_POLYGON, layer, polygon, polygon);
				CLEAR_FLAG(PCB_FLAG_SELECTED, polygon);
			}
			F->Polygon(ctx, layer, polygon);
			changed = pcb_true;
		}
	}
	ENDALL_LOOP;

	/* elements silkscreen */
	if (type & PCB_TYPE_ELEMENT && PCB->ElementOn && F->Element)
		ELEMENT_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_SELECTED, element)) {
			if (Reset) {
				AddObjectToFlagUndoList(PCB_TYPE_ELEMENT, element, element, element);
				CLEAR_FLAG(PCB_FLAG_SELECTED, element);
			}
			F->Element(ctx, element);
			changed = pcb_true;
		}
	}
	END_LOOP;
	if (type & PCB_TYPE_ELEMENT_NAME && PCB->ElementOn && F->ElementName)
		ELEMENT_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_SELECTED, &ELEMENT_TEXT(PCB, element))) {
			if (Reset) {
				AddObjectToFlagUndoList(PCB_TYPE_ELEMENT_NAME, element, &ELEMENT_TEXT(PCB, element), &ELEMENT_TEXT(PCB, element));
				CLEAR_FLAG(PCB_FLAG_SELECTED, &ELEMENT_TEXT(PCB, element));
			}
			F->ElementName(ctx, element);
			changed = pcb_true;
		}
	}
	END_LOOP;

	if (type & PCB_TYPE_PIN && PCB->PinOn && F->Pin)
		ELEMENT_LOOP(PCB->Data);
	{
		PIN_LOOP(element);
		{
			if (TEST_FLAG(PCB_FLAG_SELECTED, pin)) {
				if (Reset) {
					AddObjectToFlagUndoList(PCB_TYPE_PIN, element, pin, pin);
					CLEAR_FLAG(PCB_FLAG_SELECTED, pin);
				}
				F->Pin(ctx, element, pin);
				changed = pcb_true;
			}
		}
		END_LOOP;
	}
	END_LOOP;

	if (type & PCB_TYPE_PAD && PCB->PinOn && F->Pad)
		ELEMENT_LOOP(PCB->Data);
	{
		PAD_LOOP(element);
		{
			if (TEST_FLAG(PCB_FLAG_SELECTED, pad)) {
				if (Reset) {
					AddObjectToFlagUndoList(PCB_TYPE_PAD, element, pad, pad);
					CLEAR_FLAG(PCB_FLAG_SELECTED, pad);
				}
				F->Pad(ctx, element, pad);
				changed = pcb_true;
			}
		}
		END_LOOP;
	}
	END_LOOP;

	/* process vias */
	if (type & PCB_TYPE_VIA && PCB->ViaOn && F->Via)
		VIA_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_SELECTED, via)) {
			if (Reset) {
				AddObjectToFlagUndoList(PCB_TYPE_VIA, via, via, via);
				CLEAR_FLAG(PCB_FLAG_SELECTED, via);
			}
			F->Via(ctx, via);
			changed = pcb_true;
		}
	}
	END_LOOP;
	/* and rat-lines */
	if (type & PCB_TYPE_RATLINE && PCB->RatOn && F->Rat)
		RAT_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_SELECTED, line)) {
			if (Reset) {
				AddObjectToFlagUndoList(PCB_TYPE_RATLINE, line, line, line);
				CLEAR_FLAG(PCB_FLAG_SELECTED, line);
			}
			F->Rat(ctx, line);
			changed = pcb_true;
		}
	}
	END_LOOP;
	if (Reset && changed)
		IncrementUndoSerialNumber();
	return (changed);
}

/* ----------------------------------------------------------------------
 * selects/unselects all objects which were found during a connection scan
 * Flag determines if they are to be selected or unselected
 * returns pcb_true if the state of any object has changed
 *
 * text objects and elements cannot be selected by this routine
 */
pcb_bool SelectConnection(pcb_bool Flag)
{
	pcb_bool changed = pcb_false;

	if (PCB->RatOn)
		RAT_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_FOUND, line)) {
			AddObjectToFlagUndoList(PCB_TYPE_RATLINE, line, line, line);
			ASSIGN_FLAG(PCB_FLAG_SELECTED, Flag, line);
			DrawRat(line);
			changed = pcb_true;
		}
	}
	END_LOOP;

	VISIBLELINE_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_FOUND, line) && !TEST_FLAG(PCB_FLAG_LOCK, line)) {
			AddObjectToFlagUndoList(PCB_TYPE_LINE, layer, line, line);
			ASSIGN_FLAG(PCB_FLAG_SELECTED, Flag, line);
			DrawLine(layer, line);
			changed = pcb_true;
		}
	}
	ENDALL_LOOP;
	VISIBLEARC_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_FOUND, arc) && !TEST_FLAG(PCB_FLAG_LOCK, arc)) {
			AddObjectToFlagUndoList(PCB_TYPE_ARC, layer, arc, arc);
			ASSIGN_FLAG(PCB_FLAG_SELECTED, Flag, arc);
			DrawArc(layer, arc);
			changed = pcb_true;
		}
	}
	ENDALL_LOOP;
	VISIBLEPOLYGON_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_FOUND, polygon) && !TEST_FLAG(PCB_FLAG_LOCK, polygon)) {
			AddObjectToFlagUndoList(PCB_TYPE_POLYGON, layer, polygon, polygon);
			ASSIGN_FLAG(PCB_FLAG_SELECTED, Flag, polygon);
			DrawPolygon(layer, polygon);
			changed = pcb_true;
		}
	}
	ENDALL_LOOP;

	if (PCB->PinOn && PCB->ElementOn) {
		ALLPIN_LOOP(PCB->Data);
		{
			if (!TEST_FLAG(PCB_FLAG_LOCK, element) && TEST_FLAG(PCB_FLAG_FOUND, pin)) {
				AddObjectToFlagUndoList(PCB_TYPE_PIN, element, pin, pin);
				ASSIGN_FLAG(PCB_FLAG_SELECTED, Flag, pin);
				DrawPin(pin);
				changed = pcb_true;
			}
		}
		ENDALL_LOOP;
		ALLPAD_LOOP(PCB->Data);
		{
			if (!TEST_FLAG(PCB_FLAG_LOCK, element) && TEST_FLAG(PCB_FLAG_FOUND, pad)) {
				AddObjectToFlagUndoList(PCB_TYPE_PAD, element, pad, pad);
				ASSIGN_FLAG(PCB_FLAG_SELECTED, Flag, pad);
				DrawPad(pad);
				changed = pcb_true;
			}
		}
		ENDALL_LOOP;
	}

	if (PCB->ViaOn)
		VIA_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_FOUND, via) && !TEST_FLAG(PCB_FLAG_LOCK, via)) {
			AddObjectToFlagUndoList(PCB_TYPE_VIA, via, via, via);
			ASSIGN_FLAG(PCB_FLAG_SELECTED, Flag, via);
			DrawVia(via);
			changed = pcb_true;
		}
	}
	END_LOOP;
	return (changed);
}

/* ---------------------------------------------------------------------------
 * selects objects as defined by Type by name;
 * it's a case insensitive match
 * returns pcb_true if any object has been selected
 */
#define REGEXEC(arg) \
	(method == SM_REGEX ? regexec_match_all(regex, (arg)) : strlst_match(pat, (arg)))

static int regexec_match_all(re_sei_t *preg, const char *string)
{
	return !!re_sei_exec(preg, string);
}

/* case insensitive match of each element in the array pat against name
   returns 1 if any of them matched */
static int strlst_match(const char **pat, const char *name)
{
	for (; *pat != NULL; pat++)
		if (strcasecmp(*pat, name) == 0)
			return 1;
	return 0;
}

pcb_bool SelectObjectByName(int Type, const char *name_pattern, pcb_bool Flag, search_method_t method)
{
	pcb_bool changed = pcb_false;
	const char **pat = NULL;
	char *pattern_copy = NULL;
	re_sei_t *regex;

	if (method == SM_REGEX) {
		/* compile the regular expression */
		regex = re_sei_comp(name_pattern);
		if (re_sei_errno(regex) != 0) {
			Message(PCB_MSG_DEFAULT, _("regexp error: %s\n"), re_error_str(re_sei_errno(regex)));
			re_sei_free(regex);
			return (pcb_false);
		}
	}
	else {
		char *s, *next;
		int n, w;

		/* We're going to mess with the delimiters. Create a copy. */
		pattern_copy = pcb_strdup(name_pattern);

		/* count the number of patterns */
		for (s = pattern_copy, w = 0; *s != '\0'; s++) {
			if (*s == '|')
				w++;
		}

		pat = malloc((w + 2) * sizeof(char *));	/* make room for the NULL too */
		for (s = pattern_copy, n = 0; s != NULL; s = next) {
			char *end;
/*fprintf(stderr, "S: '%s'\n", s, next);*/
			while (isspace(*s))
				s++;
			next = strchr(s, '|');
			if (next != NULL) {
				*next = '\0';
				next++;
			}
			end = s + strlen(s) - 1;
			while ((end >= s) && (isspace(*end))) {
				*end = '\0';
				end--;
			}
			if (*s != '\0') {
				pat[n] = s;
				n++;
			}
		}
		pat[n] = NULL;
	}

	/* loop over all visible objects with names */
	if (Type & PCB_TYPE_TEXT)
		ALLTEXT_LOOP(PCB->Data);
	{
		if (!TEST_FLAG(PCB_FLAG_LOCK, text)
				&& TEXT_IS_VISIBLE(PCB, layer, text)
				&& text->TextString && REGEXEC(text->TextString)
				&& TEST_FLAG(PCB_FLAG_SELECTED, text) != Flag) {
			AddObjectToFlagUndoList(PCB_TYPE_TEXT, layer, text, text);
			ASSIGN_FLAG(PCB_FLAG_SELECTED, Flag, text);
			DrawText(layer, text);
			changed = pcb_true;
		}
	}
	ENDALL_LOOP;

	if (PCB->ElementOn && (Type & PCB_TYPE_ELEMENT))
		ELEMENT_LOOP(PCB->Data);
	{
		if (!TEST_FLAG(PCB_FLAG_LOCK, element)
				&& ((TEST_FLAG(PCB_FLAG_ONSOLDER, element) != 0) == SWAP_IDENT || PCB->InvisibleObjectsOn)
				&& TEST_FLAG(PCB_FLAG_SELECTED, element) != Flag) {
			const char* name = ELEMENT_NAME(PCB, element);
			if (name && REGEXEC(name)) {
				AddObjectToFlagUndoList(PCB_TYPE_ELEMENT, element, element, element);
				ASSIGN_FLAG(PCB_FLAG_SELECTED, Flag, element);
				PIN_LOOP(element);
				{
					AddObjectToFlagUndoList(PCB_TYPE_PIN, element, pin, pin);
					ASSIGN_FLAG(PCB_FLAG_SELECTED, Flag, pin);
				}
				END_LOOP;
				PAD_LOOP(element);
				{
					AddObjectToFlagUndoList(PCB_TYPE_PAD, element, pad, pad);
					ASSIGN_FLAG(PCB_FLAG_SELECTED, Flag, pad);
				}
				END_LOOP;
				ELEMENTTEXT_LOOP(element);
				{
					AddObjectToFlagUndoList(PCB_TYPE_ELEMENT_NAME, element, text, text);
					ASSIGN_FLAG(PCB_FLAG_SELECTED, Flag, text);
				}
				END_LOOP;
				DrawElementName(element);
				DrawElement(element);
				changed = pcb_true;
			}
		}
	}
	END_LOOP;
	if (PCB->PinOn && (Type & PCB_TYPE_PIN))
		ALLPIN_LOOP(PCB->Data);
	{
		if (!TEST_FLAG(PCB_FLAG_LOCK, element)
				&& pin->Name && REGEXEC(pin->Name)
				&& TEST_FLAG(PCB_FLAG_SELECTED, pin) != Flag) {
			AddObjectToFlagUndoList(PCB_TYPE_PIN, element, pin, pin);
			ASSIGN_FLAG(PCB_FLAG_SELECTED, Flag, pin);
			DrawPin(pin);
			changed = pcb_true;
		}
	}
	ENDALL_LOOP;
	if (PCB->PinOn && (Type & PCB_TYPE_PAD))
		ALLPAD_LOOP(PCB->Data);
	{
		if (!TEST_FLAG(PCB_FLAG_LOCK, element)
				&& ((TEST_FLAG(PCB_FLAG_ONSOLDER, pad) != 0) == SWAP_IDENT || PCB->InvisibleObjectsOn)
				&& TEST_FLAG(PCB_FLAG_SELECTED, pad) != Flag)
			if (pad->Name && REGEXEC(pad->Name)) {
				AddObjectToFlagUndoList(PCB_TYPE_PAD, element, pad, pad);
				ASSIGN_FLAG(PCB_FLAG_SELECTED, Flag, pad);
				DrawPad(pad);
				changed = pcb_true;
			}
	}
	ENDALL_LOOP;
	if (PCB->ViaOn && (Type & PCB_TYPE_VIA))
		VIA_LOOP(PCB->Data);
	{
		if (!TEST_FLAG(PCB_FLAG_LOCK, via)
				&& via->Name && REGEXEC(via->Name) && TEST_FLAG(PCB_FLAG_SELECTED, via) != Flag) {
			AddObjectToFlagUndoList(PCB_TYPE_VIA, via, via, via);
			ASSIGN_FLAG(PCB_FLAG_SELECTED, Flag, via);
			DrawVia(via);
			changed = pcb_true;
		}
	}
	END_LOOP;
	if (Type & PCB_TYPE_NET) {
		InitConnectionLookup();
		changed = ResetConnections(pcb_true) || changed;

		MENU_LOOP(&(PCB->NetlistLib[NETLIST_EDITED]));
		{
			pcb_cardinal_t i;
			LibraryEntryType *entry;
			pcb_connection_t conn;

			/* Name[0] and Name[1] are special purpose, not the actual name */
			if (menu->Name && menu->Name[0] != '\0' && menu->Name[1] != '\0' && REGEXEC(menu->Name + 2)) {
				for (i = menu->EntryN, entry = menu->Entry; i; i--, entry++)
					if (SeekPad(entry, &conn, pcb_false))
						RatFindHook(conn.type, conn.ptr1, conn.ptr2, conn.ptr2, pcb_true, pcb_true);
			}
		}
		END_LOOP;

		changed = SelectConnection(Flag) || changed;
		changed = ResetConnections(pcb_false) || changed;
		FreeConnectionLookupMemory();
	}

	if (method == SM_REGEX)
		re_sei_free(regex);

	if (changed) {
		IncrementUndoSerialNumber();
		Draw();
	}
	if (pat != NULL)
		free(pat);
	if (pattern_copy != NULL)
		free(pattern_copy);
	return (changed);
}
