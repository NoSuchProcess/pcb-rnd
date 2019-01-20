/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017..2019 Tibor 'Igor2' Palinkas
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
 */

#include "config.h"

#include <genvector/vtp0.h>

#include "buffer.h"
#include "board.h"
#include "crosshair.h"
#include "data.h"
#include "error.h"
#include "obj_subc.h"
#include "obj_subc_op.h"
#include "obj_subc_parent.h"
#include "obj_poly_op.h"
#include "obj_pstk.h"
#include "obj_pstk_inlines.h"
#include "obj_pstk_draw.h"
#include "obj_line_op.h"
#include "obj_term.h"
#include "obj_text_draw.h"
#include "rtree.h"
#include "draw.h"
#include "draw_wireframe.h"
#include "flag.h"
#include "polygon.h"
#include "operation.h"
#include "undo.h"
#include "macro.h"
#include "compat_misc.h"
#include "math_helper.h"
#include "pcb_minuid.h"
#include "conf_core.h"

#define SUBC_AUX_NAME "subc-aux"

void pcb_subc_reg(pcb_data_t *data, pcb_subc_t *subc)
{
	pcb_subclist_append(&data->subc, subc);
	pcb_obj_id_reg(data, subc);
	PCB_SET_PARENT(subc->data, subc, subc);
	PCB_SET_PARENT(subc, data, data);
}

void pcb_subc_unreg(pcb_subc_t *subc)
{
	pcb_data_t *data = subc->parent.data;
	assert(subc->parent_type == PCB_PARENT_DATA);
	pcb_subclist_remove(subc);
	pcb_obj_id_del(data, subc);
	PCB_CLEAR_PARENT(subc);
}

/* Modify dst to include src, if src is not a floater */
static void pcb_box_bump_box_noflt(pcb_box_t *dst, pcb_box_t *src)
{
	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, ((pcb_any_obj_t *)(src))))
		pcb_box_bump_box(dst, src);
}

/* update cached values: e.g. looking up refdes in attributes each time the
   netlist code needs it would be too expensive. Instead, we maintain a
   read-only ->refdes field and update it any time attributes change. */
static void pcb_subc_attrib_post_change(pcb_attribute_list_t *list, const char *name, const char *value)
{
	pcb_subc_t *sc = (pcb_subc_t *)(((char *)list) - offsetof(pcb_subc_t, Attributes));
	if (strcmp(name, "refdes") == 0) {
		const char *inv;
		sc->refdes = value;
		inv = pcb_obj_id_invalid(sc->refdes);
		if (inv != NULL)
			pcb_message(PCB_MSG_ERROR, "Invalid character '%c' in subc refdes '%s'\n", *inv, sc->refdes);
	}
	pcb_text_dyn_bbox_update(sc->data);
}

pcb_subc_t *pcb_subc_alloc_id(long int id)
{
	pcb_subc_t *sc;
	sc = calloc(sizeof(pcb_subc_t), 1);
	sc->ID = id;
	sc->Attributes.post_change = pcb_subc_attrib_post_change;
	sc->data = pcb_data_new(NULL);
	sc->type = PCB_OBJ_SUBC;
	PCB_SET_PARENT(sc->data, subc, sc);
	minuid_gen(&pcb_minuid, sc->uid);
	pcb_term_init(&sc->terminals);
	return sc;
}

pcb_subc_t *pcb_subc_alloc(void)
{
	return pcb_subc_alloc_id(pcb_create_ID_get());

}


pcb_subc_t *pcb_subc_new(void)
{
	return pcb_subc_alloc();
}

void pcb_subc_free(pcb_subc_t *sc)
{
	pcb_term_uninit(&sc->terminals);
	pcb_subc_unreg(sc);
	pcb_data_free(sc->data);
	free(sc);
}

/* Create (and append) a new bound layer to a subc */
static pcb_layer_t *pcb_subc_layer_create_buff(pcb_subc_t *sc, pcb_layer_t *src)
{
	pcb_layer_t *dst = &sc->data->Layer[sc->data->LayerN++];

	memcpy(&dst->meta, &src->meta, sizeof(src->meta));
	dst->is_bound = 1;
	dst->comb = src->comb;
	dst->parent.data = sc->data;
	dst->parent_type = PCB_PARENT_DATA;
	dst->type = PCB_OBJ_LAYER;
	dst->name = pcb_strdup(src->name);
	return dst;
}

static pcb_line_t *add_aux_line(pcb_layer_t *aux, const char *key, const char *val, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	pcb_line_t *l = pcb_line_new(aux, x1, y1, x2, y2, PCB_MM_TO_COORD(0.1), 0, pcb_no_flags());
	pcb_attribute_put(&l->Attributes, key, val);
	return l;
}

static pcb_line_t *find_aux_line(pcb_layer_t *aux, const char *key)
{
	pcb_line_t *line;
	gdl_iterator_t it;
	linelist_foreach(&aux->Line, &it, line) {
		const char *val = pcb_attribute_get(&line->Attributes, "subc-role");
		if ((val != NULL) && (strcmp(val, key) == 0))
			return line;
	}
	return NULL;
}

/* Find and save the aux layer in the cache, of it exists */
static void pcb_subc_cache_find_aux(pcb_subc_t *sc, int alloc)
{
	int n;
	for(n = 0; n < sc->data->LayerN; n++) {
		if (strcmp(sc->data->Layer[n].name, SUBC_AUX_NAME) == 0) {
			sc->aux_layer = sc->data->Layer + n;
			return;
		}
	}

	if (sc->aux_layer == NULL)
		sc->aux_layer = pcb_layer_new_bound(sc->data, PCB_LYT_VIRTUAL | PCB_LYT_NOEXPORT | PCB_LYT_MISC | PCB_LYT_TOP, SUBC_AUX_NAME, NULL);
}

/* Looking up aux objects for determining orientation/origin of a subc
   is somewhat expensive. Instead of doing that every time, we just save
   the pointer of those objects/layers. They don't change once the
   subc is loaded into memory */
static int pcb_subc_cache_update(pcb_subc_t *sc)
{
	if (sc->aux_layer == NULL)
		pcb_subc_cache_find_aux(sc, 0);

	if (sc->aux_cache[0] != NULL)
		return 0;

	if (sc->aux_layer == NULL) {
		pcb_message(PCB_MSG_WARNING, "Can't find subc aux layer\n");
		return -1;
	}

	sc->aux_cache[PCB_SUBCH_ORIGIN] = find_aux_line(sc->aux_layer, "origin");
	sc->aux_cache[PCB_SUBCH_X] = find_aux_line(sc->aux_layer, "x");
	sc->aux_cache[PCB_SUBCH_Y] = find_aux_line(sc->aux_layer, "y");

	return 0;
}

pcb_bool pcb_subc_find_aux_point(pcb_subc_t *sc, const char *role, pcb_coord_t *x, pcb_coord_t *y)
{
	pcb_line_t *l;

	if (sc->aux_layer == NULL)
		pcb_subc_cache_update(sc);
	if (sc->aux_layer == NULL)
		return pcb_false;

	l = find_aux_line(sc->aux_layer, role);
	if (l == NULL)
		return pcb_false;

	*x = l->Point1.X;
	*y = l->Point1.Y;

	return pcb_true;
}

int pcb_subc_get_origin(pcb_subc_t *sc, pcb_coord_t *x, pcb_coord_t *y)
{
	if ((pcb_subc_cache_update(sc) != 0) || (sc->aux_cache[PCB_SUBCH_ORIGIN] == NULL))
		return -1;

	*x = sc->aux_cache[PCB_SUBCH_ORIGIN]->Point1.X;
	*y = sc->aux_cache[PCB_SUBCH_ORIGIN]->Point1.Y;
	return 0;
}

int pcb_subc_get_rotation(pcb_subc_t *sc, double *rot)
{
	double r, rr;
	if ((pcb_subc_cache_update(sc) != 0) || (sc->aux_cache[PCB_SUBCH_X] == NULL))
		return -1;

	r = -1 * PCB_RAD_TO_DEG * atan2(sc->aux_cache[PCB_SUBCH_X]->Point2.Y - sc->aux_cache[PCB_SUBCH_X]->Point1.Y, sc->aux_cache[PCB_SUBCH_X]->Point2.X - sc->aux_cache[PCB_SUBCH_X]->Point1.X);

	/* ugly hack to get round angles where possible: if error to a round angle
	   is less than 1/10000, it was meant to be round, just got ruined by
	   rounding errors */
	rr = pcb_round(r*10000.0) / 10000.0;
	if (rr - r < 0.0001)
		*rot = rr;
	else
		*rot = r;
	return 0;
}

/* The subc is originally drawn on the top side, and the aux layer gets
   the PCB_LYT_TOP flag. If the subc is ever sent to the other side, the
   aux layer changed side too. This is to be detected here. */
int pcb_subc_get_side(pcb_subc_t *sc, int *on_bottom)
{
	if ((pcb_subc_cache_update(sc) != 0) || (sc->aux_layer == NULL))
		return -1;
	if (!sc->aux_layer->is_bound)
		return -1;

	if (sc->aux_layer->meta.bound.type & PCB_LYT_TOP)
		*on_bottom = 0;
	else if (sc->aux_layer->meta.bound.type & PCB_LYT_BOTTOM)
		*on_bottom = 1;
	else
		return -1;

	return 0;
}

