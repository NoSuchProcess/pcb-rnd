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

/* Drawing primitive: text */

#include "config.h"

#include "rotate.h"
#include "board.h"
#include "data.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "undo.h"
#include "polygon.h"

#include "obj_text.h"
#include "obj_text_op.h"
#include "obj_text_list.h"

/* TODO: remove this if draw.c is moved here: */
#include "draw.h"

/*** allocation ***/
/* get next slot for a text object, allocates memory if necessary */
TextTypePtr GetTextMemory(LayerType * layer)
{
	TextType *new_obj;

	new_obj = calloc(sizeof(TextType), 1);
	textlist_append(&layer->Text, new_obj);

	return new_obj;
}

void RemoveFreeText(TextType * data)
{
	textlist_remove(data);
	free(data);
}

/*** utility ***/

/* creates a new text on a layer */
TextTypePtr CreateNewText(LayerTypePtr Layer, FontTypePtr PCBFont, Coord X, Coord Y, unsigned Direction, int Scale, char *TextString, FlagType Flags)
{
	TextType *text;

	if (TextString == NULL)
		return NULL;

	text = GetTextMemory(Layer);
	if (text == NULL)
		return NULL;

	/* copy values, width and height are set by drawing routine
	 * because at this point we don't know which symbols are available
	 */
	text->X = X;
	text->Y = Y;
	text->Direction = Direction;
	text->Flags = Flags;
	text->Scale = Scale;
	text->TextString = pcb_strdup(TextString);

	pcb_add_text_on_layer(Layer, text, PCBFont);

	return (text);
}

void pcb_add_text_on_layer(LayerType *Layer, TextType *text, FontType *PCBFont)
{
	/* calculate size of the bounding box */
	SetTextBoundingBox(PCBFont, text);
	text->ID = CreateIDGet();
	if (!Layer->text_tree)
		Layer->text_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(Layer->text_tree, (BoxTypePtr) text, 0);
}

