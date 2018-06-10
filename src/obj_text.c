/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* Drawing primitive: text */

#include "config.h"

#include "rotate.h"
#include "board.h"
#include "data.h"
#include "compat_misc.h"
#include "compat_nls.h"
#include "hid_inlines.h"
#include "undo.h"
#include "polygon.h"
#include "event.h"
#include "layer.h"

#include "obj_text.h"
#include "obj_text_op.h"
#include "obj_text_list.h"
#include "obj_poly_draw.h"
#include "obj_arc_draw.h"

#include "obj_subc_parent.h"
#include "obj_hash.h"

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
	new_obj->type = PCB_OBJ_TEXT;
	new_obj->Attributes.post_change = pcb_obj_attrib_post_change;
	PCB_SET_PARENT(new_obj, layer, layer);

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
pcb_text_t *pcb_text_new(pcb_layer_t *Layer, pcb_font_t *PCBFont, pcb_coord_t X, pcb_coord_t Y, unsigned Direction, int Scale, const char *TextString, pcb_flag_t Flags)
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
	text->fid = PCBFont->id;
	text->ID = pcb_create_ID_get();

	pcb_add_text_on_layer(Layer, text, PCBFont);

	return text;
}

static pcb_text_t *pcb_text_copy_meta(pcb_text_t *dst, pcb_text_t *src)
{
	if (dst == NULL)
		return NULL;
	pcb_attribute_copy_all(&dst->Attributes, &src->Attributes);
	return dst;
}

pcb_text_t *pcb_text_dup(pcb_layer_t *dst, pcb_text_t *src)
{
	pcb_text_t *t = pcb_text_new(dst, pcb_font(PCB, src->fid, 1), src->X, src->Y, src->Direction, src->Scale, src->TextString, src->Flags);
	pcb_text_copy_meta(t, src);
	return t;
}

pcb_text_t *pcb_text_dup_at(pcb_layer_t *dst, pcb_text_t *src, pcb_coord_t dx, pcb_coord_t dy)
{
	pcb_text_t *t = pcb_text_new(dst, pcb_font(PCB, src->fid, 1), src->X+dx, src->Y+dy, src->Direction, src->Scale, src->TextString, src->Flags);
	pcb_text_copy_meta(t, src);
	return t;
}

void pcb_add_text_on_layer(pcb_layer_t *Layer, pcb_text_t *text, pcb_font_t *PCBFont)
{
	/* calculate size of the bounding box */
	pcb_text_bbox(PCBFont, text);
	if (!Layer->text_tree)
		Layer->text_tree = pcb_r_create_tree();
	pcb_r_insert_entry(Layer->text_tree, (pcb_box_t *) text);
}

static int pcb_text_render_str_cb(void *ctx, gds_t *s, const char **input)
{
	pcb_text_t *text = ctx;
	char *end, key[128], *path, *attrs;
	size_t len;

	end = strchr(*input, '%');
	len = end - *input;
	if (len > sizeof(key)-1)
		return -1;

	strncpy(key, *input, len);
	key[len] = '\0';
	*input += len+1;

	if ((key[0] == 'a') && (key[1] == '.')) {
		pcb_attribute_list_t *attr = &text->Attributes;
		path = key+2;
		if ((path[0] == 'p') && (memcmp(path, "parent.", 7) == 0)) {
			pcb_data_t *par = text->parent.layer->parent.data;
			if (par->parent_type == PCB_PARENT_SUBC)
				attr = &par->parent.subc->Attributes;
			else if (par->parent_type == PCB_PARENT_BOARD)
				attr = &par->parent.board->Attributes;
			else
				attr = NULL;
			path+=7;
		}
		if (attr != NULL) {
			attrs = pcb_attribute_get(attr, path);
			if (attrs != NULL)
				gds_append_str(s, attrs);
		}
	}
	return 0;
}

/* Render the string of a text, doing substitution if needed - don't allocate if there's no subst */
static unsigned char *pcb_text_render_str(pcb_text_t *text)
{
	unsigned char *res;

	if (!PCB_FLAG_TEST(PCB_FLAG_DYNTEXT, text))
		return (unsigned char *)text->TextString;

	res = (unsigned char *)pcb_strdup_subst(text->TextString, pcb_text_render_str_cb, text, PCB_SUBST_PERCENT | PCB_SUBST_CONF);
	if (res == NULL) {
		res = (unsigned char *)pcb_strdup("<!>");
	}
	else if (*res == '\0') {
		free(res);
		res = (unsigned char *)pcb_strdup("<?>");
	}

	return res;
}

int pcb_append_dyntext(gds_t *dst, pcb_any_obj_t *obj, const char *fmt)
{
	return pcb_subst_append(dst, fmt, pcb_text_render_str_cb, obj, PCB_SUBST_PERCENT | PCB_SUBST_CONF, 0);
}

/* Free rendered if it was allocated */
static void pcb_text_free_str(pcb_text_t *text, unsigned char *rendered)
{
	if ((unsigned char *)text->TextString != rendered)
		free(rendered);
}


