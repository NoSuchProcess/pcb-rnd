/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2018,2019,2020 Tibor 'Igor2' Palinkas
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
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 */

/* Drawing primitive: text */

#include "config.h"

#include "rotate.h"
#include "board.h"
#include "data.h"
#include <librnd/core/compat_misc.h>
#include <librnd/core/hid_inlines.h>
#include "undo.h"
#include "polygon.h"
#include <librnd/poly/offset.h>
#include "event.h"
#include "layer.h"

#include "obj_text.h"
#include "obj_text_op.h"
#include "obj_text_list.h"
#include "obj_poly_draw.h"
#include "obj_arc_draw.h"

#include "obj_subc_parent.h"
#include "obj_hash.h"

#include "obj_line_draw.h"
#include "obj_text_draw.h"
#include "conf_core.h"

/*** allocation ***/

void pcb_text_reg(pcb_layer_t *layer, pcb_text_t *text)
{
	textlist_append(&layer->Text, text);
	PCB_SET_PARENT(text, layer, layer);

	if (layer->parent_type == PCB_PARENT_UI)
		return;

	if (layer->parent_type == PCB_PARENT_DATA)
		pcb_obj_id_reg(layer->parent.data, text);
}

void pcb_text_unreg(pcb_text_t *text)
{
	pcb_layer_t *layer = text->parent.layer;
	assert(text->parent_type == PCB_PARENT_LAYER);
	textlist_remove(text);
	if (layer->parent_type != PCB_PARENT_UI) {
		assert(layer->parent_type == PCB_PARENT_DATA);
		pcb_obj_id_del(layer->parent.data, text);
	}
	PCB_CLEAR_PARENT(text);
}

pcb_text_t *pcb_text_alloc_id(pcb_layer_t *layer, long int id)
{
	pcb_text_t *new_obj;

	new_obj = calloc(sizeof(pcb_text_t), 1);
	new_obj->ID = id;
	new_obj->type = PCB_OBJ_TEXT;
	new_obj->Attributes.post_change = pcb_obj_attrib_post_change;

	pcb_text_reg(layer, new_obj);

	return new_obj;
}

pcb_text_t *pcb_text_alloc(pcb_layer_t *layer)
{
	return pcb_text_alloc_id(layer, pcb_create_ID_get());
}

void pcb_text_free(pcb_text_t *text)
{
	if ((text->parent.layer != NULL) && (text->parent.layer->text_tree != NULL))
		rnd_r_delete_entry(text->parent.layer->text_tree, (rnd_box_t *)text);
	pcb_attribute_free(&text->Attributes);
	pcb_text_unreg(text);
	free(text->TextString);
	pcb_obj_common_free((pcb_any_obj_t *)text);
	free(text);
}

/*** utility ***/

static const char core_text_cookie[] = "core-text";

typedef struct {
	pcb_text_t *text; /* it is safe to save the object pointer because it is persistent (through the removed object list) */
	int Scale;
	double scale_x, scale_y;
	rnd_coord_t X, Y;
	rnd_coord_t thickness;
	rnd_coord_t clearance;
	double rot;
} undo_text_geo_t;

static int undo_text_geo_swap(void *udata)
{
	undo_text_geo_t *g = udata;
	pcb_layer_t *layer = g->text->parent.layer;
	pcb_board_t *pcb = pcb_data_get_top(layer->parent.data);

	if (layer->text_tree != NULL)
		rnd_r_delete_entry(layer->text_tree, (rnd_box_t *)g->text);
	pcb_poly_restore_to_poly(layer->parent.data, PCB_OBJ_TEXT, layer, g->text);

	rnd_swap(int, g->Scale, g->text->Scale);
	rnd_swap(double, g->scale_x, g->text->scale_x);
	rnd_swap(double, g->scale_y, g->text->scale_y);
	rnd_swap(rnd_coord_t, g->X, g->text->X);
	rnd_swap(rnd_coord_t, g->Y, g->text->Y);
	rnd_swap(rnd_coord_t, g->thickness, g->text->thickness);
	rnd_swap(rnd_coord_t, g->clearance, g->text->clearance);
	rnd_swap(double, g->rot, g->text->rot);

	if (pcb != NULL)
		pcb_text_bbox(pcb_font(pcb, g->text->fid, 1), g->text);
	if (layer->text_tree != NULL)
		rnd_r_insert_entry(layer->text_tree, (rnd_box_t *)g->text);
	pcb_poly_clear_from_poly(layer->parent.data, PCB_OBJ_TEXT, layer, g->text);

	return 0;
}

static void undo_text_geo_print(void *udata, char *dst, size_t dst_len)
{
	rnd_snprintf(dst, dst_len, "text geo");
}

static const uundo_oper_t undo_text_geo = {
	core_text_cookie,
	NULL,
	undo_text_geo_swap,
	undo_text_geo_swap,
	undo_text_geo_print
};


pcb_text_t *pcb_text_new_(pcb_layer_t *Layer, pcb_font_t *PCBFont, rnd_coord_t X, rnd_coord_t Y, double rot, pcb_text_mirror_t mirror, int Scale, double scx, double scy, rnd_coord_t thickness, const char *TextString, pcb_flag_t Flags)
{
	pcb_text_t *text;

	if (TextString == NULL)
		return NULL;

	text = pcb_text_alloc(Layer);
	if (text == NULL)
		return NULL;

	/* Set up mirroring */
	if (mirror & PCB_TXT_MIRROR_Y)
		Flags.f |= PCB_FLAG_ONSOLDER;
	if (mirror & PCB_TXT_MIRROR_X)
		pcb_attribute_put(&text->Attributes, "mirror_x", "1");

	/* copy values, width and height are set by drawing routine
	 * because at this point we don't know which symbols are available
	 */
	text->X = X;
	text->Y = Y;
	text->rot = rot;
	text->Flags = Flags;
	text->Scale = Scale;
	text->scale_x = scx;
	text->scale_y = scy;
	text->thickness = thickness;
	text->TextString = rnd_strdup(TextString);
	text->fid = PCBFont->id;

	return text;
}

/* creates a new text on a layer */
pcb_text_t *pcb_text_new(pcb_layer_t *Layer, pcb_font_t *PCBFont, rnd_coord_t X, rnd_coord_t Y, double rot, int Scale, rnd_coord_t thickness, const char *TextString, pcb_flag_t Flags)
{
	pcb_text_t *text = pcb_text_new_(Layer, PCBFont, X, Y, rot, 0, Scale, 0, 0, thickness, TextString, Flags);

	pcb_add_text_on_layer(Layer, text, PCBFont);

	return text;
}

pcb_text_t *pcb_text_new_scaled(pcb_layer_t *Layer, pcb_font_t *PCBFont, rnd_coord_t X, rnd_coord_t Y, double rot, pcb_text_mirror_t mirror, int Scale, double scx, double scy, rnd_coord_t thickness, const char *TextString, pcb_flag_t Flags)
{
	pcb_text_t *text = pcb_text_new_(Layer, PCBFont, X, Y, rot, mirror, Scale, scx, scy, thickness, TextString, Flags);

	pcb_add_text_on_layer(Layer, text, PCBFont);

	return text;
}