int pcb_subc_get_host_trans(pcb_subc_t *sc, pcb_host_trans_t *tr, int neg)
{
	int res = 0;
	double rr;

	if (pcb_subc_get_origin(sc, &tr->ox, &tr->oy) != 0) {
		tr->ox = tr->oy = 0;
		res = -1;
	}

	if (pcb_subc_get_rotation(sc, &tr->rot) != 0) {
		tr->rot = 0;
		res = -1;
	}
	
	if (pcb_subc_get_side(sc, &tr->on_bottom) != 0) {
		tr->on_bottom = 0;
		res = -1;
	}

	rr = tr->rot / PCB_RAD_TO_DEG;

	if (neg) {
		tr->rot = -tr->rot;
		rr = -rr;
		tr->ox = -tr->ox;
		tr->oy = -tr->oy;
	}

	tr->cosa = cos(rr);
	tr->sina = sin(rr);

	return res;
}

static void pcb_subc_cache_invalidate(pcb_subc_t *sc)
{
	sc->aux_cache[0] = NULL;
}

/* Convert a square line into a polygon and add it to dst */
static pcb_poly_t *sqline2term(pcb_layer_t *dst, pcb_line_t *line)
{
	pcb_poly_t *poly;
	pcb_coord_t x[4], y[4];
	int n;

	pcb_sqline_to_rect(line, x, y);

	poly = pcb_poly_new(dst, line->Clearance, pcb_no_flags());
	for(n = 0; n < 4; n++)
		pcb_poly_point_new(poly, x[n], y[n]);
	PCB_FLAG_SET(PCB_FLAG_CLEARPOLYPOLY, poly);
	pcb_attribute_copy_all(&poly->Attributes, &line->Attributes);

	pcb_poly_init_clip(dst->parent.data, dst, poly);
	pcb_add_poly_on_layer(dst, poly);

	return poly;
}

pcb_layer_t *pcb_loose_subc_layer(pcb_board_t *pcb, pcb_layer_t *layer, pcb_bool alloc)
{
	pcb_subc_t *sc;
	int n;

	if (!pcb->is_footprint)
		return layer;

	sc = pcb_subclist_first(&pcb->Data->subc);
	if (sc == NULL)
		return layer;

	for(n = 0; n < sc->data->LayerN; n++) {
		pcb_layer_t *l = &sc->data->Layer[n];
		if (!l->is_bound)
			continue;
		if (l->meta.bound.real == layer)
			return l;
	}

	/* the subc does not have that layer */
	if (alloc) {
		pcb_layer_t *nlayer = &sc->data->Layer[sc->data->LayerN];
		sc->data->LayerN++;
		pcb_layer_real2bound(nlayer, layer, 1);
		return nlayer;
	}

	return layer;
}

void pcb_subc_as_board_update(pcb_board_t *pcb)
{
	pcb_subc_t *sc;

	if (!pcb->is_footprint)
		return;
	sc = pcb_subclist_first(&pcb->Data->subc);
	if (sc == NULL)
		return;

	pcb_subc_bbox(sc);
}

static pcb_coord_t read_mask(pcb_any_obj_t *obj)
{
	const char *smask = pcb_attribute_get(&obj->Attributes, "elem_smash_pad_mask");
	pcb_coord_t mask = 0;

	if (smask != NULL) {
		pcb_bool success;
		mask = pcb_get_value_ex(smask, NULL, NULL, NULL, "mm", &success);
		if (!success)
			mask = 0;
	}
	return mask;
}

static void get_aux_layer(pcb_subc_t *sc, int alloc)
{
	if (sc->aux_layer == NULL)
		pcb_subc_cache_find_aux(sc, 1);
	pcb_subc_cache_update(sc);
}

void pcb_subc_create_aux(pcb_subc_t *sc, pcb_coord_t ox, pcb_coord_t oy, double rot, pcb_bool bottom)
{
	double unit = PCB_MM_TO_COORD(1);
	double cs, sn;

	get_aux_layer(sc, 1);

	if (rot == 0.0) {
		cs = 1;
		sn = 0;
	}
	else {
		cs = cos(rot/PCB_RAD_TO_DEG);
		sn = sin(rot/PCB_RAD_TO_DEG);
	}

	if (bottom) {
		assert(sc->aux_layer->is_bound);
		sc->aux_layer->meta.bound.type &= ~PCB_LYT_TOP;
		sc->aux_layer->meta.bound.type |= PCB_LYT_BOTTOM;
		cs = -cs;
	}

	add_aux_line(sc->aux_layer, "subc-role", "origin", ox, oy, ox, oy);
	add_aux_line(sc->aux_layer, "subc-role", "x", ox, oy, pcb_round((double)ox + cs*unit), pcb_round(oy + sn*unit));
	add_aux_line(sc->aux_layer, "subc-role", "y", ox, oy, pcb_round((double)ox + sn*unit), pcb_round(oy + cs*unit));
}

void pcb_subc_create_aux_point(pcb_subc_t *sc, pcb_coord_t x, pcb_coord_t y, const char *role)
{
	get_aux_layer(sc, 1);
	add_aux_line(sc->aux_layer, "subc-role", role, x, y, x, y);
}

int pcb_subc_convert_from_buffer(pcb_buffer_t *buffer)
{
	pcb_subc_t *sc;
	int n, top_pads = 0, bottom_pads = 0, has_refdes_text = 0;
	pcb_layer_t *dst_top_mask = NULL, *dst_bottom_mask = NULL, *dst_top_paste = NULL, *dst_bottom_paste = NULL, *dst_top_silk = NULL;
	vtp0_t mask_pads, paste_pads;

	vtp0_init(&mask_pads);
	vtp0_init(&paste_pads);

	sc = pcb_subc_alloc();
	pcb_subc_reg(buffer->Data, sc);

	/* create layer matches and copy objects */
	for(n = 0; n < PCB_MAX_LAYER; n++) {
		pcb_layer_t *dst, *src;
		pcb_line_t *line;
		pcb_text_t *text;
		pcb_poly_t *poly;
		pcb_arc_t *arc;
		pcb_layer_type_t ltype;

		if (pcb_layer_is_pure_empty(&buffer->Data->Layer[n]))
			continue;

		src = &buffer->Data->Layer[n];
		dst = pcb_subc_layer_create_buff(sc, src);
		ltype = dst->meta.bound.type;

		if ((dst->comb & PCB_LYC_SUB) == 0) {
			if ((ltype & PCB_LYT_PASTE) && (ltype & PCB_LYT_TOP))
				dst_top_paste = dst;
			else if ((ltype & PCB_LYT_PASTE) && (ltype & PCB_LYT_BOTTOM))
				dst_bottom_paste = dst;
			else if ((ltype & PCB_LYT_SILK) && (ltype & PCB_LYT_TOP))
				dst_top_silk = dst;
		}

		if (dst->comb & PCB_LYC_SUB) {
			if ((ltype & PCB_LYT_MASK) && (ltype & PCB_LYT_TOP))
				dst_top_mask = dst;
			else if ((ltype & PCB_LYT_MASK) && (ltype & PCB_LYT_BOTTOM))
				dst_bottom_mask = dst;
		}

		while((line = linelist_first(&src->Line)) != NULL) {
			pcb_coord_t mask = 0;
			char *np, *sq, *termpad;
			const char *term;

			termpad = pcb_attribute_get(&line->Attributes, "elem_smash_pad");
			if (termpad != NULL) {
				if (ltype & PCB_LYT_TOP)
					top_pads++;
				else if (ltype & PCB_LYT_BOTTOM)
					bottom_pads++;
				mask = read_mask((pcb_any_obj_t *)line);
			}
			term = pcb_attribute_get(&line->Attributes, "term");
			np = pcb_attribute_get(&line->Attributes, "elem_smash_nopaste");
			sq = pcb_attribute_get(&line->Attributes, "elem_smash_shape_square");
			if ((sq != NULL) && (*sq == '1')) { /* convert to polygon */
				poly = sqline2term(dst, line);
				if (termpad != NULL) {
					if ((np == NULL) || (*np != '1')) {
						poly = sqline2term(dst, line);
						vtp0_append(&paste_pads, poly);
					}
					if (mask > 0) {
						line->Thickness = mask;
						poly = sqline2term(dst, line);
						if (term != NULL)
							pcb_attribute_put(&poly->Attributes, "term", term);
						vtp0_append(&mask_pads, poly);
					}
				}

				pcb_line_free(line);
			}
			else {
				/* copy the line */
				pcb_line_unreg(line);
				pcb_line_reg(dst, line);
				PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, line);

				if (termpad != NULL) {
					pcb_line_t *nl = pcb_line_dup(dst, line);
					if ((np == NULL) || (*np != '1'))
						vtp0_append(&paste_pads, nl);
					if (mask > 0) {
						nl = pcb_line_dup(dst, line);
						nl->Thickness = mask;
						if (term != NULL)
							pcb_attribute_put(&nl->Attributes, "term", term);
						vtp0_append(&mask_pads, nl);
					}
				}
			}
		}

		while((arc = arclist_first(&src->Arc)) != NULL) {
			pcb_arc_unreg(arc);
			pcb_arc_reg(dst, arc);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, arc);
		}

		while((text = textlist_first(&src->Text)) != NULL) {
			pcb_text_unreg(text);
			pcb_text_reg(dst, text);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, text);
			if (!has_refdes_text && (text->TextString != NULL) && (strstr(text->TextString, "%a.parent.refdes%") != NULL))
				has_refdes_text = 1;
		}

		while((poly = polylist_first(&src->Polygon)) != NULL) {
			pcb_poly_unreg(poly);
			pcb_poly_reg(dst, poly);

			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, poly);
		}
	}

	/* create paste and mask side effects - needed when importing from footprint */
	{
		if (top_pads > 0) {
			if (dst_top_paste == NULL)
				dst_top_paste = pcb_layer_new_bound(sc->data, PCB_LYT_TOP | PCB_LYT_PASTE, "top paste", NULL);
			if (dst_top_mask == NULL) {
				dst_top_mask = pcb_layer_new_bound(sc->data, PCB_LYT_TOP | PCB_LYT_MASK, "top mask", NULL);
				dst_top_mask->comb = PCB_LYC_SUB;
			}
		}
		if (bottom_pads > 0) {
			if (dst_bottom_paste == NULL)
				dst_bottom_paste = pcb_layer_new_bound(sc->data, PCB_LYT_BOTTOM | PCB_LYT_PASTE, "bottom paste", NULL);
			if (dst_bottom_mask == NULL) {
				dst_bottom_mask = pcb_layer_new_bound(sc->data, PCB_LYT_BOTTOM | PCB_LYT_MASK, "bottom mask", NULL);
				dst_bottom_mask->comb = PCB_LYC_SUB;
			}
		}
	}

	{ /* convert globals */
		pcb_pstk_t *ps;

		while((ps = padstacklist_first(&buffer->Data->padstack)) != NULL) {
			const pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
			pcb_pstk_unreg(ps);
			pcb_pstk_reg(sc->data, ps);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, ps);
			ps->proto = pcb_pstk_proto_insert_dup(sc->data, proto, 1);
			ps->protoi = -1;
		}
	}

	vtp0_uninit(&mask_pads);
	vtp0_uninit(&paste_pads);

	pcb_subc_create_aux(sc, buffer->X, buffer->Y, 0.0, pcb_false);

	/* Add refdes */
	if ((conf_core.editor.subc_conv_refdes != NULL) && (*conf_core.editor.subc_conv_refdes != '\0')) {
		pcb_attribute_put(&sc->Attributes, "refdes", conf_core.editor.subc_conv_refdes);
		if (!has_refdes_text) {
			if (dst_top_silk == NULL)
				dst_top_silk = pcb_layer_new_bound(sc->data, PCB_LYT_TOP | PCB_LYT_SILK, "top-silk", NULL);
			if (dst_top_silk != NULL)
				pcb_text_new(dst_top_silk, pcb_font(PCB, 0, 0), buffer->X, buffer->Y, 0, 100, 0, "%a.parent.refdes%", pcb_flag_make(PCB_FLAG_DYNTEXT | PCB_FLAG_FLOATER));
			else
				pcb_message(PCB_MSG_ERROR, "Error: can't create top silk layer in subc for placing the refdes\n");
		}
	}


	return 0;
}