/* creates the bounding box of a text object */
void pcb_text_bbox(pcb_font_t *FontPtr, pcb_text_t *Text)
{
	pcb_symbol_t *symbol;
	unsigned char *s, *rendered = pcb_text_render_str(Text);
	int i;
	int space;
	pcb_coord_t minx, miny, maxx, maxy, tx;
	pcb_coord_t min_final_radius;
	pcb_coord_t min_unscaled_radius;
	pcb_bool first_time = pcb_true;
	pcb_poly_t *poly;

	s = rendered;

	if (FontPtr == NULL)
		FontPtr = pcb_font(PCB, Text->fid, 1);

	symbol = FontPtr->Symbol;

	minx = miny = maxx = maxy = tx = 0;

	/* Calculate the bounding box based on the larger of the thicknesses
	 * the text might clamped at on silk or copper layers.
	 */
	min_final_radius = MAX(conf_core.design.min_wid, conf_core.design.min_slk) / 2;

	/* Pre-adjust the line radius for the fact we are initially computing the
	 * bounds of the un-scaled text, and the thickness clamping applies to
	 * scaled text.
	 */
	min_unscaled_radius = PCB_UNPCB_SCALE_TEXT(min_final_radius, Text->Scale);

	/* calculate size of the bounding box */
	for (; s && *s; s++) {
		if (*s <= PCB_MAX_FONTPOSITION && symbol[*s].Valid) {
			pcb_line_t *line = symbol[*s].Line;
			pcb_arc_t *arc;
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

			for(arc = arclist_first(&symbol[*s].arcs); arc != NULL; arc = arclist_next(arc)) {
				pcb_arc_bbox(arc);
				maxx = MAX(maxx, arc->BoundingBox.X2);
				maxy = MAX(maxy, arc->BoundingBox.X2);
			}

			for(poly = polylist_first(&symbol[*s].polys); poly != NULL; poly = polylist_next(poly)) {
				int n;
				pcb_point_t *pnt;
				for(n = 0, pnt = poly->Points; n < poly->PointN; n++,pnt++) {
					maxx = MAX(maxx, pnt->X + tx);
					maxy = MAX(maxy, pnt->Y);
				}
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
	Text->BoundingBox.X1 -= conf_core.design.bloat;
	Text->BoundingBox.Y1 -= conf_core.design.bloat;
	Text->BoundingBox.X2 += conf_core.design.bloat;
	Text->BoundingBox.Y2 += conf_core.design.bloat;
	pcb_close_box(&Text->BoundingBox);
	pcb_text_free_str(Text, rendered);
}

int pcb_text_eq(const pcb_host_trans_t *tr1, const pcb_text_t *t1, const pcb_host_trans_t *tr2, const pcb_text_t *t2)
{
	if (pcb_neqs(t1->TextString, t2->TextString)) return 0;
	if (pcb_neqs(t1->term, t2->term)) return 0;

	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, t1) && !PCB_FLAG_TEST(PCB_FLAG_FLOATER, t2)) {
		if (pcb_field_neq(t1, t2, Scale)) return 0;
		if (pcb_neq_tr_coords(tr1, t1->X, t1->Y, tr2, t2->X, t2->Y)) return 0;
	}

	return 1;
}

unsigned int pcb_text_hash(const pcb_host_trans_t *tr, const pcb_text_t *t)
{
	unsigned int crd = 0;

	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, t)) {
		pcb_coord_t x, y;

		pcb_hash_tr_coords(tr, &x, &y, t->X, t->Y);
		crd = pcb_hash_coord(x) ^ pcb_hash_coord(y) ^ pcb_hash_coord(t->Scale);
	}

	return pcb_hash_str(t->TextString) ^ pcb_hash_str(t->term) ^ crd;
}


/*** ops ***/
/* copies a text to buffer */
void *pcb_textop_add_to_buffer(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	pcb_layer_t *layer = &ctx->buffer.dst->Layer[pcb_layer_id(ctx->buffer.src, Layer)];
	pcb_text_t *t = pcb_text_new(layer, pcb_font(PCB, Text->fid, 1), Text->X, Text->Y, Text->Direction, Text->Scale, Text->TextString, pcb_flag_mask(Text->Flags, ctx->buffer.extraflg));

	pcb_text_copy_meta(t, Text);
	return t;
}

/* moves a text without allocating memory for the name between board and buffer */
void *pcb_textop_move_buffer(pcb_opctx_t *ctx, pcb_layer_t *dstly, pcb_text_t *text)
{
	pcb_layer_t *srcly = text->parent.layer;

	assert(text->parent_type == PCB_PARENT_LAYER);
	if ((dstly == NULL) || (dstly == srcly)) /* auto layer in dst */
		dstly = &ctx->buffer.dst->Layer[pcb_layer_id(ctx->buffer.src, srcly)];

	pcb_r_delete_entry(srcly->text_tree, (pcb_box_t *) text);
	pcb_poly_restore_to_poly(ctx->buffer.src, PCB_OBJ_TEXT, srcly, text);

	textlist_remove(text);
	textlist_append(&dstly->Text, text);

	if (!dstly->text_tree)
		dstly->text_tree = pcb_r_create_tree();
	pcb_r_insert_entry(dstly->text_tree, (pcb_box_t *) text);
	pcb_poly_clear_from_poly(ctx->buffer.dst, PCB_OBJ_TEXT, dstly, text);

	PCB_SET_PARENT(text, layer, dstly);

	return text;
}

/* changes the scaling factor of a text object */
void *pcb_textop_change_size(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	int value = ctx->chgsize.is_absolute ? PCB_COORD_TO_MIL(ctx->chgsize.value)
		: Text->Scale + PCB_COORD_TO_MIL(ctx->chgsize.value);

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Text))
		return NULL;
	if (value <= PCB_MAX_TEXTSCALE && value >= PCB_MIN_TEXTSCALE && value != Text->Scale) {
		pcb_undo_add_obj_to_size(PCB_OBJ_TEXT, Layer, Text, Text);
		pcb_text_invalidate_erase(Layer, Text);
		pcb_r_delete_entry(Layer->text_tree, (pcb_box_t *) Text);
		pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
		Text->Scale = value;
		pcb_text_bbox(pcb_font(PCB, Text->fid, 1), Text);
		pcb_r_insert_entry(Layer->text_tree, (pcb_box_t *) Text);
		pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
		pcb_text_invalidate_draw(Layer, Text);
		return Text;
	}
	return NULL;
}