pcb_text_t *pcb_text_new_by_bbox(pcb_layer_t *Layer, pcb_font_t *PCBFont, rnd_coord_t X, rnd_coord_t Y, rnd_coord_t bbw, rnd_coord_t bbh, rnd_coord_t anchx, rnd_coord_t anchy, double scxy, pcb_text_mirror_t mirror, double rot, rnd_coord_t thickness, const char *TextString, pcb_flag_t Flags)
{
	rnd_coord_t obw, obh, nbw, nbh, nanchx, nanchy;
	double gsc, gscx, gscy, cs, sn, mx = 1, my = 1;
	pcb_text_t *t;
	
	t = pcb_text_new_(Layer, PCBFont, 0, 0, 0, mirror, 100, 1, 1, thickness, TextString, Flags);

	if ((bbw <= 0) || (bbh <= 0))
		rnd_message(RND_MSG_ERROR, "internal error in pcb_text_new_by_bbox(): invalid input bbox\n");

	t->scale_x = scxy;
	t->scale_y = 1;
	pcb_text_bbox(PCBFont, t);

	/* determine the final scaling */
	obw = t->bbox_naked.X2 - t->bbox_naked.X1;
	obh = t->bbox_naked.Y2 - t->bbox_naked.Y1;

	/*rnd_trace(" pcb-rnd: %ml %ml req: %ml %ml (%s) sc: %f %f\n", obw, obh, bbw, bbh, TextString, t->scale_x, t->scale_y);*/

	gscx = (double)bbw/(double)obw;
	gscy = (double)bbh/(double)obh;
	gsc = ((gscx > 0) && (gscx < gscy)) ? gscx : gscy;

	if (gsc <= 0) {
		rnd_message(RND_MSG_ERROR, "internal error in pcb_text_new_by_bbox(): invalid text scaling\n");
		gsc = 1;
	}

	t->scale_x *= gsc;
	t->scale_y *= gsc;

	pcb_text_bbox(PCBFont, t);

	/* figure new anchor points (on the new, typically smaller bbox) */
	nbw = t->bbox_naked.X2 - t->bbox_naked.X1;
	nbh = t->bbox_naked.Y2 - t->bbox_naked.Y1;

	nanchx = (double)(anchx) / (double)bbw * (double)nbw;
	nanchy = (double)(anchy) / (double)bbh * (double)nbh;

	/* calculate final placement and enable rotation */
	if (mirror & PCB_TXT_MIRROR_X) mx = -1;
	if (mirror & PCB_TXT_MIRROR_Y) my = -1;

	cs = cos(rot*mx*my / RND_RAD_TO_DEG);
	sn = sin(rot*mx*my / RND_RAD_TO_DEG);

	t->X = X - (nanchx * cs * mx + nanchy * sn * my);
	t->Y = Y - (nanchy * cs * my - nanchx * sn * mx);
	t->rot = rot;

	/*rnd_trace(" final: %ml %ml (%f %f -> %f) got:%f wanted:%f anch: %ml %ml -> %ml %ml\n", t->bbox_naked.X2 - t->bbox_naked.X1, t->bbox_naked.Y2 - t->bbox_naked.Y1, gscx, gscy, gsc, t->scale_x/t->scale_y, scxy, anchx, anchy, nanchx, nanchy);*/

	pcb_add_text_on_layer(Layer, t, PCBFont);
	return t;
}


static pcb_text_t *pcb_text_copy_meta(pcb_text_t *dst, pcb_text_t *src)
{
	if (dst == NULL)
		return NULL;
	pcb_attribute_copy_all(&dst->Attributes, &src->Attributes);
	return dst;
}

#define text_mirror_bits(t) \
	((PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, (t)) ? PCB_TXT_MIRROR_Y : 0) | ((t)->mirror_x ? PCB_TXT_MIRROR_X : 0))

pcb_text_t *pcb_text_dup(pcb_layer_t *dst, pcb_text_t *src)
{
	pcb_text_t *t = pcb_text_new_scaled(dst, pcb_font(PCB, src->fid, 1), src->X, src->Y, src->rot, text_mirror_bits(src), src->Scale, src->scale_x, src->scale_y, src->thickness, src->TextString, src->Flags);
	pcb_text_copy_meta(t, src);
	return t;
}

pcb_text_t *pcb_text_dup_at(pcb_layer_t *dst, pcb_text_t *src, rnd_coord_t dx, rnd_coord_t dy)
{
	pcb_text_t *t = pcb_text_new_scaled(dst, pcb_font(PCB, src->fid, 1), src->X+dx, src->Y+dy, src->rot, text_mirror_bits(src), src->Scale, src->scale_x, src->scale_y, src->thickness, src->TextString, src->Flags);
	pcb_text_copy_meta(t, src);
	return t;
}

void pcb_add_text_on_layer(pcb_layer_t *Layer, pcb_text_t *text, pcb_font_t *PCBFont)
{
	/* calculate size of the bounding box */
	pcb_text_bbox(PCBFont, text);
	if (!Layer->text_tree)
		Layer->text_tree = rnd_r_create_tree();
	rnd_r_insert_entry(Layer->text_tree, (rnd_box_t *) text);
}

static int pcb_text_render_str_cb(void *ctx, gds_t *s, const char **input)
{
	const pcb_text_t *text = ctx;
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
		const pcb_attribute_list_t *attr = &text->Attributes;
		path = key+2;
		if ((path[0] == 'p') && (memcmp(path, "parent.", 7) == 0)) {
			pcb_data_t *par = text->parent.layer->parent.data;

			if (par == NULL)
				attr = NULL;
			else if (par->parent_type == PCB_PARENT_SUBC)
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

	res = (unsigned char *)rnd_strdup_subst(text->TextString, pcb_text_render_str_cb, text, RND_SUBST_PERCENT | RND_SUBST_CONF);
	if (res == NULL) {
		res = (unsigned char *)rnd_strdup("<!>");
	}
	else if (*res == '\0') {
		free(res);
		res = (unsigned char *)rnd_strdup("<?>");
	}

	return res;
}

int pcb_append_dyntext(gds_t *dst, const pcb_any_obj_t *obj, const char *fmt)
{
	return rnd_subst_append(dst, fmt, pcb_text_render_str_cb, (void *)obj, RND_SUBST_PERCENT | RND_SUBST_CONF, 0);
}

/* Free rendered if it was allocated */
static void pcb_text_free_str(pcb_text_t *text, unsigned char *rendered)
{
	if ((unsigned char *)text->TextString != rendered)
		free(rendered);
}

void pcb_text_get_scale_xy(pcb_text_t *text, double *scx, double *scy)
{
	double sc = (double)text->Scale / 100.0;
	*scx = (text->scale_x > 0) ? text->scale_x : sc;
	*scy = (text->scale_y > 0) ? text->scale_y : sc;
}

int pcb_text_old_scale(pcb_text_t *text, int *scale_out)
{
	double scx, scy;

	if ((text->scale_x <= 0) && (text->scale_y <= 0)) { /* old model */
		*scale_out = text->Scale;
		return 0;
	}

	pcb_text_get_scale_xy(text, &scx, &scy);
	if (fabs(scx - scy) < 0.01) { /* new model but scx == scy */
		*scale_out = rnd_round(scx * 100);
		return 0;
	}

	*scale_out = rnd_round((scx + scy) / 2.0 * 100);
	return -1;
}

/* creates the bounding box of a text object */
void pcb_text_bbox(pcb_font_t *FontPtr, pcb_text_t *Text)
{
	pcb_symbol_t *symbol;
	unsigned char *s, *rendered = pcb_text_render_str(Text);
	int i;
	int space;
	rnd_coord_t minx, miny, maxx, maxy, tx;
	rnd_coord_t min_final_radius;
	rnd_coord_t min_unscaled_radius;
	rnd_bool first_time = rnd_true;
	pcb_poly_t *poly;
	double scx, scy;
	pcb_xform_mx_t mx = PCB_XFORM_MX_IDENT;
	rnd_coord_t cx[4], cy[4];
	pcb_text_mirror_t mirror;

	pcb_text_get_scale_xy(Text, &scx, &scy);

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
				rnd_coord_t unscaled_radius = MAX(min_unscaled_radius, line->Thickness / 4);

				if (first_time) {
					minx = maxx = line->Point1.X;
					miny = maxy = line->Point1.Y;
					first_time = rnd_false;
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
				maxx = MAX(maxx, arc->bbox_naked.X2);
				maxy = MAX(maxy, arc->bbox_naked.Y2);
			}

			for(poly = polylist_first(&symbol[*s].polys); poly != NULL; poly = polylist_next(poly)) {
				int n;
				rnd_point_t *pnt;
				for(n = 0, pnt = poly->Points; n < poly->PointN; n++,pnt++) {
					maxx = MAX(maxx, pnt->X + tx);
					maxy = MAX(maxy, pnt->Y);
				}
			}

			space = symbol[*s].Delta;
		}
		else {
			rnd_box_t *ds = &FontPtr->DefaultSymbol;
			rnd_coord_t w = ds->X2 - ds->X1;

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

	mirror = text_mirror_bits(Text);

	/* it is enough to do the transformations only once, on the raw bounding box */
	pcb_xform_mx_translate(mx, Text->X, Text->Y);
	if (mirror != PCB_TXT_MIRROR_NO)
		pcb_xform_mx_scale(mx, (mirror & PCB_TXT_MIRROR_X) ? -1 : 1, (mirror & PCB_TXT_MIRROR_Y) ? -1 : 1);
	pcb_xform_mx_rotate(mx, Text->rot);
	pcb_xform_mx_scale(mx, scx, scy);

	/* calculate the transformed coordinates of all 4 corners of the raw
	   (non-axis-aligned) bounding box */
	cx[0] = rnd_round(pcb_xform_x(mx, minx, miny));
	cy[0] = rnd_round(pcb_xform_y(mx, minx, miny));
	cx[1] = rnd_round(pcb_xform_x(mx, maxx, miny));
	cy[1] = rnd_round(pcb_xform_y(mx, maxx, miny));
	cx[2] = rnd_round(pcb_xform_x(mx, maxx, maxy));
	cy[2] = rnd_round(pcb_xform_y(mx, maxx, maxy));
	cx[3] = rnd_round(pcb_xform_x(mx, minx, maxy));
	cy[3] = rnd_round(pcb_xform_y(mx, minx, maxy));

	/* calculate the axis-aligned version */
	Text->bbox_naked.X1 = Text->bbox_naked.X2 = cx[0];
	Text->bbox_naked.Y1 = Text->bbox_naked.Y2 = cy[0];
	rnd_box_bump_point(&Text->bbox_naked, cx[1], cy[1]);
	rnd_box_bump_point(&Text->bbox_naked, cx[2], cy[2]);
	rnd_box_bump_point(&Text->bbox_naked, cx[3], cy[3]);

	/* the bounding box covers the extent of influence
	 * so it must include the clearance values too
	 */
	Text->BoundingBox.X1 = Text->bbox_naked.X1 - conf_core.design.bloat;
	Text->BoundingBox.Y1 = Text->bbox_naked.Y1 - conf_core.design.bloat;
	Text->BoundingBox.X2 = Text->bbox_naked.X2 + conf_core.design.bloat;
	Text->BoundingBox.Y2 = Text->bbox_naked.Y2 + conf_core.design.bloat;
	rnd_close_box(&Text->bbox_naked);
	rnd_close_box(&Text->BoundingBox);
	pcb_text_free_str(Text, rendered);
}

int pcb_text_eq(const pcb_host_trans_t *tr1, const pcb_text_t *t1, const pcb_host_trans_t *tr2, const pcb_text_t *t2)
{
	double rotdir1 = tr1->on_bottom ? -1.0 : 1.0, rotdir2 = tr2->on_bottom ? -1.0 : 1.0;

	if (pcb_neqs(t1->TextString, t2->TextString)) return 0;
	if (pcb_neqs(t1->term, t2->term)) return 0;
	if (pcb_field_neq(t1, t2, thickness)) return 0;
	if (pcb_field_neq(t1, t2, fid)) return 0;

	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, t1) && !PCB_FLAG_TEST(PCB_FLAG_FLOATER, t2)) {
		if (pcb_field_neq(t1, t2, Scale)) return 0;
		if (pcb_neq_tr_coords(tr1, t1->X, t1->Y, tr2, t2->X, t2->Y)) return 0;
		if (floor(fmod((t1->rot * rotdir1) + tr1->rot, 360.0)*10000) != floor(fmod((t2->rot * rotdir2) + tr2->rot, 360.0)*10000)) return 0;
	}

	return 1;
}

