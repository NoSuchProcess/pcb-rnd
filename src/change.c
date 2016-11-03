/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996, 2005 Thomas Nau
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

/* functions used to change object properties
 *
 */

#include "config.h"

#include "conf_core.h"

#include "board.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "misc.h"
#include "mirror.h"
#include "polygon.h"
#include "select.h"
#include "undo.h"
#include "hid_actions.h"
#include "compat_nls.h"

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void *ChangePinSize(ElementTypePtr, PinTypePtr);
static void *ChangePinClearSize(ElementTypePtr, PinTypePtr);
static void *ChangePinMaskSize(ElementTypePtr, PinTypePtr);
static void *ChangePadSize(ElementTypePtr, PadTypePtr);
static void *ChangePadClearSize(ElementTypePtr, PadTypePtr);
static void *ChangePadMaskSize(ElementTypePtr, PadTypePtr);
static void *ChangePin2ndSize(ElementTypePtr, PinTypePtr);
static void *ChangeElement1stSize(ElementTypePtr);
static void *ChangeElement2ndSize(ElementTypePtr);
static void *ChangeViaSize(PinTypePtr);
static void *ChangeVia2ndSize(PinTypePtr);
static void *ChangeViaClearSize(PinTypePtr);
static void *ChangeViaMaskSize(PinTypePtr);
static void *ChangeLineSize(LayerTypePtr, LineTypePtr);
static void *ChangeLineClearSize(LayerTypePtr, LineTypePtr);
static void *ChangePolygonClearSize(LayerTypePtr, PolygonTypePtr);
static void *ChangeArcSize(LayerTypePtr, ArcTypePtr);
static void *ChangeArcClearSize(LayerTypePtr, ArcTypePtr);
static void *ChangeTextSize(LayerTypePtr, TextTypePtr);
static void *ChangeElementSize(ElementTypePtr);
static void *ChangeElementNameSize(ElementTypePtr);
static void *ChangeElementClearSize(ElementTypePtr);
static void *ChangePinName(ElementTypePtr, PinTypePtr);
static void *ChangePadName(ElementTypePtr, PadTypePtr);
static void *ChangePinNum(ElementTypePtr, PinTypePtr);
static void *ChangePadNum(ElementTypePtr, PadTypePtr);
static void *ChangeViaName(PinTypePtr);
static void *ChangeLineName(LayerTypePtr, LineTypePtr);
static void *ChangeElementName(ElementTypePtr);
static void *ChangeElementNonetlist(ElementTypePtr);
static void *ChangeTextName(LayerTypePtr, TextTypePtr);
static void *ChangeElementSquare(ElementTypePtr);
static void *SetElementSquare(ElementTypePtr);
static void *ClrElementSquare(ElementTypePtr);
static void *ChangeElementOctagon(ElementTypePtr);
static void *SetElementOctagon(ElementTypePtr);
static void *ClrElementOctagon(ElementTypePtr);
static void *ChangeViaSquare(PinTypePtr);
static void *ChangePinSquare(ElementTypePtr, PinTypePtr);
static void *SetPinSquare(ElementTypePtr, PinTypePtr);
static void *ClrPinSquare(ElementTypePtr, PinTypePtr);
static void *ChangePinOctagon(ElementTypePtr, PinTypePtr);
static void *SetPinOctagon(ElementTypePtr, PinTypePtr);
static void *ClrPinOctagon(ElementTypePtr, PinTypePtr);
static void *ChangeViaOctagon(PinTypePtr);
static void *SetViaOctagon(PinTypePtr);
static void *ClrViaOctagon(PinTypePtr);
static void *ChangePadSquare(ElementTypePtr, PadTypePtr);
static void *SetPadSquare(ElementTypePtr, PadTypePtr);
static void *ClrPadSquare(ElementTypePtr, PadTypePtr);
static void *ChangeViaThermal(PinTypePtr);
static void *ChangePinThermal(ElementTypePtr, PinTypePtr);
static void *ChangeLineJoin(LayerTypePtr, LineTypePtr);
static void *SetLineJoin(LayerTypePtr, LineTypePtr);
static void *ClrLineJoin(LayerTypePtr, LineTypePtr);
static void *ChangeArcJoin(LayerTypePtr, ArcTypePtr);
static void *SetArcJoin(LayerTypePtr, ArcTypePtr);
static void *ClrArcJoin(LayerTypePtr, ArcTypePtr);
static void *ChangeTextJoin(LayerTypePtr, TextTypePtr);
static void *SetTextJoin(LayerTypePtr, TextTypePtr);
static void *ClrTextJoin(LayerTypePtr, TextTypePtr);
static void *ChangePolyClear(LayerTypePtr, PolygonTypePtr);
static void *ChangeArcRadius(LayerTypePtr, ArcTypePtr);
static void *ChangeArcAngle(LayerTypePtr, ArcTypePtr);

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static Coord Delta;							/* change of size */
static Coord Absolute;					/* Absolute size */
static int is_primary;					/* whether the primary parameter should be changed */
static char *NewName;						/* new name */

static Angle ADelta, AAbsolute;				/* same as above, but for angles */

static ObjectFunctionType ChangeSizeFunctions = {
	ChangeLineSize,
	ChangeTextSize,
	ChangePolyClear,
	ChangeViaSize,
	ChangeElementSize,						/* changes silk screen line width */
	ChangeElementNameSize,
	ChangePinSize,
	ChangePadSize,
	NULL,
	NULL,
	ChangeArcSize,
	NULL
};

static ObjectFunctionType Change1stSizeFunctions = {
	ChangeLineSize,
	ChangeTextSize,
	ChangePolyClear,
	ChangeViaSize,
	ChangeElement1stSize,
	ChangeElementNameSize,
	ChangePinSize,
	ChangePadSize,
	NULL,
	NULL,
	ChangeArcSize,
	NULL
};