/* sets data of a text object and calculates bounding box; memory must have
   already been allocated the one for the new string is allocated */
void *pcb_textop_change_name(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	char *old = Text->TextString;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Text))
		return NULL;
	pcb_text_invalidate_erase(Layer, Text);
	pcb_r_delete_entry(Layer->text_tree, (pcb_box_t *)Text);
	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	Text->TextString = ctx->chgname.new_name;

	/* calculate size of the bounding box */
	pcb_text_bbox(pcb_font(PCB, Text->fid, 1), Text);
	pcb_r_insert_entry(Layer->text_tree, (pcb_box_t *) Text);
	pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	pcb_text_invalidate_draw(Layer, Text);
	return old;
}

/* changes the clearance flag of a text */
void *pcb_textop_change_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Text))
		return NULL;
	pcb_text_invalidate_erase(Layer, Text);
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Text)) {
		pcb_undo_add_obj_to_clear_poly(PCB_OBJ_TEXT, Layer, Text, Text, pcb_false);
		pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	}
	pcb_undo_add_obj_to_flag(Text);
	PCB_FLAG_TOGGLE(PCB_FLAG_CLEARLINE, Text);
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Text)) {
		pcb_undo_add_obj_to_clear_poly(PCB_OBJ_TEXT, Layer, Text, Text, pcb_true);
		pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	}
	pcb_text_invalidate_draw(Layer, Text);
	return Text;
}

/* sets the clearance flag of a text */
void *pcb_textop_set_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Text) || PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Text))
		return NULL;
	return pcb_textop_change_join(ctx, Layer, Text);
}

/* clears the clearance flag of a text */
void *pcb_textop_clear_join(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Text) || !PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Text))
		return NULL;
	return pcb_textop_change_join(ctx, Layer, Text);
}

/* copies a text */
void *pcb_textop_copy(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	pcb_text_t *text;

	text = pcb_text_new(Layer, pcb_font(PCB, Text->fid, 1), Text->X + ctx->copy.DeltaX,
											 Text->Y + ctx->copy.DeltaY, Text->Direction, Text->Scale, Text->TextString, pcb_flag_mask(Text->Flags, PCB_FLAG_FOUND));
	pcb_text_copy_meta(text, Text);
	pcb_text_invalidate_draw(Layer, text);
	pcb_undo_add_obj_to_create(PCB_OBJ_TEXT, Layer, text, text);
	return text;
}

/* moves a text object */
void *pcb_textop_move_noclip(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	if (Layer->meta.real.vis) {
		pcb_text_invalidate_erase(Layer, Text);
		pcb_text_move(Text, ctx->move.dx, ctx->move.dy);
		pcb_text_invalidate_draw(Layer, Text);
	}
	else
		pcb_text_move(Text, ctx->move.dx, ctx->move.dy);
	return Text;
}

void *pcb_textop_move(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	pcb_r_delete_entry(Layer->text_tree, (pcb_box_t *) Text);
	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	pcb_textop_move_noclip(ctx, Layer, Text);
	pcb_r_insert_entry(Layer->text_tree, (pcb_box_t *) Text);
	pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	return Text;
}

void *pcb_textop_clip(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	if (ctx->clip.restore) {
		pcb_r_delete_entry(Layer->text_tree, (pcb_box_t *) Text);
		pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	}
	if (ctx->clip.clear) {
		pcb_r_insert_entry(Layer->text_tree, (pcb_box_t *) Text);
		pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	}
	return Text;
}


/* moves a text object between layers; lowlevel routines */
void *pcb_textop_move_to_layer_low(pcb_opctx_t *ctx, pcb_layer_t * Source, pcb_text_t * text, pcb_layer_t * Destination)
{
	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, Source, text);
	pcb_r_delete_entry(Source->text_tree, (pcb_box_t *) text);

	textlist_remove(text);
	textlist_append(&Destination->Text, text);

	if (pcb_layer_flags_(Destination) & PCB_LYT_BOTTOM)
		PCB_FLAG_SET(PCB_FLAG_ONSOLDER, text); /* get the text mirrored on display */
	else
		PCB_FLAG_CLEAR(PCB_FLAG_ONSOLDER, text);

	/* re-calculate the bounding box (it could be mirrored now) */
	pcb_text_bbox(pcb_font(PCB, text->fid, 1), text);
	if (!Destination->text_tree)
		Destination->text_tree = pcb_r_create_tree();
	pcb_r_insert_entry(Destination->text_tree, (pcb_box_t *) text);
	pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_TEXT, Destination, text);

	PCB_SET_PARENT(text, layer, Destination);

	return text;
}

/* moves a text object between layers */
void *pcb_textop_move_to_layer(pcb_opctx_t *ctx, pcb_layer_t * layer, pcb_text_t * text)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, text)) {
		pcb_message(PCB_MSG_WARNING, _("Sorry, the object is locked\n"));
		return NULL;
	}
	if (ctx->move.dst_layer != layer) {
		pcb_undo_add_obj_to_move_to_layer(PCB_OBJ_TEXT, layer, text, text);
		if (layer->meta.real.vis)
			pcb_text_invalidate_erase(layer, text);
		text = pcb_textop_move_to_layer_low(ctx, layer, text, ctx->move.dst_layer);
		if (ctx->move.dst_layer->meta.real.vis)
			pcb_text_invalidate_draw(ctx->move.dst_layer, text);
	}
	return text;
}

