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
pcb_pad_t *pcb_pad_alloc(pcb_element_t * element)
{
	pcb_pad_t *new_obj;

	new_obj = calloc(sizeof(pcb_pad_t), 1);
	new_obj->type = PCB_OBJ_PAD;
	PCB_SET_PARENT(new_obj, element, element);

	padlist_append(&element->Pad, new_obj);

	return new_obj;
}

void pcb_pad_free(pcb_pad_t * data)
{
	padlist_remove(data);
	free(data);
}


/*** utility ***/
/* creates a new pad in an element */
pcb_pad_t *pcb_element_pad_new(pcb_element_t *Element, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t Thickness, pcb_coord_t Clearance, pcb_coord_t Mask, const char *Name, const char *Number, pcb_flag_t Flags)
{
	pcb_pad_t *pad = pcb_pad_alloc(Element);

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
	PCB_FLAG_CLEAR(PCB_FLAG_WARN, pad);
	pad->ID = pcb_create_ID_get();
	pad->Element = Element;
	return (pad);
}

pcb_pad_t *pcb_element_pad_new_rect(pcb_element_t *Element, pcb_coord_t X1, pcb_coord_t Y1, pcb_coord_t X2, pcb_coord_t Y2, pcb_coord_t Clearance, pcb_coord_t Mask, const char *Name, const char *Number, pcb_flag_t Flags)
{
	pcb_coord_t x1, y1, x2, y2, w, h, t;
	pcb_pad_t *pad;

	w = X1 - X2;
	h = Y1 - Y2;
	t = (w < h) ? w : h;
	x1 = X2 + t / 2;
	y1 = Y2 + t / 2;
	x2 = x1 + (w - t);
	y2 = y1 + (h - t);

	pad = pcb_element_pad_new(Element, x1, y1, x2, y2, t, 2 * Clearance, t + Mask, Name, Number, Flags);
	PCB_FLAG_SET(PCB_FLAG_SQUARE, pad);
	return pad;
}


/* sets the bounding box of a pad */
void pcb_pad_bbox(pcb_pad_t *Pad)
{
	pcb_coord_t width;
	pcb_coord_t deltax;
	pcb_coord_t deltay;

	/* the bounding box covers the extent of influence
	 * so it must include the clearance values too
	 */
	width = (Pad->Thickness + Pad->Clearance + 1) / 2;
	width = MAX(width, (Pad->Mask + 1) / 2);
	deltax = Pad->Point2.X - Pad->Point1.X;
	deltay = Pad->Point2.Y - Pad->Point1.Y;

	if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pad) && deltax != 0 && deltay != 0) {
		/* slanted square pad */
		double theta;
		pcb_coord_t btx, bty;

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
		width = (double) width *PCB_POLY_CIRC_RADIUS_ADJ + 0.5;

		Pad->BoundingBox.X1 = MIN(Pad->Point1.X, Pad->Point2.X) - width;
		Pad->BoundingBox.X2 = MAX(Pad->Point1.X, Pad->Point2.X) + width;
		Pad->BoundingBox.Y1 = MIN(Pad->Point1.Y, Pad->Point2.Y) - width;
		Pad->BoundingBox.Y2 = MAX(Pad->Point1.Y, Pad->Point2.Y) + width;
	}
	pcb_close_box(&Pad->BoundingBox);
}

void pcb_pad_copper_bbox(pcb_box_t *out, pcb_pad_t *Pad)
{
	pcb_coord_t width;
	pcb_coord_t deltax;
	pcb_coord_t deltay;

	/* the bounding box covers the extent of influence
	 * so it must include the clearance values too
	 */
	width = (Pad->Thickness + 1) / 2;
	deltax = Pad->Point2.X - Pad->Point1.X;
	deltay = Pad->Point2.Y - Pad->Point1.Y;

	if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pad) && deltax != 0 && deltay != 0) {
		/* slanted square pad */
		double theta;
		pcb_coord_t btx, bty;

		/* T is a vector half a thickness long, in the direction of
		   one of the corners.  */
		theta = atan2(deltay, deltax);
		btx = width * cos(theta + M_PI / 4) * sqrt(2.0);
		bty = width * sin(theta + M_PI / 4) * sqrt(2.0);


		out->X1 = MIN(MIN(Pad->Point1.X - btx, Pad->Point1.X - bty), MIN(Pad->Point2.X + btx, Pad->Point2.X + bty));
		out->X2 = MAX(MAX(Pad->Point1.X - btx, Pad->Point1.X - bty), MAX(Pad->Point2.X + btx, Pad->Point2.X + bty));
		out->Y1 = MIN(MIN(Pad->Point1.Y + btx, Pad->Point1.Y - bty), MIN(Pad->Point2.Y - btx, Pad->Point2.Y + bty));
		out->Y2 = MAX(MAX(Pad->Point1.Y + btx, Pad->Point1.Y - bty), MAX(Pad->Point2.Y - btx, Pad->Point2.Y + bty));
	}
	else {
		/* Adjust for our discrete polygon approximation */
		width = (double) width *PCB_POLY_CIRC_RADIUS_ADJ + 0.5;

		out->X1 = MIN(Pad->Point1.X, Pad->Point2.X) - width;
		out->X2 = MAX(Pad->Point1.X, Pad->Point2.X) + width;
		out->Y1 = MIN(Pad->Point1.Y, Pad->Point2.Y) - width;
		out->Y2 = MAX(Pad->Point1.Y, Pad->Point2.Y) + width;
	}
	pcb_close_box(out);
}