static void pcb_subc_draw_origin(pcb_hid_gc_t GC, pcb_subc_t *sc, pcb_coord_t DX, pcb_coord_t DY)
{
	pcb_line_t *origin;
	pcb_subc_cache_update(sc);

	origin = sc->aux_cache[PCB_SUBCH_ORIGIN];

	if (origin == NULL)
		return;

	DX += (origin->Point1.X + origin->Point2.X) / 2;
	DY += (origin->Point1.Y + origin->Point2.Y) / 2;

	pcb_gui->draw_line(GC, DX - PCB_EMARK_SIZE, DY, DX, DY - PCB_EMARK_SIZE);
	pcb_gui->draw_line(GC, DX + PCB_EMARK_SIZE, DY, DX, DY - PCB_EMARK_SIZE);
	pcb_gui->draw_line(GC, DX - PCB_EMARK_SIZE, DY, DX, DY + PCB_EMARK_SIZE);
	pcb_gui->draw_line(GC, DX + PCB_EMARK_SIZE, DY, DX, DY + PCB_EMARK_SIZE);
}

void pcb_xordraw_subc(pcb_subc_t *sc, pcb_coord_t DX, pcb_coord_t DY, int use_curr_side)
{
	int n, mirr;
	pcb_coord_t w = 0, h = 0;

	mirr = use_curr_side && conf_core.editor.show_solder_side;

	/* mirror center */
	if (mirr) {
		pcb_subc_get_origin(sc, &w, &h);
		w *= 2;
		h *= 2;
	}

	/* draw per layer objects */
	for(n = 0; n < sc->data->LayerN; n++) {
		pcb_layer_t *ly = sc->data->Layer + n;
		pcb_line_t *line;
		pcb_text_t *text;
		pcb_poly_t *poly;
		pcb_arc_t *arc;
		gdl_iterator_t it;

		linelist_foreach(&ly->Line, &it, line)
			pcb_draw_wireframe_line(pcb_crosshair.GC, 
															DX + PCB_CSWAP_X(line->Point1.X, w, mirr), 
															DY + PCB_CSWAP_Y(line->Point1.Y, h, mirr), 
															DX + PCB_CSWAP_X(line->Point2.X, w, mirr), 
															DY + PCB_CSWAP_Y(line->Point2.Y, h, mirr),
															line->Thickness,
															0 );

		arclist_foreach(&ly->Arc, &it, arc) {
			if(arc->Width != arc->Height) {
TODO(": The wireframe arc drawing code cannot draw ellipses yet so draw the elliptical arc with a thin line ")
				double sa = mirr ? PCB_SWAP_ANGLE(arc->StartAngle) : arc->StartAngle;
				double da = mirr ? PCB_SWAP_DELTA(arc->Delta) : arc->Delta;
				pcb_gui->draw_arc(pcb_crosshair.GC, DX + PCB_CSWAP_X(arc->X, w, mirr), DY + PCB_CSWAP_Y(arc->Y, h, mirr), arc->Width, arc->Height, sa, da);
			}
			else {
				pcb_arc_t temp_arc;
				temp_arc.StartAngle = mirr ? PCB_SWAP_ANGLE(arc->StartAngle) : arc->StartAngle; 
				temp_arc.Delta = mirr ? PCB_SWAP_DELTA(arc->Delta) : arc->Delta;
				temp_arc.X = DX + PCB_CSWAP_X(arc->X, w, mirr);
				temp_arc.Y = DY + PCB_CSWAP_Y(arc->Y, h, mirr);
				temp_arc.Width = arc->Width;
				temp_arc.Height = arc->Height;
				temp_arc.Thickness = arc->Thickness;
				pcb_draw_wireframe_arc(pcb_crosshair.GC, &temp_arc, arc->Thickness);
			}
		}

		polylist_foreach(&ly->Polygon, &it, poly)
			pcb_xordraw_poly_subc(poly, DX, DY, w, h, mirr);

		textlist_foreach(&ly->Text, &it, text) {
			if (mirr) {
				pcb_text_t t;
				t = *text;
				t.X = PCB_CSWAP_X(text->X, w, mirr);
				t.Y = PCB_CSWAP_Y(text->Y, h, mirr);
				PCB_FLAG_TOGGLE(PCB_FLAG_ONSOLDER, &t);
				pcb_text_draw_xor(&t, DX, DY);
			}
			else
				pcb_text_draw_xor(text, DX, DY);
		}
	}

	/* draw global objects */
	{
		pcb_pstk_t *ps;
		gdl_iterator_t it;

		padstacklist_foreach(&sc->data->padstack, &it, ps) {
			pcb_coord_t ox, oy;
			int oxm, pri;
			ox = ps->x;
			oy = ps->y;
			oxm = ps->xmirror;
			pri = ps->protoi;
			ps->x = PCB_CSWAP_X(ps->x, w, mirr);
			ps->y = PCB_CSWAP_Y(ps->y, h, mirr);
			ps->x += DX;
			ps->y += DY;
			if (mirr) {
				ps->xmirror = !ps->xmirror;
				ps->protoi = -1;
			}
			pcb_pstk_thindraw(NULL, pcb_crosshair.GC, ps);
			ps->x = ox;
			ps->y = oy;
			ps->xmirror = oxm;
			ps->protoi = pri;
		}
	}

	pcb_subc_draw_origin(pcb_crosshair.GC, sc, DX, DY);
}

#define MAYBE_KEEP_ID(dst, src) \
do { \
	if ((keep_ids) && (dst != NULL)) \
		dst->ID = src->ID; \
} while(0)

static pcb_subc_t *pcb_subc_copy_meta(pcb_subc_t *dst, pcb_subc_t *src)
{
	if (dst == NULL)
		return NULL;
	pcb_attribute_copy_all(&dst->Attributes, &src->Attributes);
	return dst;
}