/* destroys a text from a layer */
void *pcb_textop_destroy(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	free(Text->TextString);
	pcb_r_delete_entry(Layer->text_tree, (pcb_box_t *) Text);

	pcb_text_free(Text);

	return NULL;
}

/* removes a text from a layer */
void *pcb_textop_remove(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	/* erase from screen */
	if (Layer->meta.real.vis) {
		pcb_text_invalidate_erase(Layer, Text);
		pcb_r_delete_entry(Layer->text_tree, (pcb_box_t *)Text);
	}
	pcb_undo_move_obj_to_remove(PCB_OBJ_TEXT, Layer, Text, Text);
	return NULL;
}

void *pcb_text_destroy(pcb_layer_t *Layer, pcb_text_t *Text)
{
	void *res;
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.destroy_target = NULL;

	res = pcb_textop_remove(&ctx, Layer, Text);
	pcb_draw();
	return res;
}

/* rotates a text in 90 degree steps; only the bounding box is rotated,
   text rotation itself is done by the drawing routines */
void pcb_text_rotate90(pcb_text_t *Text, pcb_coord_t X, pcb_coord_t Y, unsigned Number)
{
	pcb_uint8_t number;

	number = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, Text) ? (4 - Number) & 3 : Number;
	PCB_COORD_ROTATE90(Text->X, Text->Y, X, Y, Number);

	/* set new direction, 0..3,
	 * 0-> to the right, 1-> straight up,
	 * 2-> to the left, 3-> straight down
	 */
	Text->Direction = ((Text->Direction + number) & 0x03);

	/* can't optimize with box rotation because of closed boxes */
	pcb_text_bbox(pcb_font(PCB, Text->fid, 1), Text);
}

/* rotates a text object and redraws it */
void *pcb_textop_rotate90(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	pcb_text_invalidate_erase(Layer, Text);
	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	if (Layer->text_tree != NULL)
		pcb_r_delete_entry(Layer->text_tree, (pcb_box_t *) Text);
	pcb_text_rotate90(Text, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
	if (Layer->text_tree != NULL)
		pcb_r_insert_entry(Layer->text_tree, (pcb_box_t *) Text);
	pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	pcb_text_invalidate_draw(Layer, Text);
	return Text;
}

void *pcb_textop_rotate(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	int steps;
	pcb_text_invalidate_erase(Layer, Text);
	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	if (Layer->text_tree != NULL)
		pcb_r_delete_entry(Layer->text_tree, (pcb_box_t *) Text);

	steps = (int)ctx->rotate.angle / 90;
	if (steps > 0)
		Text->Direction = ((Text->Direction + steps) & 0x03);

	pcb_rotate(&Text->X, &Text->Y, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.cosa, ctx->rotate.sina);
	pcb_text_bbox(NULL, Text);

	if (Layer->text_tree != NULL)
		pcb_r_insert_entry(Layer->text_tree, (pcb_box_t *) Text);
	pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	pcb_text_invalidate_draw(Layer, Text);
	return Text;
}

void pcb_text_flip_side(pcb_layer_t *layer, pcb_text_t *text, pcb_coord_t y_offs)
{
	if (layer->text_tree != NULL)
		pcb_r_delete_entry(layer->text_tree, (pcb_box_t *) text);
	text->X = PCB_SWAP_X(text->X);
	text->Y = PCB_SWAP_Y(text->Y) + y_offs;
	PCB_FLAG_TOGGLE(PCB_FLAG_ONSOLDER, text);
	pcb_text_bbox(pcb_font(PCB, text->fid, 1), text);
	if (layer->text_tree != NULL)
		pcb_r_insert_entry(layer->text_tree, (pcb_box_t *) text);
}

void pcb_text_mirror_coords(pcb_layer_t *layer, pcb_text_t *text, pcb_coord_t y_offs)
{
	if (layer->text_tree != NULL)
		pcb_r_delete_entry(layer->text_tree, (pcb_box_t *) text);
	text->X = PCB_SWAP_X(text->X);
	text->Y = PCB_SWAP_Y(text->Y) + y_offs;
	pcb_text_bbox(pcb_font(PCB, text->fid, 1), text);
	if (layer->text_tree != NULL)
		pcb_r_insert_entry(layer->text_tree, (pcb_box_t *) text);
}

void pcb_text_set_font(pcb_layer_t *layer, pcb_text_t *text, pcb_font_id_t fid)
{
	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, layer, text);
	pcb_r_delete_entry(layer->text_tree, (pcb_box_t *) text);
	text->fid = fid;
	pcb_text_bbox(pcb_font(PCB, text->fid, 1), text);
	pcb_r_insert_entry(layer->text_tree, (pcb_box_t *) text);
	pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_TEXT, layer, text);
}

void pcb_text_update(pcb_layer_t *layer, pcb_text_t *text)
{
	pcb_data_t *data = layer->parent.data;
	pcb_board_t *pcb = pcb_data_get_top(data);

	if (pcb == NULL)
		return;

	pcb_poly_restore_to_poly(data, PCB_OBJ_TEXT, layer, text);
	pcb_r_delete_entry(layer->text_tree, (pcb_box_t *) text);
	pcb_text_bbox(pcb_font(pcb, text->fid, 1), text);
	pcb_r_insert_entry(layer->text_tree, (pcb_box_t *) text);
	pcb_poly_clear_from_poly(data, PCB_OBJ_TEXT, layer, text);
}