/* changes the nopaste flag of a pad */
pcb_bool pcb_pad_change_paste(pcb_pad_t *Pad)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pad))
		return (pcb_false);
	pcb_pad_invalidate_erase(Pad);
	pcb_undo_add_obj_to_flag(PCB_TYPE_PAD, Pad, Pad, Pad);
	PCB_FLAG_TOGGLE(PCB_FLAG_NOPASTE, Pad);
	pcb_pad_invalidate_draw(Pad);
	pcb_draw();
	return (pcb_true);
}

int pcb_pad_eq(const pcb_element_t *e1, const pcb_pad_t *p1, const pcb_element_t *e2, const pcb_pad_t *p2)
{
	if (pcb_field_neq(p1, p2, Thickness) || pcb_field_neq(p1, p2, Clearance)) return 0;
	if (pcb_element_neq_offsx(e1, p1, e2, p2, Point1.X) || pcb_element_neq_offsy(e1, p1, e2, p2, Point1.Y)) return 0;
	if (pcb_element_neq_offsx(e1, p1, e2, p2, Point2.X) || pcb_element_neq_offsy(e1, p1, e2, p2, Point2.Y)) return 0;
	if (pcb_field_neq(p1, p2, Mask)) return 0;
	if (pcb_neqs(p1->Name, p2->Name)) return 0;
	if (pcb_neqs(p1->Number, p2->Number)) return 0;
	return 1;
}

int pcb_pad_eq_padstack(const pcb_pad_t *p1, const pcb_pad_t *p2)
{
	if (pcb_field_neq(p1, p2, Thickness) || pcb_field_neq(p1, p2, Clearance)) return 0;
	if (PCB_ABS(p1->Point1.X - p1->Point2.X) != PCB_ABS(p2->Point1.X - p2->Point2.X)) return 0;
	if (PCB_ABS(p1->Point1.Y - p1->Point2.Y) != PCB_ABS(p2->Point1.Y - p2->Point2.Y)) return 0;
	if (pcb_field_neq(p1, p2, Mask)) return 0;
	return 1;
}

unsigned int pcb_pad_hash(const pcb_element_t *e, const pcb_pad_t *p)
{
	return
		pcb_hash_coord(p->Thickness) ^ pcb_hash_coord(p->Clearance) ^
		pcb_hash_element_ox(e, p->Point1.X) ^ pcb_hash_element_oy(e, p->Point1.Y) ^
		pcb_hash_element_ox(e, p->Point2.X) ^ pcb_hash_element_oy(e, p->Point2.Y) ^
		pcb_hash_coord(p->Mask) ^
		pcb_hash_str(p->Name) ^ pcb_hash_str(p->Number);
}

unsigned int pcb_pad_hash_padstack(const pcb_pad_t *p)
{
	return
		pcb_hash_coord(p->Thickness) ^ pcb_hash_coord(p->Clearance) ^
		pcb_hash_coord(PCB_ABS(p->Point1.X - p->Point2.X)) ^
		pcb_hash_coord(PCB_ABS(p->Point1.Y - p->Point2.Y)) ^
		pcb_hash_coord(p->Mask);
}


/*** ops ***/