unsigned int pcb_text_hash(const pcb_host_trans_t *tr, const pcb_text_t *t)
{
	unsigned int crd = 0;

	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, t)) {
		rnd_coord_t x, y;
		double rotdir = tr->on_bottom ? -1.0 : 1.0;

		pcb_hash_tr_coords(tr, &x, &y, t->X, t->Y);
		crd = pcb_hash_coord(x) ^ pcb_hash_coord(y) ^ pcb_hash_coord(t->Scale) \
			^ pcb_hash_scale(t->scale_x) ^ pcb_hash_scale(t->scale_y) \
			^ pcb_hash_angle(tr, t->rot * rotdir);
	}

	return pcb_hash_str(t->TextString) ^ pcb_hash_str(t->term) ^
		pcb_hash_coord(t->thickness) ^ (unsigned)t->fid ^ crd;
}


/*** ops ***/
/* copies a text to buffer */
void *pcb_textop_add_to_buffer(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	pcb_layer_t *layer = &ctx->buffer.dst->Layer[pcb_layer_id(ctx->buffer.src, Layer)];
	pcb_text_t *t = pcb_text_new_scaled(layer, pcb_font(PCB, Text->fid, 1), Text->X, Text->Y, Text->rot, text_mirror_bits(Text), Text->Scale, Text->scale_x, Text->scale_y, Text->thickness, Text->TextString, pcb_flag_mask(Text->Flags, ctx->buffer.extraflg));

	pcb_text_copy_meta(t, Text);
	if (ctx->buffer.keep_id) pcb_obj_change_id((pcb_any_obj_t *)t, Text->ID);
	return t;
}

/* moves a text without allocating memory for the name between board and buffer */
void *pcb_textop_move_buffer(pcb_opctx_t *ctx, pcb_layer_t *dstly, pcb_text_t *text)
{
	pcb_layer_t *srcly = text->parent.layer;

	assert(text->parent_type == PCB_PARENT_LAYER);
	if ((dstly == NULL) || (dstly == srcly)) { /* auto layer in dst */
		rnd_layer_id_t lid = pcb_layer_id(ctx->buffer.src, srcly);
		if (lid < 0) {
			rnd_message(RND_MSG_ERROR, "Internal error: can't resolve source layer ID in pcb_textop_move_buffer\n");
			return NULL;
		}
		dstly = &ctx->buffer.dst->Layer[lid];
	}

	rnd_r_delete_entry(srcly->text_tree, (rnd_box_t *) text);
	pcb_poly_restore_to_poly(ctx->buffer.src, PCB_OBJ_TEXT, srcly, text);

	pcb_text_unreg(text);
	pcb_text_reg(dstly, text);

	if (!dstly->text_tree)
		dstly->text_tree = rnd_r_create_tree();
	rnd_r_insert_entry(dstly->text_tree, (rnd_box_t *) text);
	pcb_poly_clear_from_poly(ctx->buffer.dst, PCB_OBJ_TEXT, dstly, text);

	return text;
}

/* changes the scaling factor of a text object */
void *pcb_textop_change_size(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	int value = ctx->chgsize.is_absolute ? RND_COORD_TO_MIL(ctx->chgsize.value)
		: Text->Scale + RND_COORD_TO_MIL(ctx->chgsize.value);

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Text))
		return NULL;
	if (value <= PCB_MAX_TEXTSCALE && value >= PCB_MIN_TEXTSCALE && value != Text->Scale) {
		pcb_undo_add_obj_to_size(PCB_OBJ_TEXT, Layer, Text, Text);
		pcb_text_invalidate_erase(Layer, Text);
		rnd_r_delete_entry(Layer->text_tree, (rnd_box_t *) Text);
		pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
		Text->Scale = value;
		if (Text->scale_x > 0)
			Text->scale_x = (double)value / 100.0;
		if (Text->scale_y > 0)
			Text->scale_y = (double)value / 100.0;
		pcb_text_bbox(pcb_font(PCB, Text->fid, 1), Text);
		rnd_r_insert_entry(Layer->text_tree, (rnd_box_t *) Text);
		pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
		pcb_text_invalidate_draw(Layer, Text);
		return Text;
	}
	return NULL;
}

/* changes the thickness of a text object */
void *pcb_textop_change_2nd_size(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	int value = ctx->chgsize.is_absolute ? ctx->chgsize.value : Text->thickness + ctx->chgsize.value;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Text))
		return NULL;
	if (value != Text->thickness) {
		pcb_undo_add_obj_to_2nd_size(PCB_OBJ_TEXT, Layer, Text, Text);
		pcb_text_invalidate_erase(Layer, Text);
		rnd_r_delete_entry(Layer->text_tree, (rnd_box_t *) Text);
		pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
		Text->thickness = value;
		pcb_text_bbox(pcb_font(PCB, Text->fid, 1), Text);
		rnd_r_insert_entry(Layer->text_tree, (rnd_box_t *) Text);
		pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
		pcb_text_invalidate_draw(Layer, Text);
		return Text;
	}
	return NULL;
}

