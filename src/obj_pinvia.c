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

/* Drawing primitive: pins and vias */

#include "config.h"
#include "board.h"
#include "data.h"
#include "undo.h"
#include "conf_core.h"
#include "polygon.h"
#include "compat_nls.h"
#include "compat_misc.h"
#include "stub_vendor.h"

#include "obj_pinvia.h"
#include "obj_pinvia_op.h"

/* TODO: consider removing this by moving pin/via functions here: */
#include "draw.h"
#include "obj_text_draw.h"
#include "obj_pinvia_draw.h"

/*** allocation ***/

/* get next slot for a via, allocates memory if necessary */
pcb_pin_t *pcb_via_alloc(pcb_data_t * data)
{
	pcb_pin_t *new_obj;

	new_obj = calloc(sizeof(pcb_pin_t), 1);
	pinlist_append(&data->Via, new_obj);

	return new_obj;
}

void pcb_via_free(pcb_pin_t * data)
{
	pinlist_remove(data);
	free(data);
}

/* get next slot for a pin, allocates memory if necessary */
pcb_pin_t *pcb_pin_alloc(pcb_element_t * element)
{
	pcb_pin_t *new_obj;

	new_obj = calloc(sizeof(pcb_pin_t), 1);
	pinlist_append(&element->Pin, new_obj);

	return new_obj;
}

void pcb_pin_free(pcb_pin_t * data)
{
	pinlist_remove(data);
	free(data);
}



/*** utility ***/

/* creates a new via */
pcb_pin_t *pcb_via_new(pcb_data_t *Data, pcb_coord_t X, pcb_coord_t Y, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_coord_t Mask, pcb_coord_t DrillingHole, const char *Name, pcb_flag_t Flags)
{
	pcb_pin_t *Via;

	if (!pcb_create_be_lenient) {
		VIA_LOOP(Data);
		{
			if (pcb_distance(X, Y, via->X, via->Y) <= via->DrillingHole / 2 + DrillingHole / 2) {
				pcb_message(PCB_MSG_DEFAULT, _("%m+Dropping via at %$mD because it's hole would overlap with the via "
									"at %$mD\n"), conf_core.editor.grid_unit->allow, X, Y, via->X, via->Y);
				return (NULL);					/* don't allow via stacking */
			}
		}
		END_LOOP;
	}

	Via = pcb_via_alloc(Data);

	if (!Via)
		return (Via);
	/* copy values */
	Via->X = X;
	Via->Y = Y;
	Via->Thickness = Thickness;
	Via->Clearance = Clearance;
	Via->Mask = Mask;
	Via->DrillingHole = stub_vendorDrillMap(DrillingHole);
	if (Via->DrillingHole != DrillingHole) {
		pcb_message(PCB_MSG_DEFAULT, _("%m+Mapped via drill hole to %$mS from %$mS per vendor table\n"),
						conf_core.editor.grid_unit->allow, Via->DrillingHole, DrillingHole);
	}

	Via->Name = pcb_strdup_null(Name);
	Via->Flags = Flags;
	PCB_FLAG_CLEAR(PCB_FLAG_WARN, Via);
	PCB_FLAG_SET(PCB_FLAG_VIA, Via);
	Via->ID = CreateIDGet();

	/*
	 * don't complain about MIN_PINORVIACOPPER on a mounting hole (pure
	 * hole)
	 */
	if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, Via) && (Via->Thickness < Via->DrillingHole + MIN_PINORVIACOPPER)) {
		Via->Thickness = Via->DrillingHole + MIN_PINORVIACOPPER;
		pcb_message(PCB_MSG_DEFAULT, _("%m+Increased via thickness to %$mS to allow enough copper"
							" at %$mD.\n"), conf_core.editor.grid_unit->allow, Via->Thickness, Via->X, Via->Y);
	}

	pcb_add_via(Data, Via);
	return (Via);
}