void *pcb_textop_change_flag(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	static pcb_flag_values_t pcb_text_flags = 0;
	if (pcb_text_flags == 0)
		pcb_text_flags = pcb_obj_valid_flags(PCB_OBJ_TEXT);

	if ((ctx->chgflag.flag & pcb_text_flags) != ctx->chgflag.flag)
		return NULL;
	if ((ctx->chgflag.flag & PCB_FLAG_TERMNAME) && (Text->term == NULL))
		return NULL;
	pcb_undo_add_obj_to_flag(Text);

	if (ctx->chgflag.flag & PCB_FLAG_CLEARLINE)
		pcb_poly_restore_to_poly(ctx->chgflag.pcb->Data, PCB_OBJ_TEXT, Text->parent.layer, Text);

	PCB_FLAG_CHANGE(ctx->chgflag.how, ctx->chgflag.flag, Text);

	if (ctx->chgflag.flag & PCB_FLAG_CLEARLINE)
		pcb_poly_clear_from_poly(ctx->chgflag.pcb->Data, PCB_OBJ_TEXT, Text->parent.layer, Text);

	return Text;
}

void *pcb_textop_invalidate_label(pcb_opctx_t *ctx, pcb_layer_t *layer, pcb_text_t *text)
{
	pcb_text_name_invalidate_draw(text);
	return text;
}

void pcb_text_dyn_bbox_update(pcb_data_t *data)
{
	PCB_TEXT_ALL_LOOP(data); {
		if (PCB_FLAG_TEST(PCB_FLAG_DYNTEXT, text))
			pcb_text_update(layer, text);
	} PCB_ENDALL_LOOP;
}

/*** draw ***/

#define MAX_SIMPLE_POLY_POINTS 256
static void draw_text_poly(pcb_poly_t *poly, pcb_coord_t tx0, pcb_coord_t ty0, pcb_coord_t x0, int xordraw, pcb_coord_t xordx, pcb_coord_t xordy, int scale, int direction, int mirror)
{
	pcb_coord_t x[MAX_SIMPLE_POLY_POINTS], y[MAX_SIMPLE_POLY_POINTS];
	int max, n;
	pcb_point_t *p;


	max = poly->PointN;
	if (max > MAX_SIMPLE_POLY_POINTS) {
		max = MAX_SIMPLE_POLY_POINTS;
	}

	/* transform each coordinate */
	for(n = 0, p = poly->Points; n < max; n++,p++) {
		x[n] = PCB_SCALE_TEXT(p->X + x0, scale);
		y[n] = PCB_SCALE_TEXT(p->Y, scale);
		PCB_COORD_ROTATE90(x[n], y[n], 0, 0, direction);

		if (mirror) {
			x[n] = PCB_SWAP_SIGN_X(x[n]);
			y[n] = PCB_SWAP_SIGN_Y(y[n]);
		}

		x[n] += tx0;
		y[n] += ty0;

		if (xordraw && (n > 0))
			pcb_gui->draw_line(pcb_crosshair.GC, xordx + x[n-1], xordy + y[n-1], xordx + x[n], xordy + y[n]);
		
	}

	if (!xordraw)
		pcb_gui->fill_polygon(pcb_draw_out.fgGC, poly->PointN, x, y);
}

/* Very rough estimation on the full width of the text */
pcb_coord_t pcb_text_width(pcb_font_t *font, int scale, const unsigned char *string)
{
	pcb_coord_t w = 0;
	const pcb_box_t *defaultsymbol;
	if (string == NULL)
		return 0;
	defaultsymbol = &font->DefaultSymbol;
	while(*string) {
		/* draw lines if symbol is valid and data is present */
		if (*string <= PCB_MAX_FONTPOSITION && font->Symbol[*string].Valid)
			w += (font->Symbol[*string].Width + font->Symbol[*string].Delta);
		else
			w += (defaultsymbol->X2 - defaultsymbol->X1) * 6 / 5;
		string++;
	}
	return PCB_SCALE_TEXT(w, scale);
}

pcb_coord_t pcb_text_height(pcb_font_t *font, int scale, const unsigned char *string)
{
	pcb_coord_t h;
	if (string == NULL)
		return 0;
	h = font->MaxHeight;
	while(*string != '\0') {
		if (*string == '\n')
			h += font->MaxHeight;
		string++;
	}
	return PCB_SCALE_TEXT(h, scale);
}


PCB_INLINE void cheap_text_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, pcb_coord_t xordx, pcb_coord_t xordy, int direction, int mirror)
{
	PCB_COORD_ROTATE90(x1, y1, 0, 0, direction);
	PCB_COORD_ROTATE90(x2, y2, 0, 0, direction);

	if (mirror) {
		x1 = PCB_SWAP_SIGN_X(x1);
		y1 = PCB_SWAP_SIGN_Y(y1);
		x2 = PCB_SWAP_SIGN_X(x2);
		y2 = PCB_SWAP_SIGN_Y(y2);
	}

	x1 += xordx;
	y1 += xordy;
	x2 += xordx;
	y2 += xordy;
	pcb_gui->draw_line(gc, x1, y1, x2, y2);
}