static ObjectFunctionType Change2ndSizeFunctions = {
	NULL,
	NULL,
	NULL,
	ChangeVia2ndSize,
	ChangeElement2ndSize,
	NULL,
	ChangePin2ndSize,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static ObjectFunctionType ChangeThermalFunctions = {
	NULL,
	NULL,
	NULL,
	ChangeViaThermal,
	NULL,
	NULL,
	ChangePinThermal,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static ObjectFunctionType ChangeClearSizeFunctions = {
	ChangeLineClearSize,
	NULL,
	ChangePolygonClearSize,				/* just to tell the user not to :-) */
	ChangeViaClearSize,
	ChangeElementClearSize,
	NULL,
	ChangePinClearSize,
	ChangePadClearSize,
	NULL,
	NULL,
	ChangeArcClearSize,
	NULL
};

static ObjectFunctionType ChangeNameFunctions = {
	ChangeLineName,
	ChangeTextName,
	NULL,
	ChangeViaName,
	ChangeElementName,
	NULL,
	ChangePinName,
	ChangePadName,
	NULL,
	NULL,
	NULL,
	NULL
};

static ObjectFunctionType ChangePinnumFunctions = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ChangePinNum,
	ChangePadNum,
	NULL,
	NULL,
	NULL,
	NULL
};

static ObjectFunctionType ChangeSquareFunctions = {
	NULL,
	NULL,
	NULL,
	ChangeViaSquare,
	ChangeElementSquare,
	NULL,
	ChangePinSquare,
	ChangePadSquare,
	NULL,
	NULL,
	NULL,
	NULL
};

static ObjectFunctionType ChangeNonetlistFunctions = {
	NULL,
	NULL,
	NULL,
	NULL,
	ChangeElementNonetlist,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static ObjectFunctionType ChangeJoinFunctions = {
	ChangeLineJoin,
	ChangeTextJoin,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ChangeArcJoin,
	NULL
};

static ObjectFunctionType ChangeOctagonFunctions = {
	NULL,
	NULL,
	NULL,
	ChangeViaOctagon,
	ChangeElementOctagon,
	NULL,
	ChangePinOctagon,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static ObjectFunctionType ChangeMaskSizeFunctions = {
	NULL,
	NULL,
	NULL,
	ChangeViaMaskSize,
#if 0
	ChangeElementMaskSize,
#else
	NULL,
#endif
	NULL,
	ChangePinMaskSize,
	ChangePadMaskSize,
	NULL,
	NULL,
	NULL,
	NULL
};

static ObjectFunctionType SetSquareFunctions = {
	NULL,
	NULL,
	NULL,
	NULL,
	SetElementSquare,
	NULL,
	SetPinSquare,
	SetPadSquare,
	NULL,
	NULL,
	NULL,
	NULL
};

static ObjectFunctionType SetJoinFunctions = {
	SetLineJoin,
	SetTextJoin,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	SetArcJoin,
	NULL
};

static ObjectFunctionType SetOctagonFunctions = {
	NULL,
	NULL,
	NULL,
	SetViaOctagon,
	SetElementOctagon,
	NULL,
	SetPinOctagon,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static ObjectFunctionType ClrSquareFunctions = {
	NULL,
	NULL,
	NULL,
	NULL,
	ClrElementSquare,
	NULL,
	ClrPinSquare,
	ClrPadSquare,
	NULL,
	NULL,
	NULL,
	NULL
};

static ObjectFunctionType ClrJoinFunctions = {
	ClrLineJoin,
	ClrTextJoin,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ClrArcJoin,
	NULL
};

static ObjectFunctionType ClrOctagonFunctions = {
	NULL,
	NULL,
	NULL,
	ClrViaOctagon,
	ClrElementOctagon,
	NULL,
	ClrPinOctagon,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

static ObjectFunctionType ChangeRadiusFunctions = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ChangeArcRadius,
	NULL
};

static ObjectFunctionType ChangeAngleFunctions = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	ChangeArcAngle,
	NULL
};


/* ---------------------------------------------------------------------------
 * changes the thermal on a via
 * returns pcb_true if changed
 */
static void *ChangeViaThermal(PinTypePtr Via)
{
	AddObjectToClearPolyUndoList(PCB_TYPE_VIA, Via, Via, Via, pcb_false);
	RestoreToPolygon(PCB->Data, PCB_TYPE_VIA, CURRENT, Via);
	AddObjectToFlagUndoList(PCB_TYPE_VIA, Via, Via, Via);
	if (!Delta)										/* remove the thermals */
		CLEAR_THERM(INDEXOFCURRENT, Via);
	else
		ASSIGN_THERM(INDEXOFCURRENT, Delta, Via);
	AddObjectToClearPolyUndoList(PCB_TYPE_VIA, Via, Via, Via, pcb_true);
	ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, CURRENT, Via);
	DrawVia(Via);
	return Via;
}

/* ---------------------------------------------------------------------------
 * changes the thermal on a pin
 * returns pcb_true if changed
 */
static void *ChangePinThermal(ElementTypePtr element, PinTypePtr Pin)
{
	AddObjectToClearPolyUndoList(PCB_TYPE_PIN, element, Pin, Pin, pcb_false);
	RestoreToPolygon(PCB->Data, PCB_TYPE_VIA, CURRENT, Pin);
	AddObjectToFlagUndoList(PCB_TYPE_PIN, element, Pin, Pin);
	if (!Delta)										/* remove the thermals */
		CLEAR_THERM(INDEXOFCURRENT, Pin);
	else
		ASSIGN_THERM(INDEXOFCURRENT, Delta, Pin);
	AddObjectToClearPolyUndoList(PCB_TYPE_PIN, element, Pin, Pin, pcb_true);
	ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, CURRENT, Pin);
	DrawPin(Pin);
	return Pin;
}

/* ---------------------------------------------------------------------------
 * changes the size of a via
 * returns pcb_true if changed
 */
static void *ChangeViaSize(PinTypePtr Via)
{
	Coord value = Absolute ? Absolute : Via->Thickness + Delta;

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

/* ---------------------------------------------------------------------------
 * changes the drilling hole of a via
 * returns pcb_true if changed
 */
static void *ChangeVia2ndSize(PinTypePtr Via)
{
	Coord value = (Absolute) ? Absolute : Via->DrillingHole + Delta;

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

/* ---------------------------------------------------------------------------
 * changes the clearance size of a via
 * returns pcb_true if changed
 */
static void *ChangeViaClearSize(PinTypePtr Via)
{
	Coord value = (Absolute) ? Absolute : Via->Clearance + Delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Via))
		return (NULL);
	value = MIN(MAX_LINESIZE, value);
	if (value < 0)
		value = 0;
	if (Delta < 0 && value < PCB->Bloat * 2)
		value = 0;
	if ((Delta > 0 || Absolute) && value < PCB->Bloat * 2)
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


/* ---------------------------------------------------------------------------
 * changes the size of a pin
 * returns pcb_true if changed
 */
static void *ChangePinSize(ElementTypePtr Element, PinTypePtr Pin)
{
	Coord value = (Absolute) ? Absolute : Pin->Thickness + Delta;

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

/* ---------------------------------------------------------------------------
 * changes the clearance size of a pin
 * returns pcb_true if changed
 */
static void *ChangePinClearSize(ElementTypePtr Element, PinTypePtr Pin)
{
	Coord value = (Absolute) ? Absolute : Pin->Clearance + Delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Pin))
		return (NULL);
	value = MIN(MAX_LINESIZE, value);
	if (value < 0)
		value = 0;
	if (Delta < 0 && value < PCB->Bloat * 2)
		value = 0;
	if ((Delta > 0 || Absolute) && value < PCB->Bloat * 2)
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

/* ---------------------------------------------------------------------------
 * changes the size of a pad
 * returns pcb_true if changed
 */
static void *ChangePadSize(ElementTypePtr Element, PadTypePtr Pad)
{
	Coord value = (Absolute) ? Absolute : Pad->Thickness + Delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Pad))
		return (NULL);
	if (value <= MAX_PADSIZE && value >= MIN_PADSIZE && value != Pad->Thickness) {
		AddObjectToSizeUndoList(PCB_TYPE_PAD, Element, Pad, Pad);
		AddObjectToMaskSizeUndoList(PCB_TYPE_PAD, Element, Pad, Pad);
		RestoreToPolygon(PCB->Data, PCB_TYPE_PAD, Element, Pad);
		ErasePad(Pad);
		r_delete_entry(PCB->Data->pad_tree, &Pad->BoundingBox);
		Pad->Mask += value - Pad->Thickness;
		Pad->Thickness = value;
		/* SetElementBB updates all associated rtrees */
		SetElementBoundingBox(PCB->Data, Element, &PCB->Font);
		ClearFromPolygon(PCB->Data, PCB_TYPE_PAD, Element, Pad);
		DrawPad(Pad);
		return (Pad);
	}
	return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the clearance size of a pad
 * returns pcb_true if changed
 */
static void *ChangePadClearSize(ElementTypePtr Element, PadTypePtr Pad)
{
	Coord value = (Absolute) ? Absolute : Pad->Clearance + Delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Pad))
		return (NULL);
	value = MIN(MAX_LINESIZE, value);
	if (value < 0)
		value = 0;
	if (Delta < 0 && value < PCB->Bloat * 2)
		value = 0;
	if ((Delta > 0 || Absolute) && value < PCB->Bloat * 2)
		value = PCB->Bloat * 2 + 2;
	if (value == Pad->Clearance)
		return NULL;
	AddObjectToClearSizeUndoList(PCB_TYPE_PAD, Element, Pad, Pad);
	RestoreToPolygon(PCB->Data, PCB_TYPE_PAD, Element, Pad);
	ErasePad(Pad);
	r_delete_entry(PCB->Data->pad_tree, &Pad->BoundingBox);
	Pad->Clearance = value;
	/* SetElementBB updates all associated rtrees */
	SetElementBoundingBox(PCB->Data, Element, &PCB->Font);
	ClearFromPolygon(PCB->Data, PCB_TYPE_PAD, Element, Pad);
	DrawPad(Pad);
	return Pad;
}

/* ---------------------------------------------------------------------------
 * changes the drilling hole of all pins of an element
 * returns pcb_true if changed
 */
