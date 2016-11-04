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
#include "global_objs.h"
#include "global_element.h"
#include "const.h"
#include "board.h"
#include "data.h"
#include "undo.h"
#include "box.h"
#include "math_helper.h"
#include "conf_core.h"
#include "polygon.h"
#include "create.h"
#include "compat_nls.h"
#include "compat_misc.h"
#include "stub_vendor.h"
#include "misc.h"

#include "obj_pinvia.h"
#include "obj_pinvia_op.h"

/* TODO: consider removing this by moving pin/via functions here: */
#include "draw.h"

/*** allocation ***/

/* get next slot for a via, allocates memory if necessary */
PinType *GetViaMemory(DataType * data)
{
	PinType *new_obj;

	new_obj = calloc(sizeof(PinType), 1);
	pinlist_append(&data->Via, new_obj);

	return new_obj;
}

void RemoveFreeVia(PinType * data)
{
	pinlist_remove(data);
	free(data);
}

/* get next slot for a pin, allocates memory if necessary */
PinType *GetPinMemory(ElementType * element)
{
	PinType *new_obj;

	new_obj = calloc(sizeof(PinType), 1);
	pinlist_append(&element->Pin, new_obj);

	return new_obj;
}

void RemoveFreePin(PinType * data)
{
	pinlist_remove(data);
	free(data);
}



/*** utility ***/

/* creates a new via */
PinTypePtr CreateNewVia(DataTypePtr Data, Coord X, Coord Y, Coord Thickness, Coord Clearance, Coord Mask, Coord DrillingHole, const char *Name, FlagType Flags)
{
	PinTypePtr Via;

	if (!pcb_create_be_lenient) {
		VIA_LOOP(Data);
		{
			if (Distance(X, Y, via->X, via->Y) <= via->DrillingHole / 2 + DrillingHole / 2) {
				Message(PCB_MSG_DEFAULT, _("%m+Dropping via at %$mD because it's hole would overlap with the via "
									"at %$mD\n"), conf_core.editor.grid_unit->allow, X, Y, via->X, via->Y);
				return (NULL);					/* don't allow via stacking */
			}
		}
		END_LOOP;
	}

	Via = GetViaMemory(Data);

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
		Message(PCB_MSG_DEFAULT, _("%m+Mapped via drill hole to %$mS from %$mS per vendor table\n"),
						conf_core.editor.grid_unit->allow, Via->DrillingHole, DrillingHole);
	}

	Via->Name = pcb_strdup_null(Name);
	Via->Flags = Flags;
	CLEAR_FLAG(PCB_FLAG_WARN, Via);
	SET_FLAG(PCB_FLAG_VIA, Via);
	Via->ID = CreateIDGet();

	/*
	 * don't complain about MIN_PINORVIACOPPER on a mounting hole (pure
	 * hole)
	 */
	if (!TEST_FLAG(PCB_FLAG_HOLE, Via) && (Via->Thickness < Via->DrillingHole + MIN_PINORVIACOPPER)) {
		Via->Thickness = Via->DrillingHole + MIN_PINORVIACOPPER;
		Message(PCB_MSG_DEFAULT, _("%m+Increased via thickness to %$mS to allow enough copper"
							" at %$mD.\n"), conf_core.editor.grid_unit->allow, Via->Thickness, Via->X, Via->Y);
	}

	pcb_add_via(Data, Via);
	return (Via);
}

void pcb_add_via(DataType *Data, PinType *Via)
{
	SetPinBoundingBox(Via);
	if (!Data->via_tree)
		Data->via_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(Data->via_tree, (BoxTypePtr) Via, 0);
}