/* creates a new pin in an element */
pcb_pin_t *pcb_element_pin_new(pcb_element_t *Element, pcb_coord_t X, pcb_coord_t Y, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_coord_t Mask, pcb_coord_t DrillingHole, char *Name, char *Number, pcb_flag_t Flags)
{
	pcb_pin_t *pin = pcb_pin_alloc(Element);

	/* copy values */
	pin->X = X;
	pin->Y = Y;
	pin->Thickness = Thickness;
	pin->Clearance = Clearance;
	pin->Mask = Mask;
	pin->Name = pcb_strdup_null(Name);
	pin->Number = pcb_strdup_null(Number);
	pin->Flags = Flags;
	PCB_FLAG_CLEAR(PCB_FLAG_WARN, pin);
	PCB_FLAG_SET(PCB_FLAG_PIN, pin);
	pin->ID = CreateIDGet();
	pin->Element = Element;

	/*
	 * If there is no vendor drill map installed, this will simply
	 * return DrillingHole.
	 */
	pin->DrillingHole = stub_vendorDrillMap(DrillingHole);

	/* Unless we should not map drills on this element, map them! */
	if (stub_vendorIsElementMappable(Element)) {
		if (pin->DrillingHole < MIN_PINORVIASIZE) {
			pcb_message(PCB_MSG_DEFAULT, _("%m+Did not map pin #%s (%s) drill hole because %$mS is below the minimum allowed size\n"),
							conf_core.editor.grid_unit->allow, PCB_UNKNOWN(Number), PCB_UNKNOWN(Name), pin->DrillingHole);
			pin->DrillingHole = DrillingHole;
		}
		else if (pin->DrillingHole > MAX_PINORVIASIZE) {
			pcb_message(PCB_MSG_DEFAULT, _("%m+Did not map pin #%s (%s) drill hole because %$mS is above the maximum allowed size\n"),
							conf_core.editor.grid_unit->allow, PCB_UNKNOWN(Number), PCB_UNKNOWN(Name), pin->DrillingHole);
			pin->DrillingHole = DrillingHole;
		}
		else if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, pin)
						 && (pin->DrillingHole > pin->Thickness - MIN_PINORVIACOPPER)) {
			pcb_message(PCB_MSG_DEFAULT, _("%m+Did not map pin #%s (%s) drill hole because %$mS does not leave enough copper\n"),
							conf_core.editor.grid_unit->allow, PCB_UNKNOWN(Number), PCB_UNKNOWN(Name), pin->DrillingHole);
			pin->DrillingHole = DrillingHole;
		}
	}
	else {
		pin->DrillingHole = DrillingHole;
	}

	if (pin->DrillingHole != DrillingHole) {
		pcb_message(PCB_MSG_DEFAULT, _("%m+Mapped pin drill hole to %$mS from %$mS per vendor table\n"),
						conf_core.editor.grid_unit->allow, pin->DrillingHole, DrillingHole);
	}

	return (pin);
}



void pcb_add_via(pcb_data_t *Data, pcb_pin_t *Via)
{
	pcb_pin_bbox(Via);
	if (!Data->via_tree)
		Data->via_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(Data->via_tree, (pcb_box_t *) Via, 0);
}

/* sets the bounding box of a pin or via */
void pcb_pin_bbox(pcb_pin_t *Pin)
{
	pcb_coord_t width;

	if ((PCB_FLAG_SQUARE_GET(Pin) > 1) && (PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pin))) {
		pcb_polyarea_t *p = PinPoly(Pin, PIN_SIZE(Pin), Pin->Clearance);
		poly_bbox(p, &Pin->BoundingBox);
		poly_Free(&p);
	}

	/* the bounding box covers the extent of influence
	 * so it must include the clearance values too
	 */
	width = MAX(Pin->Clearance + PIN_SIZE(Pin), Pin->Mask) / 2;

	/* Adjust for our discrete polygon approximation */
	width = (double) width *POLY_CIRC_RADIUS_ADJ + 0.5;

	Pin->BoundingBox.X1 = Pin->X - width;
	Pin->BoundingBox.Y1 = Pin->Y - width;
	Pin->BoundingBox.X2 = Pin->X + width;
	Pin->BoundingBox.Y2 = Pin->Y + width;
	pcb_close_box(&Pin->BoundingBox);
}



/*** ops ***/
/* copies a via to paste buffer */
void *AddViaToBuffer(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	return (pcb_via_new(ctx->buffer.dst, Via->X, Via->Y, Via->Thickness, Via->Clearance, Via->Mask, Via->DrillingHole, Via->Name, pcb_flag_mask(Via->Flags, PCB_FLAG_FOUND | ctx->buffer.extraflg)));
}

/* moves a via to paste buffer without allocating memory for the name */
void *MoveViaToBuffer(pcb_opctx_t *ctx, pcb_pin_t * via)
{
	RestoreToPolygon(ctx->buffer.src, PCB_TYPE_VIA, via, via);

	r_delete_entry(ctx->buffer.src->via_tree, (pcb_box_t *) via);
	pinlist_remove(via);
	pinlist_append(&ctx->buffer.dst->Via, via);

	PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND, via);

	if (!ctx->buffer.dst->via_tree)
		ctx->buffer.dst->via_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(ctx->buffer.dst->via_tree, (pcb_box_t *) via, 0);
	ClearFromPolygon(ctx->buffer.dst, PCB_TYPE_VIA, via, via);
	return via;
}