/* creates the bounding box of a text object */
void SetTextBoundingBox(FontTypePtr FontPtr, TextTypePtr Text)
{
	SymbolTypePtr symbol = FontPtr->Symbol;
	unsigned char *s = (unsigned char *) Text->TextString;
	int i;
	int space;

	Coord minx, miny, maxx, maxy, tx;
	Coord min_final_radius;
	Coord min_unscaled_radius;
	pcb_bool first_time = pcb_true;

	minx = miny = maxx = maxy = tx = 0;

	/* Calculate the bounding box based on the larger of the thicknesses
	 * the text might clamped at on silk or copper layers.
	 */
	min_final_radius = MAX(PCB->minWid, PCB->minSlk) / 2;

	/* Pre-adjust the line radius for the fact we are initially computing the
	 * bounds of the un-scaled text, and the thickness clamping applies to
	 * scaled text.
	 */
	min_unscaled_radius = PCB_UNPCB_SCALE_TEXT(min_final_radius, Text->Scale);

	/* calculate size of the bounding box */
	for (; s && *s; s++) {
		if (*s <= MAX_FONTPOSITION && symbol[*s].Valid) {
			LineTypePtr line = symbol[*s].Line;
			for (i = 0; i < symbol[*s].LineN; line++, i++) {
				/* Clamp the width of text lines at the minimum thickness.
				 * NB: Divide 4 in thickness calculation is comprised of a factor
				 *     of 1/2 to get a radius from the center-line, and a factor
				 *     of 1/2 because some stupid reason we render our glyphs
				 *     at half their defined stroke-width.
				 */
				Coord unscaled_radius = MAX(min_unscaled_radius, line->Thickness / 4);

				if (first_time) {
					minx = maxx = line->Point1.X;
					miny = maxy = line->Point1.Y;
					first_time = pcb_false;
				}

				minx = MIN(minx, line->Point1.X - unscaled_radius + tx);
				miny = MIN(miny, line->Point1.Y - unscaled_radius);
				minx = MIN(minx, line->Point2.X - unscaled_radius + tx);
				miny = MIN(miny, line->Point2.Y - unscaled_radius);
				maxx = MAX(maxx, line->Point1.X + unscaled_radius + tx);
				maxy = MAX(maxy, line->Point1.Y + unscaled_radius);
				maxx = MAX(maxx, line->Point2.X + unscaled_radius + tx);
				maxy = MAX(maxy, line->Point2.Y + unscaled_radius);
			}
			space = symbol[*s].Delta;
		}
		else {
			BoxType *ds = &FontPtr->DefaultSymbol;
			Coord w = ds->X2 - ds->X1;

			minx = MIN(minx, ds->X1 + tx);
			miny = MIN(miny, ds->Y1);
			minx = MIN(minx, ds->X2 + tx);
			miny = MIN(miny, ds->Y2);
			maxx = MAX(maxx, ds->X1 + tx);
			maxy = MAX(maxy, ds->Y1);
			maxx = MAX(maxx, ds->X2 + tx);
			maxy = MAX(maxy, ds->Y2);

			space = w / 5;
		}
		tx += symbol[*s].Width + space;
	}

	/* scale values */
	minx = PCB_SCALE_TEXT(minx, Text->Scale);
	miny = PCB_SCALE_TEXT(miny, Text->Scale);
	maxx = PCB_SCALE_TEXT(maxx, Text->Scale);
	maxy = PCB_SCALE_TEXT(maxy, Text->Scale);

	/* set upper-left and lower-right corner;
	 * swap coordinates if necessary (origin is already in 'swapped')
	 * and rotate box
	 */

	if (TEST_FLAG(PCB_FLAG_ONSOLDER, Text)) {
		Text->BoundingBox.X1 = Text->X + minx;
		Text->BoundingBox.Y1 = Text->Y - miny;
		Text->BoundingBox.X2 = Text->X + maxx;
		Text->BoundingBox.Y2 = Text->Y - maxy;
		RotateBoxLowLevel(&Text->BoundingBox, Text->X, Text->Y, (4 - Text->Direction) & 0x03);
	}
	else {
		Text->BoundingBox.X1 = Text->X + minx;
		Text->BoundingBox.Y1 = Text->Y + miny;
		Text->BoundingBox.X2 = Text->X + maxx;
		Text->BoundingBox.Y2 = Text->Y + maxy;
		RotateBoxLowLevel(&Text->BoundingBox, Text->X, Text->Y, Text->Direction);
	}

	/* the bounding box covers the extent of influence
	 * so it must include the clearance values too
	 */
	Text->BoundingBox.X1 -= PCB->Bloat;
	Text->BoundingBox.Y1 -= PCB->Bloat;
	Text->BoundingBox.X2 += PCB->Bloat;
	Text->BoundingBox.Y2 += PCB->Bloat;
	close_box(&Text->BoundingBox);
}



/*** ops ***/
/* copies a text to buffer */
void *AddTextToBuffer(pcb_opctx_t *ctx, LayerTypePtr Layer, TextTypePtr Text)
{
	LayerTypePtr layer = &ctx->buffer.dst->Layer[GetLayerNumber(ctx->buffer.src, Layer)];

	return (CreateNewText(layer, &PCB->Font, Text->X, Text->Y, Text->Direction, Text->Scale, Text->TextString, MaskFlags(Text->Flags, ctx->buffer.extraflg)));
}

/* moves a text to buffer without allocating memory for the name */
void *MoveTextToBuffer(pcb_opctx_t *ctx, LayerType * layer, TextType * text)
{
	LayerType *lay = &ctx->buffer.dst->Layer[GetLayerNumber(ctx->buffer.src, layer)];

	r_delete_entry(layer->text_tree, (BoxType *) text);
	RestoreToPolygon(ctx->buffer.src, PCB_TYPE_TEXT, layer, text);

	textlist_remove(text);
	textlist_append(&lay->Text, text);

	if (!lay->text_tree)
		lay->text_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(lay->text_tree, (BoxType *) text, 0);
	ClearFromPolygon(ctx->buffer.dst, PCB_TYPE_TEXT, lay, text);
	return (text);
}