/* changes the rotation of a text object */
void *pcb_textop_change_rot(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	int value = ctx->chgsize.is_absolute ? ctx->chgsize.value : Text->rot + ctx->chgsize.value;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, Text))
		return NULL;
	if (value != Text->rot) {
		pcb_undo_add_obj_to_rot(PCB_OBJ_TEXT, Layer, Text, Text);
		pcb_text_invalidate_erase(Layer, Text);
		rnd_r_delete_entry(Layer->text_tree, (rnd_box_t *) Text);
		pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
		Text->rot = value;
		pcb_text_bbox(pcb_font(PCB, Text->fid, 1), Text);
		rnd_r_insert_entry(Layer->text_tree, (rnd_box_t *) Text);
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
	rnd_r_delete_entry(Layer->text_tree, (rnd_box_t *)Text);
	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	Text->TextString = ctx->chgname.new_name;

	/* calculate size of the bounding box */
	pcb_text_bbox(pcb_font(PCB, Text->fid, 1), Text);
	rnd_r_insert_entry(Layer->text_tree, (rnd_box_t *) Text);
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
		pcb_undo_add_obj_to_clear_poly(PCB_OBJ_TEXT, Layer, Text, Text, rnd_false);
		pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	}
	pcb_undo_add_obj_to_flag(Text);
	PCB_FLAG_TOGGLE(PCB_FLAG_CLEARLINE, Text);
	if (PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, Text)) {
		pcb_undo_add_obj_to_clear_poly(PCB_OBJ_TEXT, Layer, Text, Text, rnd_true);
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

	text = pcb_text_new_scaled(Layer, pcb_font(PCB, Text->fid, 1), Text->X + ctx->copy.DeltaX,
											 Text->Y + ctx->copy.DeltaY, Text->rot, text_mirror_bits(Text), Text->Scale, Text->scale_x, Text->scale_y, Text->thickness, Text->TextString, pcb_flag_mask(Text->Flags, PCB_FLAG_FOUND));
	if (ctx->copy.keep_id)
		text->ID = Text->ID;
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
	rnd_r_delete_entry(Layer->text_tree, (rnd_box_t *) Text);
	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	pcb_textop_move_noclip(ctx, Layer, Text);
	rnd_r_insert_entry(Layer->text_tree, (rnd_box_t *) Text);
	pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	return Text;
}

void *pcb_textop_clip(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	if (ctx->clip.restore) {
		rnd_r_delete_entry(Layer->text_tree, (rnd_box_t *) Text);
		pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	}
	if (ctx->clip.clear) {
		rnd_r_insert_entry(Layer->text_tree, (rnd_box_t *) Text);
		pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	}
	return Text;
}


/* moves a text object between layers; lowlevel routines */
void *pcb_textop_move_to_layer_low(pcb_opctx_t *ctx, pcb_layer_t * Source, pcb_text_t * text, pcb_layer_t * Destination)
{
	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, Source, text);
	rnd_r_delete_entry(Source->text_tree, (rnd_box_t *) text);

	pcb_text_unreg(text);
	pcb_text_reg(Destination, text);

	if (pcb_layer_flags_(Destination) & PCB_LYT_BOTTOM)
		PCB_FLAG_SET(PCB_FLAG_ONSOLDER, text); /* get the text mirrored on display */
	else
		PCB_FLAG_CLEAR(PCB_FLAG_ONSOLDER, text);

	/* re-calculate the bounding box (it could be mirrored now) */
	pcb_text_bbox(pcb_font(PCB, text->fid, 1), text);
	if (!Destination->text_tree)
		Destination->text_tree = rnd_r_create_tree();
	rnd_r_insert_entry(Destination->text_tree, (rnd_box_t *) text);
	pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_TEXT, Destination, text);

	return text;
}

/* moves a text object between layers */
void *pcb_textop_move_to_layer(pcb_opctx_t *ctx, pcb_layer_t * layer, pcb_text_t * text)
{
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, text)) {
		rnd_message(RND_MSG_WARNING, "Sorry, text object is locked\n");
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
	pcb_text_free(Text);
	return NULL;
}

/* removes a text from a layer */
void *pcb_textop_remove(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	/* erase from screen */
	if (Layer->meta.real.vis) {
		pcb_text_invalidate_erase(Layer, Text);
		rnd_r_delete_entry(Layer->text_tree, (rnd_box_t *)Text);
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
void pcb_text_rotate90(pcb_text_t *Text, rnd_coord_t X, rnd_coord_t Y, unsigned Number)
{
	int number = PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, Text) ? (4 - Number) & 3 : Number;
	RND_COORD_ROTATE90(Text->X, Text->Y, X, Y, Number);

	Text->rot += number*90.0;
	if (Text->rot > 360.0)
		Text->rot -= 360.0;
	else if (Text->rot < 0.0)
		Text->rot += 360.0;

	/* can't optimize with box rotation because of closed boxes */
	pcb_text_bbox(pcb_font(PCB, Text->fid, 1), Text);
}

/* rotates a text; only the bounding box is rotated,
   text rotation itself is done by the drawing routines */
void pcb_text_rotate(pcb_text_t *Text, rnd_coord_t X, rnd_coord_t Y, double cosa, double sina, double rotdeg)
{
	rnd_rotate(&Text->X, &Text->Y, X, Y, cosa, sina);
	Text->rot += rotdeg;
	if (Text->rot > 360.0)
		Text->rot -= 360.0;
	else if (Text->rot < 0.0)
		Text->rot += 360.0;

	/* can't optimize with box rotation because of closed boxes */
	pcb_text_bbox(pcb_font(PCB, Text->fid, 1), Text);
}

/* rotates a text object and redraws it */
void *pcb_textop_rotate90(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	pcb_text_invalidate_erase(Layer, Text);
	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	if (Layer->text_tree != NULL)
		rnd_r_delete_entry(Layer->text_tree, (rnd_box_t *) Text);
	pcb_text_rotate90(Text, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.number);
	if (Layer->text_tree != NULL)
		rnd_r_insert_entry(Layer->text_tree, (rnd_box_t *) Text);
	pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	pcb_text_invalidate_draw(Layer, Text);
	return Text;
}

void *pcb_textop_rotate(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	pcb_text_invalidate_erase(Layer, Text);
	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	if (Layer->text_tree != NULL)
		rnd_r_delete_entry(Layer->text_tree, (rnd_box_t *) Text);

	if (Text->rot < 0.0)
		Text->rot += 360.0;

	rnd_rotate(&Text->X, &Text->Y, ctx->rotate.center_x, ctx->rotate.center_y, ctx->rotate.cosa, ctx->rotate.sina);
	if (PCB_FLAG_TEST(PCB_FLAG_ONSOLDER, Text))
		Text->rot -= ctx->rotate.angle;
	else
		Text->rot += ctx->rotate.angle;
	pcb_text_bbox(NULL, Text);

	if (Text->rot < 0)
		Text->rot = fmod(360 - Text->rot, 360);
	if (Text->rot > 360)
		Text->rot = fmod(Text->rot - 360, 360);

	if (Layer->text_tree != NULL)
		rnd_r_insert_entry(Layer->text_tree, (rnd_box_t *) Text);
	pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_TEXT, Layer, Text);
	pcb_text_invalidate_draw(Layer, Text);
	return Text;
}

void pcb_text_flip_side(pcb_layer_t *layer, pcb_text_t *text, rnd_coord_t y_offs, rnd_bool undoable)
{
	if (layer->text_tree != NULL)
		rnd_r_delete_entry(layer->text_tree, (rnd_box_t *) text);
	text->X = PCB_SWAP_X(text->X);
	text->Y = PCB_SWAP_Y(text->Y) + y_offs;
	PCB_FLAG_TOGGLE(PCB_FLAG_ONSOLDER, text);
	pcb_text_bbox(pcb_font(PCB, text->fid, 1), text);
	if (layer->text_tree != NULL)
		rnd_r_insert_entry(layer->text_tree, (rnd_box_t *) text);
}

