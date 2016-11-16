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
#include "obj_line_draw.h"
#include "obj_text_draw.h"
#include "conf_core.h"

/*** allocation ***/
/* get next slot for a text object, allocates memory if necessary */
pcb_text_t *pcb_text_alloc(pcb_layer_t * layer)
{
	pcb_text_t *new_obj;

	new_obj = calloc(sizeof(pcb_text_t), 1);
	textlist_append(&layer->Text, new_obj);

	return new_obj;
}

void pcb_text_free(pcb_text_t * data)
{
	textlist_remove(data);
	free(data);
}

/*** utility ***/

/* creates a new text on a layer */
pcb_text_t *pcb_text_new(pcb_layer_t *Layer, pcb_font_t *PCBFont, pcb_coord_t X, pcb_coord_t Y, unsigned Direction, int Scale, char *TextString, pcb_flag_t Flags)
{
	pcb_text_t *text;

	if (TextString == NULL)
		return NULL;

	text = pcb_text_alloc(Layer);
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

void pcb_add_text_on_layer(pcb_layer_t *Layer, pcb_text_t *text, pcb_font_t *PCBFont)
{
	/* calculate size of the bounding box */
	pcb_text_bbox(PCBFont, text);
	text->ID = CreateIDGet();
	if (!Layer->text_tree)
		Layer->text_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(Layer->text_tree, (pcb_box_t *) text, 0);
}

/* creates the bounding box of a text object */
void pcb_text_bbox(pcb_font_t *FontPtr, pcb_text_t *Text)
{
	pcb_symbol_t *symbol = FontPtr->Symbol;
	unsigned char *s = (unsigned char *) Text->TextString;
	int i;
	int space;

	pcb_coord_t minx, miny, maxx, maxy, tx;
	pcb_coord_t min_final_radius;
	pcb_coord_t min_unscaled_radius;
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
			pcb_line_t *line = symbol[*s].Line;
			for (i = 0; i < symbol[*s].LineN; line++, i++) {
				/* Clamp the width of text lines at the minimum thickness.
				 * NB: Divide 4 in thickness calculation is comprised of a factor
				 *     of 1/2 to get a radius from the center-line, and a factor
				 *     of 1/2 because some stupid reason we render our glyphs
				 *     at half their defined stroke-width.
				 */
				pcb_coord_t unscaled_radius = MAX(min_unscaled_radius, line->Thickness / 4);

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
			pcb_box_t *ds = &FontPtr->DefaultSymbol;
			pcb_coord_t w = ds->X2 - ds->X1;

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

	if (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, Text)) {
		Text->BoundingBox.X1 = Text->X + minx;
		Text->BoundingBox.Y1 = Text->Y - miny;
		Text->BoundingBox.X2 = Text->X + maxx;
		Text->BoundingBox.Y2 = Text->Y - maxy;
		pcb_box_rotate90(&Text->BoundingBox, Text->X, Text->Y, (4 - Text->Direction) & 0x03);
	}
	else {
		Text->BoundingBox.X1 = Text->X + minx;
		Text->BoundingBox.Y1 = Text->Y + miny;
		Text->BoundingBox.X2 = Text->X + maxx;
		Text->BoundingBox.Y2 = Text->Y + maxy;
		pcb_box_rotate90(&Text->BoundingBox, Text->X, Text->Y, Text->Direction);
	}

	/* the bounding box covers the extent of influence
	 * so it must include the clearance values too
	 */
	Text->BoundingBox.X1 -= PCB->Bloat;
	Text->BoundingBox.Y1 -= PCB->Bloat;
	Text->BoundingBox.X2 += PCB->Bloat;
	Text->BoundingBox.Y2 += PCB->Bloat;
	pcb_close_box(&Text->BoundingBox);
}



/*** ops ***/
/* copies a text to buffer */
void *AddTextToBuffer(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	pcb_layer_t *layer = &ctx->buffer.dst->Layer[GetLayerNumber(ctx->buffer.src, Layer)];

	return (pcb_text_new(layer, &PCB->Font, Text->X, Text->Y, Text->Direction, Text->Scale, Text->TextString, pcb_flag_mask(Text->Flags, ctx->buffer.extraflg)));
}

/* moves a text to buffer without allocating memory for the name */
void *MoveTextToBuffer(pcb_opctx_t *ctx, pcb_layer_t * layer, pcb_text_t * text)
{
	pcb_layer_t *lay = &ctx->buffer.dst->Layer[GetLayerNumber(ctx->buffer.src, layer)];

	pcb_r_delete_entry(layer->text_tree, (pcb_box_t *) text);
	pcb_poly_restore_to_poly(ctx->buffer.src, PCB_TYPE_TEXT, layer, text);

	textlist_remove(text);
	textlist_append(&lay->Text, text);

	if (!lay->text_tree)
		lay->text_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(lay->text_tree, (pcb_box_t *) text, 0);
	pcb_poly_clear_from_poly(ctx->buffer.dst, PCB_TYPE_TEXT, lay, text);
	return (text);
}

/* changes the scaling factor of a text object */
void *ChangeTextSize(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	int value = ctx->chgsize.absolute ? PCB_COORD_TO_MIL(ctx->chgsize.absolute)
		: Text->Scale + PCB_COORD_TO_MIL(ctx->chgsize.delta);

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Text))
		return (NULL);
	if (value <= MAX_TEXTSCALE && value >= MIN_TEXTSCALE && value != Text->Scale) {
		AddObjectToSizeUndoList(PCB_TYPE_TEXT, Layer, Text, Text);
		EraseText(Layer, Text);
		pcb_r_delete_entry(Layer->text_tree, (pcb_box_t *) Text);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
		Text->Scale = value;
		pcb_text_bbox(&PCB->Font, Text);
		pcb_r_insert_entry(Layer->text_tree, (pcb_box_t *) Text, 0);
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
		DrawText(Layer, Text);
		return (Text);
	}
	return (NULL);
}