/* sets the bounding box of a pin or via */
void SetPinBoundingBox(PinTypePtr Pin)
{
	Coord width;

	if ((GET_SQUARE(Pin) > 1) && (TEST_FLAG(PCB_FLAG_SQUARE, Pin))) {
		POLYAREA *p = PinPoly(Pin, PIN_SIZE(Pin), Pin->Clearance);
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
	close_box(&Pin->BoundingBox);
}



/*** ops ***/
/* copies a via to paste buffer */
void *AddViaToBuffer(pcb_opctx_t *ctx, PinTypePtr Via)
{
	return (CreateNewVia(ctx->buffer.dst, Via->X, Via->Y, Via->Thickness, Via->Clearance, Via->Mask, Via->DrillingHole, Via->Name, MaskFlags(Via->Flags, PCB_FLAG_FOUND | ctx->buffer.extraflg)));
}

/* moves a via to paste buffer without allocating memory for the name */
void *MoveViaToBuffer(pcb_opctx_t *ctx, PinType * via)
{
	RestoreToPolygon(ctx->buffer.src, PCB_TYPE_VIA, via, via);

	r_delete_entry(ctx->buffer.src->via_tree, (BoxType *) via);
	pinlist_remove(via);
	pinlist_append(&ctx->buffer.dst->Via, via);

	CLEAR_FLAG(PCB_FLAG_WARN | PCB_FLAG_FOUND, via);

	if (!ctx->buffer.dst->via_tree)
		ctx->buffer.dst->via_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(ctx->buffer.dst->via_tree, (BoxType *) via, 0);
	ClearFromPolygon(ctx->buffer.dst, PCB_TYPE_VIA, via, via);
	return via;
}

/* changes the thermal on a via */
void *ChangeViaThermal(pcb_opctx_t *ctx, PinTypePtr Via)
{
	AddObjectToClearPolyUndoList(PCB_TYPE_VIA, Via, Via, Via, pcb_false);
	RestoreToPolygon(PCB->Data, PCB_TYPE_VIA, CURRENT, Via);
	AddObjectToFlagUndoList(PCB_TYPE_VIA, Via, Via, Via);
	if (!ctx->chgtherm.style)										/* remove the thermals */
		CLEAR_THERM(INDEXOFCURRENT, Via);
	else
		ASSIGN_THERM(INDEXOFCURRENT, ctx->chgtherm.style, Via);
	AddObjectToClearPolyUndoList(PCB_TYPE_VIA, Via, Via, Via, pcb_true);
	ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, CURRENT, Via);
	DrawVia(Via);
	return Via;
}

/* changes the thermal on a pin */
void *ChangePinThermal(pcb_opctx_t *ctx, ElementTypePtr element, PinTypePtr Pin)
{
	AddObjectToClearPolyUndoList(PCB_TYPE_PIN, element, Pin, Pin, pcb_false);
	RestoreToPolygon(PCB->Data, PCB_TYPE_VIA, CURRENT, Pin);
	AddObjectToFlagUndoList(PCB_TYPE_PIN, element, Pin, Pin);
	if (!ctx->chgtherm.style)										/* remove the thermals */
		CLEAR_THERM(INDEXOFCURRENT, Pin);
	else
		ASSIGN_THERM(INDEXOFCURRENT, ctx->chgtherm.style, Pin);
	AddObjectToClearPolyUndoList(PCB_TYPE_PIN, element, Pin, Pin, pcb_true);
	ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, CURRENT, Pin);
	DrawPin(Pin);
	return Pin;
}

/* changes the size of a via */
void *ChangeViaSize(pcb_opctx_t *ctx, PinTypePtr Via)
{
	Coord value = ctx->chgsize.absolute ? ctx->chgsize.absolute : Via->Thickness + ctx->chgsize.delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Via))
		return (NULL);
	if (!TEST_FLAG(PCB_FLAG_HOLE, Via) && value <= MAX_PINORVIASIZE &&
			value >= MIN_PINORVIASIZE && value >= Via->DrillingHole + MIN_PINORVIACOPPER && value != Via->Thickness) {
		AddObjectToSizeUndoList(PCB_TYPE_VIA, Via, Via, Via);
		EraseVia(Via);
		r_delete_entry(PCB->Data->via_tree, (BoxType *) Via);
		RestoreToPolygon(PCB->Data, PCB_TYPE_PIN, Via, Via);
		if (Via->Mask) {
			AddObjectToMaskSizeUndoList(PCB_TYPE_VIA, Via, Via, Via);
			Via->Mask += value - Via->Thickness;
		}
		Via->Thickness = value;
		SetPinBoundingBox(Via);
		r_insert_entry(PCB->Data->via_tree, (BoxType *) Via, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
		DrawVia(Via);
		return (Via);
	}
	return (NULL);
}