void pcb_text_mirror_coords(pcb_text_t *text, rnd_coord_t y_offs, rnd_bool undoable)
{
	undo_text_geo_t gtmp, *g = &gtmp;

	if (undoable) g = pcb_undo_alloc(pcb_data_get_top(text->parent.layer->parent.data), &undo_text_geo, sizeof(undo_text_geo_t));

	g->text = text;
	g->X = PCB_SWAP_X(text->X);
	g->Y = PCB_SWAP_Y(text->Y) + y_offs;
	g->Scale = text->Scale;
	g->scale_x = text->scale_x; g->scale_y = text->scale_y;
	g->thickness = text->thickness;
	g->clearance = text->clearance;
	g->rot = text->rot;

	undo_text_geo_swap(g);
	if (undoable) pcb_undo_inc_serial();
}

void pcb_text_scale(pcb_text_t *text, double sx, double sy, double sth)
{
	int onbrd = (text->parent.layer != NULL) && (!text->parent.layer->is_bound);

	if (onbrd)
		pcb_text_pre(text);

	if (sx != 1.0) {
		text->X = rnd_round((double)text->X * sx);
		if (text->scale_x != 0) text->scale_x *= sx;
	}

	if (sy != 1.0) {
		text->Y = rnd_round((double)text->Y * sy);
		if (text->scale_y != 0) text->scale_y *= sy;
	}

	if ((sx != 1.0) || (sy != 1.0))
		text->Scale = rnd_round((double)text->Scale * (sy+sx)/2.0);

	if ((sth != 1.0) && (text->thickness > 0.0))
		text->thickness = rnd_round((double)text->thickness * sth);

	pcb_text_bbox(pcb_font(PCB, text->fid, 1), text);
	if (onbrd)
		pcb_text_post(text);
}


void pcb_text_set_font(pcb_text_t *text, pcb_font_id_t fid)
{
	pcb_layer_t *layer = text->parent.layer;

	assert(text->parent_type = PCB_PARENT_LAYER);

	pcb_poly_restore_to_poly(PCB->Data, PCB_OBJ_TEXT, layer, text);
	rnd_r_delete_entry(layer->text_tree, (rnd_box_t *) text);
	text->fid = fid;
	pcb_text_bbox(pcb_font(PCB, text->fid, 1), text);
	rnd_r_insert_entry(layer->text_tree, (rnd_box_t *) text);
	pcb_poly_clear_from_poly(PCB->Data, PCB_OBJ_TEXT, layer, text);
}

void pcb_text_pre(pcb_text_t *text)
{
	pcb_layer_t *ly = pcb_layer_get_real(text->parent.layer);
	if (ly == NULL)
		return;
	if (ly->text_tree != NULL)
		rnd_r_delete_entry(ly->text_tree, (rnd_box_t *)text);
	pcb_poly_restore_to_poly(ly->parent.data, PCB_OBJ_TEXT, ly, text);
}

void pcb_text_post(pcb_text_t *text)
{
	pcb_layer_t *ly = pcb_layer_get_real(text->parent.layer);
	if (ly == NULL)
		return;
	if (ly->text_tree != NULL)
		rnd_r_insert_entry(ly->text_tree, (rnd_box_t *)text);
	pcb_poly_clear_from_poly(ly->parent.data, PCB_OBJ_TEXT, ly, text);
}


void pcb_text_update(pcb_layer_t *layer, pcb_text_t *text)
{
	pcb_data_t *data = layer->parent.data;
	pcb_board_t *pcb = pcb_data_get_top(data);

	if (pcb == NULL)
		return;

	pcb_poly_restore_to_poly(data, PCB_OBJ_TEXT, layer, text);
	rnd_r_delete_entry(layer->text_tree, (rnd_box_t *) text);
	pcb_text_bbox(pcb_font(pcb, text->fid, 1), text);
	rnd_r_insert_entry(layer->text_tree, (rnd_box_t *) text);
	pcb_poly_clear_from_poly(data, PCB_OBJ_TEXT, layer, text);
}

void pcb_text_flagchg_pre(pcb_text_t *Text, unsigned long flagbits, void **save)
{
	pcb_data_t *data = Text->parent.layer->parent.data;

	*save = NULL;
	if ((flagbits & PCB_FLAG_CLEARLINE) || (flagbits & PCB_FLAG_ONSOLDER))
		pcb_poly_restore_to_poly(data, PCB_OBJ_TEXT, Text->parent.layer, Text);
	if (flagbits & PCB_FLAG_ONSOLDER) { /* bbox will also change, need to do rtree administration */
		*save = Text->parent.layer;
		rnd_r_delete_entry(Text->parent.layer->text_tree, (rnd_box_t *)Text);
	}
}

void pcb_text_flagchg_post(pcb_text_t *Text, unsigned long oldflagbits, void **save)
{
	pcb_data_t *data = Text->parent.layer->parent.data;
	pcb_layer_t *orig_layer = *save;
	unsigned long newflagbits = Text->Flags.f;

	if ((oldflagbits & PCB_FLAG_DYNTEXT) || (newflagbits & PCB_FLAG_DYNTEXT) || (orig_layer != NULL))
		pcb_text_bbox(pcb_font(PCB, Text->fid, 1), Text);

	if (orig_layer != NULL)
		rnd_r_insert_entry(orig_layer->text_tree, (rnd_box_t *)Text);

	if ((newflagbits & PCB_FLAG_CLEARLINE) || (orig_layer != NULL))
		pcb_poly_clear_from_poly(data, PCB_OBJ_TEXT, Text->parent.layer, Text);

	*save = NULL;
}