/* changes the thermal on a via */
void *ChangeViaThermal(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	AddObjectToClearPolyUndoList(PCB_TYPE_VIA, Via, Via, Via, pcb_false);
	RestoreToPolygon(PCB->Data, PCB_TYPE_VIA, CURRENT, Via);
	AddObjectToFlagUndoList(PCB_TYPE_VIA, Via, Via, Via);
	if (!ctx->chgtherm.style)										/* remove the thermals */
		PCB_FLAG_THERM_CLEAR(INDEXOFCURRENT, Via);
	else
		PCB_FLAG_THERM_ASSIGN(INDEXOFCURRENT, ctx->chgtherm.style, Via);
	AddObjectToClearPolyUndoList(PCB_TYPE_VIA, Via, Via, Via, pcb_true);
	ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, CURRENT, Via);
	DrawVia(Via);
	return Via;
}

/* changes the thermal on a pin */
void *ChangePinThermal(pcb_opctx_t *ctx, pcb_element_t *element, pcb_pin_t *Pin)
{
	AddObjectToClearPolyUndoList(PCB_TYPE_PIN, element, Pin, Pin, pcb_false);
	RestoreToPolygon(PCB->Data, PCB_TYPE_VIA, CURRENT, Pin);
	AddObjectToFlagUndoList(PCB_TYPE_PIN, element, Pin, Pin);
	if (!ctx->chgtherm.style)										/* remove the thermals */
		PCB_FLAG_THERM_CLEAR(INDEXOFCURRENT, Pin);
	else
		PCB_FLAG_THERM_ASSIGN(INDEXOFCURRENT, ctx->chgtherm.style, Pin);
	AddObjectToClearPolyUndoList(PCB_TYPE_PIN, element, Pin, Pin, pcb_true);
	ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, CURRENT, Pin);
	DrawPin(Pin);
	return Pin;
}

/* changes the size of a via */
void *ChangeViaSize(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	pcb_coord_t value = ctx->chgsize.absolute ? ctx->chgsize.absolute : Via->Thickness + ctx->chgsize.delta;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via))
		return (NULL);
	if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, Via) && value <= MAX_PINORVIASIZE &&
			value >= MIN_PINORVIASIZE && value >= Via->DrillingHole + MIN_PINORVIACOPPER && value != Via->Thickness) {
		AddObjectToSizeUndoList(PCB_TYPE_VIA, Via, Via, Via);
		EraseVia(Via);
		r_delete_entry(PCB->Data->via_tree, (pcb_box_t *) Via);
		RestoreToPolygon(PCB->Data, PCB_TYPE_PIN, Via, Via);
		if (Via->Mask) {
			AddObjectToMaskSizeUndoList(PCB_TYPE_VIA, Via, Via, Via);
			Via->Mask += value - Via->Thickness;
		}
		Via->Thickness = value;
		pcb_pin_bbox(Via);
		r_insert_entry(PCB->Data->via_tree, (pcb_box_t *) Via, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
		DrawVia(Via);
		return (Via);
	}
	return (NULL);
}

/* changes the drilling hole of a via */
void *ChangeVia2ndSize(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Via->DrillingHole + ctx->chgsize.delta;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via))
		return (NULL);
	if (value <= MAX_PINORVIASIZE &&
			value >= MIN_PINORVIAHOLE && (PCB_FLAG_TEST(PCB_FLAG_HOLE, Via) || value <= Via->Thickness - MIN_PINORVIACOPPER)
			&& value != Via->DrillingHole) {
		AddObjectTo2ndSizeUndoList(PCB_TYPE_VIA, Via, Via, Via);
		EraseVia(Via);
		RestoreToPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
		Via->DrillingHole = value;
		if (PCB_FLAG_TEST(PCB_FLAG_HOLE, Via)) {
			AddObjectToSizeUndoList(PCB_TYPE_VIA, Via, Via, Via);
			Via->Thickness = value;
		}
		ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
		DrawVia(Via);
		return (Via);
	}
	return (NULL);
}

/* changes the drilling hole of a pin */
void *ChangePin2ndSize(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pin->DrillingHole + ctx->chgsize.delta;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin))
		return (NULL);
	if (value <= MAX_PINORVIASIZE &&
			value >= MIN_PINORVIAHOLE && (PCB_FLAG_TEST(PCB_FLAG_HOLE, Pin) || value <= Pin->Thickness - MIN_PINORVIACOPPER)
			&& value != Pin->DrillingHole) {
		AddObjectTo2ndSizeUndoList(PCB_TYPE_PIN, Element, Pin, Pin);
		ErasePin(Pin);
		RestoreToPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
		Pin->DrillingHole = value;
		if (PCB_FLAG_TEST(PCB_FLAG_HOLE, Pin)) {
			AddObjectToSizeUndoList(PCB_TYPE_PIN, Element, Pin, Pin);
			Pin->Thickness = value;
		}
		ClearFromPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
		DrawPin(Pin);
		return (Pin);
	}
	return (NULL);
}