/* changes the size of a pad */
void *pcb_padop_change_size(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pad->Thickness + ctx->chgsize.delta;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pad))
		return (NULL);
	if (value <= PCB_MAX_PADSIZE && value >= PCB_MIN_PADSIZE && value != Pad->Thickness) {
		pcb_undo_add_obj_to_size(PCB_TYPE_PAD, Element, Pad, Pad);
		pcb_undo_add_obj_to_mask_size(PCB_TYPE_PAD, Element, Pad, Pad);
		pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_PAD, Element, Pad);
		pcb_pad_invalidate_erase(Pad);
		pcb_r_delete_entry(PCB->Data->pad_tree, &Pad->BoundingBox);
		Pad->Mask += value - Pad->Thickness;
		Pad->Thickness = value;
		/* SetElementBB updates all associated rtrees */
		pcb_element_bbox(PCB->Data, Element, pcb_font(PCB, 0, 1));
		pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_PAD, Element, Pad);
		pcb_pad_invalidate_draw(Pad);
		return (Pad);
	}
	return (NULL);
}

/* changes the clearance size of a pad */
void *pcb_padop_change_clear_size(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pad->Clearance + ctx->chgsize.delta;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pad))
		return (NULL);
	if (value < 0)
		value = 0;
	else
		value = MIN(PCB_MAX_LINESIZE, value);
	if (ctx->chgsize.delta < 0 && value < PCB->Bloat * 2)
		value = 0;
	if ((ctx->chgsize.delta > 0 || ctx->chgsize.absolute) && value < PCB->Bloat * 2)
		value = PCB->Bloat * 2 + 2;
	if (value == Pad->Clearance)
		return NULL;
	pcb_undo_add_obj_to_clear_size(PCB_TYPE_PAD, Element, Pad, Pad);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_PAD, Element, Pad);
	pcb_pad_invalidate_erase(Pad);
	pcb_r_delete_entry(PCB->Data->pad_tree, &Pad->BoundingBox);
	Pad->Clearance = value;
	/* SetElementBB updates all associated rtrees */
	pcb_element_bbox(PCB->Data, Element, pcb_font(PCB, 0, 1));
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_PAD, Element, Pad);
	pcb_pad_invalidate_draw(Pad);
	return Pad;
}

/* changes the name of a pad */
void *pcb_padop_change_name(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad)
{
	char *old = Pad->Name;

	Element = Element;						/* get rid of 'unused...' warnings */
	if (PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, Pad)) {
		pcb_pad_name_invalidate_erase(Pad);
		Pad->Name = ctx->chgname.new_name;
		pcb_pad_name_invalidate_draw(Pad);
	}
	else
		Pad->Name = ctx->chgname.new_name;
	return (old);
}

/* changes the number of a pad */
void *pcb_padop_change_num(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad)
{
	char *old = Pad->Number;

	Element = Element;						/* get rid of 'unused...' warnings */
	if (PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, Pad)) {
		pcb_pad_name_invalidate_erase(Pad);
		Pad->Number = ctx->chgname.new_name;
		pcb_pad_name_invalidate_draw(Pad);
	}
	else
		Pad->Number = ctx->chgname.new_name;
	return (old);
}

/* changes the square flag of a pad */
void *pcb_padop_change_square(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pad))
		return (NULL);
	pcb_pad_invalidate_erase(Pad);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_PAD, Element, Pad, Pad, pcb_false);
	pcb_poly_restore_to_poly(PCB->Data, PCB_TYPE_PAD, Element, Pad);
	pcb_undo_add_obj_to_flag(PCB_TYPE_PAD, Element, Pad, Pad);
	PCB_FLAG_TOGGLE(PCB_FLAG_SQUARE, Pad);
	pcb_undo_add_obj_to_clear_poly(PCB_TYPE_PAD, Element, Pad, Pad, pcb_true);
	pcb_poly_clear_from_poly(PCB->Data, PCB_TYPE_PAD, Element, Pad);
	pcb_pad_invalidate_draw(Pad);
	return (Pad);
}

/* sets the square flag of a pad */
void *pcb_padop_set_square(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad)
{

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pad) || PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pad))
		return (NULL);

	return (pcb_padop_change_square(ctx, Element, Pad));
}


/* clears the square flag of a pad */
void *pcb_padop_clear_square(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad)
{

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Pad) || !PCB_FLAG_TEST(PCB_FLAG_SQUARE, Pad))
		return (NULL);

	return (pcb_padop_change_square(ctx, Element, Pad));
}