/* changes the scaling factor of a text object */
void *ChangeTextSize(pcb_opctx_t *ctx, LayerTypePtr Layer, TextTypePtr Text)
{
	int value = ctx->chgsize.absolute ? PCB_COORD_TO_MIL(ctx->chgsize.absolute)
		: Text->Scale + PCB_COORD_TO_MIL(ctx->chgsize.delta);

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

/* sets data of a text object and calculates bounding box; memory must have
   already been allocated the one for the new string is allocated */
void *ChangeTextName(pcb_opctx_t *ctx, LayerTypePtr Layer, TextTypePtr Text)
{
	char *old = Text->TextString;

	if (TEST_FLAG(PCB_FLAG_LOCK, Text))
		return (NULL);
	EraseText(Layer, Text);
	RestoreToPolygon(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	Text->TextString = ctx->chgname.new_name;

	/* calculate size of the bounding box */
	SetTextBoundingBox(&PCB->Font, Text);
	r_insert_entry(Layer->text_tree, (BoxTypePtr) Text, 0);
	ClearFromPolygon(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	DrawText(Layer, Text);
	return (old);
}

/* changes the clearance flag of a text */
void *ChangeTextJoin(pcb_opctx_t *ctx, LayerTypePtr Layer, TextTypePtr Text)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Text))
		return (NULL);
	EraseText(Layer, Text);
	if (TEST_FLAG(PCB_FLAG_CLEARLINE, Text)) {
		AddObjectToClearPolyUndoList(PCB_TYPE_TEXT, Layer, Text, Text, pcb_false);
		RestoreToPolygon(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	}
	AddObjectToFlagUndoList(PCB_TYPE_TEXT, Layer, Text, Text);
	TOGGLE_FLAG(PCB_FLAG_CLEARLINE, Text);
	if (TEST_FLAG(PCB_FLAG_CLEARLINE, Text)) {
		AddObjectToClearPolyUndoList(PCB_TYPE_TEXT, Layer, Text, Text, pcb_true);
		ClearFromPolygon(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	}
	DrawText(Layer, Text);
	return (Text);
}

/* sets the clearance flag of a text */
void *SetTextJoin(pcb_opctx_t *ctx, LayerTypePtr Layer, TextTypePtr Text)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Text) || TEST_FLAG(PCB_FLAG_CLEARLINE, Text))
		return (NULL);
	return ChangeTextJoin(ctx, Layer, Text);
}

/* clears the clearance flag of a text */
void *ClrTextJoin(pcb_opctx_t *ctx, LayerTypePtr Layer, TextTypePtr Text)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, Text) || !TEST_FLAG(PCB_FLAG_CLEARLINE, Text))
		return (NULL);
	return ChangeTextJoin(ctx, Layer, Text);
}

/* copies a text */
void *CopyText(pcb_opctx_t *ctx, LayerTypePtr Layer, TextTypePtr Text)
{
	TextTypePtr text;

	text = CreateNewText(Layer, &PCB->Font, Text->X + ctx->copy.DeltaX,
											 Text->Y + ctx->copy.DeltaY, Text->Direction, Text->Scale, Text->TextString, MaskFlags(Text->Flags, PCB_FLAG_FOUND));
	DrawText(Layer, text);
	AddObjectToCreateUndoList(PCB_TYPE_TEXT, Layer, text, text);
	return (text);
}