/* changes the clearance size of a via */
void *ChangeViaClearSize(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Via->Clearance + ctx->chgsize.delta;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via))
		return (NULL);
	value = MIN(MAX_LINESIZE, value);
	if (value < 0)
		value = 0;
	if (ctx->chgsize.delta < 0 && value < PCB->Bloat * 2)
		value = 0;
	if ((ctx->chgsize.delta > 0 || ctx->chgsize.absolute) && value < PCB->Bloat * 2)
		value = PCB->Bloat * 2 + 2;
	if (Via->Clearance == value)
		return NULL;
	RestoreToPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
	AddObjectToClearSizeUndoList(PCB_TYPE_VIA, Via, Via, Via);
	EraseVia(Via);
	r_delete_entry(PCB->Data->via_tree, (pcb_box_t *) Via);
	Via->Clearance = value;
	pcb_pin_bbox(Via);
	r_insert_entry(PCB->Data->via_tree, (pcb_box_t *) Via, 0);
	ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
	DrawVia(Via);
	Via->Element = NULL;
	return (Via);
}


/* changes the size of a pin */
void *ChangePinSize(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pin->Thickness + ctx->chgsize.delta;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin))
		return (NULL);
	if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, Pin) && value <= MAX_PINORVIASIZE &&
			value >= MIN_PINORVIASIZE && value >= Pin->DrillingHole + MIN_PINORVIACOPPER && value != Pin->Thickness) {
		AddObjectToSizeUndoList(PCB_TYPE_PIN, Element, Pin, Pin);
		AddObjectToMaskSizeUndoList(PCB_TYPE_PIN, Element, Pin, Pin);
		ErasePin(Pin);
		r_delete_entry(PCB->Data->pin_tree, &Pin->BoundingBox);
		RestoreToPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
		Pin->Mask += value - Pin->Thickness;
		Pin->Thickness = value;
		/* SetElementBB updates all associated rtrees */
		pcb_element_bbox(PCB->Data, Element, &PCB->Font);
		ClearFromPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
		DrawPin(Pin);
		return (Pin);
	}
	return (NULL);
}

/* changes the clearance size of a pin */
void *ChangePinClearSize(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pin->Clearance + ctx->chgsize.delta;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin))
		return (NULL);
	value = MIN(MAX_LINESIZE, value);
	if (value < 0)
		value = 0;
	if (ctx->chgsize.delta < 0 && value < PCB->Bloat * 2)
		value = 0;
	if ((ctx->chgsize.delta > 0 || ctx->chgsize.absolute) && value < PCB->Bloat * 2)
		value = PCB->Bloat * 2 + 2;
	if (Pin->Clearance == value)
		return NULL;
	RestoreToPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	AddObjectToClearSizeUndoList(PCB_TYPE_PIN, Element, Pin, Pin);
	ErasePin(Pin);
	r_delete_entry(PCB->Data->pin_tree, &Pin->BoundingBox);
	Pin->Clearance = value;
	/* SetElementBB updates all associated rtrees */
	pcb_element_bbox(PCB->Data, Element, &PCB->Font);
	ClearFromPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	DrawPin(Pin);
	return (Pin);
}

/* changes the name of a via */
void *ChangeViaName(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	char *old = Via->Name;

	if (PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, Via)) {
		ErasePinName(Via);
		Via->Name = ctx->chgname.new_name;
		DrawPinName(Via);
	}
	else
		Via->Name = ctx->chgname.new_name;
	return (old);
}

/* changes the name of a pin */
void *ChangePinName(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	char *old = Pin->Name;

	Element = Element;						/* get rid of 'unused...' warnings */
	if (PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, Pin)) {
		ErasePinName(Pin);
		Pin->Name = ctx->chgname.new_name;
		DrawPinName(Pin);
	}
	else
		Pin->Name = ctx->chgname.new_name;
	return (old);
}

/* changes the number of a pin */
void *ChangePinNum(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	char *old = Pin->Number;

	Element = Element;						/* get rid of 'unused...' warnings */
	if (PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, Pin)) {
		ErasePinName(Pin);
		Pin->Number = ctx->chgname.new_name;
		DrawPinName(Pin);
	}
	else
		Pin->Number = ctx->chgname.new_name;
	return (old);
}