pcb_subc_t *pcb_subc_dup_at(pcb_board_t *pcb, pcb_data_t *dst, pcb_subc_t *src, pcb_coord_t dx, pcb_coord_t dy, pcb_bool keep_ids)
{
	pcb_board_t *src_pcb;
	int n;
	pcb_subc_t *sc = pcb_subc_alloc();

	if (keep_ids)
		sc->ID = src->ID;
	else
		sc->ID = pcb_create_ID_get();

	minuid_cpy(sc->uid, src->uid);
	pcb_subc_reg(dst, sc);

	src_pcb = pcb_data_get_top(src->data);

	pcb_subc_copy_meta(sc, src);

	sc->BoundingBox.X1 = sc->BoundingBox.Y1 = PCB_MAX_COORD;
	sc->BoundingBox.X2 = sc->BoundingBox.Y2 = -PCB_MAX_COORD;
	sc->bbox_naked.X1 = sc->bbox_naked.Y1 = PCB_MAX_COORD;
	sc->bbox_naked.X2 = sc->bbox_naked.Y2 = -PCB_MAX_COORD;

	/* make a copy of layer data */
	for(n = 0; n < src->data->LayerN; n++) {
		pcb_layer_t *sl = src->data->Layer + n;
		pcb_layer_t *dl = sc->data->Layer + n;
		pcb_line_t *line, *nline;
		pcb_text_t *text, *ntext;
		pcb_arc_t *arc, *narc;
		gdl_iterator_t it;

		/* bind layer/resolve layers */
		dl->is_bound = 1;
		dl->type = PCB_OBJ_LAYER;
		if ((pcb != NULL) && (pcb == src_pcb)) {
			/* copy within the same board */
			memcpy(&dl->meta.bound, &sl->meta.bound, sizeof(sl->meta.bound));
			dl->name = pcb_strdup(sl->name);
			dl->comb = sl->comb;
			if (dl->meta.bound.real != NULL)
				pcb_layer_link_trees(dl, dl->meta.bound.real);
		}
		else if (pcb != NULL) {
			/* copying from buffer to board */
			memcpy(&dl->meta.bound, &sl->meta.bound, sizeof(sl->meta.bound));
			dl->name = pcb_strdup(sl->name);
			dl->meta.bound.real = pcb_layer_resolve_binding(pcb, sl);
			dl->comb = sl->comb;

			if (dl->meta.bound.real == NULL) {
				if (!(dl->meta.bound.type & PCB_LYT_VIRTUAL)) {
					const char *name = dl->name;
					if (name == NULL) name = "<anonymous>";
					pcb_message(PCB_MSG_WARNING, "Couldn't bind a layer %s of subcircuit while placing it\n", name);
				}
			}
			else
				pcb_layer_link_trees(dl, dl->meta.bound.real);
		}
		else {
			/* copying from board to buffer */
			memcpy(&dl->meta.bound, &sl->meta.bound, sizeof(sl->meta.bound));

			dl->meta.bound.real = NULL;
			dl->name = pcb_strdup(sl->name);
			dl->comb = sl->comb;
		}


		linelist_foreach(&sl->Line, &it, line) {
			nline = pcb_line_dup_at(dl, line, dx, dy);
			MAYBE_KEEP_ID(nline, line);
			if (nline != NULL) {
				PCB_SET_PARENT(nline, layer, dl);
				if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, nline)) {
					pcb_box_bump_box_noflt(&sc->BoundingBox, &nline->BoundingBox);
					pcb_box_bump_box_noflt(&sc->bbox_naked, &nline->bbox_naked);
				}
			}
		}

		arclist_foreach(&sl->Arc, &it, arc) {
			narc = pcb_arc_dup_at(dl, arc, dx, dy);
			MAYBE_KEEP_ID(narc, arc);
			if (narc != NULL) {
				PCB_SET_PARENT(narc, layer, dl);
				if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, narc)) {
					pcb_box_bump_box_noflt(&sc->BoundingBox, &narc->BoundingBox);
					pcb_box_bump_box_noflt(&sc->bbox_naked, &narc->bbox_naked);
				}
			}
		}

		textlist_foreach(&sl->Text, &it, text) {
			ntext = pcb_text_dup_at(dl, text, dx, dy);
			MAYBE_KEEP_ID(ntext, text);
			if (ntext != NULL) {
				PCB_SET_PARENT(ntext, layer, dl);
				if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, ntext)) {
					pcb_box_bump_box_noflt(&sc->BoundingBox, &ntext->BoundingBox);
					pcb_box_bump_box_noflt(&sc->bbox_naked, &ntext->bbox_naked);
				}
			}
		}

	}
	sc->data->LayerN = src->data->LayerN;

	/* bind the via rtree so that vias added in this subc show up on the board */
	if (pcb != NULL) {
		if (pcb->Data->padstack_tree == NULL)
			pcb->Data->padstack_tree = pcb_r_create_tree();
		sc->data->padstack_tree = pcb->Data->padstack_tree;
	}

	{ /* make a copy of global data */
		pcb_pstk_t *ps, *nps;
		gdl_iterator_t it;

		padstacklist_foreach(&src->data->padstack, &it, ps) {
			pcb_cardinal_t pid = pcb_pstk_proto_insert_dup(sc->data, pcb_pstk_get_proto(ps), 1);
			nps = pcb_pstk_new_tr(sc->data, -1, pid, ps->x+dx, ps->y+dy, ps->Clearance, ps->Flags, ps->rot, ps->xmirror, ps->smirror);
			pcb_pstk_copy_meta(nps, ps);
			MAYBE_KEEP_ID(nps, ps);
			if (nps != NULL) {
				if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, nps)) {
					pcb_box_bump_box_noflt(&sc->BoundingBox, &nps->BoundingBox);
					pcb_box_bump_box_noflt(&sc->bbox_naked, &nps->bbox_naked);
				}
			}
		}
	}

	/* make a copy of polygons at the end so clipping caused by other objects are calculated only once */
	for(n = 0; n < src->data->LayerN; n++) {
		pcb_layer_t *sl = src->data->Layer + n;
		pcb_layer_t *dl = sc->data->Layer + n;
		pcb_poly_t *poly, *npoly;
		gdl_iterator_t it;

		polylist_foreach(&sl->Polygon, &it, poly) {
			npoly = pcb_poly_dup_at(dl, poly, dx, dy);
			MAYBE_KEEP_ID(npoly, poly);
			if (npoly != NULL) {
				PCB_SET_PARENT(npoly, layer, dl);
				if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, npoly)) {
					pcb_box_bump_box_noflt(&sc->BoundingBox, &npoly->BoundingBox);
					pcb_box_bump_box_noflt(&sc->bbox_naked, &npoly->bbox_naked);
				}
				pcb_poly_ppclear(npoly);
			}
		}
	}

	memcpy(&sc->Flags, &src->Flags, sizeof(sc->Flags));

	pcb_close_box(&sc->BoundingBox);
	pcb_close_box(&sc->bbox_naked);

	if (pcb != NULL) {
		if (!dst->subc_tree)
			dst->subc_tree = pcb_r_create_tree();
		pcb_r_insert_entry(dst->subc_tree, (pcb_box_t *)sc);
	}

	return sc;
}

pcb_subc_t *pcb_subc_dup(pcb_board_t *pcb, pcb_data_t *dst, pcb_subc_t *src)
{
	return pcb_subc_dup_at(pcb, dst, src, 0, 0, pcb_false);
}

void pcb_subc_bbox(pcb_subc_t *sc)
{
	pcb_data_bbox(&sc->BoundingBox, sc->data, pcb_true);
	pcb_data_bbox_naked(&sc->bbox_naked, sc->data, pcb_true);
}


/* erases a subc on a layer */
void EraseSubc(pcb_subc_t *sc)
{
	pcb_draw_invalidate(sc);
/*	pcb_flag_erase(&sc->Flags); ??? */
}

void DrawSubc(pcb_subc_t *sc)
{
	pcb_draw_invalidate(sc);
}