/* changes the mask size of a pad */
void *pcb_padop_change_mask_size(pcb_opctx_t *ctx, pcb_element_t *Element, pcb_pad_t *Pad)
{
	pcb_coord_t value = (ctx->chgsize.absolute) ? ctx->chgsize.absolute : Pad->Mask + ctx->chgsize.delta;

	value = MAX(value, 0);
	if (value == Pad->Mask && ctx->chgsize.absolute == 0)
		value = Pad->Thickness;
	if (value != Pad->Mask) {
		pcb_undo_add_obj_to_mask_size(PCB_TYPE_PAD, Element, Pad, Pad);
		pcb_pad_invalidate_erase(Pad);
		pcb_r_delete_entry(PCB->Data->pad_tree, &Pad->BoundingBox);
		Pad->Mask = value;
		pcb_element_bbox(PCB->Data, Element, pcb_font(PCB, 0, 1));
		pcb_pad_invalidate_draw(Pad);
		return (Pad);
	}
	return (NULL);
}

#define PCB_PAD_FLAGS (PCB_FLAG_FOUND | PCB_FLAG_NOPASTE | PCB_FLAG_PININPOLY | PCB_FLAG_SELECTED | PCB_FLAG_AUTO | PCB_FLAG_LOCK | PCB_FLAG_VISIT)
void *pcb_padop_change_flag(pcb_opctx_t *ctx, pcb_data_t *data, pcb_pad_t *pad)
{
	if ((ctx->chgflag.flag & PCB_PAD_FLAGS) != ctx->chgflag.flag)
		return NULL;
	PCB_FLAG_CHANGE(ctx->chgflag.how, ctx->chgflag.flag, pad);
	return pad;
}

/*** draw ***/
static void draw_pad_name(pcb_pad_t * pad)
{
	pcb_box_t box;
	pcb_bool vert, flip_x, flip_y;
	pcb_coord_t x_off, y_off;
	pcb_text_t text;
	char buff[128];
	const char *pn;

	if (!pad->Name || !pad->Name[0]) {
		pn = PCB_EMPTY(pad->Number);
	}
	else {
		pn = conf_core.editor.show_number ? pad->Number : pad->Name;
		if (pn == NULL)
			pn = "n/a";
	}

	if (PCB_FLAG_INTCONN_GET(pad) > 0)
		pcb_snprintf(buff, sizeof(buff), "%s[%d]", pn, PCB_FLAG_INTCONN_GET(pad));
	else
		strcpy(buff, pn);
	text.TextString = buff;

	/* should text be vertical ? */
	vert = (pad->Point1.X == pad->Point2.X);
	flip_x = conf_core.editor.view.flip_x;
	flip_y = conf_core.editor.view.flip_y;

	if (vert) {
		x_off = -pad->Thickness / 2 + conf_core.appearance.pinout.text_offset_y;
		y_off = pad->Thickness / 2 - conf_core.appearance.pinout.text_offset_x;
		box.X1 = pad->Point1.X + (flip_x ? -x_off : x_off);
		box.Y1 = flip_y ? (MIN(pad->Point1.Y, pad->Point2.Y) - y_off) : (MAX(pad->Point1.Y, pad->Point2.Y) + y_off);
	}
	else {
		x_off = -pad->Thickness / 2 + conf_core.appearance.pinout.text_offset_x;
		y_off = -pad->Thickness / 2 + conf_core.appearance.pinout.text_offset_y;
		box.X1 = flip_x ? (MAX(pad->Point1.X, pad->Point2.X) - x_off) : (MIN(pad->Point1.X, pad->Point2.X) + x_off);
		box.Y1 = pad->Point1.Y + (flip_y ? -y_off : y_off);
	}

	pcb_gui->set_color(Output.fgGC, conf_core.appearance.color.pin_name);

	text.Flags = (flip_x ^ flip_y) ? pcb_flag_make (PCB_FLAG_ONSOLDER) : pcb_no_flags();
	/* Set font height to approx 90% of pin thickness */
	text.Scale = 90 * pad->Thickness / PCB_FONT_CAPHEIGHT;
	text.X = box.X1;
	text.Y = box.Y1;
	text.fid = 0;
	text.Direction = (vert ? 1 : 0) + (flip_x ? 2 : 0);

	pcb_text_draw(&text, 0);
}

static void _draw_pad(pcb_hid_gc_t gc, pcb_pad_t * pad, pcb_bool clear, pcb_bool mask)
{
	if (clear && !mask && pad->Clearance <= 0)
		return;

	PCB_DRAW_BBOX(pad);

	if (conf_core.editor.thin_draw || (clear && conf_core.editor.thin_draw_poly))
		pcb_gui->thindraw_pcb_pad(gc, pad, clear, mask);
	else
		pcb_gui->fill_pcb_pad(gc, pad, clear, mask);
}