/* changes the square flag of a via */
void *ChangeViaSquare(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via))
		return (NULL);
	EraseVia(Via);
	AddObjectToClearPolyUndoList(PCB_TYPE_VIA, NULL, Via, Via, pcb_false);
	RestoreToPolygon(PCB->Data, PCB_TYPE_VIA, NULL, Via);
	AddObjectToFlagUndoList(PCB_TYPE_VIA, NULL, Via, Via);
	PCB_FLAG_SQUARE_ASSIGN(ctx->chgsize.absolute, Via);
	if (ctx->chgsize.absolute == 0)
		PCB_FLAG_CLEAR(PCB_FLAG_SQUARE, Via);
	else
		PCB_FLAG_SET(PCB_FLAG_SQUARE, Via);
	pcb_pin_bbox(Via);
	AddObjectToClearPolyUndoList(PCB_TYPE_VIA, NULL, Via, Via, pcb_true);
	ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, NULL, Via);
	DrawVia(Via);
	return (Via);
}

/* changes the square flag of a pin */
void *ChangePinSquare(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin))
		return (NULL);
	ErasePin(Pin);
	AddObjectToClearPolyUndoList(PCB_TYPE_PIN, Element, Pin, Pin, pcb_false);
	RestoreToPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	AddObjectToFlagUndoList(PCB_TYPE_PIN, Element, Pin, Pin);
	PCB_FLAG_SQUARE_ASSIGN(ctx->chgsize.absolute, Pin);
	if (ctx->chgsize.absolute == 0)
		PCB_FLAG_CLEAR(PCB_FLAG_SQUARE, Pin);
	else
		PCB_FLAG_SET(PCB_FLAG_SQUARE, Pin);
	pcb_pin_bbox(Pin);
	AddObjectToClearPolyUndoList(PCB_TYPE_PIN, Element, Pin, Pin, pcb_true);
	ClearFromPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	DrawPin(Pin);
	return (Pin);
}

/* sets the square flag of a pin */
void *SetPinSquare(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin) || PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pin))
		return (NULL);

	return (ChangePinSquare(ctx, Element, Pin));
}

/* clears the square flag of a pin */
void *ClrPinSquare(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin) || !PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pin))
		return (NULL);

	return (ChangePinSquare(ctx, Element, Pin));
}

/* changes the octagon flag of a via */
void *ChangeViaOctagon(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via))
		return (NULL);
	EraseVia(Via);
	AddObjectToClearPolyUndoList(PCB_TYPE_VIA, Via, Via, Via, pcb_false);
	RestoreToPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
	AddObjectToFlagUndoList(PCB_TYPE_VIA, Via, Via, Via);
	PCB_FLAG_TOGGLE(PCB_FLAG_OCTAGON, Via);
	AddObjectToClearPolyUndoList(PCB_TYPE_VIA, Via, Via, Via, pcb_true);
	ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
	DrawVia(Via);
	return (Via);
}

/* sets the octagon flag of a via */
void *SetViaOctagon(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via) || PCB_FLAG_TEST(PCB_FLAG_OCTAGON, Via))
		return (NULL);

	return (ChangeViaOctagon(ctx, Via));
}

/* clears the octagon flag of a via */
void *ClrViaOctagon(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via) || !PCB_FLAG_TEST(PCB_FLAG_OCTAGON, Via))
		return (NULL);

	return (ChangeViaOctagon(ctx, Via));
}

/* changes the octagon flag of a pin */
void *ChangePinOctagon(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin))
		return (NULL);
	ErasePin(Pin);
	AddObjectToClearPolyUndoList(PCB_TYPE_PIN, Element, Pin, Pin, pcb_false);
	RestoreToPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	AddObjectToFlagUndoList(PCB_TYPE_PIN, Element, Pin, Pin);
	PCB_FLAG_TOGGLE(PCB_FLAG_OCTAGON, Pin);
	AddObjectToClearPolyUndoList(PCB_TYPE_PIN, Element, Pin, Pin, pcb_true);
	ClearFromPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	DrawPin(Pin);
	return (Pin);
}

/* sets the octagon flag of a pin */
void *SetPinOctagon(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin) || PCB_FLAG_TEST(PCB_FLAG_OCTAGON, Pin))
		return (NULL);

	return (ChangePinOctagon(ctx, Element, Pin));
}

/* clears the octagon flag of a pin */
void *ClrPinOctagon(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pin) || !PCB_FLAG_TEST(PCB_FLAG_OCTAGON, Pin))
		return (NULL);

	return (ChangePinOctagon(ctx, Element, Pin));
}