/* sets data of a text object and calculates bounding box; memory must have
   already been allocated the one for the new string is allocated */
void *ChangeTextName(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	char *old = Text->TextString;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Text))
		return (NULL);
	EraseText(Layer, Text);
	pcb_r_delete_entry(Layer->text_tree, (pcb_box_t *)Text);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	Text->TextString = ctx->chgname.new_name;

	/* calculate size of the bounding box */
	pcb_text_bbox(&PCB->Font, Text);
	pcb_r_insert_entry(Layer->text_tree, (pcb_box_t *) Text, 0);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	DrawText(Layer, Text);
	return (old);
}

/* changes the clearance flag of a text */
void *ChangeTextJoin(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Text))
		return (NULL);
	EraseText(Layer, Text);
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Text)) {
		AddObjectToClearPolyUndoList(PCB_TYPE_TEXT, Layer, Text, Text, pcb_false);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	}
	AddObjectToFlagUndoList(PCB_TYPE_TEXT, Layer, Text, Text);
	PCB_FLAG_TOGGLE(PCB_FLAG_CLEARLINE, Text);
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Text)) {
		AddObjectToClearPolyUndoList(PCB_TYPE_TEXT, Layer, Text, Text, pcb_true);
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	}
	DrawText(Layer, Text);
	return (Text);
}

/* sets the clearance flag of a text */
void *SetTextJoin(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Text) || PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Text))
		return (NULL);
	return ChangeTextJoin(ctx, Layer, Text);
}

/* clears the clearance flag of a text */
void *ClrTextJoin(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Text) || !PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Text))
		return (NULL);
	return ChangeTextJoin(ctx, Layer, Text);
}

/* copies a text */
void *CopyText(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	pcb_text_t *text;

	text = pcb_text_new(Layer, &PCB->Font, Text->X + ctx->copy.DeltaX,
											 Text->Y + ctx->copy.DeltaY, Text->Direction, Text->Scale, Text->TextString, pcb_flag_mask(Text->Flags, PCB_FLAG_FOUND));
	DrawText(Layer, text);
	AddObjectToCreateUndoList(PCB_TYPE_TEXT, Layer, text, text);
	return (text);
}