/* changes the drilling hole of a via */
void *ChangeVia2ndSize(pcb_opctx_t *ctx, PinTypePtr Via)
{
	Coord value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Via->DrillingHole + ctx->chgsize.delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Via))
		return (NULL);
	if (value <= MAX_PINORVIASIZE &&
			value >= MIN_PINORVIAHOLE && (TEST_FLAG(PCB_FLAG_HOLE, Via) || value <= Via->Thickness - MIN_PINORVIACOPPER)
			&& value != Via->DrillingHole) {
		AddObjectTo2ndSizeUndoList(PCB_TYPE_VIA, Via, Via, Via);
		EraseVia(Via);
		RestoreToPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
		Via->DrillingHole = value;
		if (TEST_FLAG(PCB_FLAG_HOLE, Via)) {
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
void *ChangePin2ndSize(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin)
{
	Coord value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pin->DrillingHole + ctx->chgsize.delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Pin))
		return (NULL);
	if (value <= MAX_PINORVIASIZE &&
			value >= MIN_PINORVIAHOLE && (TEST_FLAG(PCB_FLAG_HOLE, Pin) || value <= Pin->Thickness - MIN_PINORVIACOPPER)
			&& value != Pin->DrillingHole) {
		AddObjectTo2ndSizeUndoList(PCB_TYPE_PIN, Element, Pin, Pin);
		ErasePin(Pin);
		RestoreToPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
		Pin->DrillingHole = value;
		if (TEST_FLAG(PCB_FLAG_HOLE, Pin)) {
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
void *ChangeViaClearSize(pcb_opctx_t *ctx, PinTypePtr Via)
{
	Coord value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Via->Clearance + ctx->chgsize.delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Via))
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
	r_delete_entry(PCB->Data->via_tree, (BoxType *) Via);
	Via->Clearance = value;
	SetPinBoundingBox(Via);
	r_insert_entry(PCB->Data->via_tree, (BoxType *) Via, 0);
	ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
	DrawVia(Via);
	Via->Element = NULL;
	return (Via);
}


/* changes the size of a pin */
void *ChangePinSize(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin)
{
	Coord value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pin->Thickness + ctx->chgsize.delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Pin))
		return (NULL);
	if (!TEST_FLAG(PCB_FLAG_HOLE, Pin) && value <= MAX_PINORVIASIZE &&
			value >= MIN_PINORVIASIZE && value >= Pin->DrillingHole + MIN_PINORVIACOPPER && value != Pin->Thickness) {
		AddObjectToSizeUndoList(PCB_TYPE_PIN, Element, Pin, Pin);
		AddObjectToMaskSizeUndoList(PCB_TYPE_PIN, Element, Pin, Pin);
		ErasePin(Pin);
		r_delete_entry(PCB->Data->pin_tree, &Pin->BoundingBox);
		RestoreToPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
		Pin->Mask += value - Pin->Thickness;
		Pin->Thickness = value;
		/* SetElementBB updates all associated rtrees */
		SetElementBoundingBox(PCB->Data, Element, &PCB->Font);
		ClearFromPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
		DrawPin(Pin);
		return (Pin);
	}
	return (NULL);
}

/* changes the clearance size of a pin */
void *ChangePinClearSize(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin)
{
	Coord value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pin->Clearance + ctx->chgsize.delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Pin))
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
	SetElementBoundingBox(PCB->Data, Element, &PCB->Font);
	ClearFromPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	DrawPin(Pin);
	return (Pin);
}

/* changes the name of a via */
void *ChangeViaName(pcb_opctx_t *ctx, PinTypePtr Via)
{
	char *old = Via->Name;

	if (TEST_FLAG(PCB_FLAG_DISPLAYNAME, Via)) {
		ErasePinName(Via);
		Via->Name = ctx->chgname.new_name;
		DrawPinName(Via);
	}
	else
		Via->Name = ctx->chgname.new_name;
	return (old);
}

/* changes the name of a pin */
void *ChangePinName(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin)
{
	char *old = Pin->Name;

	Element = Element;						/* get rid of 'unused...' warnings */
	if (TEST_FLAG(PCB_FLAG_DISPLAYNAME, Pin)) {
		ErasePinName(Pin);
		Pin->Name = ctx->chgname.new_name;
		DrawPinName(Pin);
	}
	else
		Pin->Name = ctx->chgname.new_name;
	return (old);
}

/* changes the number of a pin */
void *ChangePinNum(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin)
{
	char *old = Pin->Number;

	Element = Element;						/* get rid of 'unused...' warnings */
	if (TEST_FLAG(PCB_FLAG_DISPLAYNAME, Pin)) {
		ErasePinName(Pin);
		Pin->Number = ctx->chgname.new_name;
		DrawPinName(Pin);
	}
	else
		Pin->Number = ctx->chgname.new_name;
	return (old);
}