/* Execute an operation on all children on an sc and update the bbox of the sc */
void *pcb_subc_op(pcb_data_t *Data, pcb_subc_t *sc, pcb_opfunc_t *opfunc, pcb_opctx_t *ctx, pcb_subc_op_undo_t batch_undo)
{
	int n;

	switch(batch_undo) {
		case PCB_SUBCOP_UNDO_SUBC:
			pcb_undo_freeze_serial();
			pcb_undo_freeze_add();
			break;
		case PCB_SUBCOP_UNDO_BATCH:
			pcb_undo_inc_serial();
			pcb_undo_save_serial();
			pcb_undo_freeze_serial();
			break;
	}

	EraseSubc(sc);
	if (pcb_data_get_top(Data) != NULL) {
		if (Data->subc_tree != NULL)
			pcb_r_delete_entry(Data->subc_tree, (pcb_box_t *)sc);
		else
			Data->subc_tree = pcb_r_create_tree();
	}

	/* for calculating the new bounding box on the fly */
	sc->BoundingBox.X1 = sc->BoundingBox.Y1 = PCB_MAX_COORD;
	sc->BoundingBox.X2 = sc->BoundingBox.Y2 = -PCB_MAX_COORD;
	sc->bbox_naked.X1 = sc->bbox_naked.Y1 = PCB_MAX_COORD;
	sc->bbox_naked.X2 = sc->bbox_naked.Y2 = -PCB_MAX_COORD;

	/* execute on layer locals */
	for(n = 0; n < sc->data->LayerN; n++) {
		pcb_layer_t *sl = sc->data->Layer + n;
		pcb_line_t *line;
		pcb_text_t *text;
		pcb_poly_t *poly;
		pcb_arc_t *arc;
		gdl_iterator_t it;


		linelist_foreach(&sl->Line, &it, line) {
			pcb_object_operation(opfunc, ctx, PCB_OBJ_LINE, sl, line, line);
			if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, line)) {
				pcb_box_bump_box_noflt(&sc->BoundingBox, &line->BoundingBox);
				pcb_box_bump_box_noflt(&sc->bbox_naked, &line->bbox_naked);
			}
		}

		arclist_foreach(&sl->Arc, &it, arc) {
			pcb_object_operation(opfunc, ctx, PCB_OBJ_ARC, sl, arc, arc);
			if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, arc)) {
				pcb_box_bump_box_noflt(&sc->BoundingBox, &arc->BoundingBox);
				pcb_box_bump_box_noflt(&sc->bbox_naked, &arc->bbox_naked);
			}
		}

		textlist_foreach(&sl->Text, &it, text) {
			pcb_object_operation(opfunc, ctx, PCB_OBJ_TEXT, sl, text, text);
			if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, text)) {
				pcb_box_bump_box_noflt(&sc->BoundingBox, &text->BoundingBox);
				pcb_box_bump_box_noflt(&sc->bbox_naked, &text->bbox_naked);
			}
		}

		polylist_foreach(&sl->Polygon, &it, poly) {
			pcb_object_operation(opfunc, ctx, PCB_OBJ_POLY, sl, poly, poly);
			if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, poly)) {
				pcb_box_bump_box_noflt(&sc->BoundingBox, &poly->BoundingBox);
				pcb_box_bump_box_noflt(&sc->bbox_naked, &poly->bbox_naked);
			}
		}

	}


	/* execute on globals */
	{
		pcb_pstk_t *ps;
		gdl_iterator_t it;

		padstacklist_foreach(&sc->data->padstack, &it, ps) {
			pcb_object_operation(opfunc, ctx, PCB_OBJ_PSTK, ps, ps, ps);
			if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, ps)) {
				pcb_box_bump_box_noflt(&sc->BoundingBox, &ps->BoundingBox);
				pcb_box_bump_box_noflt(&sc->bbox_naked, &ps->bbox_naked);
			}
		}
	}

	pcb_close_box(&sc->BoundingBox);
	pcb_close_box(&sc->bbox_naked);
	if (pcb_data_get_top(Data) != NULL)
		pcb_r_insert_entry(Data->subc_tree, (pcb_box_t *)sc);
	DrawSubc(sc);

	switch(batch_undo) {
		case PCB_SUBCOP_UNDO_SUBC: pcb_undo_unfreeze_add(); pcb_undo_unfreeze_serial(); break;
		case PCB_SUBCOP_UNDO_BATCH: pcb_undo_unfreeze_serial(); break;
	}

	return sc;
}


/* copies a subcircuit onto the PCB (a.k.a "paste"). Then does a draw. */
void *pcb_subcop_copy(pcb_opctx_t *ctx, pcb_subc_t *src)
{
	pcb_subc_t *sc;

	sc = pcb_subc_dup_at(PCB, PCB->Data, src, ctx->copy.DeltaX, ctx->copy.DeltaY, pcb_false);

	pcb_undo_add_obj_to_create(PCB_OBJ_SUBC, sc, sc, sc);

	if (ctx->copy.from_outside && conf_core.editor.show_solder_side) {
		uundo_serial_t last;

		/* move-to-the-other-side is not undoable: it's part of the placement */
		pcb_undo_inc_serial();
		last = pcb_undo_serial();
TODO("subc: should not depend on crosshair, because of automatic/scripted placement; test case 1: load subc footprint in buffer, swap side to bottom, place; test case 2: bug_files/cmd_element, execute the cmd while being on the bottom side, without crosshair set subcircuits catapult to negative y")
		pcb_subc_change_side(sc, 2 * pcb_crosshair.Y - PCB->MaxHeight);
		pcb_undo_truncate_from(last);
		
	}

	pcb_text_dyn_bbox_update(sc->data);

	return sc;
}

extern pcb_opfunc_t ClipFunctions, MoveFunctions, MoveFunctions_noclip, Rotate90Functions, RotateFunctions, ChgFlagFunctions, ChangeSizeFunctions, ChangeClearSizeFunctions, Change1stSizeFunctions, Change2ndSizeFunctions;

/* drag&drop move when not selected */
void *pcb_subcop_move(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_board_t *pcb = pcb_data_get_top(sc->data);
	pcb_data_t *data = (pcb != NULL ? pcb->Data : NULL);
	pcb_opctx_t clip;

	/* restore all pins/pads at once, at the old location */
	clip.clip.restore = 1; clip.clip.clear = 0;
	clip.clip.pcb = ctx->move.pcb;
	pcb_subc_op(data, sc, &ClipFunctions, &clip, PCB_SUBCOP_UNDO_NORMAL);

	/* do the move without messing with the clipping */
	pcb_subc_op(data, sc, &MoveFunctions_noclip, ctx, PCB_SUBCOP_UNDO_NORMAL);

	/* clear all pins/pads at once, at the new location */
	clip.clip.restore = 0; clip.clip.clear = 1;
	pcb_subc_op(data, sc, &ClipFunctions, &clip, PCB_SUBCOP_UNDO_NORMAL);
	return sc;
}

void *pcb_subcop_rotate90(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_board_t *pcb = pcb_data_get_top(sc->data);
	return pcb_subc_op((pcb != NULL ? pcb->Data : NULL), sc, &Rotate90Functions, ctx, PCB_SUBCOP_UNDO_SUBC);
}

void *pcb_subcop_rotate(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_data_t *data;

	ctx->rotate.pcb = pcb_data_get_top(sc->data);
	data = (ctx->rotate.pcb != NULL ? ctx->rotate.pcb->Data : NULL);

	return pcb_subc_op(data, sc, &RotateFunctions, ctx, PCB_SUBCOP_UNDO_SUBC);
}

void pcb_subc_rotate90(pcb_subc_t *subc, pcb_coord_t cx, pcb_coord_t cy, int steps)
{
	pcb_opctx_t ctx;

	ctx.rotate.center_x = cx;
	ctx.rotate.center_y = cy;
	ctx.rotate.number = steps;
	pcb_subcop_rotate90(&ctx, subc);
}

void pcb_subc_rotate(pcb_subc_t *subc, pcb_coord_t cx, pcb_coord_t cy, double cosa, double sina, double angle)
{
	pcb_opctx_t ctx;

	ctx.rotate.center_x = cx;
	ctx.rotate.center_y = cy;
	ctx.rotate.angle = angle;
	ctx.rotate.cosa = cosa;
	ctx.rotate.sina = sina;
	pcb_subcop_rotate(&ctx, subc);
}

void pcb_subc_move(pcb_subc_t *sc, pcb_coord_t dx, pcb_coord_t dy, pcb_bool more_to_come)
{
	pcb_opctx_t ctx;
	ctx.move.pcb = NULL;
	ctx.move.dx = dx;
	ctx.move.dy = dy;
	ctx.move.more_to_come = more_to_come;
	pcb_subcop_move(&ctx, sc);
}

pcb_bool pcb_selected_subc_change_side(void)
{
	pcb_bool change = pcb_false;

	if (PCB->pstk_on && pcb_silk_on(PCB)) {
		PCB_SUBC_LOOP(PCB->Data);
		{
			if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc)) {
				change |= pcb_subc_change_side(subc, 2 * pcb_crosshair.Y - PCB->MaxHeight);
			}
		}
		PCB_END_LOOP;
	}
	return change;
}


static int subc_relocate_layer_objs(pcb_layer_t *dl, pcb_data_t *src_data, pcb_layer_t *sl, int src_has_real_layer, int dst_is_pcb)
{
	pcb_line_t *line;
	pcb_text_t *text;
	pcb_poly_t *poly;
	pcb_arc_t *arc;
	gdl_iterator_t it;
	int chg = 0;
	pcb_data_t *dst_data = dl == NULL ? NULL : dl->parent.data;

	linelist_foreach(&sl->Line, &it, line) {
		if (src_has_real_layer) {
			pcb_poly_restore_to_poly(src_data, PCB_OBJ_LINE, sl, line);
			pcb_r_delete_entry(sl->line_tree, (pcb_box_t *)line);
			chg++;
		}
		PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, line);
		if ((dl != NULL) && (dl->line_tree != NULL)) {
			pcb_r_insert_entry(dl->line_tree, (pcb_box_t *)line);
			chg++;
		}
		if (dst_is_pcb && (dl != NULL))
			pcb_poly_clear_from_poly(dst_data, PCB_OBJ_LINE, dl, line);
	}

	arclist_foreach(&sl->Arc, &it, arc) {
		if (src_has_real_layer) {
			pcb_poly_restore_to_poly(src_data, PCB_OBJ_ARC, sl, arc);
			pcb_r_delete_entry(sl->arc_tree, (pcb_box_t *)arc);
			chg++;
		}
		PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, arc);
		if ((dl != NULL) && (dl->arc_tree != NULL)) {
			pcb_r_insert_entry(dl->arc_tree, (pcb_box_t *)arc);
			chg++;
		}
		if (dst_is_pcb && (dl != NULL))
			pcb_poly_clear_from_poly(dst_data, PCB_OBJ_ARC, dl, arc);
	}

	textlist_foreach(&sl->Text, &it, text) {
		if (src_has_real_layer) {
			pcb_poly_restore_to_poly(src_data, PCB_OBJ_LINE, sl, text);
			pcb_r_delete_entry(sl->text_tree, (pcb_box_t *)text);
			chg++;
		}
		PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, text);
		if ((dl != NULL) && (dl->text_tree != NULL)) {
			pcb_r_insert_entry(dl->text_tree, (pcb_box_t *)text);
			chg++;
		}
		if (dst_is_pcb && (dl != NULL))
			pcb_poly_clear_from_poly(dst_data, PCB_OBJ_TEXT, dl, text);
	}

	polylist_foreach(&sl->Polygon, &it, poly) {
		pcb_poly_pprestore(poly);
		if (src_has_real_layer) {
			pcb_r_delete_entry(sl->polygon_tree, (pcb_box_t *)poly);
			chg++;
		}
		PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, poly);
		if ((dl != NULL) && (dl->polygon_tree != NULL)) {
			pcb_r_insert_entry(dl->polygon_tree, (pcb_box_t *)poly);
			chg++;
		}
		if (dst_is_pcb && (dl != NULL))
			pcb_poly_ppclear_at(poly, dl);
	}

	if (!dst_is_pcb) {
		/* keep only the layer binding match, unbound other aspects */
		sl->meta.bound.real = NULL;
		sl->arc_tree = sl->line_tree = sl->text_tree = sl->polygon_tree = NULL;
		chg++;
	}
	else {
		sl->meta.bound.real = dl;
		chg++;
	}
	return chg;
}