void pcb_pad_draw(pcb_pad_t * pad)
{
	const char *color = NULL;
	char buf[sizeof("#XXXXXX")];

	if (pcb_draw_doing_pinout)
		pcb_gui->set_color(Output.fgGC, conf_core.appearance.color.pin);
	else if (PCB_FLAG_TEST(PCB_FLAG_WARN | PCB_FLAG_SELECTED | PCB_FLAG_FOUND, pad)) {
		if (PCB_FLAG_TEST(PCB_FLAG_WARN, pad))
			color = conf_core.appearance.color.warn;
		else if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, pad))
			color = conf_core.appearance.color.pin_selected;
		else
			color = conf_core.appearance.color.connected;
	}
	else if (PCB_FRONT(pad))
		color = conf_core.appearance.color.pin;
	else
		color = conf_core.appearance.color.invisible_objects;

	if (PCB_FLAG_TEST(PCB_FLAG_ONPOINT, pad)) {
		assert(color != NULL);
		pcb_lighten_color(color, buf, 1.75);
		color = buf;
	}

	if (color != NULL)
		pcb_gui->set_color(Output.fgGC, color);

	_draw_pad(Output.fgGC, pad, pcb_false, pcb_false);

	if (pcb_draw_doing_pinout)
		draw_pad_name(pad);
}

pcb_r_dir_t pcb_pad_draw_callback(const pcb_box_t * b, void *cl)
{
	pcb_pad_t *pad = (pcb_pad_t *) b;
	int *side = cl;

	if (PCB_ON_SIDE(pad, *side))
		pcb_pad_draw(pad);
	return PCB_R_DIR_FOUND_CONTINUE;
}

pcb_r_dir_t pcb_pad_name_draw_callback(const pcb_box_t * b, void *cl)
{
	pcb_pad_t *pad = (pcb_pad_t *) b;
	int *side = cl;

	if (PCB_ON_SIDE(pad, *side)) {
		if (PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, pad))
			draw_pad_name(pad);
	}
	return PCB_R_DIR_FOUND_CONTINUE;
}

pcb_r_dir_t pcb_pad_clear_callback(const pcb_box_t * b, void *cl)
{
	pcb_pad_t *pad = (pcb_pad_t *) b;
	int *side = cl;
	if (PCB_ON_SIDE(pad, *side) && pad->Mask)
		_draw_pad(Output.pmGC, pad, pcb_true, pcb_true);
	return PCB_R_DIR_FOUND_CONTINUE;
}

/* draws solder paste layer for a given side of the board - only pads get paste */
void pcb_pad_paste_draw(int side, const pcb_box_t * drawn_area)
{
	pcb_gui->set_color(Output.fgGC, conf_core.appearance.color.paste);
	PCB_PAD_ALL_LOOP(PCB->Data);
	{
		if (PCB_ON_SIDE(pad, side) && !PCB_FLAG_TEST(PCB_FLAG_NOPASTE, pad) && pad->Mask > 0) {
			pcb_coord_t save_thickness = pad->Thickness;
			if (conf_core.design.paste_adjust)
				pad->Thickness = max(0, pad->Thickness + conf_core.design.paste_adjust);
			if (pad->Mask < pad->Thickness)
				_draw_pad(Output.fgGC, pad, pcb_true, pcb_true);
			else
				_draw_pad(Output.fgGC, pad, pcb_false, pcb_false);
			pad->Thickness = save_thickness;
		}
	}
	PCB_ENDALL_LOOP;
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

void pcb_pad_invalidate_erase(pcb_pad_t *Pad)
{
	pcb_draw_invalidate(Pad);
	if (PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, Pad))
		pcb_pad_name_invalidate_erase(Pad);
	pcb_flag_erase(&Pad->Flags);
}

void pcb_pad_name_invalidate_erase(pcb_pad_t *Pad)
{
	GatherPadName(Pad);
}

void pcb_pad_invalidate_draw(pcb_pad_t *Pad)
{
	pcb_draw_invalidate(Pad);
	if (pcb_draw_doing_pinout || PCB_FLAG_TEST(PCB_FLAG_DISPLAYNAME, Pad))
		pcb_pad_name_invalidate_draw(Pad);
}

void pcb_pad_name_invalidate_draw(pcb_pad_t *Pad)
{
	GatherPadName(Pad);
}