static void *ChangeElement2ndSize(ElementTypePtr Element)
{
	pcb_bool changed = pcb_false;
	Coord value;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	PIN_LOOP(Element);
	{
		value = (Absolute) ? Absolute : pin->DrillingHole + Delta;
		if (value <= MAX_PINORVIASIZE &&
				value >= MIN_PINORVIAHOLE && (TEST_FLAG(PCB_FLAG_HOLE, pin) || value <= pin->Thickness - MIN_PINORVIACOPPER)
				&& value != pin->DrillingHole) {
			changed = pcb_true;
			AddObjectTo2ndSizeUndoList(PCB_TYPE_PIN, Element, pin, pin);
			ErasePin(pin);
			RestoreToPolygon(PCB->Data, PCB_TYPE_PIN, Element, pin);
			pin->DrillingHole = value;
			if (TEST_FLAG(PCB_FLAG_HOLE, pin)) {
				AddObjectToSizeUndoList(PCB_TYPE_PIN, Element, pin, pin);
				pin->Thickness = value;
			}
			ClearFromPolygon(PCB->Data, PCB_TYPE_PIN, Element, pin);
			DrawPin(pin);
		}
	}
	END_LOOP;
	if (changed)
		return (Element);
	else
		return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes ring dia of all pins of an element
 * returns pcb_true if changed
 */
static void *ChangeElement1stSize(ElementTypePtr Element)
{
	pcb_bool changed = pcb_false;
	Coord value;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	PIN_LOOP(Element);
	{
		value = (Absolute) ? Absolute : pin->DrillingHole + Delta;
		if (value <= MAX_PINORVIASIZE && value >= pin->DrillingHole + MIN_PINORVIACOPPER && value != pin->Thickness) {
			changed = pcb_true;
			AddObjectToSizeUndoList(PCB_TYPE_PIN, Element, pin, pin);
			ErasePin(pin);
			RestoreToPolygon(PCB->Data, PCB_TYPE_PIN, Element, pin);
			pin->Thickness = value;
			if (TEST_FLAG(PCB_FLAG_HOLE, pin)) {
				AddObjectToSizeUndoList(PCB_TYPE_PIN, Element, pin, pin);
				pin->Thickness = value;
			}
			ClearFromPolygon(PCB->Data, PCB_TYPE_PIN, Element, pin);
			DrawPin(pin);
		}
	}
	END_LOOP;
	if (changed)
		return (Element);
	else
		return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the clearance of all pins of an element
 * returns pcb_true if changed
 */
static void *ChangeElementClearSize(ElementTypePtr Element)
{
	pcb_bool changed = pcb_false;
	Coord value;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	PIN_LOOP(Element);
	{
		value = (Absolute) ? Absolute : pin->Clearance + Delta;
		if (value <= MAX_PINORVIASIZE &&
				value >= MIN_PINORVIAHOLE && (TEST_FLAG(PCB_FLAG_HOLE, pin) || value <= pin->Thickness - MIN_PINORVIACOPPER)
				&& value != pin->Clearance) {
			changed = pcb_true;
			AddObjectToClearSizeUndoList(PCB_TYPE_PIN, Element, pin, pin);
			ErasePin(pin);
			RestoreToPolygon(PCB->Data, PCB_TYPE_PIN, Element, pin);
			pin->Clearance = value;
			if (TEST_FLAG(PCB_FLAG_HOLE, pin)) {
				AddObjectToSizeUndoList(PCB_TYPE_PIN, Element, pin, pin);
				pin->Thickness = value;
			}
			ClearFromPolygon(PCB->Data, PCB_TYPE_PIN, Element, pin);
			DrawPin(pin);
		}
	}
	END_LOOP;

	PAD_LOOP(Element);
	{
		value = (Absolute) ? Absolute : pad->Clearance + Delta;
		if (value <= MAX_PINORVIASIZE && value >= MIN_PINORVIAHOLE && value != pad->Clearance) {
			changed = pcb_true;
			AddObjectToClearSizeUndoList(PCB_TYPE_PAD, Element, pad, pad);
			ErasePad(pad);
			RestoreToPolygon(PCB->Data, PCB_TYPE_PAD, Element, pad);
			r_delete_entry(PCB->Data->pad_tree, &pad->BoundingBox);
			pad->Clearance = value;
			if (TEST_FLAG(PCB_FLAG_HOLE, pad)) {
				AddObjectToSizeUndoList(PCB_TYPE_PAD, Element, pad, pad);
				pad->Thickness = value;
			}
			/* SetElementBB updates all associated rtrees */
			SetElementBoundingBox(PCB->Data, Element, &PCB->Font);

			ClearFromPolygon(PCB->Data, PCB_TYPE_PAD, Element, pad);
			DrawPad(pad);
		}
	}
	END_LOOP;

	if (changed)
		return (Element);
	else
		return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the drilling hole of a pin
 * returns pcb_true if changed
 */
static void *ChangePin2ndSize(ElementTypePtr Element, PinTypePtr Pin)
{
	Coord value = (Absolute) ? Absolute : Pin->DrillingHole + Delta;

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

/* ---------------------------------------------------------------------------
 * changes the size of a line
 * returns pcb_true if changed
 */
static void *ChangeLineSize(LayerTypePtr Layer, LineTypePtr Line)
{
	Coord value = (Absolute) ? Absolute : Line->Thickness + Delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Line))
		return (NULL);
	if (value <= MAX_LINESIZE && value >= MIN_LINESIZE && value != Line->Thickness) {
		AddObjectToSizeUndoList(PCB_TYPE_LINE, Layer, Line, Line);
		EraseLine(Line);
		r_delete_entry(Layer->line_tree, (BoxTypePtr) Line);
		RestoreToPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		Line->Thickness = value;
		SetLineBoundingBox(Line);
		r_insert_entry(Layer->line_tree, (BoxTypePtr) Line, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		DrawLine(Layer, Line);
		return (Line);
	}
	return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the clearance size of a line
 * returns pcb_true if changed
 */
static void *ChangeLineClearSize(LayerTypePtr Layer, LineTypePtr Line)
{
	Coord value = (Absolute) ? Absolute : Line->Clearance + Delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Line) || !TEST_FLAG(PCB_FLAG_CLEARLINE, Line))
		return (NULL);
	value = MIN(MAX_LINESIZE, MAX(value, PCB->Bloat * 2 + 2));
	if (value != Line->Clearance) {
		AddObjectToClearSizeUndoList(PCB_TYPE_LINE, Layer, Line, Line);
		RestoreToPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		EraseLine(Line);
		r_delete_entry(Layer->line_tree, (BoxTypePtr) Line);
		Line->Clearance = value;
		if (Line->Clearance == 0) {
			CLEAR_FLAG(PCB_FLAG_CLEARLINE, Line);
			Line->Clearance = PCB_MIL_TO_COORD(10);
		}
		SetLineBoundingBox(Line);
		r_insert_entry(Layer->line_tree, (BoxTypePtr) Line, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
		DrawLine(Layer, Line);
		return (Line);
	}
	return (NULL);
}

/* ---------------------------------------------------------------------------
 * Handle attempts to change the clearance of a polygon.
 */
static void *ChangePolygonClearSize(LayerTypePtr Layer, PolygonTypePtr poly)
{
	static int shown_this_message = 0;
	if (!shown_this_message) {
		gui->confirm_dialog(_("To change the clearance of objects in a polygon, "
													"change the objects, not the polygon.\n"
													"Hint: To set a minimum clearance for a group of objects, "
													"select them all then :MinClearGap(Selected,=10,mil)"), "Ok", NULL);
		shown_this_message = 1;
	}

	return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the size of an arc
 * returns pcb_true if changed
 */
static void *ChangeArcSize(LayerTypePtr Layer, ArcTypePtr Arc)
{
	Coord value = (Absolute) ? Absolute : Arc->Thickness + Delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Arc))
		return (NULL);
	if (value <= MAX_LINESIZE && value >= MIN_LINESIZE && value != Arc->Thickness) {
		AddObjectToSizeUndoList(PCB_TYPE_ARC, Layer, Arc, Arc);
		EraseArc(Arc);
		r_delete_entry(Layer->arc_tree, (BoxTypePtr) Arc);
		RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		Arc->Thickness = value;
		SetArcBoundingBox(Arc);
		r_insert_entry(Layer->arc_tree, (BoxTypePtr) Arc, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		DrawArc(Layer, Arc);
		return (Arc);
	}
	return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the clearance size of an arc
 * returns pcb_true if changed
 */
static void *ChangeArcClearSize(LayerTypePtr Layer, ArcTypePtr Arc)
{
	Coord value = (Absolute) ? Absolute : Arc->Clearance + Delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Arc) || !TEST_FLAG(PCB_FLAG_CLEARLINE, Arc))
		return (NULL);
	value = MIN(MAX_LINESIZE, MAX(value, PCB->Bloat * 2 + 2));
	if (value != Arc->Clearance) {
		AddObjectToClearSizeUndoList(PCB_TYPE_ARC, Layer, Arc, Arc);
		EraseArc(Arc);
		r_delete_entry(Layer->arc_tree, (BoxTypePtr) Arc);
		RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		Arc->Clearance = value;
		if (Arc->Clearance == 0) {
			CLEAR_FLAG(PCB_FLAG_CLEARLINE, Arc);
			Arc->Clearance = PCB_MIL_TO_COORD(10);
		}
		SetArcBoundingBox(Arc);
		r_insert_entry(Layer->arc_tree, (BoxTypePtr) Arc, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		DrawArc(Layer, Arc);
		return (Arc);
	}
	return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the radius of an arc (is_primary 0=width or 1=height or 2=both)
 * returns pcb_true if changed
 */
static void *ChangeArcRadius(LayerTypePtr Layer, ArcTypePtr Arc)
{
	Coord value, *dst;
	void *a0, *a1;

	if (TEST_FLAG(PCB_FLAG_LOCK, Arc))
		return (NULL);

	switch(is_primary) {
		case 0: dst = &Arc->Width; break;
		case 1: dst = &Arc->Height; break;
		case 2:
			is_primary = 0; a0 = ChangeArcRadius(Layer, Arc);
			is_primary = 1; a1 = ChangeArcRadius(Layer, Arc);
			if ((a0 != NULL) || (a1 != NULL))
				return Arc;
			return NULL;
	}

	value = (Absolute) ? Absolute : (*dst) + Delta;
	value = MIN(MAX_ARCSIZE, MAX(value, MIN_ARCSIZE));
	if (value != *dst) {
		AddObjectToChangeRadiiUndoList(PCB_TYPE_ARC, Layer, Arc, Arc);
		EraseArc(Arc);
		r_delete_entry(Layer->arc_tree, (BoxTypePtr) Arc);
		RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		*dst = value;
		SetArcBoundingBox(Arc);
		r_insert_entry(Layer->arc_tree, (BoxTypePtr) Arc, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		DrawArc(Layer, Arc);
		return (Arc);
	}
	return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the angle of an arc (is_primary 0=start or 1=end)
 * returns pcb_true if changed
 */
static void *ChangeArcAngle(LayerTypePtr Layer, ArcTypePtr Arc)
{
	Angle value, *dst;
	void *a0, *a1;

	if (TEST_FLAG(PCB_FLAG_LOCK, Arc))
		return (NULL);

	switch(is_primary) {
		case 0: dst = &Arc->StartAngle; break;
		case 1: dst = &Arc->Delta; break;
		case 2:
			is_primary = 0; a0 = ChangeArcAngle(Layer, Arc);
			is_primary = 1; a1 = ChangeArcAngle(Layer, Arc);
			if ((a0 != NULL) || (a1 != NULL))
				return Arc;
			return NULL;
	}

	value = (AAbsolute) ? AAbsolute : (*dst) + ADelta;
	value = fmod(value, 360.0);
	if (value < 0)
		value += 360;

	if (value != *dst) {
		AddObjectToChangeAnglesUndoList(PCB_TYPE_ARC, Layer, Arc, Arc);
		EraseArc(Arc);
		r_delete_entry(Layer->arc_tree, (BoxTypePtr) Arc);
		RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		*dst = value;
		SetArcBoundingBox(Arc);
		r_insert_entry(Layer->arc_tree, (BoxTypePtr) Arc, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		DrawArc(Layer, Arc);
		return (Arc);
	}
	return (NULL);
}


/* ---------------------------------------------------------------------------
 * changes the scaling factor of a text object
 * returns pcb_true if changed
 */
static void *ChangeTextSize(LayerTypePtr Layer, TextTypePtr Text)
{
	int value = Absolute ? PCB_COORD_TO_MIL(Absolute)
		: Text->Scale + PCB_COORD_TO_MIL(Delta);

	if (TEST_FLAG(PCB_FLAG_LOCK, Text))
		return (NULL);
	if (value <= MAX_TEXTSCALE && value >= MIN_TEXTSCALE && value != Text->Scale) {
		AddObjectToSizeUndoList(PCB_TYPE_TEXT, Layer, Text, Text);
		EraseText(Layer, Text);
		r_delete_entry(Layer->text_tree, (BoxTypePtr) Text);
		RestoreToPolygon(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
		Text->Scale = value;
		SetTextBoundingBox(&PCB->Font, Text);
		r_insert_entry(Layer->text_tree, (BoxTypePtr) Text, 0);
		ClearFromPolygon(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
		DrawText(Layer, Text);
		return (Text);
	}
	return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the scaling factor of an element's outline
 * returns pcb_true if changed
 */
static void *ChangeElementSize(ElementTypePtr Element)
{
	Coord value;
	pcb_bool changed = pcb_false;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	if (PCB->ElementOn)
		EraseElement(Element);
	ELEMENTLINE_LOOP(Element);
	{
		value = (Absolute) ? Absolute : line->Thickness + Delta;
		if (value <= MAX_LINESIZE && value >= MIN_LINESIZE && value != line->Thickness) {
			AddObjectToSizeUndoList(PCB_TYPE_ELEMENT_LINE, Element, line, line);
			line->Thickness = value;
			changed = pcb_true;
		}
	}
	END_LOOP;
	ARC_LOOP(Element);
	{
		value = (Absolute) ? Absolute : arc->Thickness + Delta;
		if (value <= MAX_LINESIZE && value >= MIN_LINESIZE && value != arc->Thickness) {
			AddObjectToSizeUndoList(PCB_TYPE_ELEMENT_ARC, Element, arc, arc);
			arc->Thickness = value;
			changed = pcb_true;
		}
	}
	END_LOOP;
	if (PCB->ElementOn) {
		DrawElement(Element);
	}
	if (changed)
		return (Element);
	return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the scaling factor of a elementname object
 * returns pcb_true if changed
 */
static void *ChangeElementNameSize(ElementTypePtr Element)
{
	int value = Absolute ? PCB_COORD_TO_MIL(Absolute)
		: DESCRIPTION_TEXT(Element).Scale + PCB_COORD_TO_MIL(Delta);

	if (TEST_FLAG(PCB_FLAG_LOCK, &Element->Name[0]))
		return (NULL);
	if (value <= MAX_TEXTSCALE && value >= MIN_TEXTSCALE) {
		EraseElementName(Element);
		ELEMENTTEXT_LOOP(Element);
		{
			AddObjectToSizeUndoList(PCB_TYPE_ELEMENT_NAME, Element, text, text);
			r_delete_entry(PCB->Data->name_tree[n], (BoxType *) text);
			text->Scale = value;
			SetTextBoundingBox(&PCB->Font, text);
			r_insert_entry(PCB->Data->name_tree[n], (BoxType *) text, 0);
		}
		END_LOOP;
		DrawElementName(Element);
		return (Element);
	}
	return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the name of a via
 */
static void *ChangeViaName(PinTypePtr Via)
{
	char *old = Via->Name;

	if (TEST_FLAG(PCB_FLAG_DISPLAYNAME, Via)) {
		ErasePinName(Via);
		Via->Name = NewName;
		DrawPinName(Via);
	}
	else
		Via->Name = NewName;
	return (old);
}

/* ---------------------------------------------------------------------------
 * changes the name of a pin
 */
static void *ChangePinName(ElementTypePtr Element, PinTypePtr Pin)
{
	char *old = Pin->Name;

	Element = Element;						/* get rid of 'unused...' warnings */
	if (TEST_FLAG(PCB_FLAG_DISPLAYNAME, Pin)) {
		ErasePinName(Pin);
		Pin->Name = NewName;
		DrawPinName(Pin);
	}
	else
		Pin->Name = NewName;
	return (old);
}

/* ---------------------------------------------------------------------------
 * changes the name of a pad
 */
static void *ChangePadName(ElementTypePtr Element, PadTypePtr Pad)
{
	char *old = Pad->Name;

	Element = Element;						/* get rid of 'unused...' warnings */
	if (TEST_FLAG(PCB_FLAG_DISPLAYNAME, Pad)) {
		ErasePadName(Pad);
		Pad->Name = NewName;
		DrawPadName(Pad);
	}
	else
		Pad->Name = NewName;
	return (old);
}

/* ---------------------------------------------------------------------------
 * changes the number of a pin
 */
static void *ChangePinNum(ElementTypePtr Element, PinTypePtr Pin)
{
	char *old = Pin->Number;

	Element = Element;						/* get rid of 'unused...' warnings */
	if (TEST_FLAG(PCB_FLAG_DISPLAYNAME, Pin)) {
		ErasePinName(Pin);
		Pin->Number = NewName;
		DrawPinName(Pin);
	}
	else
		Pin->Number = NewName;
	return (old);
}

/* ---------------------------------------------------------------------------
 * changes the number of a pad
 */
static void *ChangePadNum(ElementTypePtr Element, PadTypePtr Pad)
{
	char *old = Pad->Number;

	Element = Element;						/* get rid of 'unused...' warnings */
	if (TEST_FLAG(PCB_FLAG_DISPLAYNAME, Pad)) {
		ErasePadName(Pad);
		Pad->Number = NewName;
		DrawPadName(Pad);
	}
	else
		Pad->Number = NewName;
	return (old);
}

/* ---------------------------------------------------------------------------
 * changes the name of a line
 */
static void *ChangeLineName(LayerTypePtr Layer, LineTypePtr Line)
{
	char *old = Line->Number;

	Layer = Layer;
	Line->Number = NewName;
	return (old);
}

/* ---------------------------------------------------------------------------
 * changes the layout-name of an element
 */

char *ChangeElementText(PCBType * pcb, DataType * data, ElementTypePtr Element, int which, char *new_name)
{
	char *old = Element->Name[which].TextString;

#ifdef DEBUG
	printf("In ChangeElementText, updating old TextString %s to %s\n", old, new_name);
#endif

	if (pcb && which == NAME_INDEX())
		EraseElementName(Element);

	r_delete_entry(data->name_tree[which], &Element->Name[which].BoundingBox);

	Element->Name[which].TextString = new_name;
	SetTextBoundingBox(&PCB->Font, &Element->Name[which]);

	r_insert_entry(data->name_tree[which], &Element->Name[which].BoundingBox, 0);

	if (pcb && which == NAME_INDEX())
		DrawElementName(Element);

	return old;
}

static void *ChangeElementName(ElementTypePtr Element)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, &Element->Name[0]))
		return (NULL);
	if (NAME_INDEX() == NAMEONPCB_INDEX) {
		if (conf_core.editor.unique_names && UniqueElementName(PCB->Data, NewName) != NewName) {
			Message(PCB_MSG_DEFAULT, _("Error: The name \"%s\" is not unique!\n"), NewName);
			return ((char *) -1);
		}
	}

	return ChangeElementText(PCB, PCB->Data, Element, NAME_INDEX(), NewName);
}

static void *ChangeElementNonetlist(ElementTypePtr Element)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	TOGGLE_FLAG(PCB_FLAG_NONETLIST, Element);
	return Element;
}

/* ---------------------------------------------------------------------------
 * sets data of a text object and calculates bounding box
 * memory must have already been allocated
 * the one for the new string is allocated
 * returns pcb_true if the string has been changed
 */
static void *ChangeTextName(LayerTypePtr Layer, TextTypePtr Text)
{
	char *old = Text->TextString;

	if (TEST_FLAG(PCB_FLAG_LOCK, Text))
		return (NULL);
	EraseText(Layer, Text);
	RestoreToPolygon(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	Text->TextString = NewName;

	/* calculate size of the bounding box */
	SetTextBoundingBox(&PCB->Font, Text);
	r_insert_entry(Layer->text_tree, (BoxTypePtr) Text, 0);
	ClearFromPolygon(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	DrawText(Layer, Text);
	return (old);
}

/* ---------------------------------------------------------------------------
 * changes the name of a layout; memory has to be already allocated
 */
pcb_bool ChangeLayoutName(char *Name)
{
	free(PCB->Name);
	PCB->Name = Name;
	if (gui != NULL)
		hid_action("PCBChanged");
	return (pcb_true);
}

/* ---------------------------------------------------------------------------
 * changes the side of the board an element is on
 * returns pcb_true if done
 */
pcb_bool ChangeElementSide(ElementTypePtr Element, Coord yoff)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (pcb_false);
	EraseElement(Element);
	AddObjectToMirrorUndoList(PCB_TYPE_ELEMENT, Element, Element, Element, yoff);
	MirrorElementCoordinates(PCB->Data, Element, yoff);
	DrawElement(Element);
	return (pcb_true);
}

/* ---------------------------------------------------------------------------
 * changes the name of a layer; memory has to be already allocated
 */
pcb_bool ChangeLayerName(LayerTypePtr Layer, char *Name)
{
	free((char*)CURRENT->Name);
	CURRENT->Name = Name;
	hid_action("LayersChanged");
	return (pcb_true);
}

/* ---------------------------------------------------------------------------
 * changes the clearance flag of a line
 */
static void *ChangeLineJoin(LayerTypePtr Layer, LineTypePtr Line)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Line))
		return (NULL);
	EraseLine(Line);
	if (TEST_FLAG(PCB_FLAG_CLEARLINE, Line)) {
		AddObjectToClearPolyUndoList(PCB_TYPE_LINE, Layer, Line, Line, pcb_false);
		RestoreToPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
	}
	AddObjectToFlagUndoList(PCB_TYPE_LINE, Layer, Line, Line);
	TOGGLE_FLAG(PCB_FLAG_CLEARLINE, Line);
	if (TEST_FLAG(PCB_FLAG_CLEARLINE, Line)) {
		AddObjectToClearPolyUndoList(PCB_TYPE_LINE, Layer, Line, Line, pcb_true);
		ClearFromPolygon(PCB->Data, PCB_TYPE_LINE, Layer, Line);
	}
	DrawLine(Layer, Line);
	return (Line);
}

/* ---------------------------------------------------------------------------
 * sets the clearance flag of a line
 */
static void *SetLineJoin(LayerTypePtr Layer, LineTypePtr Line)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Line) || TEST_FLAG(PCB_FLAG_CLEARLINE, Line))
		return (NULL);
	return ChangeLineJoin(Layer, Line);
}

/* ---------------------------------------------------------------------------
 * clears the clearance flag of a line
 */
static void *ClrLineJoin(LayerTypePtr Layer, LineTypePtr Line)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Line) || !TEST_FLAG(PCB_FLAG_CLEARLINE, Line))
		return (NULL);
	return ChangeLineJoin(Layer, Line);
}