PCB_INLINE int draw_text_cheap(pcb_font_t *font, pcb_coord_t x0, pcb_coord_t y0, int scale, int direction, int mirror, const unsigned char *string, int xordraw, pcb_coord_t xordx, pcb_coord_t xordy, pcb_text_tiny_t tiny)
{
	pcb_coord_t w, h = PCB_SCALE_TEXT(font->MaxHeight, scale);
	if (tiny == PCB_TXT_TINY_HIDE) {
		if (h <= pcb_gui->coord_per_pix*6) /* <= 6 pixel high: unreadable */
			return 1;
	}
	else if (tiny == PCB_TXT_TINY_CHEAP) {
		if (h <= pcb_gui->coord_per_pix*2) { /* <= 1 pixel high: draw a single line in the middle */
			w = pcb_text_width(font, scale, string);
			if (xordraw) {
				cheap_text_line(pcb_crosshair.GC, 0, h/2, w, h/2, xordx + x0, xordy + y0, direction, mirror);
			}
			else {
				pcb_hid_set_line_width(pcb_draw_out.fgGC, -1);
				pcb_hid_set_line_cap(pcb_draw_out.fgGC, pcb_cap_square);
				cheap_text_line(pcb_draw_out.fgGC, 0, h/2, w, h/2, x0, y0, direction, mirror);
			}
			return 1;
		}
		else if (h <= pcb_gui->coord_per_pix*4) { /* <= 4 pixel high: draw a mirrored Z-shape */
			w = pcb_text_width(font, scale, string);
			if (xordraw) {
				h /= 4;
				cheap_text_line(pcb_crosshair.GC, 0, h,   w, h,   xordx + x0, xordy + y0, direction, mirror);
				cheap_text_line(pcb_crosshair.GC, 0, h,   w, h*3, xordx + x0, xordy + y0, direction, mirror);
				cheap_text_line(pcb_crosshair.GC, 0, h*3, w, h*3, xordx + x0, xordy + y0, direction, mirror);
			}
			else {
				h /= 4;
				pcb_hid_set_line_width(pcb_draw_out.fgGC, -1);
				pcb_hid_set_line_cap(pcb_draw_out.fgGC, pcb_cap_square);
				cheap_text_line(pcb_draw_out.fgGC, 0, h,   w, h,   x0, y0, direction, mirror);
				cheap_text_line(pcb_draw_out.fgGC, 0, h,   w, h*3, x0, y0, direction, mirror);
				cheap_text_line(pcb_draw_out.fgGC, 0, h*3, w, h*3, x0, y0, direction, mirror);
			}
			return 1;
		}
	}
	return 0;
}

static void pcb_text_draw_string_(pcb_font_t *font, const unsigned char *string, pcb_coord_t x0, pcb_coord_t y0, int scale, int direction, int mirror, pcb_coord_t min_line_width, int xordraw, pcb_coord_t xordx, pcb_coord_t xordy, pcb_text_tiny_t tiny)
{
	pcb_coord_t x = 0;
	pcb_cardinal_t n;

	/* cheap draw */
	if (tiny != PCB_TXT_TINY_ACCURATE) {
		if (draw_text_cheap(font, x0, y0, scale, direction, mirror, string, xordraw, xordx, xordy, tiny))
			return;
	}

	/* normal draw */
	while (string && *string) {
		/* draw lines if symbol is valid and data is present */
		if (*string <= PCB_MAX_FONTPOSITION && font->Symbol[*string].Valid) {
			pcb_line_t *line = font->Symbol[*string].Line;
			pcb_line_t newline;
			pcb_poly_t *p;
			pcb_arc_t *a, newarc;

			for (n = font->Symbol[*string].LineN; n; n--, line++) {
				/* create one line, scale, move, rotate and swap it */
				newline = *line;
				newline.Point1.X = PCB_SCALE_TEXT(newline.Point1.X + x, scale);
				newline.Point1.Y = PCB_SCALE_TEXT(newline.Point1.Y, scale);
				newline.Point2.X = PCB_SCALE_TEXT(newline.Point2.X + x, scale);
				newline.Point2.Y = PCB_SCALE_TEXT(newline.Point2.Y, scale);
				newline.Thickness = PCB_SCALE_TEXT(newline.Thickness, scale / 2);
				if (newline.Thickness < min_line_width)
					newline.Thickness = min_line_width;

				pcb_line_rotate90(&newline, 0, 0, direction);

				/* the labels of SMD objects on the bottom
				 * side haven't been swapped yet, only their offset
				 */
				if (mirror) {
					newline.Point1.X = PCB_SWAP_SIGN_X(newline.Point1.X);
					newline.Point1.Y = PCB_SWAP_SIGN_Y(newline.Point1.Y);
					newline.Point2.X = PCB_SWAP_SIGN_X(newline.Point2.X);
					newline.Point2.Y = PCB_SWAP_SIGN_Y(newline.Point2.Y);
				}
				/* add offset and draw line */
				newline.Point1.X += x0;
				newline.Point1.Y += y0;
				newline.Point2.X += x0;
				newline.Point2.Y += y0;
				if (xordraw)
					pcb_gui->draw_line(pcb_crosshair.GC, xordx + newline.Point1.X, xordy + newline.Point1.Y, xordx + newline.Point2.X, xordy + newline.Point2.Y);
				else
					pcb_line_draw_(&newline, 0);
			}

			/* draw the arcs */
			for(a = arclist_first(&font->Symbol[*string].arcs); a != NULL; a = arclist_next(a)) {
				newarc = *a;

				newarc.X = PCB_SCALE_TEXT(newarc.X + x, scale);
				newarc.Y = PCB_SCALE_TEXT(newarc.Y, scale);
				newarc.Height = newarc.Width = PCB_SCALE_TEXT(newarc.Height, scale);
				newarc.Thickness = PCB_SCALE_TEXT(newarc.Thickness, scale / 2);
				if (newarc.Thickness < min_line_width)
					newarc.Thickness = min_line_width;
				pcb_arc_rotate90(&newarc, 0, 0, direction);

				if (mirror) {
					newarc.X = PCB_SWAP_SIGN_X(newarc.X);
					newarc.Y = PCB_SWAP_SIGN_Y(newarc.Y);
					newarc.StartAngle = PCB_SWAP_ANGLE(newarc.StartAngle);
					newarc.Delta = PCB_SWAP_DELTA(newarc.Delta);
				}
				newarc.X += x0;
				newarc.Y += y0;
				if (xordraw)
					pcb_gui->draw_arc(pcb_crosshair.GC, xordx + newarc.X, xordy + newarc.Y, newarc.Width, newarc.Height, newarc.StartAngle, newarc.Delta);
				else
					pcb_arc_draw_(&newarc, 0);
			}

			/* draw the polygons */
			for(p = polylist_first(&font->Symbol[*string].polys); p != NULL; p = polylist_next(p))
				draw_text_poly(p, x0, y0, x, xordraw, xordx, xordy, scale, direction, mirror);

			/* move on to next cursor position */
			x += (font->Symbol[*string].Width + font->Symbol[*string].Delta);
		}
		else {
			/* the default symbol is a filled box */
			pcb_box_t defaultsymbol = font->DefaultSymbol;
			pcb_coord_t size = (defaultsymbol.X2 - defaultsymbol.X1) * 6 / 5;

			defaultsymbol.X1 = PCB_SCALE_TEXT(defaultsymbol.X1 + x, scale);
			defaultsymbol.Y1 = PCB_SCALE_TEXT(defaultsymbol.Y1, scale);
			defaultsymbol.X2 = PCB_SCALE_TEXT(defaultsymbol.X2 + x, scale);
			defaultsymbol.Y2 = PCB_SCALE_TEXT(defaultsymbol.Y2, scale);

			pcb_box_rotate90(&defaultsymbol, 0, 0, direction);

			/* add offset and draw box */
			defaultsymbol.X1 += x0;
			defaultsymbol.Y1 += y0;
			defaultsymbol.X2 += x0;
			defaultsymbol.Y2 += y0;
			if (xordraw)
				pcb_gui->draw_rect(pcb_crosshair.GC, xordx+defaultsymbol.X1, xordy+defaultsymbol.Y1, xordx+defaultsymbol.X2, xordy+defaultsymbol.Y2);
			else
				pcb_gui->fill_rect(pcb_draw_out.fgGC, defaultsymbol.X1, defaultsymbol.Y1, defaultsymbol.X2, defaultsymbol.Y2);

			/* move on to next cursor position */
			x += size;
		}
		string++;
	}
}