/* moves a text object */
void *MoveText(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	pcb_r_delete_entry(Layer->text_tree, (pcb_box_t *) Text);
	if (Layer->On) {
		EraseText(Layer, Text);
		pcb_text_move(Text, ctx->move.dx, ctx->move.dy);
		DrawText(Layer, Text);
		pcb_draw();
	}
	else
		pcb_text_move(Text, ctx->move.dx, ctx->move.dy);
	pcb_r_insert_entry(Layer->text_tree, (pcb_box_t *) Text, 0);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	return (Text);
}

/* moves a text object between layers; lowlevel routines */
void *MoveTextToLayerLowLevel(pcb_opctx_t *ctx, pcb_layer_t * Source, pcb_text_t * text, pcb_layer_t * Destination)
{
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_TEXT, Source, text);
	pcb_r_delete_entry(Source->text_tree, (pcb_box_t *) text);

	textlist_remove(text);
	textlist_append(&Destination->Text, text);

	if (GetLayerGroupNumberByNumber(solder_silk_layer) == GetLayerGroupNumberByPointer(Destination))
		PCB_FLAG_SET(PCB_FLAG_ONSOLDER, text);
	else
		PCB_FLAG_CLEAR(PCB_FLAG_ONSOLDER, text);

	/* re-calculate the bounding box (it could be mirrored now) */
	pcb_text_bbox(&PCB->Font, text);
	if (!Destination->text_tree)
		Destination->text_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(Destination->text_tree, (pcb_box_t *) text, 0);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_TEXT, Destination, text);

	return text;
}

/* moves a text object between layers */
void *MoveTextToLayer(pcb_opctx_t *ctx, pcb_layer_t * layer, pcb_text_t * text)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, text)) {
		pcb_message(PCB_MSG_DEFAULT, _("Sorry, the object is locked\n"));
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
			pcb_draw();
	}
	return text;
}

/* destroys a text from a layer */
void *DestroyText(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	free(Text->TextString);
	pcb_r_delete_entry(Layer->text_tree, (pcb_box_t *) Text);

	pcb_text_free(Text);

	return NULL;
}

/* removes a text from a layer */
void *RemoveText_op(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	/* erase from screen */
	if (Layer->On) {
		EraseText(Layer, Text);
		pcb_r_delete_entry(Layer->text_tree, (pcb_box_t *)Text);
		if (!ctx->remove.bulk)
			pcb_draw();
	}
	MoveObjectToRemoveUndoList(PCB_TYPE_TEXT, Layer, Text, Text);
	return NULL;
}

void *pcb_text_destroy(pcb_layer_t *Layer, pcb_text_t *Text)
{
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.bulk = pcb_false;
	ctx.remove.destroy_target = NULL;

	return RemoveText_op(&ctx, Layer, Text);
}

/* rotates a text in 90 degree steps; only the bounding box is rotated,
   text rotation itself is done by the drawing routines */
void pcb_text_rotate90(pcb_text_t *Text, pcb_coord_t X, pcb_coord_t Y, unsigned Number)
{
	pcb_uint8_t number;

	number = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, Text) ? (4 - Number) & 3 : Number;
	pcb_box_rotate90(&Text->BoundingBox, X, Y, Number);
	PCB_COORD_ROTATE90(Text->X, Text->Y, X, Y, Number);

	/* set new direction, 0..3,
	 * 0-> to the right, 1-> straight up,
	 * 2-> to the left, 3-> straight down
	 */
	Text->Direction = ((Text->Direction + number) & 0x03);
}

/* rotates a text object and redraws it */
void *RotateText(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	EraseText(Layer, Text);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	pcb_r_delete_entry(Layer->text_tree, (pcb_box_t *) Text);
	pcb_text_rotate90(Text, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
	pcb_r_insert_entry(Layer->text_tree, (pcb_box_t *) Text, 0);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_TEXT, Layer, Text);
	DrawText(Layer, Text);
	pcb_draw();
	return (Text);
}

/*** draw ***/

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for text objects
 */
