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

/* Drawing primitive: smd pads */

#include "config.h"

#include "board.h"
#include "data.h"
#include "undo.h"
#include "polygon.h"
#include "compat_misc.h"
#include "conf_core.h"

#include "obj_pad.h"
#include "obj_pad_list.h"
#include "obj_pad_op.h"


/* TODO: remove this if draw.[ch] pads are moved */
#include "draw.h"
#include "obj_text_draw.h"
#include "obj_pad_draw.h"

/*** allocation ***/
/* get next slot for a pad, allocates memory if necessary */
pcb_pad_t *GetPadMemory(pcb_element_t * element)
{
	pcb_pad_t *new_obj;

	new_obj = calloc(sizeof(pcb_pad_t), 1);
	padlist_append(&element->Pad, new_obj);

	return new_obj;
}

void RemoveFreePad(pcb_pad_t * data)
{
	padlist_remove(data);
	free(data);
}


/*** utility ***/
/* creates a new pad in an element */
pcb_pad_t *CreateNewPad(pcb_element_t *Element, Coord X1, Coord Y1, Coord X2, Coord Y2, Coord Thickness, Coord Clearance, Coord Mask, char *Name, char *Number, FlagType Flags)
{
	pcb_pad_t *pad = GetPadMemory(Element);

	/* copy values */
	if (X1 > X2 || (X1 == X2 && Y1 > Y2)) {
		pad->Point1.X = X2;
		pad->Point1.Y = Y2;
		pad->Point2.X = X1;
		pad->Point2.Y = Y1;
	}
	else {
		pad->Point1.X = X1;
		pad->Point1.Y = Y1;
		pad->Point2.X = X2;
		pad->Point2.Y = Y2;
	}
	pad->Thickness = Thickness;
	pad->Clearance = Clearance;
	pad->Mask = Mask;
	pad->Name = pcb_strdup_null(Name);
	pad->Number = pcb_strdup_null(Number);
	pad->Flags = Flags;
	CLEAR_FLAG(PCB_FLAG_WARN, pad);
	pad->ID = CreateIDGet();
	pad->Element = Element;
	return (pad);
}

/* sets the bounding box of a pad */
void SetPadBoundingBox(pcb_pad_t *Pad)
{
	Coord width;
	Coord deltax;
	Coord deltay;

	/* the bounding box covers the extent of influence
	 * so it must include the clearance values too
	 */
	width = (Pad->Thickness + Pad->Clearance + 1) / 2;
	width = MAX(width, (Pad->Mask + 1) / 2);
	deltax = Pad->Point2.X - Pad->Point1.X;
	deltay = Pad->Point2.Y - Pad->Point1.Y;

	if (TEST_FLAG(PCB_FLAG_SQUARE, Pad) && deltax != 0 && deltay != 0) {
		/* slanted square pad */
		double theta;
		Coord btx, bty;

		/* T is a vector half a thickness long, in the direction of
		   one of the corners.  */
		theta = atan2(deltay, deltax);
		btx = width * cos(theta + M_PI / 4) * sqrt(2.0);
		bty = width * sin(theta + M_PI / 4) * sqrt(2.0);


		Pad->BoundingBox.X1 = MIN(MIN(Pad->Point1.X - btx, Pad->Point1.X - bty), MIN(Pad->Point2.X + btx, Pad->Point2.X + bty));
		Pad->BoundingBox.X2 = MAX(MAX(Pad->Point1.X - btx, Pad->Point1.X - bty), MAX(Pad->Point2.X + btx, Pad->Point2.X + bty));
		Pad->BoundingBox.Y1 = MIN(MIN(Pad->Point1.Y + btx, Pad->Point1.Y - bty), MIN(Pad->Point2.Y - btx, Pad->Point2.Y + bty));
		Pad->BoundingBox.Y2 = MAX(MAX(Pad->Point1.Y + btx, Pad->Point1.Y - bty), MAX(Pad->Point2.Y - btx, Pad->Point2.Y + bty));
	}
	else {
		/* Adjust for our discrete polygon approximation */
		width = (double) width *POLY_CIRC_RADIUS_ADJ + 0.5;

		Pad->BoundingBox.X1 = MIN(Pad->Point1.X, Pad->Point2.X) - width;
		Pad->BoundingBox.X2 = MAX(Pad->Point1.X, Pad->Point2.X) + width;
		Pad->BoundingBox.Y1 = MIN(Pad->Point1.Y, Pad->Point2.Y) - width;
		Pad->BoundingBox.Y2 = MAX(Pad->Point1.Y, Pad->Point2.Y) + width;
	}
	close_box(&Pad->BoundingBox);
}

/* changes the nopaste flag of a pad */
pcb_bool ChangePaste(pcb_pad_t *Pad)
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