static int subc_relocate_globals(pcb_data_t *dst, pcb_data_t *new_parent, pcb_subc_t *sc, int dst_is_pcb)
{
	pcb_pstk_t *ps;
	gdl_iterator_t it;
	int chg = 0;

	padstacklist_foreach(&sc->data->padstack, &it, ps) {
		const pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
		assert(proto != NULL); /* the prototype must be accessible at the source else we can't add it in the dest */
		pcb_poly_restore_to_poly(ps->parent.data, PCB_OBJ_PSTK, NULL, ps);
		if (sc->data->padstack_tree != NULL)
			pcb_r_delete_entry(sc->data->padstack_tree, (pcb_box_t *)ps);
		PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, ps);
		if ((dst != NULL) && (dst->padstack_tree != NULL))
			pcb_r_insert_entry(dst->padstack_tree, (pcb_box_t *)ps);
		if (dst != NULL)
			ps->proto = pcb_pstk_proto_insert_dup(ps->parent.data, proto, 1);
		ps->protoi = -1;
		ps->parent.data = new_parent;
		if (dst_is_pcb)
			pcb_poly_clear_from_poly(ps->parent.data, PCB_OBJ_PSTK, NULL, ps);
		chg++;
	}

	/* bind globals */
	if ((dst != NULL) && (dst_is_pcb)) {
		if (dst->padstack_tree == NULL)
			dst->padstack_tree = pcb_r_create_tree();
		sc->data->padstack_tree = dst->padstack_tree;
		chg++;
	}
	else
		sc->data->padstack_tree = NULL;
	return chg;
}

/* change the parent of all global objects to new_parent */
static void subc_set_parent_globals(pcb_subc_t *sc, pcb_data_t *new_parent)
{
	pcb_data_set_parent_globals(sc->data, new_parent);
}

void *pcb_subcop_move_buffer(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	int n;
	pcb_board_t *dst_top = pcb_data_get_top(ctx->buffer.dst);
	int dst_is_pcb = ((dst_top != NULL) && (dst_top->Data == ctx->buffer.dst));

	EraseSubc(sc);

	/* move the subc */
	if ((ctx->buffer.pcb != NULL) && (ctx->buffer.pcb->Data->subc_tree != NULL))
		pcb_r_delete_entry(ctx->buffer.pcb->Data->subc_tree, (pcb_box_t *)sc);

	pcb_subc_unreg(sc);
	pcb_subc_reg(ctx->buffer.dst, sc);

	if (dst_is_pcb) {
		if (ctx->buffer.dst->subc_tree == NULL)
			ctx->buffer.dst->subc_tree = pcb_r_create_tree();
		pcb_r_insert_entry(ctx->buffer.dst->subc_tree, (pcb_box_t *)sc);
	}

	if (dst_is_pcb)
		PCB_SET_PARENT(sc, data, ctx->buffer.dst);  /* have to set sc parent before relocate globals so that poly clearings will work */

	/* move layer local */
	for(n = 0; n < sc->data->LayerN; n++) {
		pcb_layer_t *sl = sc->data->Layer + n;
		pcb_layer_t *dl;
		int src_has_real_layer = (sl->meta.bound.real != NULL);

		if (dst_is_pcb) {
			dl = pcb_layer_resolve_binding(dst_top, sl);
			if (dl != NULL) {
				pcb_layer_link_trees(sl, dl);
			}
			else {
				/* need to create the trees so that move and other ops work */
				if (sl->line_tree == NULL) sl->line_tree = pcb_r_create_tree();
				if (sl->arc_tree == NULL) sl->arc_tree = pcb_r_create_tree();
				if (sl->text_tree == NULL) sl->text_tree = pcb_r_create_tree();
				if (sl->polygon_tree == NULL) sl->polygon_tree = pcb_r_create_tree();
				if (!(sl->meta.bound.type & PCB_LYT_VIRTUAL))
					pcb_message(PCB_MSG_ERROR, "Couldn't bind subc layer %s on buffer move\n", sl->name == NULL ? "<anonymous>" : sl->name);
			}
		}
		else
			dl = ctx->buffer.dst->Layer + n;

		subc_relocate_layer_objs(dl, ctx->buffer.src, sl, src_has_real_layer, dst_is_pcb);
	}

	subc_relocate_globals(ctx->buffer.dst, sc->data, sc, dst_is_pcb);

	PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, sc);
	return sc;
}

void *pcb_subcop_add_to_buffer(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_t *nsc;
	nsc = pcb_subc_dup_at(NULL, ctx->buffer.dst, sc, 0, 0, pcb_false);

	if (ctx->buffer.extraflg & PCB_FLAG_SELECTED) {
		pcb_undo_freeze_serial();
		pcb_undo_freeze_add();
		pcb_subc_select(NULL, nsc, PCB_CHGFLG_CLEAR, 0);
		pcb_undo_unfreeze_add();
		pcb_undo_unfreeze_serial();
	}

	return nsc;
}

/* break buffer subc into pieces */
pcb_bool pcb_subc_smash_buffer(pcb_buffer_t *buff)
{
	pcb_subc_t *subc;
	int n;

	if (pcb_subclist_length(&buff->Data->subc) != 1)
		return pcb_false;

	subc = pcb_subclist_first(&buff->Data->subc);
	pcb_subc_unreg(subc);

	/* relocate to NULL to get trees detached */
	for(n = 0; n < subc->data->LayerN; n++) {
		pcb_layer_t *sl = subc->data->Layer + n;
		int src_has_real_layer = (sl->meta.bound.real != NULL);
		subc_relocate_layer_objs(NULL, subc->data, sl, src_has_real_layer, 0);
	}
	subc_relocate_globals(NULL, NULL, subc, 0);

	pcb_data_free(buff->Data);
	buff->Data = subc->data;
	buff->Data->parent_type = PCB_PARENT_INVALID;
	buff->Data->parent.data = NULL;

	subc_set_parent_globals(subc, buff->Data);

	return pcb_true;
}

int pcb_subc_rebind(pcb_board_t *pcb, pcb_subc_t *sc)
{
	int n, chgly = 0;
	pcb_board_t *dst_top = pcb_data_get_top(sc->data);
	int dst_is_pcb = ((dst_top != NULL) && (dst_top->Data == pcb->Data));

	if (!dst_is_pcb)
		return -1;

	EraseSubc(sc);

/* relocate bindings */
	for(n = 0; n < sc->data->LayerN; n++) {
		pcb_layer_t *sl = sc->data->Layer + n;
		pcb_layer_t *dl = pcb_layer_resolve_binding(dst_top, sl);
		int src_has_real_layer = (sl->meta.bound.real != NULL);

		if (dl != NULL) {
			if (sl->meta.bound.real == dl)
				continue;

			/* make sure all trees exist on the dest layer - if these are the first objects there we may need to create them */
			if (dl->line_tree == NULL) dl->line_tree = pcb_r_create_tree();
			if (dl->arc_tree == NULL) dl->arc_tree = pcb_r_create_tree();
			if (dl->text_tree == NULL) dl->text_tree = pcb_r_create_tree();
			if (dl->polygon_tree == NULL) dl->polygon_tree = pcb_r_create_tree();
		}

		if (subc_relocate_layer_objs(dl, pcb->Data, sl, src_has_real_layer, 1) > 0)
			chgly++;

		if (dl != NULL)
			pcb_layer_link_trees(sl, dl);
	}

	return chgly;
}

void pcb_subc_bind_globals(pcb_board_t *pcb, pcb_subc_t *subc)
{
	if (pcb->Data->padstack_tree == NULL)
		pcb->Data->padstack_tree = pcb_r_create_tree();
	subc->data->padstack_tree = pcb->Data->padstack_tree;
TODO("subc: subc-in-subc: bind subc rtree")
}

void *pcb_subcop_change_size(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &ChangeSizeFunctions, ctx, PCB_SUBCOP_UNDO_BATCH);
	return sc;
}

void *pcb_subcop_change_clear_size(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &ChangeClearSizeFunctions, ctx, PCB_SUBCOP_UNDO_BATCH);
	return sc;
}