void *pcb_textop_change_flag(pcb_opctx_t *ctx, pcb_layer_t *Layer, pcb_text_t *Text)
{
	void *save;
	static pcb_flag_values_t pcb_text_flags = 0;
	unsigned long oldflg = Text->Flags.f;

	if (pcb_text_flags == 0)
		pcb_text_flags = pcb_obj_valid_flags(PCB_OBJ_TEXT);

	if ((ctx->chgflag.flag & pcb_text_flags) != ctx->chgflag.flag)
		return NULL;
	if ((ctx->chgflag.flag & PCB_FLAG_TERMNAME) && (Text->term == NULL))
		return NULL;
	pcb_undo_add_obj_to_flag(Text);

	pcb_text_flagchg_pre(/*ctx->chgflag.pcb->Data, */Text, ctx->chgflag.flag, &save);
	PCB_FLAG_CHANGE(ctx->chgflag.how, ctx->chgflag.flag, Text);
	pcb_text_flagchg_post(Text, oldflg, &save);

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

int pcb_text_chg_scale(pcb_text_t *text, double scx, rnd_bool absx, double scy, rnd_bool absy, rnd_bool undoable)
{
	undo_text_geo_t gtmp, *g = &gtmp;

	if (undoable) g = pcb_undo_alloc(pcb_data_get_top(text->parent.layer->parent.data), &undo_text_geo, sizeof(undo_text_geo_t));

	g->text = text;
	g->X = text->X;
	g->Y = text->Y;
	g->Scale = text->Scale;
	g->scale_x = absx ? scx : text->scale_x + scx;
	g->scale_y = absy ? scy : text->scale_y + scy;
	g->thickness = text->thickness;
	g->clearance = text->clearance;
	g->rot = text->rot;

	undo_text_geo_swap(g);
	if (undoable) pcb_undo_inc_serial();

	return 0;
}

/*** draw ***/

#define MAX_SIMPLE_POLY_POINTS 256
static void draw_text_poly(pcb_draw_info_t *info, pcb_poly_t *poly, pcb_xform_mx_t mx, rnd_coord_t xo, int xordraw, int thindraw, rnd_coord_t xordx, rnd_coord_t xordy, pcb_draw_text_cb cb, void *cb_ctx)
{
	rnd_coord_t x[MAX_SIMPLE_POLY_POINTS], y[MAX_SIMPLE_POLY_POINTS];
	int max, n;
	rnd_point_t *p;

	max = poly->PointN;
	if (max > MAX_SIMPLE_POLY_POINTS) {
		max = MAX_SIMPLE_POLY_POINTS;
	}

	/* transform each coordinate */
	for(n = 0, p = poly->Points; n < max; n++,p++) {
		x[n] = rnd_round(pcb_xform_x(mx, p->X + xo, p->Y));
		y[n] = rnd_round(pcb_xform_y(mx, p->X + xo, p->Y));
	}

	if ((info != NULL) && (info->xform != NULL) && (info->xform->bloat != 0)) {
		rnd_polo_t *p, pp[MAX_SIMPLE_POLY_POINTS];
		double a2, dv;
		for(n = 0, p = pp; n < max; n++,p++) {
			p->x = x[n];
			p->y = y[n];
		}
		rnd_polo_norms(pp, max);
		a2 = rnd_polo_2area(pp, max);
		if (a2 < 0)
			dv = -0.5;
		else
			dv = 0.5;
		rnd_polo_offs(info->xform->bloat*dv, pp, max);
		for(n = 0, p = pp; n < max; n++,p++) {
			x[n] = rnd_round(p->x);
			y[n] = rnd_round(p->y);
		}
	}

	if (cb != NULL) {
		pcb_poly_t po;
		rnd_point_t pt[MAX_SIMPLE_POLY_POINTS];

		memset(&po, 0, sizeof(po));
		po.type = PCB_OBJ_POLY;
		po.PointN = max;
		po.Points = pt;
		for(n = 0, p = po.Points; n < max; n++,p++) {
			p->X = x[n];
			p->Y = y[n];
		}

		cb(cb_ctx, (pcb_any_obj_t *)&po);
		return;
	}


	if (xordraw || thindraw) {
		rnd_hid_gc_t gc = xordraw ? pcb_crosshair.GC : pcb_draw_out.fgGC;
		for(n = 1, p = poly->Points+1; n < max; n++,p++)
			rnd_render->draw_line(gc, xordx + x[n-1], xordy + y[n-1], xordx + x[n], xordy + y[n]);
		rnd_render->draw_line(gc, xordx + x[0], xordy + y[0], xordx + x[max-1], xordy + y[max-1]);
	}
	else
		rnd_render->fill_polygon(pcb_draw_out.fgGC, poly->PointN, x, y);
}

/* Very rough estimation on the full width of the text */
rnd_coord_t pcb_text_width(pcb_font_t *font, double scx, const unsigned char *string)
{
	rnd_coord_t w = 0;
	const rnd_box_t *defaultsymbol;
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
	return rnd_round((double)w * scx);
}

rnd_coord_t pcb_text_height(pcb_font_t *font, double scy, const unsigned char *string)
{
	rnd_coord_t h;
	if (string == NULL)
		return 0;
	h = font->MaxHeight;
	while(*string != '\0') {
		if (*string == '\n')
			h += font->MaxHeight;
		string++;
	}
	return rnd_round((double)h * scy);
}


RND_INLINE void cheap_text_line(rnd_hid_gc_t gc, pcb_xform_mx_t mx, rnd_coord_t x1, rnd_coord_t y1, rnd_coord_t x2, rnd_coord_t y2, rnd_coord_t xordx, rnd_coord_t xordy)
{
	rnd_coord_t tx1, ty1, tx2, ty2;

	tx1 = rnd_round(pcb_xform_x(mx, x1, y1));
	ty1 = rnd_round(pcb_xform_y(mx, x1, y1));
	tx2 = rnd_round(pcb_xform_x(mx, x2, y2));
	ty2 = rnd_round(pcb_xform_y(mx, x2, y2));

	tx1 += xordx;
	ty1 += xordy;
	tx2 += xordx;
	ty2 += xordy;

	rnd_render->draw_line(gc, tx1, ty1, tx2, ty2);
}


/* Decreased level-of-detail: draw only a few lines for the whole text */
RND_INLINE int draw_text_cheap(pcb_font_t *font, pcb_xform_mx_t mx, const unsigned char *string, double scx, double scy, int xordraw, rnd_coord_t xordx, rnd_coord_t xordy, pcb_text_tiny_t tiny)
{
	rnd_coord_t w, h = rnd_round((double)font->MaxHeight * scy);
	if (tiny == PCB_TXT_TINY_HIDE) {
		if (h <= rnd_render->coord_per_pix*6) /* <= 6 pixel high: unreadable */
			return 1;
	}
	else if (tiny == PCB_TXT_TINY_CHEAP) {
		if (h <= rnd_render->coord_per_pix*2) { /* <= 1 pixel high: draw a single line in the middle */
			w = pcb_text_width(font, scx, string);
			if (xordraw) {
				cheap_text_line(pcb_crosshair.GC, mx, 0, h/2, w, h/2, xordx, xordy);
			}
			else {
				rnd_hid_set_line_width(pcb_draw_out.fgGC, -1);
				rnd_hid_set_line_cap(pcb_draw_out.fgGC, rnd_cap_square);
				cheap_text_line(pcb_draw_out.fgGC, mx, 0, h/2, w, h/2, 0, 0);
			}
			return 1;
		}
		else if (h <= rnd_render->coord_per_pix*4) { /* <= 4 pixel high: draw a mirrored Z-shape */
			w = pcb_text_width(font, scx, string);
			if (xordraw) {
				h /= 4;
				cheap_text_line(pcb_crosshair.GC, mx, 0, h,   w, h,   xordx, xordy);
				cheap_text_line(pcb_crosshair.GC, mx, 0, h,   w, h*3, xordx, xordy);
				cheap_text_line(pcb_crosshair.GC, mx, 0, h*3, w, h*3, xordx, xordy);
			}
			else {
				h /= 4;
				rnd_hid_set_line_width(pcb_draw_out.fgGC, -1);
				rnd_hid_set_line_cap(pcb_draw_out.fgGC, rnd_cap_square);
				cheap_text_line(pcb_draw_out.fgGC, mx, 0, h,   w, h,   0, 0);
				cheap_text_line(pcb_draw_out.fgGC, mx, 0, h,   w, h*3, 0, 0);
				cheap_text_line(pcb_draw_out.fgGC, mx, 0, h*3, w, h*3, 0, 0);
			}
			return 1;
		}
	}
	return 0;
}

RND_INLINE void pcb_text_draw_string_(pcb_draw_info_t *info, pcb_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, pcb_text_mirror_t mirror, rnd_coord_t thickness, rnd_coord_t min_line_width, int xordraw, rnd_coord_t xordx, rnd_coord_t xordy, pcb_text_tiny_t tiny, pcb_draw_text_cb cb, void *cb_ctx)
{
	rnd_coord_t x = 0;
	rnd_cardinal_t n;
	pcb_xform_mx_t mx = PCB_XFORM_MX_IDENT;

	pcb_xform_mx_translate(mx, x0, y0);
	if (mirror != PCB_TXT_MIRROR_NO)
		pcb_xform_mx_scale(mx, (mirror & PCB_TXT_MIRROR_X) ? -1 : 1, (mirror & PCB_TXT_MIRROR_Y) ? -1 : 1);
	pcb_xform_mx_rotate(mx, rotdeg);
	pcb_xform_mx_scale(mx, scx, scy);

	/* Text too small at this zoom level: cheap draw */
	if ((tiny != PCB_TXT_TINY_ACCURATE) && (cb == NULL)) {
		if (draw_text_cheap(font, mx, string, scx, scy, xordraw, xordx, xordy, tiny))
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
			int poly_thin;

			for (n = font->Symbol[*string].LineN; n; n--, line++) {
				/* create one line, scale, move, rotate and swap it */
				newline = *line;
				newline.Point1.X = rnd_round(pcb_xform_x(mx, line->Point1.X+x, line->Point1.Y));
				newline.Point1.Y = rnd_round(pcb_xform_y(mx, line->Point1.X+x, line->Point1.Y));
				newline.Point2.X = rnd_round(pcb_xform_x(mx, line->Point2.X+x, line->Point2.Y));
				newline.Point2.Y = rnd_round(pcb_xform_y(mx, line->Point2.X+x, line->Point2.Y));
				newline.Thickness = rnd_round(newline.Thickness * (scx+scy) / 4.0);

				if (newline.Thickness < min_line_width)
					newline.Thickness = min_line_width;
				if (thickness > 0)
					newline.Thickness = thickness;
				if (cb != NULL) {
					newline.type = PCB_OBJ_LINE;
					cb(cb_ctx, (pcb_any_obj_t *)&newline);
				}
				else if (xordraw)
					rnd_render->draw_line(pcb_crosshair.GC, xordx + newline.Point1.X, xordy + newline.Point1.Y, xordx + newline.Point2.X, xordy + newline.Point2.Y);
				else
					pcb_line_draw_(info, &newline, 0);
			}

			/* draw the arcs */
			for(a = arclist_first(&font->Symbol[*string].arcs); a != NULL; a = arclist_next(a)) {
				newarc = *a;

				newarc.X = rnd_round(pcb_xform_x(mx, a->X + x, a->Y));
				newarc.Y = rnd_round(pcb_xform_y(mx, a->X + x, a->Y));
				newarc.Height = newarc.Width = rnd_round(newarc.Height * scx);
				newarc.Thickness = rnd_round(newarc.Thickness * (scx+scy) / 4.0);
				newarc.StartAngle += rotdeg;
				if (mirror) {
					newarc.StartAngle = RND_SWAP_ANGLE(newarc.StartAngle);
					newarc.Delta = RND_SWAP_DELTA(newarc.Delta);
				}
				if (newarc.Thickness < min_line_width)
					newarc.Thickness = min_line_width;
				if (thickness > 0)
					newarc.Thickness = thickness;
				if (cb != NULL) {
					newarc.type = PCB_OBJ_ARC;
					cb(cb_ctx, (pcb_any_obj_t *)&newarc);
				}
				else if (xordraw)
					rnd_render->draw_arc(pcb_crosshair.GC, xordx + newarc.X, xordy + newarc.Y, newarc.Width, newarc.Height, newarc.StartAngle, newarc.Delta);
				else
					pcb_arc_draw_(info, &newarc, 0);
			}

			/* draw the polygons */
			poly_thin = info->xform->thin_draw || info->xform->wireframe;
			for(p = polylist_first(&font->Symbol[*string].polys); p != NULL; p = polylist_next(p))
				draw_text_poly(info, p, mx, x, xordraw, poly_thin, xordx, xordy, cb, cb_ctx);

			/* move on to next cursor position */
			x += (font->Symbol[*string].Width + font->Symbol[*string].Delta);
		}
		else {
			/* the default symbol is a filled box */
			rnd_coord_t size = (font->DefaultSymbol.X2 - font->DefaultSymbol.X1) * 6 / 5;
			rnd_coord_t px[4], py[4];

			px[0] = rnd_round(pcb_xform_x(mx, font->DefaultSymbol.X1 + x, font->DefaultSymbol.Y1));
			py[0] = rnd_round(pcb_xform_y(mx, font->DefaultSymbol.X1 + x, font->DefaultSymbol.Y1));
			px[1] = rnd_round(pcb_xform_x(mx, font->DefaultSymbol.X2 + x, font->DefaultSymbol.Y1));
			py[1] = rnd_round(pcb_xform_y(mx, font->DefaultSymbol.X2 + x, font->DefaultSymbol.Y1));
			px[2] = rnd_round(pcb_xform_x(mx, font->DefaultSymbol.X2 + x, font->DefaultSymbol.Y2));
			py[2] = rnd_round(pcb_xform_y(mx, font->DefaultSymbol.X2 + x, font->DefaultSymbol.Y2));
			px[3] = rnd_round(pcb_xform_x(mx, font->DefaultSymbol.X1 + x, font->DefaultSymbol.Y2));
			py[3] = rnd_round(pcb_xform_y(mx, font->DefaultSymbol.X1 + x, font->DefaultSymbol.Y2));

			/* draw move on to next cursor position */
			if ((cb == NULL) && (xordraw || (info->xform->thin_draw || info->xform->wireframe))) {
				if (xordraw) {
					rnd_render->draw_line(pcb_crosshair.GC, px[0] + xordx, py[0] + xordy, px[1] + xordx, py[1] + xordy);
					rnd_render->draw_line(pcb_crosshair.GC, px[1] + xordx, py[1] + xordy, px[2] + xordx, py[2] + xordy);
					rnd_render->draw_line(pcb_crosshair.GC, px[2] + xordx, py[2] + xordy, px[3] + xordx, py[3] + xordy);
					rnd_render->draw_line(pcb_crosshair.GC, px[3] + xordx, py[3] + xordy, px[0] + xordx, py[0] + xordy);
				}
				else {
					rnd_render->draw_line(pcb_draw_out.fgGC, px[0], py[0], px[1], py[1]);
					rnd_render->draw_line(pcb_draw_out.fgGC, px[1], py[1], px[2], py[2]);
					rnd_render->draw_line(pcb_draw_out.fgGC, px[2], py[2], px[3], py[3]);
					rnd_render->draw_line(pcb_draw_out.fgGC, px[3], py[3], px[0], py[0]);
				}
			}
			else {
				if (cb != NULL) {
					pcb_poly_t po;
					rnd_point_t pt[4], *p;

					memset(&po, 0, sizeof(po));
					po.type = PCB_OBJ_POLY;
					po.PointN = 4;
					po.Points = pt;
					for(n = 0, p = po.Points; n < po.PointN; n++,p++) {
						p->X = px[n];
						p->Y = py[n];
					}
					cb(cb_ctx, (pcb_any_obj_t *)&po);
				}
				else
					rnd_render->fill_polygon(pcb_draw_out.fgGC, 4, px, py);
			}
			x += size;
		}
		string++;
	}
}