/*** ops ***/

/* changes the size of a pad */
void *ChangePadSize(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad)
{
	Coord value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pad->Thickness + ctx->chgsize.delta;

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

/* changes the clearance size of a pad */
void *ChangePadClearSize(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad)
{
	Coord value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pad->Clearance + ctx->chgsize.delta;

	if (TEST_FLAG(PCB_FLAG_LOCK, Pad))
		return (NULL);
	value = MIN(MAX_LINESIZE, value);
	if (value < 0)
		value = 0;
	if (ctx->chgsize.delta < 0 && value < PCB->Bloat * 2)
		value = 0;
	if ((ctx->chgsize.delta > 0 || ctx->chgsize.absolute) && value < PCB->Bloat * 2)
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

/* changes the name of a pad */
void *ChangePadName(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad)
{
	char *old = Pad->Name;

	Element = Element;						/* get rid of 'unused...' warnings */
	if (TEST_FLAG(PCB_FLAG_DISPLAYNAME, Pad)) {
		ErasePadName(Pad);
		Pad->Name = ctx->chgname.new_name;
		DrawPadName(Pad);
	}
	else
		Pad->Name = ctx->chgname.new_name;
	return (old);
}

/* changes the number of a pad */
void *ChangePadNum(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad)
{
	char *old = Pad->Number;

	Element = Element;						/* get rid of 'unused...' warnings */
	if (TEST_FLAG(PCB_FLAG_DISPLAYNAME, Pad)) {
		ErasePadName(Pad);
		Pad->Number = ctx->chgname.new_name;
		DrawPadName(Pad);
	}
	else
		Pad->Number = ctx->chgname.new_name;
	return (old);
}

/* changes the square flag of a pad */
void *ChangePadSquare(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad)
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

/* sets the square flag of a pad */
void *SetPadSquare(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad)
{

	if (TEST_FLAG(PCB_FLAG_LOCK, Pad) || TEST_FLAG(PCB_FLAG_SQUARE, Pad))
		return (NULL);

	return (ChangePadSquare(ctx, Element, Pad));
}


/* clears the square flag of a pad */
void *ClrPadSquare(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad)
{

	if (TEST_FLAG(PCB_FLAG_LOCK, Pad) || !TEST_FLAG(PCB_FLAG_SQUARE, Pad))
		return (NULL);

	return (ChangePadSquare(ctx, Element, Pad));
}

/* changes the mask size of a pad */
void *ChangePadMaskSize(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad)
{
	Coord value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pad->Mask + ctx->chgsize.delta;

	value = MAX(value, 0);
	if (value == Pad->Mask && ctx->chgsize.absolute == 0)
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

/*** draw ***/
static void draw_pad_name(pcb_pad_t * pad)
{
	pcb_box_t box;
	pcb_bool vert;
	TextType text;
	char buff[128];
	const char *pn;

	if (!pad->Name || !pad->Name[0])
		pn = EMPTY(pad->Number);
	else
		pn = conf_core.editor.show_number ? pad->Number : pad->Name;

	if (GET_INTCONN(pad) > 0)
		pcb_snprintf(buff, sizeof(buff), "%s[%d]", pn, GET_INTCONN(pad));
	else
		strcpy(buff, pn);
	text.TextString = buff;

	/* should text be vertical ? */
	vert = (pad->Point1.X == pad->Point2.X);

	if (vert) {
		box.X1 = pad->Point1.X - pad->Thickness / 2;
		box.Y1 = MAX(pad->Point1.Y, pad->Point2.Y) + pad->Thickness / 2;
		box.X1 += conf_core.appearance.pinout.text_offset_y;
		box.Y1 -= conf_core.appearance.pinout.text_offset_x;
	}
	else {
		box.X1 = MIN(pad->Point1.X, pad->Point2.X) - pad->Thickness / 2;
		box.Y1 = pad->Point1.Y - pad->Thickness / 2;
		box.X1 += conf_core.appearance.pinout.text_offset_x;
		box.Y1 += conf_core.appearance.pinout.text_offset_y;
	}

	gui->set_color(Output.fgGC, PCB->PinNameColor);

	text.Flags = NoFlags();
	/* Set font height to approx 90% of pin thickness */
	text.Scale = 90 * pad->Thickness / FONT_CAPHEIGHT;
	text.X = box.X1;
	text.Y = box.Y1;
	text.Direction = vert ? 1 : 0;

	DrawTextLowLevel(&text, 0);
}

static void _draw_pad(hidGC gc, pcb_pad_t * pad, pcb_bool clear, pcb_bool mask)
{
	if (clear && !mask && pad->Clearance <= 0)
		return;

	if (conf_core.editor.thin_draw || (clear && conf_core.editor.thin_draw_poly))
		gui->thindraw_pcb_pad(gc, pad, clear, mask);
	else
		gui->fill_pcb_pad(gc, pad, clear, mask);
}

void draw_pad(pcb_pad_t * pad)
{
	const char *color = NULL;
	char buf[sizeof("#XXXXXX")];

	if (pcb_draw_doing_pinout)
		gui->set_color(Output.fgGC, PCB->PinColor);
	else if (TEST_FLAG(PCB_FLAG_WARN | PCB_FLAG_SELECTED | PCB_FLAG_FOUND, pad)) {
		if (TEST_FLAG(PCB_FLAG_WARN, pad))
			color = PCB->WarnColor;
		else if (TEST_FLAG(PCB_FLAG_SELECTED, pad))
			color = PCB->PinSelectedColor;
		else
			color = PCB->ConnectedColor;
	}
	else if (FRONT(pad))
		color = PCB->PinColor;
	else
		color = PCB->InvisibleObjectsColor;

	if (TEST_FLAG(PCB_FLAG_ONPOINT, pad)) {
		assert(color != NULL);
		LightenColor(color, buf, 1.75);
		color = buf;
	}

	if (color != NULL)
		gui->set_color(Output.fgGC, color);

	_draw_pad(Output.fgGC, pad, pcb_false, pcb_false);

	if (pcb_draw_doing_pinout || TEST_FLAG(PCB_FLAG_DISPLAYNAME, pad))
		draw_pad_name(pad);
}

r_dir_t draw_pad_callback(const pcb_box_t * b, void *cl)
{
	pcb_pad_t *pad = (pcb_pad_t *) b;
	int *side = cl;

	if (ON_SIDE(pad, *side))
		draw_pad(pad);
	return R_DIR_FOUND_CONTINUE;
}

r_dir_t clear_pad_callback(const pcb_box_t * b, void *cl)
{
	pcb_pad_t *pad = (pcb_pad_t *) b;
	int *side = cl;
	if (ON_SIDE(pad, *side) && pad->Mask)
		_draw_pad(Output.pmGC, pad, pcb_true, pcb_true);
	return R_DIR_FOUND_CONTINUE;
}

/* draws solder paste layer for a given side of the board - only pads get paste */
void DrawPaste(int side, const pcb_box_t * drawn_area)
{
	gui->set_color(Output.fgGC, PCB->ElementColor);
	ALLPAD_LOOP(PCB->Data);
	{
		if (ON_SIDE(pad, side) && !TEST_FLAG(PCB_FLAG_NOPASTE, pad) && pad->Mask > 0) {
			if (pad->Mask < pad->Thickness)
				_draw_pad(Output.fgGC, pad, pcb_true, pcb_true);
			else
				_draw_pad(Output.fgGC, pad, pcb_false, pcb_false);
		}
	}
	ENDALL_LOOP;
}

static void GatherPadName(pcb_pad_t *Pad)
{
	pcb_box_t box;
	pcb_bool vert;

	/* should text be vertical ? */
	vert = (Pad->Point1.X == Pad->Point2.X);

	if (vert) {
		box.X1 = Pad->Point1.X - Pad->Thickness / 2;
		box.Y1 = MAX(Pad->Point1.Y, Pad->Point2.Y) + Pad->Thickness / 2;
		box.X1 += conf_core.appearance.pinout.text_offset_y;
		box.Y1 -= conf_core.appearance.pinout.text_offset_x;
		box.X2 = box.X1;
		box.Y2 = box.Y1;
	}
	else {
		box.X1 = MIN(Pad->Point1.X, Pad->Point2.X) - Pad->Thickness / 2;
		box.Y1 = Pad->Point1.Y - Pad->Thickness / 2;
		box.X1 += conf_core.appearance.pinout.text_offset_x;
		box.Y1 += conf_core.appearance.pinout.text_offset_y;
		box.X2 = box.X1;
		box.Y2 = box.Y1;
	}

	pcb_draw_invalidate(&box);
	return;
}

void ErasePad(pcb_pad_t *Pad)
{
	pcb_draw_invalidate(Pad);
	if (TEST_FLAG(PCB_FLAG_DISPLAYNAME, Pad))
		ErasePadName(Pad);
	EraseFlags(&Pad->Flags);
}

void ErasePadName(pcb_pad_t *Pad)
{
	GatherPadName(Pad);
}

void DrawPad(pcb_pad_t *Pad)
{
	pcb_draw_invalidate(Pad);
	if (pcb_draw_doing_pinout || TEST_FLAG(PCB_FLAG_DISPLAYNAME, Pad))
		DrawPadName(Pad);
}

void DrawPadName(pcb_pad_t *Pad)
{
	GatherPadName(Pad);
}