void *pcb_subcop_change_1st_size(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &Change1stSizeFunctions, ctx, PCB_SUBCOP_UNDO_BATCH);
	return sc;
}

void *pcb_subcop_change_2nd_size(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &Change2ndSizeFunctions, ctx, PCB_SUBCOP_UNDO_BATCH);
	return sc;
}

void *pcb_subcop_change_nonetlist(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
TODO("subc: add undo")
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, sc))
		return NULL;
	PCB_FLAG_TOGGLE(PCB_FLAG_NONETLIST, sc);
	return sc;
}

void *pcb_subcop_change_name(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	void *old;
	if (sc->refdes != NULL)
		old = pcb_strdup(sc->refdes); /* strdup because the pcb_attribute_put() is going to free the original */
	else
		old = NULL;
	pcb_attribute_put(&sc->Attributes, "refdes", ctx->chgname.new_name);
	return old;
}

void *pcb_subcop_destroy(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	if (ctx->remove.pcb->Data->subc_tree != NULL)
		pcb_r_delete_entry(ctx->remove.pcb->Data->subc_tree, (pcb_box_t *)sc);

	EraseSubc(sc);
	pcb_subc_free(sc);
	return NULL;
}

void *pcb_subcop_remove(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	EraseSubc(sc);
	pcb_undo_move_obj_to_remove(PCB_OBJ_SUBC, sc, sc, sc);
	return NULL;
}

void *pcb_subc_remove(pcb_subc_t *sc)
{
	void *res;
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.destroy_target = NULL;
/*	PCB_CLEAR_PARENT(subc);*/

	res = pcb_subcop_remove(&ctx, sc);
	pcb_draw();
	return res;
}

void *pcb_subcop_change_flag(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	static pcb_flag_values_t pcb_subc_flags = 0;
	if (pcb_subc_flags == 0)
		pcb_subc_flags = pcb_obj_valid_flags(PCB_OBJ_SUBC);

	if (ctx->chgflag.flag == PCB_FLAG_FLOATER) {
TODO("subc: subc-in-subc: can a whole subc be a floater? - add undo!")
		PCB_FLAG_CHANGE(ctx->chgflag.how, ctx->chgflag.flag, sc);
		return sc;
	}

	pcb_subc_op(ctx->chgflag.pcb->Data, sc, &ChgFlagFunctions, ctx, PCB_SUBCOP_UNDO_NORMAL);
	if ((ctx->chgflag.flag & pcb_subc_flags) == ctx->chgflag.flag)
		PCB_FLAG_CHANGE(ctx->chgflag.how, ctx->chgflag.flag, sc);
	return sc;
}


void pcb_subc_select(pcb_board_t *pcb, pcb_subc_t *sc, pcb_change_flag_t how, int redraw)
{
	pcb_opctx_t ctx;

	pcb_undo_add_obj_to_flag(sc);

	ctx.chgflag.pcb = pcb;
	ctx.chgflag.how = how;
	ctx.chgflag.flag = PCB_FLAG_SELECTED;

	pcb_subc_op((pcb == NULL ? NULL : pcb->Data), sc, &ChgFlagFunctions, &ctx, PCB_SUBCOP_UNDO_NORMAL);
	PCB_FLAG_CHANGE(how, PCB_FLAG_SELECTED, sc);
	if (redraw)
		DrawSubc(sc);
}

/* mirrors the coordinates of a subcircuit; an additional offset is passed */
void pcb_subc_mirror(pcb_data_t *data, pcb_subc_t *subc, pcb_coord_t y_offs, pcb_bool smirror)
{
	if ((data != NULL) && (data->subc_tree != NULL))
		pcb_r_delete_entry(data->subc_tree, (pcb_box_t *)subc);

	pcb_undo_freeze_add();
	pcb_data_mirror(subc->data, y_offs, smirror ? PCB_TXM_SIDE : PCB_TXM_COORD, smirror);
	pcb_undo_unfreeze_add();
	pcb_subc_bbox(subc);

	if ((data != NULL) && (data->subc_tree != NULL))
		pcb_r_insert_entry(data->subc_tree, (pcb_box_t *)subc);
}

void pcb_subc_scale(pcb_data_t *data, pcb_subc_t *subc, double sx, double sy, double sth, int recurse)
{
	if ((data != NULL) && (data->subc_tree != NULL))
		pcb_r_delete_entry(data->subc_tree, (pcb_box_t *)subc);

	pcb_data_scale(subc->data, sx, sy, sth, recurse);
	pcb_subc_bbox(subc);

	if ((data != NULL) && (data->subc_tree != NULL))
		pcb_r_insert_entry(data->subc_tree, (pcb_box_t *)subc);
}


pcb_bool pcb_subc_change_side(pcb_subc_t *subc, pcb_coord_t yoff)
{
	int n;
	pcb_board_t *pcb;
	pcb_data_t *data;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, subc))
		return pcb_false;

	assert(subc->parent_type = PCB_PARENT_DATA);
	data = subc->parent.data;
	pcb = pcb_data_get_top(data);

	/* mirror object geometry and stackup */

	if ((data != NULL) && (data->subc_tree != NULL))
		pcb_r_delete_entry(data->subc_tree, (pcb_box_t *)subc);

	pcb_undo_freeze_add();
	pcb_data_mirror(subc->data, yoff, PCB_TXM_SIDE, 1);
	pcb_undo_unfreeze_add();

	for(n = 0; n < subc->data->LayerN; n++) {
		pcb_layer_t *ly = subc->data->Layer + n;
		ly->meta.bound.type = pcb_layer_mirror_type(ly->meta.bound.type);
		ly->meta.bound.stack_offs = -ly->meta.bound.stack_offs;
	}
	pcb_subc_rebind(pcb, subc);

	pcb_subc_bbox(subc);

	if ((data != NULL) && (data->subc_tree != NULL))
		pcb_r_insert_entry(data->subc_tree, (pcb_box_t *)subc);

	pcb_undo_add_subc_to_otherside(PCB_OBJ_SUBC, subc, subc, subc, yoff);

	return pcb_true;
}


	/* in per-side mode, do not show subc mark if it is on the other side */
#define draw_subc_per_layer() \
do { \
	int per_side = 0; \
	if (cl != NULL) \
		per_side = *(int *)cl; \
	if (per_side) { \
		int on_bottom; \
		if (pcb_subc_get_side(subc, &on_bottom) == 0) { \
			if ((!!on_bottom) != (!!conf_core.editor.show_solder_side)) \
				return PCB_R_DIR_FOUND_CONTINUE; \
		} \
	} \
} while(0)


#include "conf_core.h"
#include "draw.h"
pcb_r_dir_t draw_subc_mark_callback(const pcb_box_t *b, void *cl)
{
	pcb_draw_info_t *info = cl;
	pcb_subc_t *subc = (pcb_subc_t *) b;
	pcb_box_t *bb = &subc->BoundingBox;
	int selected = PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc);
	int freq = conf_core.appearance.subc.dash_freq;
	const pcb_color_t *nnclr;

	draw_subc_per_layer();

	if (freq > 32)
		freq = 32;

	nnclr = (PCB_FLAG_TEST(PCB_FLAG_NONETLIST, subc)) ? &conf_core.appearance.color.subc_nonetlist : &conf_core.appearance.color.subc;
	pcb_gui->set_color(pcb_draw_out.fgGC, selected ? &conf_core.appearance.color.selected : nnclr);
	pcb_subc_draw_origin(pcb_draw_out.fgGC, subc, 0, 0);

	if (freq >= 0) {
		pcb_draw_dashed_line(info, pcb_draw_out.fgGC, bb->X1, bb->Y1, bb->X2, bb->Y1, freq, pcb_true);
		pcb_draw_dashed_line(info, pcb_draw_out.fgGC, bb->X1, bb->Y1, bb->X1, bb->Y2, freq, pcb_true);
		pcb_draw_dashed_line(info, pcb_draw_out.fgGC, bb->X2, bb->Y2, bb->X2, bb->Y1, freq, pcb_true);
		pcb_draw_dashed_line(info, pcb_draw_out.fgGC, bb->X2, bb->Y2, bb->X1, bb->Y2, freq, pcb_true);
	}

	return PCB_R_DIR_FOUND_CONTINUE;
}

pcb_r_dir_t draw_subc_label_callback(const pcb_box_t *b, void *cl)
{
	pcb_draw_info_t *info = cl;
	pcb_subc_t *subc = (pcb_subc_t *) b;
	pcb_box_t *bb = &subc->BoundingBox;
	pcb_coord_t x0, y0, dx, dy;
	pcb_font_t *font = &PCB->fontkit.dflt;

	/* do not display anyting from the other-side subcs */
	draw_subc_per_layer();

	/* calculate where the label ends up - always top-right, even if board is flipped */
	dx = font->MaxWidth/2;
	dy = font->MaxHeight/2;

	if (conf_core.editor.view.flip_x) {
		x0 = bb->X2;
		dx = -dx;
	}
	else
		x0 = bb->X1;

	if (conf_core.editor.view.flip_y) {
		y0 = bb->Y2;
		dy = -dy;
	}
	else
		y0 = bb->Y1;

	/* draw the label from template; if template is not available, draw the refdes */
	if ((conf_core.editor.subc_id != NULL) && (*conf_core.editor.subc_id != '\0')) {
		static gds_t s;

		s.used = 0;
		if (pcb_append_dyntext(&s, (pcb_any_obj_t *)subc, conf_core.editor.subc_id) == 0) {
			char *curr, *next;
			pcb_coord_t x = x0, y = y0;

			for(curr = s.array; curr != NULL; curr = next) {
				int ctrl = 0;
				next = strchr(curr, '\\');
				if (next != NULL) {
					*next = '\0';
					next++;
					ctrl = 1;
				}
				pcb_term_label_draw(info, x, y, conf_core.appearance.term_label_size/2, 0, 0, curr, subc->intconn);
				if (ctrl) {
					switch(*next) {
						case 'n': y += dy; x = x0; break;
					}
					next++;
				}
			}
		}
		else
			pcb_term_label_draw(info, x0, y0, conf_core.appearance.term_label_size/2.0, 0, 0, "<err>", subc->intconn);
	}
	else if (subc->refdes != NULL)
		pcb_term_label_draw(info, x0, y0, conf_core.appearance.term_label_size/2.0, 0, 0, subc->refdes, subc->intconn);

	return PCB_R_DIR_FOUND_CONTINUE;
}