void pcb_text_draw_string(pcb_draw_info_t *info, pcb_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, pcb_text_mirror_t mirror, rnd_coord_t thickness, rnd_coord_t min_line_width, int xordraw, rnd_coord_t xordx, rnd_coord_t xordy, pcb_text_tiny_t tiny)
{
	pcb_text_draw_string_(info, font, string, x0, y0, scx, scy, rotdeg, mirror, thickness, min_line_width, xordraw, xordx, xordy, tiny, NULL, NULL);
}

void pcb_text_draw_string_simple(pcb_font_t *font, const char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, pcb_text_mirror_t mirror, rnd_coord_t thickness, int xordraw, rnd_coord_t xordx, rnd_coord_t xordy)
{
	static rnd_xform_t xform = {0};
	static pcb_draw_info_t info = {0};

	info.xform = &xform;
	if (font == NULL)
		font = pcb_font(PCB, 0, 0);

	pcb_text_draw_string_(&info, font, (const unsigned char *)string, x0, y0, scx, scy, rotdeg, mirror, thickness, 0, xordraw, xordx, xordy, PCB_TXT_TINY_CHEAP, NULL, NULL);
}


void pcb_text_decompose_string(pcb_draw_info_t *info, pcb_font_t *font, const unsigned char *string, rnd_coord_t x0, rnd_coord_t y0, double scx, double scy, double rotdeg, pcb_text_mirror_t mirror, rnd_coord_t thickness, pcb_draw_text_cb cb, void *cb_ctx)
{
	pcb_text_draw_string_(info, font, string, x0, y0, scx, scy, rotdeg, mirror, thickness, 0, 0, 0, 0, PCB_TXT_TINY_ACCURATE, cb, cb_ctx);
}

void pcb_text_decompose_text(pcb_draw_info_t *info, pcb_text_t *text, pcb_draw_text_cb cb, void *cb_ctx)
{
	unsigned char *rendered = pcb_text_render_str(text);
	double scx, scy;
	pcb_text_get_scale_xy(text, &scx, &scy);
	pcb_text_decompose_string(info, pcb_font(PCB, text->fid, 1), rendered, text->X, text->Y, scx, scy, text->rot, text_mirror_bits(text), text->thickness, cb, cb_ctx);
	pcb_text_free_str(text, rendered);
}