/* ---------------------------------------------------------------------------
 * changes the clearance flag of an arc
 */
static void *ChangeArcJoin(LayerTypePtr Layer, ArcTypePtr Arc)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Arc))
		return (NULL);
	EraseArc(Arc);
	if (TEST_FLAG(PCB_FLAG_CLEARLINE, Arc)) {
		RestoreToPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		AddObjectToClearPolyUndoList(PCB_TYPE_ARC, Layer, Arc, Arc, pcb_false);
	}
	AddObjectToFlagUndoList(PCB_TYPE_ARC, Layer, Arc, Arc);
	TOGGLE_FLAG(PCB_FLAG_CLEARLINE, Arc);
	if (TEST_FLAG(PCB_FLAG_CLEARLINE, Arc)) {
		ClearFromPolygon(PCB->Data, PCB_TYPE_ARC, Layer, Arc);
		AddObjectToClearPolyUndoList(PCB_TYPE_ARC, Layer, Arc, Arc, pcb_true);
	}
	DrawArc(Layer, Arc);
	return (Arc);
}

/* ---------------------------------------------------------------------------
 * sets the clearance flag of an arc
 */
static void *SetArcJoin(LayerTypePtr Layer, ArcTypePtr Arc)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Arc) || TEST_FLAG(PCB_FLAG_CLEARLINE, Arc))
		return (NULL);
	return ChangeArcJoin(Layer, Arc);
}