/* moves a text object */
void *MoveText(pcb_opctx_t *ctx, LayerTypePtr Layer, TextTypePtr Text)
{
	RestoreToPolygon(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	r_delete_entry(Layer->text_tree, (BoxType *) Text);
	if (Layer->On) {
		EraseText(Layer, Text);
		MOVE_TEXT_LOWLEVEL(Text, ctx->move.dx, ctx->move.dy);
		DrawText(Layer, Text);
		Draw();
	}
	else
		MOVE_TEXT_LOWLEVEL(Text, ctx->move.dx, ctx->move.dy);
	r_insert_entry(Layer->text_tree, (BoxType *) Text, 0);
	ClearFromPolygon(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	return (Text);
}

/* moves a text object between layers; lowlevel routines */
void *MoveTextToLayerLowLevel(pcb_opctx_t *ctx, LayerType * Source, TextType * text, LayerType * Destination)
{
	RestoreToPolygon(PCB->Data, PCB_TYPE_TEXT, Source, text);
	r_delete_entry(Source->text_tree, (BoxType *) text);

	textlist_remove(text);
	textlist_append(&Destination->Text, text);

	if (GetLayerGroupNumberByNumber(solder_silk_layer) == GetLayerGroupNumberByPointer(Destination))
		SET_FLAG(PCB_FLAG_ONSOLDER, text);
	else
		CLEAR_FLAG(PCB_FLAG_ONSOLDER, text);

	/* re-calculate the bounding box (it could be mirrored now) */
	SetTextBoundingBox(&PCB->Font, text);
	if (!Destination->text_tree)
		Destination->text_tree = r_create_tree(NULL, 0, 0);
	r_insert_entry(Destination->text_tree, (BoxType *) text, 0);
	ClearFromPolygon(PCB->Data, PCB_TYPE_TEXT, Destination, text);

	return text;
}

/* moves a text object between layers */
void *MoveTextToLayer(pcb_opctx_t *ctx, LayerType * layer, TextType * text)
{
	if (TEST_FLAG(PCB_FLAG_LOCK, text)) {
		Message(PCB_MSG_DEFAULT, _("Sorry, the object is locked\n"));
		return NULL;
	}
	if (ctx->move.dst_layer != layer) {
		AddObjectToMoveToLayerUndoList(PCB_TYPE_TEXT, layer, text, text);
		if (layer->On)
			EraseText(layer, text);
		text = MoveTextToLayerLowLevel(ctx, layer, text, ctx->move.dst_layer);
		if (ctx->move.dst_layer->On)
			DrawText(ctx->move.dst_layer, text);
		if (layer->On || ctx->move.dst_layer->On)
			Draw();
	}
	return text;
}

/* destroys a text from a layer */
void *DestroyText(pcb_opctx_t *ctx, LayerTypePtr Layer, TextTypePtr Text)
{
	free(Text->TextString);
	r_delete_entry(Layer->text_tree, (BoxTypePtr) Text);

	RemoveFreeText(Text);

	return NULL;
}

/* removes a text from a layer */
void *RemoveText_op(pcb_opctx_t *ctx, LayerTypePtr Layer, TextTypePtr Text)
{
	/* erase from screen */
	if (Layer->On) {
		EraseText(Layer, Text);
		if (!ctx->remove.bulk)
			Draw();
	}
	MoveObjectToRemoveUndoList(PCB_TYPE_TEXT, Layer, Text, Text);
	return NULL;
}

void *RemoveText(LayerTypePtr Layer, TextTypePtr Text)
{
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.bulk = pcb_false;
	ctx.remove.destroy_target = NULL;

	return RemoveText_op(&ctx, Layer, Text);
}

/* rotates a text in 90 degree steps; only the bounding box is rotated,
   text rotation itself is done by the drawing routines */
void RotateTextLowLevel(TextTypePtr Text, Coord X, Coord Y, unsigned Number)
{
	pcb_uint8_t number;

	number = TEST_FLAG(PCB_FLAG_ONSOLDER, Text) ? (4 - Number) & 3 : Number;
	RotateBoxLowLevel(&Text->BoundingBox, X, Y, Number);
	ROTATE(Text->X, Text->Y, X, Y, Number);

	/* set new direction, 0..3,
	 * 0-> to the right, 1-> straight up,
	 * 2-> to the left, 3-> straight down
	 */
	Text->Direction = ((Text->Direction + number) & 0x03);
}

/* rotates a text object and redraws it */
void *RotateText(pcb_opctx_t *ctx, LayerTypePtr Layer, TextTypePtr Text)
{
	EraseText(Layer, Text);
	RestoreToPolygon(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	r_delete_entry(Layer->text_tree, (BoxTypePtr) Text);
	RotateTextLowLevel(Text, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
	r_insert_entry(Layer->text_tree, (BoxTypePtr) Text, 0);
	ClearFromPolygon(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	DrawText(Layer, Text);
	Draw();
	return (Text);
}