/* lowlevel drawing routine for text objects */
static void DrawTextLowLevel_(pcb_draw_info_t *info, pcb_text_t *Text, rnd_coord_t min_line_width, int xordraw, rnd_coord_t xordx, rnd_coord_t xordy, pcb_text_tiny_t tiny)
{
	unsigned char *rendered = pcb_text_render_str(Text);
	double scx, scy;
	pcb_text_get_scale_xy(Text, &scx, &scy);
	pcb_text_draw_string_(info, pcb_font(PCB, Text->fid, 1), rendered, Text->X, Text->Y, scx, scy, Text->rot, text_mirror_bits(Text), Text->thickness, min_line_width, xordraw, xordx, xordy, tiny, NULL, NULL);
	pcb_text_free_str(Text, rendered);
}

static rnd_bool is_text_term_vert(const pcb_text_t *text)
{
	rnd_coord_t dx, dy;

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
			100.0, is_text_term_vert(text), rnd_true, (pcb_any_obj_t *)text);
}

void pcb_text_draw_label(pcb_draw_info_t *info, pcb_text_t *text, rnd_bool vis_side)
{
	if ((text->term != NULL) && vis_side)
		pcb_term_label_draw(info, (text->BoundingBox.X1 + text->BoundingBox.X2)/2, (text->BoundingBox.Y1 + text->BoundingBox.Y2)/2,
			conf_core.appearance.term_label_size, is_text_term_vert(text), rnd_true, (pcb_any_obj_t *)text);
	if (text->noexport && vis_side)
		pcb_obj_noexport_mark(text, (text->BoundingBox.X1 + text->BoundingBox.X2)/2, (text->BoundingBox.Y1 + text->BoundingBox.Y2)/2);
}


void pcb_text_draw_(pcb_draw_info_t *info, pcb_text_t *text, rnd_coord_t min_line_width, int allow_term_gfx, pcb_text_tiny_t tiny)
{
	if (delayed_terms_enabled && (text->term != NULL)) {
		pcb_draw_delay_obj_add((pcb_any_obj_t *)text);
		return;
	}

	DrawTextLowLevel_(info, text, min_line_width, 0, 0, 0, tiny);

	if (text->term != NULL) {
		if ((allow_term_gfx) && ((pcb_draw_force_termlab) || PCB_FLAG_TEST(PCB_FLAG_TERMNAME, text)))
			pcb_draw_delay_label_add((pcb_any_obj_t *)text);
	}
}

static void pcb_text_draw(pcb_draw_info_t *info, pcb_text_t *text, int allow_term_gfx)
{
	int min_silk_line;
	unsigned int flg = 0;
	const pcb_layer_t *layer = info->layer != NULL ? info->layer : pcb_layer_get_real(text->parent.layer);

	pcb_obj_noexport(info, text, return);

	if (layer == NULL) /* if the layer is inbound, e.g. in preview, fall back using the layer recipe */
		layer = text->parent.layer;

	if (info->xform->flag_color && PCB_FLAG_TEST(PCB_FLAG_SELECTED, text)) {
		if (layer->is_bound) {
			const rnd_color_t *color;
			PCB_OBJ_COLOR_ON_BOUND_LAYER(color, layer, 1);
			rnd_render->set_color(pcb_draw_out.fgGC, color);
		}
		else
			rnd_render->set_color(pcb_draw_out.fgGC, &conf_core.appearance.color.selected);
	}
	else if (info->xform->flag_color && PCB_HAS_COLOROVERRIDE(text)) {
		rnd_render->set_color(pcb_draw_out.fgGC, text->override_color);
	}
	else if (layer->is_bound) {
		const rnd_color_t *color;
		PCB_OBJ_COLOR_ON_BOUND_LAYER(color, layer, 0);
		rnd_render->set_color(pcb_draw_out.fgGC, color);
	}
	else
		rnd_render->set_color(pcb_draw_out.fgGC, &layer->meta.real.color);

	if ((!layer->is_bound) && (layer->meta.real.grp >= 0))
		flg = pcb_layergrp_flags(PCB, layer->meta.real.grp);

	if (flg & PCB_LYT_SILK)
		min_silk_line = conf_core.design.min_slk;
	else
		min_silk_line = conf_core.design.min_wid;

	pcb_text_draw_(info, text, min_silk_line, allow_term_gfx, PCB_TXT_TINY_CHEAP);
}

rnd_r_dir_t pcb_text_draw_callback(const rnd_box_t * b, void *cl)
{
	pcb_text_t *text = (pcb_text_t *) b;
	pcb_draw_info_t *info = cl;

	if (pcb_hidden_floater((pcb_any_obj_t*)b, info) || pcb_partial_export((pcb_any_obj_t*)b, info))
		return RND_R_DIR_FOUND_CONTINUE;

	if (!PCB->SubcPartsOn && pcb_lobj_parent_subc(text->parent_type, &text->parent))
		return RND_R_DIR_FOUND_CONTINUE;

	pcb_text_draw(info, text, 0);
	return RND_R_DIR_FOUND_CONTINUE;
}

rnd_r_dir_t pcb_text_draw_term_callback(const rnd_box_t * b, void *cl)
{
	pcb_text_t *text = (pcb_text_t *) b;
	pcb_draw_info_t *info = cl;

	if (pcb_hidden_floater((pcb_any_obj_t*)b, info) || pcb_partial_export((pcb_any_obj_t*)b, info))
		return RND_R_DIR_FOUND_CONTINUE;

	if (!PCB->SubcPartsOn && pcb_lobj_parent_subc(text->parent_type, &text->parent))
		return RND_R_DIR_FOUND_CONTINUE;

	pcb_text_draw(info, text, 1);
	return RND_R_DIR_FOUND_CONTINUE;
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

static pcb_draw_info_t txor_info;
static rnd_xform_t txor_xform;

void pcb_text_draw_xor(pcb_text_t *text, rnd_coord_t x, rnd_coord_t y, rnd_bool want_box)
{
	txor_info.xform = &txor_xform;
	DrawTextLowLevel_(&txor_info, text, 0, 1, x, y, PCB_TXT_TINY_CHEAP);
	if (want_box && (conf_core.appearance.text_host_bbox) && (text->BoundingBox.X1 != text->BoundingBox.X2)) {
		rnd_render->draw_line(pcb_crosshair.GC, x + text->BoundingBox.X1, y + text->BoundingBox.Y1, x + text->BoundingBox.X1, y + text->BoundingBox.Y2);
		rnd_render->draw_line(pcb_crosshair.GC, x + text->BoundingBox.X1, y + text->BoundingBox.Y1, x + text->BoundingBox.X2, y + text->BoundingBox.Y1);
		rnd_render->draw_line(pcb_crosshair.GC, x + text->BoundingBox.X2, y + text->BoundingBox.Y2, x + text->BoundingBox.X1, y + text->BoundingBox.Y2);
		rnd_render->draw_line(pcb_crosshair.GC, x + text->BoundingBox.X2, y + text->BoundingBox.Y2, x + text->BoundingBox.X2, y + text->BoundingBox.Y1);
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
			rnd_r_delete_entry(data->subc_tree, (rnd_box_t *)subc);
			pcb_subc_bbox(subc);
			rnd_r_insert_entry(data->subc_tree, (rnd_box_t *)subc);
		}
	} PCB_END_LOOP;

	return need_redraw;
}

static void pcb_text_font_chg(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{

	if ((argc < 2) || (argv[1].type != RND_EVARG_INT))
		return;

	if (pcb_text_font_chg_data(PCB->Data, argv[1].d.i))
		rnd_gui->invalidate_all(rnd_gui); /* can't just redraw the text, as the old text may have been bigger, before the change! */

	rnd_trace("font change %d\n", argv[1].d.i);
}

rnd_bool pcb_text_old_direction(int *dir_out, double rot)
{
	double r;

	r = fmod(rot, 90.0);

	if (dir_out != NULL) {
		int d = rnd_round(rot / 90);
		if (d < 0)
			d += 4;
		*dir_out = d;
	}

	return (r <= 0.5);
}

void pcb_text_init(void)
{
	rnd_event_bind(PCB_EVENT_FONT_CHANGED, pcb_text_font_chg, NULL, text_cookie);
}

void pcb_text_uninit(void)
{
	rnd_event_unbind_allcookie(text_cookie);
}