/* ---------------------------------------------------------------------------
 * clears the clearance flag of an arc
 */
static void *ClrArcJoin(LayerTypePtr Layer, ArcTypePtr Arc)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Arc) || !TEST_FLAG(PCB_FLAG_CLEARLINE, Arc))
		return (NULL);
	return ChangeArcJoin(Layer, Arc);
}

/* ---------------------------------------------------------------------------
 * changes the clearance flag of a text
 */
static void *ChangeTextJoin(LayerTypePtr Layer, TextTypePtr Text)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Text))
		return (NULL);
	EraseText(Layer, Text);
	if (TEST_FLAG(PCB_FLAG_CLEARLINE, Text)) {
		AddObjectToClearPolyUndoList(PCB_TYPE_TEXT, Layer, Text, Text, pcb_false);
		RestoreToPolygon(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	}
	AddObjectToFlagUndoList(PCB_TYPE_LINE, Layer, Text, Text);
	TOGGLE_FLAG(PCB_FLAG_CLEARLINE, Text);
	if (TEST_FLAG(PCB_FLAG_CLEARLINE, Text)) {
		AddObjectToClearPolyUndoList(PCB_TYPE_TEXT, Layer, Text, Text, pcb_true);
		ClearFromPolygon(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	}
	DrawText(Layer, Text);
	return (Text);
}

/* ---------------------------------------------------------------------------
 * sets the clearance flag of a text
 */
static void *SetTextJoin(LayerTypePtr Layer, TextTypePtr Text)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Text) || TEST_FLAG(PCB_FLAG_CLEARLINE, Text))
		return (NULL);
	return ChangeTextJoin(Layer, Text);
}

/* ---------------------------------------------------------------------------
 * clears the clearance flag of a text
 */
static void *ClrTextJoin(LayerTypePtr Layer, TextTypePtr Text)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Text) || !TEST_FLAG(PCB_FLAG_CLEARLINE, Text))
		return (NULL);
	return ChangeTextJoin(Layer, Text);
}

/* ---------------------------------------------------------------------------
 * changes the square flag of all pins on an element
 */
static void *ChangeElementSquare(ElementTypePtr Element)
{
	void *ans = NULL;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	PIN_LOOP(Element);
	{
		ans = ChangePinSquare(Element, pin);
	}
	END_LOOP;
	PAD_LOOP(Element);
	{
		ans = ChangePadSquare(Element, pad);
	}
	END_LOOP;
	return (ans);
}

/* ---------------------------------------------------------------------------
 * sets the square flag of all pins on an element
 */
static void *SetElementSquare(ElementTypePtr Element)
{
	void *ans = NULL;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	PIN_LOOP(Element);
	{
		ans = SetPinSquare(Element, pin);
	}
	END_LOOP;
	PAD_LOOP(Element);
	{
		ans = SetPadSquare(Element, pad);
	}
	END_LOOP;
	return (ans);
}

/* ---------------------------------------------------------------------------
 * clears the square flag of all pins on an element
 */
static void *ClrElementSquare(ElementTypePtr Element)
{
	void *ans = NULL;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	PIN_LOOP(Element);
	{
		ans = ClrPinSquare(Element, pin);
	}
	END_LOOP;
	PAD_LOOP(Element);
	{
		ans = ClrPadSquare(Element, pad);
	}
	END_LOOP;
	return (ans);
}

/* ---------------------------------------------------------------------------
 * changes the octagon flags of all pins of an element
 */
static void *ChangeElementOctagon(ElementTypePtr Element)
{
	void *result = NULL;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	PIN_LOOP(Element);
	{
		ChangePinOctagon(Element, pin);
		result = Element;
	}
	END_LOOP;
	return (result);
}

/* ---------------------------------------------------------------------------
 * sets the octagon flags of all pins of an element
 */
static void *SetElementOctagon(ElementTypePtr Element)
{
	void *result = NULL;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	PIN_LOOP(Element);
	{
		SetPinOctagon(Element, pin);
		result = Element;
	}
	END_LOOP;
	return (result);
}