void pcb_text_draw_string(pcb_font_t *font, const unsigned char *string, pcb_coord_t x0, pcb_coord_t y0, int scale, int direction, int mirror, pcb_coord_t min_line_width, int xordraw, pcb_coord_t xordx, pcb_coord_t xordy, pcb_text_tiny_t tiny)
{
	pcb_text_draw_string_(font, string, x0, y0, scale, direction, mirror, min_line_width, xordraw, xordx, xordy, tiny);
}

/* lowlevel drawing routine for text objects */
static void DrawTextLowLevel_(pcb_text_t *Text, pcb_coord_t min_line_width, int xordraw, pcb_coord_t xordx, pcb_coord_t xordy, pcb_text_tiny_t tiny)
{
	unsigned char *rendered = pcb_text_render_str(Text);
	pcb_text_draw_string_(pcb_font(PCB, Text->fid, 1), rendered, Text->X, Text->Y, Text->Scale, Text->Direction, PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, Text), min_line_width, xordraw, xordx, xordy, tiny);
	pcb_text_free_str(Text, rendered);
}

static pcb_bool is_text_term_vert(const pcb_text_t *text)
{
	pcb_coord_t dx, dy;

	dx = text->BoundingBox.X2 - text->BoundingBox.X1;
	if (dx < 0)
		dx = -dx;

	dy = text->BoundingBox.Y2 - text->BoundingBox.Y1;
	if (dy < 0)
		dy = -dy;

	return dx < dy;
}


void pcb_text_name_invalidate_draw(pcb_text_t *text)
{
	if (text->term != NULL)
		pcb_term_label_invalidate((text->BoundingBox.X1 + text->BoundingBox.X2)/2, (text->BoundingBox.Y1 + text->BoundingBox.Y2)/2,
			100.0, is_text_term_vert(text), pcb_true, text->term, text->intconn);
}

void pcb_text_draw_label(pcb_text_t *text)
{
	if (text->term != NULL)
		pcb_term_label_draw((text->BoundingBox.X1 + text->BoundingBox.X2)/2, (text->BoundingBox.Y1 + text->BoundingBox.Y2)/2,
			100.0, is_text_term_vert(text), pcb_true, text->term, text->intconn);
}


void pcb_text_draw_(pcb_text_t *text, pcb_coord_t min_line_width, int allow_term_gfx, pcb_text_tiny_t tiny)
{
	DrawTextLowLevel_(text, min_line_width, 0, 0, 0, tiny);

	if (text->term != NULL) {
		if ((allow_term_gfx) && ((pcb_draw_doing_pinout) || PCB_FLAG_TEST(PCB_FLAG_TERMNAME, text)))
			pcb_draw_delay_label_add((pcb_any_obj_t *)text);
	}
}