/* changes the hole flag of a via */
pcb_bool pcb_pin_change_hole(pcb_pin_t *Via)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Via))
		return (pcb_false);
	EraseVia(Via);
	AddObjectToFlagUndoList(PCB_TYPE_VIA, Via, Via, Via);
	AddObjectToMaskSizeUndoList(PCB_TYPE_VIA, Via, Via, Via);
	r_delete_entry(PCB->Data->via_tree, (pcb_box_t *) Via);
	RestoreToPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
	PCB_FLAG_TOGGLE(PCB_FLAG_HOLE, Via);

	if (PCB_FLAG_TEST(PCB_FLAG_HOLE, Via)) {
		/* A tented via becomes an minimally untented hole.  An untented
		   via retains its mask clearance.  */
		if (Via->Mask > Via->Thickness) {
			Via->Mask = (Via->DrillingHole + (Via->Mask - Via->Thickness));
		}
		else if (Via->Mask < Via->DrillingHole) {
			Via->Mask = Via->DrillingHole + 2 * MASKFRAME;
		}
	}
	else {
		Via->Mask = (Via->Thickness + (Via->Mask - Via->DrillingHole));
	}

	pcb_pin_bbox(Via);
	r_insert_entry(PCB->Data->via_tree, (pcb_box_t *) Via, 0);
	ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
	DrawVia(Via);
	pcb_draw();
	return (pcb_true);
}

/* changes the mask size of a pin */
void *ChangePinMaskSize(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pin_t *Pin)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pin->Mask + ctx->chgsize.delta;

	value = MAX(value, 0);
	if (value == Pin->Mask && ctx->chgsize.absolute == 0)
		value = Pin->Thickness;
	if (value != Pin->Mask) {
		AddObjectToMaskSizeUndoList(PCB_TYPE_PIN, Element, Pin, Pin);
		ErasePin(Pin);
		r_delete_entry(PCB->Data->pin_tree, &Pin->BoundingBox);
		Pin->Mask = value;
		pcb_element_bbox(PCB->Data, Element, &PCB->Font);
		DrawPin(Pin);
		return (Pin);
	}
	return (NULL);
}

/* changes the mask size of a via */
void *ChangeViaMaskSize(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	pcb_coord_t value;

	value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Via->Mask + ctx->chgsize.delta;
	value = MAX(value, 0);
	if (value != Via->Mask) {
		AddObjectToMaskSizeUndoList(PCB_TYPE_VIA, Via, Via, Via);
		EraseVia(Via);
		r_delete_entry(PCB->Data->via_tree, &Via->BoundingBox);
		Via->Mask = value;
		pcb_pin_bbox(Via);
		r_insert_entry(PCB->Data->via_tree, &Via->BoundingBox, 0);
		DrawVia(Via);
		return (Via);
	}
	return (NULL);
}

/* copies a via */
void *CopyVia(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	pcb_pin_t *via;

	via = pcb_via_new(PCB->Data, Via->X + ctx->copy.DeltaX, Via->Y + ctx->copy.DeltaY,
										 Via->Thickness, Via->Clearance, Via->Mask, Via->DrillingHole, Via->Name, pcb_flag_mask(Via->Flags, PCB_FLAG_FOUND));
	if (!via)
		return (via);
	DrawVia(via);
	AddObjectToCreateUndoList(PCB_TYPE_VIA, via, via, via);
	return (via);
}

/* moves a via */
void *MoveVia(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	r_delete_entry(PCB->Data->via_tree, (pcb_box_t *) Via);
	RestoreToPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
	MOVE_VIA_LOWLEVEL(Via, ctx->move.dx, ctx->move.dy);
	if (PCB->ViaOn)
		EraseVia(Via);
	r_insert_entry(PCB->Data->via_tree, (pcb_box_t *) Via, 0);
	ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
	if (PCB->ViaOn) {
		DrawVia(Via);
		pcb_draw();
	}
	return (Via);
}

/* destroys a via */
void *DestroyVia(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	r_delete_entry(ctx->remove.destroy_target->via_tree, (pcb_box_t *) Via);
	free(Via->Name);

	pcb_via_free(Via);
	return NULL;
}

/* removes a via */
void *RemoveVia(pcb_opctx_t *ctx, pcb_pin_t *Via)
{
	/* erase from screen and memory */
	if (PCB->ViaOn) {
		EraseVia(Via);
		if (!ctx->remove.bulk)
			pcb_draw();
	}
	MoveObjectToRemoveUndoList(PCB_TYPE_VIA, Via, Via, Via);
	return NULL;
}

/*** draw ***/