/* ---------------------------------------------------------------------------
 * clears the octagon flags of all pins of an element
 */
static void *ClrElementOctagon(ElementTypePtr Element)
{
	void *result = NULL;

	if (TEST_FLAG(PCB_FLAG_LOCK, Element))
		return (NULL);
	PIN_LOOP(Element);
	{
		ClrPinOctagon(Element, pin);
		result = Element;
	}
	END_LOOP;
	return (result);
}

/* ---------------------------------------------------------------------------
 * changes the square flag of a pad
 */
static void *ChangePadSquare(ElementTypePtr Element, PadTypePtr Pad)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Pad))
		return (NULL);
	ErasePad(Pad);
	AddObjectToClearPolyUndoList(PCB_TYPE_PAD, Element, Pad, Pad, pcb_false);
	RestoreToPolygon(PCB->Data, PCB_TYPE_PAD, Element, Pad);
	AddObjectToFlagUndoList(PCB_TYPE_PAD, Element, Pad, Pad);
	TOGGLE_FLAG(PCB_FLAG_SQUARE, Pad);
	AddObjectToClearPolyUndoList(PCB_TYPE_PAD, Element, Pad, Pad, pcb_true);
	ClearFromPolygon(PCB->Data, PCB_TYPE_PAD, Element, Pad);
	DrawPad(Pad);
	return (Pad);
}

/* ---------------------------------------------------------------------------
 * sets the square flag of a pad
 */
static void *SetPadSquare(ElementTypePtr Element, PadTypePtr Pad)
{

	if (TEST_FLAG(PCB_FLAG_LOCK, Pad) || TEST_FLAG(PCB_FLAG_SQUARE, Pad))
		return (NULL);

	return (ChangePadSquare(Element, Pad));
}


/* ---------------------------------------------------------------------------
 * clears the square flag of a pad
 */
static void *ClrPadSquare(ElementTypePtr Element, PadTypePtr Pad)
{

	if (TEST_FLAG(PCB_FLAG_LOCK, Pad) || !TEST_FLAG(PCB_FLAG_SQUARE, Pad))
		return (NULL);

	return (ChangePadSquare(Element, Pad));
}


/* ---------------------------------------------------------------------------
 * changes the square flag of a via
 */
static void *ChangeViaSquare(PinTypePtr Via)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Via))
		return (NULL);
	EraseVia(Via);
	AddObjectToClearPolyUndoList(PCB_TYPE_VIA, NULL, Via, Via, pcb_false);
	RestoreToPolygon(PCB->Data, PCB_TYPE_VIA, NULL, Via);
	AddObjectToFlagUndoList(PCB_TYPE_VIA, NULL, Via, Via);
	ASSIGN_SQUARE(Absolute, Via);
	if (Absolute == 0)
		CLEAR_FLAG(PCB_FLAG_SQUARE, Via);
	else
		SET_FLAG(PCB_FLAG_SQUARE, Via);
	SetPinBoundingBox(Via);
	AddObjectToClearPolyUndoList(PCB_TYPE_VIA, NULL, Via, Via, pcb_true);
	ClearFromPolygon(PCB->Data, PCB_TYPE_VIA, NULL, Via);
	DrawVia(Via);
	return (Via);
}

/* ---------------------------------------------------------------------------
 * changes the square flag of a pin
 */
static void *ChangePinSquare(ElementTypePtr Element, PinTypePtr Pin)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Pin))
		return (NULL);
	ErasePin(Pin);
	AddObjectToClearPolyUndoList(PCB_TYPE_PIN, Element, Pin, Pin, pcb_false);
	RestoreToPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	AddObjectToFlagUndoList(PCB_TYPE_PIN, Element, Pin, Pin);
	ASSIGN_SQUARE(Absolute, Pin);
	if (Absolute == 0)
		CLEAR_FLAG(PCB_FLAG_SQUARE, Pin);
	else
		SET_FLAG(PCB_FLAG_SQUARE, Pin);
	SetPinBoundingBox(Pin);
	AddObjectToClearPolyUndoList(PCB_TYPE_PIN, Element, Pin, Pin, pcb_true);
	ClearFromPolygon(PCB->Data, PCB_TYPE_PIN, Element, Pin);
	DrawPin(Pin);
	return (Pin);
}

/* ---------------------------------------------------------------------------
 * sets the square flag of a pin
 */
static void *SetPinSquare(ElementTypePtr Element, PinTypePtr Pin)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Pin) || TEST_FLAG(PCB_FLAG_SQUARE, Pin))
		return (NULL);

	return (ChangePinSquare(Element, Pin));
}

/* ---------------------------------------------------------------------------
 * clears the square flag of a pin
 */
static void *ClrPinSquare(ElementTypePtr Element, PinTypePtr Pin)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Pin) || !TEST_FLAG(PCB_FLAG_SQUARE, Pin))
		return (NULL);

	return (ChangePinSquare(Element, Pin));
}

/* ---------------------------------------------------------------------------
 * changes the octagon flag of a via
 */
static void *ChangeViaOctagon(PinTypePtr Via)
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

/* ---------------------------------------------------------------------------
 * sets the octagon flag of a via
 */
static void *SetViaOctagon(PinTypePtr Via)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Via) || TEST_FLAG(PCB_FLAG_OCTAGON, Via))
		return (NULL);

	return (ChangeViaOctagon(Via));
}

/* ---------------------------------------------------------------------------
 * clears the octagon flag of a via
 */
static void *ClrViaOctagon(PinTypePtr Via)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Via) || !TEST_FLAG(PCB_FLAG_OCTAGON, Via))
		return (NULL);

	return (ChangeViaOctagon(Via));
}

/* ---------------------------------------------------------------------------
 * changes the octagon flag of a pin
 */
static void *ChangePinOctagon(ElementTypePtr Element, PinTypePtr Pin)
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

/* ---------------------------------------------------------------------------
 * sets the octagon flag of a pin
 */
static void *SetPinOctagon(ElementTypePtr Element, PinTypePtr Pin)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Pin) || TEST_FLAG(PCB_FLAG_OCTAGON, Pin))
		return (NULL);

	return (ChangePinOctagon(Element, Pin));
}

/* ---------------------------------------------------------------------------
 * clears the octagon flag of a pin
 */
static void *ClrPinOctagon(ElementTypePtr Element, PinTypePtr Pin)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Pin) || !TEST_FLAG(PCB_FLAG_OCTAGON, Pin))
		return (NULL);

	return (ChangePinOctagon(Element, Pin));
}

/* ---------------------------------------------------------------------------
 * changes the hole flag of a via
 */
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

/* ---------------------------------------------------------------------------
 * changes the nopaste flag of a pad
 */
pcb_bool ChangePaste(PadTypePtr Pad)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Pad))
		return (pcb_false);
	ErasePad(Pad);
	AddObjectToFlagUndoList(PCB_TYPE_PAD, Pad, Pad, Pad);
	TOGGLE_FLAG(PCB_FLAG_NOPASTE, Pad);
	DrawPad(Pad);
	Draw();
	return (pcb_true);
}

/* ---------------------------------------------------------------------------
 * changes the CLEARPOLY flag of a polygon
 */
static void *ChangePolyClear(LayerTypePtr Layer, PolygonTypePtr Polygon)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Polygon))
		return (NULL);
	AddObjectToClearPolyUndoList(PCB_TYPE_POLYGON, Layer, Polygon, Polygon, pcb_true);
	AddObjectToFlagUndoList(PCB_TYPE_POLYGON, Layer, Polygon, Polygon);
	TOGGLE_FLAG(PCB_FLAG_CLEARPOLY, Polygon);
	InitClip(PCB->Data, Layer, Polygon);
	DrawPolygon(Layer, Polygon);
	return (Polygon);
}

/* ----------------------------------------------------------------------
 * changes the side of all selected and visible elements
 * returns pcb_true if anything has changed
 */
pcb_bool ChangeSelectedElementSide(void)
{
	pcb_bool change = pcb_false;

	/* setup identifiers */
	if (PCB->PinOn && PCB->ElementOn)
		ELEMENT_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_SELECTED, element)) {
			change |= ChangeElementSide(element, 0);
		}
	}
	END_LOOP;
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * changes the thermals on all selected and visible pins
 * and/or vias. Returns pcb_true if anything has changed
 */