void DrawTextLowLevel(pcb_text_t *Text, pcb_coord_t min_line_width)
{
	pcb_coord_t x = 0;
	unsigned char *string = (unsigned char *) Text->TextString;
	pcb_cardinal_t n;
	pcb_font_t *font = &PCB->Font;

	while (string && *string) {
		/* draw lines if symbol is valid and data is present */
		if (*string <= MAX_FONTPOSITION && font->Symbol[*string].Valid) {
			pcb_line_t *line = font->Symbol[*string].Line;
			pcb_line_t newline;

			for (n = font->Symbol[*string].LineN; n; n--, line++) {
				/* create one line, scale, move, rotate and swap it */
				newline = *line;
				newline.Point1.X = PCB_SCALE_TEXT(newline.Point1.X + x, Text->Scale);
				newline.Point1.Y = PCB_SCALE_TEXT(newline.Point1.Y, Text->Scale);
				newline.Point2.X = PCB_SCALE_TEXT(newline.Point2.X + x, Text->Scale);
				newline.Point2.Y = PCB_SCALE_TEXT(newline.Point2.Y, Text->Scale);
				newline.Thickness = PCB_SCALE_TEXT(newline.Thickness, Text->Scale / 2);
				if (newline.Thickness < min_line_width)
					newline.Thickness = min_line_width;

				pcb_line_rotate90(&newline, 0, 0, Text->Direction);

				/* the labels of SMD objects on the bottom
				 * side haven't been swapped yet, only their offset
				 */
				if (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, Text)) {
					newline.Point1.X = PCB_SWAP_SIGN_X(newline.Point1.X);
					newline.Point1.Y = PCB_SWAP_SIGN_Y(newline.Point1.Y);
					newline.Point2.X = PCB_SWAP_SIGN_X(newline.Point2.X);
					newline.Point2.Y = PCB_SWAP_SIGN_Y(newline.Point2.Y);
				}
				/* add offset and draw line */
				newline.Point1.X += Text->X;
				newline.Point1.Y += Text->Y;
				newline.Point2.X += Text->X;
				newline.Point2.Y += Text->Y;
				_draw_line(&newline);
			}

			/* move on to next cursor position */
			x += (font->Symbol[*string].Width + font->Symbol[*string].Delta);
		}
		else {
			/* the default symbol is a filled box */
			pcb_box_t defaultsymbol = PCB->Font.DefaultSymbol;
			pcb_coord_t size = (defaultsymbol.X2 - defaultsymbol.X1) * 6 / 5;

			defaultsymbol.X1 = PCB_SCALE_TEXT(defaultsymbol.X1 + x, Text->Scale);
			defaultsymbol.Y1 = PCB_SCALE_TEXT(defaultsymbol.Y1, Text->Scale);
			defaultsymbol.X2 = PCB_SCALE_TEXT(defaultsymbol.X2 + x, Text->Scale);
			defaultsymbol.Y2 = PCB_SCALE_TEXT(defaultsymbol.Y2, Text->Scale);

			pcb_box_rotate90(&defaultsymbol, 0, 0, Text->Direction);

			/* add offset and draw box */
			defaultsymbol.X1 += Text->X;
			defaultsymbol.Y1 += Text->Y;
			defaultsymbol.X2 += Text->X;
			defaultsymbol.Y2 += Text->Y;
			gui->fill_rect(Output.fgGC, defaultsymbol.X1, defaultsymbol.Y1, defaultsymbol.X2, defaultsymbol.Y2);

			/* move on to next cursor position */
			x += size;
		}
		string++;
	}
}


pcb_r_dir_t draw_text_callback(const pcb_box_t * b, void *cl)
{
	pcb_layer_t *layer = cl;
	pcb_text_t *text = (pcb_text_t *) b;
	int min_silk_line;

	if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, text))
		gui->set_color(Output.fgGC, layer->SelectedColor);
	else
		gui->set_color(Output.fgGC, layer->Color);
	if (layer == &PCB->Data->SILKLAYER || layer == &PCB->Data->BACKSILKLAYER)
		min_silk_line = PCB->minSlk;
	else
		min_silk_line = PCB->minWid;
	DrawTextLowLevel(text, min_silk_line);
	return R_DIR_FOUND_CONTINUE;
}

/* erases a text on a layer */
void EraseText(pcb_layer_t *Layer, pcb_text_t *Text)
{
	pcb_draw_invalidate(Text);
}

void DrawText(pcb_layer_t *Layer, pcb_text_t *Text)
{
	pcb_draw_invalidate(Text);
}