/* setup color for pin or via */
static void SetPVColor(pcb_pin_t *Pin, int Type)
{
	char *color;
	char buf[sizeof("#XXXXXX")];

	if (Type == PCB_TYPE_VIA) {
		if (!pcb_draw_doing_pinout && PCB_FLAG_TEST(PCB_FLAG_WARN | PCB_FLAG_SELECTED | PCB_FLAG_FOUND, Pin)) {
			if (PCB_FLAG_TEST(PCB_FLAG_WARN, Pin))
				color = PCB->WarnColor;
			else if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, Pin))
				color = PCB->ViaSelectedColor;
			else
				color = PCB->ConnectedColor;

			if (PCB_FLAG_TEST(PCB_FLAG_ONPOINT, Pin)) {
				assert(color != NULL);
				pcb_lighten_color(color, buf, 1.75);
				color = buf;
			}
		}
		else
			color = PCB->ViaColor;
	}
	else {
		if (!pcb_draw_doing_pinout && PCB_FLAG_TEST(PCB_FLAG_WARN | PCB_FLAG_SELECTED | PCB_FLAG_FOUND, Pin)) {
			if (PCB_FLAG_TEST(PCB_FLAG_WARN, Pin))
				color = PCB->WarnColor;
			else if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, Pin))
				color = PCB->PinSelectedColor;
			else
				color = PCB->ConnectedColor;

			if (PCB_FLAG_TEST(PCB_FLAG_ONPOINT, Pin)) {
				assert(color != NULL);
				pcb_lighten_color(color, buf, 1.75);
				color = buf;
			}
		}
		else
			color = PCB->PinColor;
	}

	gui->set_color(Output.fgGC, color);
}

static void _draw_pv_name(pcb_pin_t * pv)
{
	pcb_box_t box;
	pcb_bool vert;
	pcb_text_t text;
	char buff[128];
	const char *pn;

	if (!pv->Name || !pv->Name[0])
		pn = PCB_EMPTY(pv->Number);
	else
		pn = PCB_EMPTY(conf_core.editor.show_number ? pv->Number : pv->Name);

	if (PCB_FLAG_INTCONN_GET(pv) > 0)
		pcb_snprintf(buff, sizeof(buff), "%s[%d]", pn, PCB_FLAG_INTCONN_GET(pv));
	else
		strcpy(buff, pn);
	text.TextString = buff;

	vert = PCB_FLAG_TEST(PCB_FLAG_EDGE2, pv);

	if (vert) {
		box.X1 = pv->X - pv->Thickness / 2 + conf_core.appearance.pinout.text_offset_y;
		box.Y1 = pv->Y - pv->DrillingHole / 2 - conf_core.appearance.pinout.text_offset_x;
	}
	else {
		box.X1 = pv->X + pv->DrillingHole / 2 + conf_core.appearance.pinout.text_offset_x;
		box.Y1 = pv->Y - pv->Thickness / 2 + conf_core.appearance.pinout.text_offset_y;
	}

	gui->set_color(Output.fgGC, PCB->PinNameColor);

	text.Flags = pcb_no_flags();
	/* Set font height to approx 56% of pin thickness */
	text.Scale = 56 * pv->Thickness / FONT_CAPHEIGHT;
	text.X = box.X1;
	text.Y = box.Y1;
	text.Direction = vert ? 1 : 0;

	if (gui->gui)
		pcb_draw_doing_pinout++;
	DrawTextLowLevel(&text, 0);
	if (gui->gui)
		pcb_draw_doing_pinout--;
}

static void _draw_pv(pcb_pin_t *pv, pcb_bool draw_hole)
{
	if (conf_core.editor.thin_draw)
		gui->thindraw_pcb_pv(Output.fgGC, Output.fgGC, pv, draw_hole, pcb_false);
	else
		gui->fill_pcb_pv(Output.fgGC, Output.bgGC, pv, draw_hole, pcb_false);

	if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, pv) && PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, pv))
		_draw_pv_name(pv);
}

void draw_pin(pcb_pin_t *pin, pcb_bool draw_hole)
{
	SetPVColor(pin, PCB_TYPE_PIN);
	_draw_pv(pin, draw_hole);
}

pcb_r_dir_t draw_pin_callback(const pcb_box_t * b, void *cl)
{
	draw_pin((pcb_pin_t *) b, pcb_false);
	return R_DIR_FOUND_CONTINUE;
}

pcb_r_dir_t clear_pin_callback(const pcb_box_t * b, void *cl)
{
	pcb_pin_t *pin = (pcb_pin_t *) b;
	if (conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly)
		gui->thindraw_pcb_pv(Output.pmGC, Output.pmGC, pin, pcb_false, pcb_true);
	else
		gui->fill_pcb_pv(Output.pmGC, Output.pmGC, pin, pcb_false, pcb_true);
	return R_DIR_FOUND_CONTINUE;
}