static void pcb_text_draw(pcb_layer_t *layer, pcb_text_t *text, int allow_term_gfx)
{
	int min_silk_line;
	unsigned int flg = 0;

	if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, text)) {
		if (layer->is_bound) {
			const char *color;
			PCB_OBJ_COLOR_ON_BOUND_LAYER(color, layer, 1);
			pcb_gui->set_color(pcb_draw_out.fgGC, color);
		}
		else
			pcb_gui->set_color(pcb_draw_out.fgGC, layer->meta.real.selected_color);
	}
	else if (PCB_HAS_COLOROVERRIDE(text)) {
		pcb_gui->set_color(pcb_draw_out.fgGC, text->override_color);
	}
	else if (layer->is_bound) {
		const char *color;
		PCB_OBJ_COLOR_ON_BOUND_LAYER(color, layer, 0);
		pcb_gui->set_color(pcb_draw_out.fgGC, color);
	}
	else
		pcb_gui->set_color(pcb_draw_out.fgGC, layer->meta.real.color);

	if ((!layer->is_bound) && (layer->meta.real.grp >= 0))
		flg = pcb_layergrp_flags(PCB, layer->meta.real.grp);

	if (flg & PCB_LYT_SILK)
		min_silk_line = conf_core.design.min_slk;
	else
		min_silk_line = conf_core.design.min_wid;

	pcb_text_draw_(text, min_silk_line, allow_term_gfx, PCB_TXT_TINY_CHEAP);
}

pcb_r_dir_t pcb_text_draw_callback(const pcb_box_t * b, void *cl)
{
	pcb_layer_t *layer = cl;
	pcb_text_t *text = (pcb_text_t *) b;

	if (pcb_hidden_floater((pcb_any_obj_t*)b))
		return PCB_R_DIR_FOUND_CONTINUE;

	if (!PCB->SubcPartsOn && pcb_lobj_parent_subc(text->parent_type, &text->parent))
		return PCB_R_DIR_FOUND_CONTINUE;

	pcb_text_draw(layer, text, 0);
	return PCB_R_DIR_FOUND_CONTINUE;
}

pcb_r_dir_t pcb_text_draw_term_callback(const pcb_box_t * b, void *cl)
{
	pcb_layer_t *layer = cl;
	pcb_text_t *text = (pcb_text_t *) b;

	if (pcb_hidden_floater((pcb_any_obj_t*)b))
		return PCB_R_DIR_FOUND_CONTINUE;

	if (!PCB->SubcPartsOn && pcb_lobj_parent_subc(text->parent_type, &text->parent))
		return PCB_R_DIR_FOUND_CONTINUE;

	pcb_text_draw(layer, text, 1);
	return PCB_R_DIR_FOUND_CONTINUE;
}

/* erases a text on a layer */
void pcb_text_invalidate_erase(pcb_layer_t *Layer, pcb_text_t *Text)
{
	pcb_draw_invalidate(Text);
}

void pcb_text_invalidate_draw(pcb_layer_t *Layer, pcb_text_t *Text)
{
	pcb_draw_invalidate(Text);
}

void pcb_text_draw_xor(pcb_text_t *text, pcb_coord_t x, pcb_coord_t y)
{
	DrawTextLowLevel_(text, 0, 1, x, y, PCB_TXT_TINY_CHEAP);
	if (conf_core.appearance.text_host_bbox) {
		pcb_gui->draw_line(pcb_crosshair.GC, x + text->BoundingBox.X1, y + text->BoundingBox.Y1, x + text->BoundingBox.X1, y + text->BoundingBox.Y2);
		pcb_gui->draw_line(pcb_crosshair.GC, x + text->BoundingBox.X1, y + text->BoundingBox.Y1, x + text->BoundingBox.X2, y + text->BoundingBox.Y1);
		pcb_gui->draw_line(pcb_crosshair.GC, x + text->BoundingBox.X2, y + text->BoundingBox.Y2, x + text->BoundingBox.X1, y + text->BoundingBox.Y2);
		pcb_gui->draw_line(pcb_crosshair.GC, x + text->BoundingBox.X2, y + text->BoundingBox.Y2, x + text->BoundingBox.X2, y + text->BoundingBox.Y1);
	}
}

/*** init ***/
static const char *text_cookie = "obj_text";

/* Recursively update the text objects of data and subcircuits; returns non-zero
   if a redraw is needed */
static int pcb_text_font_chg_data(pcb_data_t *data, pcb_font_id_t fid)
{
	int need_redraw = 0;

	LAYER_LOOP(data, data->LayerN); {
		PCB_TEXT_LOOP(layer); {
			if (text->fid == fid) {
				pcb_text_update(layer, text);
				need_redraw = 1;
			}
		} PCB_END_LOOP;
	} PCB_END_LOOP;

	PCB_SUBC_LOOP(data); {
		int chg = pcb_text_font_chg_data(subc->data, fid);
		if (chg) {
			need_redraw = 1;
			pcb_r_delete_entry(data->subc_tree, (pcb_box_t *)subc);
			pcb_subc_bbox(subc);
			pcb_r_insert_entry(data->subc_tree, (pcb_box_t *)subc);
		}
	} PCB_END_LOOP;

	return need_redraw;
}

static void pcb_text_font_chg(void *user_data, int argc, pcb_event_arg_t argv[])
{

	if ((argc < 2) || (argv[1].type != PCB_EVARG_INT))
		return;

	if (pcb_text_font_chg_data(PCB->Data, argv[1].d.i))
		pcb_gui->invalidate_all(); /* can't just redraw the text, as the old text may have been bigger, before the change! */

	pcb_trace("font change %d\n", argv[1].d.i);
}

void pcb_text_init(void)
{
	pcb_event_bind(PCB_EVENT_FONT_CHANGED, pcb_text_font_chg, NULL, text_cookie);
}

void pcb_text_uninit(void)
{
	pcb_event_unbind_allcookie(text_cookie);
}