/* changes the square flag of a via */
void *ChangeViaSquare(pcb_opctx_t *ctx, PinTypePtr Via)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Via))
		return (NULL);
	EraseVia(Via);
	AddObjectToClearPolyUndoList(PCB_TYPE_VIA, NULL, Via, Via, pcb_false);
	RestoreToPolygon(PCB->Data, PCB_TYPE_VIA, NULL, Via);
	AddObjectToFlagUndoList(PCB_TYPE_VIA, NULL, Via, Via);
	ASSIGN_SQUARE(ctx->chgsize.absolute, Via);
	if (ctx->chgsize.absolute == 0)
		CLEAR_FLAG(PCB_FLAG_SQUARE, Via);
	else
		SET_FLAG(PCB_FLAG_SQUARE, Via);
	SetPinBoundingBox(Via);
	AddObjectToClearPolyUndoList(PCB_TYPE_VIA, NULL, Via, Via, pcb_true);
	ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, NULL, Via);
	DrawVia(Via);
	return (Via);
}

/* changes the square flag of a pin */
void *ChangePinSquare(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Pin))
		return (NULL);
	ErasePin(Pin);
	AddObjectToClearPolyUndoList(PCB_TYPE_PIN, Element, Pin, Pin, pcb_false);
	RestoreToPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	AddObjectToFlagUndoList(PCB_TYPE_PIN, Element, Pin, Pin);
	ASSIGN_SQUARE(ctx->chgsize.absolute, Pin);
	if (ctx->chgsize.absolute == 0)
		CLEAR_FLAG(PCB_FLAG_SQUARE, Pin);
	else
		SET_FLAG(PCB_FLAG_SQUARE, Pin);
	SetPinBoundingBox(Pin);
	AddObjectToClearPolyUndoList(PCB_TYPE_PIN, Element, Pin, Pin, pcb_true);
	ClearFromPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	DrawPin(Pin);
	return (Pin);
}

/* sets the square flag of a pin */
void *SetPinSquare(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Pin) || TEST_FLAG(PCB_FLAG_SQUARE, Pin))
		return (NULL);

	return (ChangePinSquare(ctx, Element, Pin));
}

/* clears the square flag of a pin */
void *ClrPinSquare(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Pin) || !TEST_FLAG(PCB_FLAG_SQUARE, Pin))
		return (NULL);

	return (ChangePinSquare(ctx, Element, Pin));
}

/* changes the octagon flag of a via */
void *ChangeViaOctagon(pcb_opctx_t *ctx, PinTypePtr Via)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Via))
		return (NULL);
	EraseVia(Via);
	AddObjectToClearPolyUndoList(PCB_TYPE_VIA, Via, Via, Via, pcb_false);
	RestoreToPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
	AddObjectToFlagUndoList(PCB_TYPE_VIA, Via, Via, Via);
	TOGGLE_FLAG(PCB_FLAG_OCTAGON, Via);
	AddObjectToClearPolyUndoList(PCB_TYPE_VIA, Via, Via, Via, pcb_true);
	ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
	DrawVia(Via);
	return (Via);
}

/* sets the octagon flag of a via */
void *SetViaOctagon(pcb_opctx_t *ctx, PinTypePtr Via)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Via) || TEST_FLAG(PCB_FLAG_OCTAGON, Via))
		return (NULL);

	return (ChangeViaOctagon(ctx, Via));
}

/* clears the octagon flag of a via */
void *ClrViaOctagon(pcb_opctx_t *ctx, PinTypePtr Via)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Via) || !TEST_FLAG(PCB_FLAG_OCTAGON, Via))
		return (NULL);

	return (ChangeViaOctagon(ctx, Via));
}

/* changes the octagon flag of a pin */
void *ChangePinOctagon(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Pin))
		return (NULL);
	ErasePin(Pin);
	AddObjectToClearPolyUndoList(PCB_TYPE_PIN, Element, Pin, Pin, pcb_false);
	RestoreToPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	AddObjectToFlagUndoList(PCB_TYPE_PIN, Element, Pin, Pin);
	TOGGLE_FLAG(PCB_FLAG_OCTAGON, Pin);
	AddObjectToClearPolyUndoList(PCB_TYPE_PIN, Element, Pin, Pin, pcb_true);
	ClearFromPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	DrawPin(Pin);
	return (Pin);
}

/* sets the octagon flag of a pin */
void *SetPinOctagon(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Pin) || TEST_FLAG(PCB_FLAG_OCTAGON, Pin))
		return (NULL);

	return (ChangePinOctagon(ctx, Element, Pin));
}