static void draw_via(pcb_pin_t *via, pcb_bool draw_hole)
{
	SetPVColor(via, PCB_TYPE_VIA);
	_draw_pv(via, draw_hole);
}

pcb_r_dir_t draw_via_callback(const pcb_box_t * b, void *cl)
{
	draw_via((pcb_pin_t *) b, pcb_false);
	return R_DIR_FOUND_CONTINUE;
}

pcb_r_dir_t draw_hole_callback(const pcb_box_t * b, void *cl)
{
	pcb_pin_t *pv = (pcb_pin_t *) b;
	int plated = cl ? *(int *) cl : -1;
	const char *color;
	char buf[sizeof("#XXXXXX")];

	if ((plated == 0 && !PCB_FLAG_TEST(PCB_FLAG_HOLE, pv)) || (plated == 1 && PCB_FLAG_TEST(PCB_FLAG_HOLE, pv)))
		return R_DIR_FOUND_CONTINUE;

	if (conf_core.editor.thin_draw) {
		if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, pv)) {
			gui->set_line_cap(Output.fgGC, Round_Cap);
			gui->set_line_width(Output.fgGC, 0);
			gui->draw_arc(Output.fgGC, pv->X, pv->Y, pv->DrillingHole / 2, pv->DrillingHole / 2, 0, 360);
		}
	}
	else
		gui->fill_circle(Output.bgGC, pv->X, pv->Y, pv->DrillingHole / 2);

	if (PCB_FLAG_TEST(PCB_FLAG_HOLE, pv)) {
		if (PCB_FLAG_TEST(PCB_FLAG_WARN, pv))
			color = PCB->WarnColor;
		else if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, pv))
			color = PCB->ViaSelectedColor;
		else
			color = conf_core.appearance.color.black;

		if (PCB_FLAG_TEST(PCB_FLAG_ONPOINT, pv)) {
			assert(color != NULL);
			pcb_lighten_color(color, buf, 1.75);
			color = buf;
		}
		gui->set_color(Output.fgGC, color);

		gui->set_line_cap(Output.fgGC, Round_Cap);
		gui->set_line_width(Output.fgGC, 0);
		gui->draw_arc(Output.fgGC, pv->X, pv->Y, pv->DrillingHole / 2, pv->DrillingHole / 2, 0, 360);
	}
	return R_DIR_FOUND_CONTINUE;
}

static void GatherPVName(pcb_pin_t *Ptr)
{
	pcb_box_t box;
	pcb_bool vert = PCB_FLAG_TEST(PCB_FLAG_EDGE2, Ptr);

	if (vert) {
		box.X1 = Ptr->X - Ptr->Thickness / 2 + conf_core.appearance.pinout.text_offset_y;
		box.Y1 = Ptr->Y - Ptr->DrillingHole / 2 - conf_core.appearance.pinout.text_offset_x;
	}
	else {
		box.X1 = Ptr->X + Ptr->DrillingHole / 2 + conf_core.appearance.pinout.text_offset_x;
		box.Y1 = Ptr->Y - Ptr->Thickness / 2 + conf_core.appearance.pinout.text_offset_y;
	}

	if (vert) {
		box.X2 = box.X1;
		box.Y2 = box.Y1;
	}
	else {
		box.X2 = box.X1;
		box.Y2 = box.Y1;
	}
	pcb_draw_invalidate(&box);
}

void EraseVia(pcb_pin_t *Via)
{
	pcb_draw_invalidate(Via);
	if (PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, Via))
		EraseViaName(Via);
	pcb_flag_erase(&Via->Flags);
}

void EraseViaName(pcb_pin_t *Via)
{
	GatherPVName(Via);
}

void ErasePin(pcb_pin_t *Pin)
{
	pcb_draw_invalidate(Pin);
	if (PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, Pin))
		ErasePinName(Pin);
	pcb_flag_erase(&Pin->Flags);
}

void ErasePinName(pcb_pin_t *Pin)
{
	GatherPVName(Pin);
}

void DrawVia(pcb_pin_t *Via)
{
	pcb_draw_invalidate(Via);
	if (!PCB_FLAG_TEST(PCB_FLAG_HOLE, Via) && PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, Via))
		DrawViaName(Via);
}

void DrawViaName(pcb_pin_t *Via)
{
	GatherPVName(Via);
}

void DrawPin(pcb_pin_t *Pin)
{
	pcb_draw_invalidate(Pin);
	if ((!PCB_FLAG_TEST(PCB_FLAG_HOLE, Pin) && PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, Pin))
			|| pcb_draw_doing_pinout)
		DrawPinName(Pin);
}

void DrawPinName(pcb_pin_t *Pin)
{
	GatherPVName(Pin);
}