void pcb_subc_draw_preview(const pcb_subc_t *sc, const pcb_box_t *drawn_area)
{
	int n;
	pcb_draw_info_t info;
	pcb_rtree_it_t it;
	pcb_any_obj_t *o;

	/* draw copper only first - order doesn't matter */
	for(n = 0; n < sc->data->LayerN; n++) {
		pcb_layer_t *layer = &sc->data->Layer[n];
		if (layer->meta.bound.type & PCB_LYT_COPPER)
			pcb_draw_layer_under(PCB, layer, drawn_area, sc->data);
	}

	/* draw padstacks */
	info.pcb = PCB;
	info.drawn_area = drawn_area;
	info.xform_caller = info.xform_exporter = info.xform = NULL;
	info.layer = NULL;
	info.objcb.pstk.gid = -1;
	info.objcb.pstk.is_current = 1;
	info.objcb.pstk.comb = 0;
	info.objcb.pstk.shape_mask = PCB_LYT_COPPER | PCB_LYT_TOP;
	info.objcb.pstk.holetype = PCB_PHOLE_UNPLATED | PCB_PHOLE_PLATED;

	for(o = pcb_rtree_first(&it, sc->data->padstack_tree, (pcb_rtree_box_t *)drawn_area); o != NULL; o = pcb_rtree_next(&it)) {
		if (pcb_obj_is_under(o, sc->data)) {
			pcb_pstk_draw_callback((pcb_box_t *)o, &info);
			if (PCB->hole_on)
				pcb_pstk_draw_hole_callback((pcb_box_t *)o, &info);
		}
	}

	/* draw silk and mech and doc layers, above padstacks */
	for(n = 0; n < sc->data->LayerN; n++) {
		pcb_layer_t *layer = &sc->data->Layer[n];
		if (layer->meta.bound.type & (PCB_LYT_SILK | PCB_LYT_BOUNDARY | PCB_LYT_MECH | PCB_LYT_DOC))
			pcb_draw_layer_under(PCB, layer, drawn_area, sc->data);
	}

	/* padstack mark goes on top */
	for(o = pcb_rtree_first(&it, sc->data->padstack_tree, (pcb_rtree_box_t *)drawn_area); o != NULL; o = pcb_rtree_next(&it)) {
		if (pcb_obj_is_under(o, sc->data)) {
			pcb_pstk_draw_mark_callback((pcb_box_t *)o, &info);
			pcb_pstk_draw_label_callback((pcb_box_t *)o, &info);
		}
	}
}


pcb_subc_t *pcb_subc_by_refdes(pcb_data_t *base, const char *name)
{
TODO("subc: hierarchy")
	PCB_SUBC_LOOP(base);
	{
		if ((subc->refdes != NULL) && (PCB_NSTRCMP(subc->refdes, name) == 0))
			return subc;
	}
	PCB_END_LOOP;
	return NULL;
}

pcb_subc_t *pcb_subc_by_id(pcb_data_t *base, long int ID)
{
	/* We can not have an rtree based search here: we are often called
	   in the middle of an operation, after the subc got already removed
	   from the rtree. It happens in e.g. undoable padstack operations
	   where the padstack tries to look up its parent subc by ID, while
	   the subc is being rotated.
	
	   The solution will be the ID hash. */
TODO("subc: hierarchy")
	PCB_SUBC_LOOP(base);
	{
		if (subc->ID == ID)
			return subc;
	}
	PCB_END_LOOP;
	return NULL;
}

pcb_bool pcb_subc_is_empty(pcb_subc_t *subc)
{
	return pcb_data_is_empty(subc->data);
}

pcb_layer_t *pcb_subc_get_layer(pcb_subc_t *sc, pcb_layer_type_t lyt, pcb_layer_combining_t comb, pcb_bool_t alloc, const char *name, pcb_bool req_name_match)
{
	int n;

	/* look for an existing layer with matching lyt and comb first */
	for(n = 0; n < sc->data->LayerN; n++) {
		if (sc->data->Layer[n].meta.bound.type != lyt)
			continue;
		if ((comb != -1) && (sc->data->Layer[n].comb != comb))
			continue;
		if (req_name_match && (strcmp(sc->data->Layer[n].name, name) != 0))
			continue;
		return &sc->data->Layer[n];
	}

	if (!alloc)
		return NULL;

	if (sc->data->LayerN == PCB_MAX_LAYER)
		return NULL;

	n = sc->data->LayerN++;
	if (name == NULL)
		name = "";

	if (comb == -1) {
		/* "any" means default, for the given layer type */
		if (lyt & PCB_LYT_MASK)
			comb = PCB_LYC_SUB;
		else
			comb = 0; /* positive, manual */
	}

	memset(&sc->data->Layer[n], 0, sizeof(sc->data->Layer[n]));
	sc->data->Layer[n].name = pcb_strdup(name);
	sc->data->Layer[n].comb = comb;
	sc->data->Layer[n].is_bound = 1;
	sc->data->Layer[n].meta.bound.type = lyt;
	sc->data->Layer[n].parent.data = sc->data;
	sc->data->Layer[n].parent_type = PCB_PARENT_DATA;
	sc->data->Layer[n].type = PCB_OBJ_LAYER;
	return &sc->data->Layer[n];
}

void pcb_subc_part_changed_(pcb_any_obj_t *obj)
{
	pcb_subc_t *sc = pcb_obj_parent_subc(obj);
	if (sc == NULL)
		return;

	/* can't do this incrementally: if a boundary object is smaller than before
	   it has to make the subc bbox smaller too */
	pcb_subc_bbox(sc);
}


const char *pcb_subc_name(pcb_subc_t *subc, const char *local_name)
{
	const char *val;

	if (local_name != NULL) {
		val = pcb_attribute_get(&subc->Attributes, local_name);
		if (val != NULL)
			return val;
	}
	val = pcb_attribute_get(&subc->Attributes, "visible_footprint");
	if (val != NULL)
		return val;
	val = pcb_attribute_get(&subc->Attributes, "footprint");
	return val;
}

pcb_subc_t *pcb_subc_replace(pcb_board_t *pcb, pcb_subc_t *dst, pcb_subc_t *src, pcb_bool add_undo, pcb_bool dumb)
{
	pcb_data_t *data = dst->parent.data;
	pcb_subc_t *placed;
	pcb_coord_t ox, oy;
	double rot = 0;
	long int target_id;
	int dst_on_bottom = 0, src_on_bottom = 0;

	assert(dst->parent_type == PCB_PARENT_DATA);
	assert(src != NULL);

	if (dumb) {
		ox = pcb_crosshair.X;
		oy = pcb_crosshair.Y;
	}
	else {
		if (pcb_subc_get_origin(dst, &ox, &oy) != 0) {
			ox = (dst->BoundingBox.X1 + dst->BoundingBox.X2) / 2;
			oy = (dst->BoundingBox.Y1 + dst->BoundingBox.Y2) / 2;
		}
		pcb_subc_get_rotation(dst, &rot);
		pcb_subc_get_side(dst, &dst_on_bottom);
	}
	

	placed = pcb_subc_dup_at(pcb, data, src, ox, oy, 0);
	pcb_subc_get_side(placed, &src_on_bottom);

	{ /* copy attributes */
		int n;
		pcb_attribute_list_t *adst = &placed->Attributes, *asrc = &dst->Attributes;
		for (n = 0; n < asrc->Number; n++)
			if (strcmp(asrc->List[n].name, "footprint") != 0)
				pcb_attribute_put(adst, asrc->List[n].name, asrc->List[n].value);
	}
	target_id = dst->ID;
	pcb_subc_remove(dst);

	pcb_obj_id_del(data, placed);
	placed->ID = target_id;
	pcb_obj_id_reg(data, placed);

	if (rot != 0) {
		if (dst_on_bottom != src_on_bottom)
			rot = -rot;
		pcb_subc_rotate(placed, ox, oy, cos(rot / PCB_RAD_TO_DEG), sin(rot / PCB_RAD_TO_DEG), rot);
	}

	if (dst_on_bottom != src_on_bottom)
		pcb_subc_change_side(placed, 2 * oy - PCB->MaxHeight);

	if (add_undo)
		pcb_undo_add_obj_to_create(PCB_OBJ_SUBC, placed, placed, placed);


	return placed;
}