pcb_bool ChangeSelectedThermals(int types, int therm_style)
{
	pcb_bool change = pcb_false;

	Delta = therm_style;
	change = SelectedOperation(&ChangeThermalFunctions, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * changes the size of all selected and visible object types
 * returns pcb_true if anything has changed
 */
pcb_bool ChangeSelectedSize(int types, Coord Difference, pcb_bool fixIt)
{
	pcb_bool change = pcb_false;

	/* setup identifiers */
	Absolute = (fixIt) ? Difference : 0;
	Delta = Difference;

	change = SelectedOperation(&ChangeSizeFunctions, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * changes the clearance size of all selected and visible objects
 * returns pcb_true if anything has changed
 */
pcb_bool ChangeSelectedClearSize(int types, Coord Difference, pcb_bool fixIt)
{
	pcb_bool change = pcb_false;

	/* setup identifiers */
	Absolute = (fixIt) ? Difference : 0;
	Delta = Difference;
	if (conf_core.editor.show_mask)
		change = SelectedOperation(&ChangeMaskSizeFunctions, pcb_false, types);
	else
		change = SelectedOperation(&ChangeClearSizeFunctions, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* --------------------------------------------------------------------------
 * changes the 2nd size (drilling hole) of all selected and visible objects
 * returns pcb_true if anything has changed
 */
pcb_bool ChangeSelected2ndSize(int types, Coord Difference, pcb_bool fixIt)
{
	pcb_bool change = pcb_false;

	/* setup identifiers */
	Absolute = (fixIt) ? Difference : 0;
	Delta = Difference;
	change = SelectedOperation(&Change2ndSizeFunctions, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * changes the clearance flag (join) of all selected and visible lines
 * and/or arcs. Returns pcb_true if anything has changed
 */
pcb_bool ChangeSelectedJoin(int types)
{
	pcb_bool change = pcb_false;

	change = SelectedOperation(&ChangeJoinFunctions, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * changes the clearance flag (join) of all selected and visible lines
 * and/or arcs. Returns pcb_true if anything has changed
 */
pcb_bool SetSelectedJoin(int types)
{
	pcb_bool change = pcb_false;

	change = SelectedOperation(&SetJoinFunctions, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * changes the clearance flag (join) of all selected and visible lines
 * and/or arcs. Returns pcb_true if anything has changed
 */
pcb_bool ClrSelectedJoin(int types)
{
	pcb_bool change = pcb_false;

	change = SelectedOperation(&ClrJoinFunctions, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * changes the nonetlist-flag of all selected and visible elements
 * returns pcb_true if anything has changed
 */
pcb_bool ChangeSelectedNonetlist(int types)
{
	pcb_bool change = pcb_false;

	change = SelectedOperation(&ChangeNonetlistFunctions, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

#if 0
/* ----------------------------------------------------------------------
 * sets the square-flag of all selected and visible pins or pads
 * returns pcb_true if anything has changed
 */
pcb_bool SetSelectedNonetlist(int types)
{
	pcb_bool change = pcb_false;

	change = SelectedOperation(&SetNonetlistFunctions, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * clears the square-flag of all selected and visible pins or pads
 * returns pcb_true if anything has changed
 */
pcb_bool ClrSelectedNonetlist(int types)
{
	pcb_bool change = pcb_false;

	change = SelectedOperation(&ClrNonetlistFunctions, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}
#endif

/* ----------------------------------------------------------------------
 * changes the square-flag of all selected and visible pins or pads
 * returns pcb_true if anything has changed
 */
pcb_bool ChangeSelectedSquare(int types)
{
	pcb_bool change = pcb_false;

	change = SelectedOperation(&ChangeSquareFunctions, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * changes the angle of all selected and visible object types
 * returns pcb_true if anything has changed
 */
pcb_bool ChangeSelectedAngle(int types, int is_start, Angle Difference, pcb_bool fixIt)
{
	pcb_bool change = pcb_false;

	/* setup identifiers */
	AAbsolute = (fixIt) ? Difference : 0;
	ADelta = Difference;
	is_primary = is_start;

	change = SelectedOperation(&ChangeAngleFunctions, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * changes the radius of all selected and visible object types
 * returns pcb_true if anything has changed
 */
pcb_bool ChangeSelectedRadius(int types, int is_start, Angle Difference, pcb_bool fixIt)
{
	pcb_bool change = pcb_false;

	/* setup identifiers */
	Absolute = (fixIt) ? Difference : 0;
	Delta = Difference;
	is_primary = is_start;

	change = SelectedOperation(&ChangeRadiusFunctions, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}


/* ----------------------------------------------------------------------
 * sets the square-flag of all selected and visible pins or pads
 * returns pcb_true if anything has changed
 */
pcb_bool SetSelectedSquare(int types)
{
	pcb_bool change = pcb_false;

	change = SelectedOperation(&SetSquareFunctions, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * clears the square-flag of all selected and visible pins or pads
 * returns pcb_true if anything has changed
 */
pcb_bool ClrSelectedSquare(int types)
{
	pcb_bool change = pcb_false;

	change = SelectedOperation(&ClrSquareFunctions, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * changes the octagon-flag of all selected and visible pins and vias
 * returns pcb_true if anything has changed
 */
pcb_bool ChangeSelectedOctagon(int types)
{
	pcb_bool change = pcb_false;

	change = SelectedOperation(&ChangeOctagonFunctions, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * sets the octagon-flag of all selected and visible pins and vias
 * returns pcb_true if anything has changed
 */
pcb_bool SetSelectedOctagon(int types)
{
	pcb_bool change = pcb_false;

	change = SelectedOperation(&SetOctagonFunctions, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * clears the octagon-flag of all selected and visible pins and vias
 * returns pcb_true if anything has changed
 */
pcb_bool ClrSelectedOctagon(int types)
{
	pcb_bool change = pcb_false;

	change = SelectedOperation(&ClrOctagonFunctions, pcb_false, types);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * changes the hole-flag of all selected and visible vias
 * returns pcb_true if anything has changed
 */
pcb_bool ChangeSelectedHole(void)
{
	pcb_bool change = pcb_false;

	if (PCB->ViaOn)
		VIA_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_SELECTED, via))
			change |= ChangeHole(via);
	}
	END_LOOP;
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ----------------------------------------------------------------------
 * changes the no paste-flag of all selected and visible pads
 * returns pcb_true if anything has changed
 */
pcb_bool ChangeSelectedPaste(void)
{
	pcb_bool change = pcb_false;

	ALLPAD_LOOP(PCB->Data);
	{
		if (TEST_FLAG(PCB_FLAG_SELECTED, pad))
			change |= ChangePaste(pad);
	}
	ENDALL_LOOP;
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}


/* ---------------------------------------------------------------------------
 * changes the size of the passed object; element size is silk size
 * Returns pcb_true if anything is changed
 */
pcb_bool ChangeObjectSize(int Type, void *Ptr1, void *Ptr2, void *Ptr3, Coord Difference, pcb_bool fixIt)
{
	pcb_bool change;

	/* setup identifier */
	Absolute = (fixIt) ? Difference : 0;
	Delta = Difference;
	change = (ObjectOperation(&ChangeSizeFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ---------------------------------------------------------------------------
 * changes the size of the passed object; element size is pin ring sizes
 * Returns pcb_true if anything is changed
 */
pcb_bool ChangeObject1stSize(int Type, void *Ptr1, void *Ptr2, void *Ptr3, Coord Difference, pcb_bool fixIt)
{
	pcb_bool change;

	/* setup identifier */
	Absolute = (fixIt) ? Difference : 0;
	Delta = Difference;
	change = (ObjectOperation(&Change1stSizeFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ---------------------------------------------------------------------------
 * changes the radius of the passed object (e.g. arc width/height)
 * Returns pcb_true if anything is changed
 */
pcb_bool ChangeObjectRadius(int Type, void *Ptr1, void *Ptr2, void *Ptr3, int is_x, Coord r, pcb_bool fixIt)
{
	pcb_bool change;

	/* setup identifier */
	Absolute = (fixIt) ? r : 0;
	Delta = r;
	is_primary = is_x;
	change = (ObjectOperation(&ChangeRadiusFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ---------------------------------------------------------------------------
 * changes the angles of the passed object (e.g. arc start/delta)
 * Returns pcb_true if anything is changed
 */
pcb_bool ChangeObjectAngle(int Type, void *Ptr1, void *Ptr2, void *Ptr3, int is_start, Angle a, pcb_bool fixIt)
{
	pcb_bool change;

	/* setup identifier */
	AAbsolute = (fixIt) ? a : 0;
	ADelta = a;
	is_primary = is_start;
	change = (ObjectOperation(&ChangeAngleFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}


/* ---------------------------------------------------------------------------
 * changes the clearance size of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool ChangeObjectClearSize(int Type, void *Ptr1, void *Ptr2, void *Ptr3, Coord Difference, pcb_bool fixIt)
{
	pcb_bool change;

	/* setup identifier */
	Absolute = (fixIt) ? Difference : 0;
	Delta = Difference;
	if (conf_core.editor.show_mask)
		change = (ObjectOperation(&ChangeMaskSizeFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL);
	else
		change = (ObjectOperation(&ChangeClearSizeFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ---------------------------------------------------------------------------
 * changes the thermal of the passed object
 * Returns pcb_true if anything is changed
 *
 */
pcb_bool ChangeObjectThermal(int Type, void *Ptr1, void *Ptr2, void *Ptr3, int therm_type)
{
	pcb_bool change;

	Delta = Absolute = therm_type;
	change = (ObjectOperation(&ChangeThermalFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ---------------------------------------------------------------------------
 * changes the 2nd size of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool ChangeObject2ndSize(int Type, void *Ptr1, void *Ptr2, void *Ptr3, Coord Difference, pcb_bool fixIt, pcb_bool incundo)
{
	pcb_bool change;

	/* setup identifier */
	Absolute = (fixIt) ? Difference : 0;
	Delta = Difference;
	change = (ObjectOperation(&Change2ndSizeFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		Draw();
		if (incundo)
			IncrementUndoSerialNumber();
	}
	return (change);
}

/* ---------------------------------------------------------------------------
 * changes the mask size of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool ChangeObjectMaskSize(int Type, void *Ptr1, void *Ptr2, void *Ptr3, Coord Difference, pcb_bool fixIt)
{
	pcb_bool change;

	/* setup identifier */
	Absolute = (fixIt) ? Difference : 0;
	Delta = Difference;
	change = (ObjectOperation(&ChangeMaskSizeFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL);
	if (change) {
		Draw();
		IncrementUndoSerialNumber();
	}
	return (change);
}

/* ---------------------------------------------------------------------------
 * changes the name of the passed object
 * returns the old name
 *
 * The allocated memory isn't freed because the old string is used
 * by the undo module.
 */
void *ChangeObjectName(int Type, void *Ptr1, void *Ptr2, void *Ptr3, char *Name)
{
	void *result;
	/* setup identifier */
	NewName = Name;
	result = ObjectOperation(&ChangeNameFunctions, Type, Ptr1, Ptr2, Ptr3);
	Draw();
	return (result);
}

/* ---------------------------------------------------------------------------
 * changes the pin number of the passed object
 * returns the old name
 *
 * The allocated memory isn't freed because the old string is used
 * by the undo module.
 */
void *ChangeObjectPinnum(int Type, void *Ptr1, void *Ptr2, void *Ptr3, char *Name)
{
	void *result;
	/* setup identifier */
	NewName = Name;
	result = ObjectOperation(&ChangePinnumFunctions, Type, Ptr1, Ptr2, Ptr3);
	Draw();
	return (result);
}

/* ---------------------------------------------------------------------------
 * changes the clearance-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool ChangeObjectJoin(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	if (ObjectOperation(&ChangeJoinFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * sets the clearance-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool SetObjectJoin(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	if (ObjectOperation(&SetJoinFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * clears the clearance-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool ClrObjectJoin(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	if (ObjectOperation(&ClrJoinFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * changes the square-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool ChangeObjectNonetlist(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	if (ObjectOperation(&ChangeNonetlistFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * changes the square-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool ChangeObjectSquare(int Type, void *Ptr1, void *Ptr2, void *Ptr3, int style)
{
	Absolute = style;
	if (ObjectOperation(&ChangeSquareFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * sets the square-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool SetObjectSquare(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	if (ObjectOperation(&SetSquareFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * clears the square-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool ClrObjectSquare(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	if (ObjectOperation(&ClrSquareFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * changes the octagon-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool ChangeObjectOctagon(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	if (ObjectOperation(&ChangeOctagonFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * sets the octagon-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool SetObjectOctagon(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	if (ObjectOperation(&SetOctagonFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * clears the octagon-flag of the passed object
 * Returns pcb_true if anything is changed
 */
pcb_bool ClrObjectOctagon(int Type, void *Ptr1, void *Ptr2, void *Ptr3)
{
	if (ObjectOperation(&ClrOctagonFunctions, Type, Ptr1, Ptr2, Ptr3) != NULL) {
		Draw();
		IncrementUndoSerialNumber();
		return (pcb_true);
	}
	return (pcb_false);
}

/* ---------------------------------------------------------------------------
 * queries the user for a new object name and changes it
 *
 * The allocated memory isn't freed because the old string is used
 * by the undo module.
 */
void *QueryInputAndChangeObjectName(int Type, void *Ptr1, void *Ptr2, void *Ptr3, int pinnum)
{
	char *name = NULL;
	char msg[513];

	/* if passed an element name, make it an element reference instead */
	if (Type == PCB_TYPE_ELEMENT_NAME) {
		Type = PCB_TYPE_ELEMENT;
		Ptr2 = Ptr1;
		Ptr3 = Ptr1;
	}
	switch (Type) {
	case PCB_TYPE_LINE:
		name = gui->prompt_for(_("Linename:"), EMPTY(((LineTypePtr) Ptr2)->Number));
		break;

	case PCB_TYPE_VIA:
		name = gui->prompt_for(_("Vianame:"), EMPTY(((PinTypePtr) Ptr2)->Name));
		break;

	case PCB_TYPE_PIN:
		if (pinnum)
			sprintf(msg, _("%s Pin Number:"), EMPTY(((PinTypePtr) Ptr2)->Number));
		else
			sprintf(msg, _("%s Pin Name:"), EMPTY(((PinTypePtr) Ptr2)->Number));
		name = gui->prompt_for(msg, EMPTY(((PinTypePtr) Ptr2)->Name));
		break;

	case PCB_TYPE_PAD:
		if (pinnum)
			sprintf(msg, _("%s Pad Number:"), EMPTY(((PadTypePtr) Ptr2)->Number));
		else
			sprintf(msg, _("%s Pad Name:"), EMPTY(((PadTypePtr) Ptr2)->Number));
		name = gui->prompt_for(msg, EMPTY(((PadTypePtr) Ptr2)->Name));
		break;

	case PCB_TYPE_TEXT:
		name = gui->prompt_for(_("Enter text:"), EMPTY(((TextTypePtr) Ptr2)->TextString));
		break;

	case PCB_TYPE_ELEMENT:
		name = gui->prompt_for(_("Elementname:"), EMPTY(ELEMENT_NAME(PCB, (ElementTypePtr) Ptr2)));
		break;
	}
	if (name) {
		/* NB: ChangeObjectName takes ownership of the passed memory */
		char *old;
		if (pinnum)
			old = (char *) ChangeObjectPinnum(Type, Ptr1, Ptr2, Ptr3, name);
		else
			old = (char *) ChangeObjectName(Type, Ptr1, Ptr2, Ptr3, name);

		if (old != (char *) -1) {
			if (pinnum)
				AddObjectToChangePinnumUndoList(Type, Ptr1, Ptr2, Ptr3, old);
			else
				AddObjectToChangeNameUndoList(Type, Ptr1, Ptr2, Ptr3, old);
			IncrementUndoSerialNumber();
		}
		Draw();
		return (Ptr3);
	}
	return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the maximum size of a layout
 * adjusts the scrollbars
 * releases the saved pixmap if necessary
 * and adjusts the cursor confinement box
 */
void ChangePCBSize(Coord Width, Coord Height)
{
	PCB->MaxWidth = Width;
	PCB->MaxHeight = Height;

	/* crosshair range is different if pastebuffer-mode
	 * is enabled
	 */
	if (conf_core.editor.mode == PCB_MODE_PASTE_BUFFER)
		SetCrosshairRange(PASTEBUFFER->X - PASTEBUFFER->BoundingBox.X1,
											PASTEBUFFER->Y - PASTEBUFFER->BoundingBox.Y1,
											MAX(0,
													Width - (PASTEBUFFER->BoundingBox.X2 -
																	 PASTEBUFFER->X)), MAX(0, Height - (PASTEBUFFER->BoundingBox.Y2 - PASTEBUFFER->Y)));
	else
		SetCrosshairRange(0, 0, Width, Height);

	if (gui != NULL)
		hid_action("PCBChanged");
}

/* ---------------------------------------------------------------------------
 * changes the mask size of a pad
 * returns pcb_true if changed
 */
static void *ChangePadMaskSize(ElementTypePtr Element, PadTypePtr Pad)
{
	Coord value = (Absolute) ? Absolute : Pad->Mask + Delta;

	value = MAX(value, 0);
	if (value == Pad->Mask && Absolute == 0)
		value = Pad->Thickness;
	if (value != Pad->Mask) {
		AddObjectToMaskSizeUndoList(PCB_TYPE_PAD, Element, Pad, Pad);
		ErasePad(Pad);
		r_delete_entry(PCB->Data->pad_tree, &Pad->BoundingBox);
		Pad->Mask = value;
		SetElementBoundingBox(PCB->Data, Element, &PCB->Font);
		DrawPad(Pad);
		return (Pad);
	}
	return (NULL);
}

/* ---------------------------------------------------------------------------
 * changes the mask size of a pin
 * returns pcb_true if changed
 */
static void *ChangePinMaskSize(ElementTypePtr Element, PinTypePtr Pin)
{
	Coord value = (Absolute) ? Absolute : Pin->Mask + Delta;

	value = MAX(value, 0);
	if (value == Pin->Mask && Absolute == 0)
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

/* ---------------------------------------------------------------------------
 * changes the mask size of a via
 * returns pcb_true if changed
 */
static void *ChangeViaMaskSize(PinTypePtr Via)
{
	Coord value;

	value = (Absolute) ? Absolute : Via->Mask + Delta;
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