/* clears the octagon flag of a pin */
void *ClrPinOctagon(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Pin) || !TEST_FLAG(PCB_FLAG_OCTAGON, Pin))
		return (NULL);

	return (ChangePinOctagon(ctx, Element, Pin));
}

/* changes the hole flag of a via */
pcb_bool ChangeHole(PinTypePtr Via)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Via))
		return (pcb_false);
	EraseVia(Via);
	AddObjectToFlagUndoList(PCB_TYPE_VIA, Via, Via, Via);
	AddObjectToMaskSizeUndoList(PCB_TYPE_VIA, Via, Via, Via);
	r_delete_entry(PCB->Data->via_tree, (BoxType *) Via);
	RestoreToPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
	TOGGLE_FLAG(PCB_FLAG_HOLE, Via);

	if (TEST_FLAG(PCB_FLAG_HOLE, Via)) {
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

	SetPinBoundingBox(Via);
	r_insert_entry(PCB->Data->via_tree, (BoxType *) Via, 0);
	ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
	DrawVia(Via);
	Draw();
	return (pcb_true);
}

/* changes the mask size of a pin */
void *ChangePinMaskSize(pcb_opctx_t *ctx, ElementTypePtr Element, PinTypePtr Pin)
{
	Coord value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pin->Mask + ctx->chgsize.delta;

	value = MAX(value, 0);
	if (value == Pin->Mask && ctx->chgsize.absolute == 0)
		value = Pin->Thickness;
	if (value != Pin->Mask) {
		AddObjectToMaskSizeUndoList(PCB_TYPE_PIN, Element, Pin, Pin);
		ErasePin(Pin);
		r_delete_entry(PCB->Data->pin_tree, &Pin->BoundingBox);
		Pin->Mask = value;
		SetElementBoundingBox(PCB->Data, Element, &PCB->Font);
		DrawPin(Pin);
		return (Pin);
	}
	return (NULL);
}

/* changes the mask size of a via */
void *ChangeViaMaskSize(pcb_opctx_t *ctx, PinTypePtr Via)
{
	Coord value;

	value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Via->Mask + ctx->chgsize.delta;
	value = MAX(value, 0);
	if (value != Via->Mask) {
		AddObjectToMaskSizeUndoList(PCB_TYPE_VIA, Via, Via, Via);
		EraseVia(Via);
		r_delete_entry(PCB->Data->via_tree, &Via->BoundingBox);
		Via->Mask = value;
		SetPinBoundingBox(Via);
		r_insert_entry(PCB->Data->via_tree, &Via->BoundingBox, 0);
		DrawVia(Via);
		return (Via);
	}
	return (NULL);
}

/* copies a via */
void *CopyVia(pcb_opctx_t *ctx, PinTypePtr Via)
{
	PinTypePtr via;

	via = CreateNewVia(PCB->Data, Via->X + ctx->copy.DeltaX, Via->Y + ctx->copy.DeltaY,
										 Via->Thickness, Via->Clearance, Via->Mask, Via->DrillingHole, Via->Name, MaskFlags(Via->Flags, PCB_FLAG_FOUND));
	if (!via)
		return (via);
	DrawVia(via);
	AddObjectToCreateUndoList(PCB_TYPE_VIA, via, via, via);
	return (via);
}

/* moves a via */
void *MoveVia(pcb_opctx_t *ctx, PinTypePtr Via)
{
	r_delete_entry(PCB->Data->via_tree, (BoxType *) Via);
	RestoreToPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
	MOVE_VIA_LOWLEVEL(Via, ctx->move.dx, ctx->move.dy);
	if (PCB->ViaOn)
		EraseVia(Via);
	r_insert_entry(PCB->Data->via_tree, (BoxType *) Via, 0);
	ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, Via, Via);
	if (PCB->ViaOn) {
		DrawVia(Via);
		Draw();
	}
	return (Via);
}

/* destroys a via */
void *DestroyVia(pcb_opctx_t *ctx, PinTypePtr Via)
{
	r_delete_entry(ctx->remove.destroy_target->via_tree, (BoxTypePtr) Via);
	free(Via->Name);

	RemoveFreeVia(Via);
	return NULL;
}

/* removes a via */
void *RemoveVia(pcb_opctx_t *ctx, PinTypePtr Via)
{
	/* erase from screen and memory */
	if (PCB->ViaOn) {
		EraseVia(Via);
		if (!ctx->remove.bulk)
			Draw();
	}
	MoveObjectToRemoveUndoList(PCB_TYPE_VIA, Via, Via, Via);
	return NULL;
}
