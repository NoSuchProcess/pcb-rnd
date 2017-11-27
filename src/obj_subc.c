/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
#include "obj_poly_op.h"
#include "obj_pstk.h"
#include "obj_pstk_inlines.h"
#include "obj_pstk_draw.h"
#include "obj_line_op.h"
#include "obj_term.h"
#include "obj_text_draw.h"
#include "rtree.h"
#include "draw.h"
#include "flag.h"
#include "polygon.h"
#include "operation.h"
#include "undo.h"
#include "compat_misc.h"
#include "math_helper.h"
#include "pcb_minuid.h"
#include "conf_core.h"

#define SUBC_AUX_NAME "subc-aux"

/* Modify dst to include src, if src is not a floater */
static void pcb_box_bump_box_noflt(pcb_box_t *dst, pcb_box_t *src)
{
	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, ((pcb_any_obj_t *)(src))))
		pcb_box_bump_box(dst, src);
}

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

pcb_subc_t *pcb_subc_alloc(void)
{
	pcb_subc_t *sc;
	sc = calloc(sizeof(pcb_subc_t), 1);
	sc->Attributes.post_change = pcb_subc_attrib_post_change;
	sc->data = pcb_data_new(NULL);
	sc->type = PCB_OBJ_SUBC;
	PCB_SET_PARENT(sc->data, subc, sc);
	minuid_gen(&pcb_minuid, sc->uid);
	pcb_term_init(&sc->terminals);
	return sc;
}

void pcb_subc_free(pcb_subc_t *sc)
{
	pcb_term_uninit(&sc->terminals);
	pcb_subclist_remove(sc);
	pcb_data_free(sc->data);
	free(sc->data);
	free(sc);
}


void pcb_add_subc_to_data(pcb_data_t *dt, pcb_subc_t *sc)
{
	PCB_SET_PARENT(sc->data, subc, sc);
	PCB_SET_PARENT(sc, data, dt);
	pcb_subclist_append(&dt->subc, sc);
}


static pcb_layer_t *pcb_subc_layer_create_buff(pcb_subc_t *sc, pcb_layer_t *src)
{
	pcb_layer_t *dst = &sc->data->Layer[sc->data->LayerN++];

	memcpy(&dst->meta, &src->meta, sizeof(src->meta));
	dst->is_bound = 1;
	dst->comb = src->comb;
	dst->parent = sc->data;
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

static int pcb_subc_cache_update(pcb_subc_t *sc)
{
	if (sc->aux_layer == NULL) {
		int n;
		for(n = 0; n < sc->data->LayerN; n++) {
			if (strcmp(sc->data->Layer[n].name, SUBC_AUX_NAME) == 0) {
				sc->aux_layer = sc->data->Layer + n;
				break;
			}
		}
	}

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
	if ((pcb_subc_cache_update(sc) != 0) || (sc->aux_cache[PCB_SUBCH_X] == NULL))
		return -1;

	*rot = PCB_RAD_TO_DEG * atan2(sc->aux_cache[PCB_SUBCH_X]->Point2.Y - sc->aux_cache[PCB_SUBCH_X]->Point1.Y, sc->aux_cache[PCB_SUBCH_X]->Point2.X - sc->aux_cache[PCB_SUBCH_X]->Point1.X);
	return 0;
}

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

static void pcb_subc_cache_invalidate(pcb_subc_t *sc)
{
	sc->aux_cache[0] = NULL;
}

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

	pcb_poly_init_clip(dst->parent, dst, poly);
	pcb_add_poly_on_layer(dst, poly);

	return poly;
}

extern unsigned long pcb_obj_type2oldtype(pcb_objtype_t type);

/* Move the pad-side-effect objects to the appropriate layer */
static void move_pad_side_effect(pcb_any_obj_t *o, pcb_layer_t *top, pcb_layer_t *bottom)
{
	pcb_layer_t *source = o->parent.layer;
	pcb_layer_t *target = (source->meta.bound.type & PCB_LYT_TOP) ? top : bottom;
	switch(o->type) {
		case PCB_OBJ_POLY:
			pcb_polyop_move_to_layer_low(NULL, source, (pcb_poly_t *)o, target);
			break;
		case PCB_OBJ_LINE:
			pcb_lineop_move_to_layer_low(NULL, source, (pcb_line_t *)o, target);
			break;
		default:
			assert(!"internal error: invalid mask/paste side effect");
	}
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

int pcb_subc_convert_from_buffer(pcb_buffer_t *buffer)
{
	pcb_subc_t *sc;
	int n, top_pads = 0, bottom_pads = 0, has_refdes_text = 0;
	pcb_layer_t *dst_top_mask = NULL, *dst_bottom_mask = NULL, *dst_top_paste = NULL, *dst_bottom_paste = NULL, *dst_top_silk = NULL;
	vtp0_t mask_pads, paste_pads;

	vtp0_init(&mask_pads);
	vtp0_init(&paste_pads);

	sc = pcb_subc_alloc();
	sc->ID = pcb_create_ID_get();
	pcb_add_subc_to_data(buffer->Data, sc);

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
				linelist_remove(line);
				linelist_append(&dst->Line, line);
				PCB_SET_PARENT(line, layer, dst);
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
			arclist_remove(arc);
			arclist_append(&dst->Arc, arc);
			PCB_SET_PARENT(arc, layer, dst);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, arc);
		}

		while((text = textlist_first(&src->Text)) != NULL) {
			textlist_remove(text);
			textlist_append(&dst->Text, text);
			PCB_SET_PARENT(text, layer, dst);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, text);
			if (!has_refdes_text && (text->TextString != NULL) && (strstr(text->TextString, "%a.parent.refdes%") != NULL))
				has_refdes_text = 1;
		}

		while((poly = polylist_first(&src->Polygon)) != NULL) {
			polylist_remove(poly);
			polylist_append(&dst->Polygon, poly);
			PCB_SET_PARENT(poly, layer, dst);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, poly);
		}
	}

	{ /* check if we have pins - they need mask */
		pcb_pin_t *via;

		for(via = pinlist_first(&buffer->Data->Via); via != NULL; via = pinlist_next(via)) {
			if (pcb_attribute_get(&via->Attributes, "elem_smash_pad") != NULL) {
				top_pads++;
				bottom_pads++;
				break;
			}
		}
	}

	/* create paste and mask side effects - needed when importing from footprint */
	{
		if (top_pads > 0) {
			if (dst_top_paste == NULL)
				dst_top_paste = pcb_layer_new_bound(sc->data, PCB_LYT_TOP | PCB_LYT_PASTE, "top paste");
			if (dst_top_mask == NULL) {
				dst_top_mask = pcb_layer_new_bound(sc->data, PCB_LYT_TOP | PCB_LYT_MASK, "top mask");
				dst_top_mask->comb = PCB_LYC_SUB;
			}
		}
		if (bottom_pads > 0) {
			if (dst_bottom_paste == NULL)
				dst_bottom_paste = pcb_layer_new_bound(sc->data, PCB_LYT_BOTTOM | PCB_LYT_PASTE, "bottom paste");
			if (dst_bottom_mask == NULL) {
				dst_bottom_mask = pcb_layer_new_bound(sc->data, PCB_LYT_BOTTOM | PCB_LYT_MASK, "bottom mask");
				dst_bottom_mask->comb = PCB_LYC_SUB;
			}
		}
	}

	{ /* convert globals */
		pcb_pin_t *via;
		pcb_pstk_t *ps;

		while((via = pinlist_first(&buffer->Data->Via)) != NULL) {
			const char *term;
			pinlist_remove(via);
			pinlist_append(&sc->data->Via, via);
			PCB_SET_PARENT(via, data, sc->data);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, via);
			term = pcb_attribute_get(&via->Attributes, "term");
			if (pcb_attribute_get(&via->Attributes, "elem_smash_pad") != NULL) {
				pcb_coord_t mask = read_mask((pcb_any_obj_t *)via);
				
				if (mask == 0)
					mask = via->Mask;

				if (mask > 0) {
					if (PCB_FLAG_TEST(PCB_FLAG_SQUARE, via)) {
						pcb_line_t line;
						pcb_poly_t *poly;
						memset(&line, 0, sizeof(line));
						line.Point1.X = line.Point2.X = via->X;
						line.Point1.Y = line.Point2.Y = via->Y;
						line.Thickness = mask;
						poly = sqline2term(dst_top_mask, &line);
						if (term != NULL)
							pcb_attribute_put(&poly->Attributes, "term", term);
						poly = sqline2term(dst_bottom_mask, &line);
						if (term != NULL)
							pcb_attribute_put(&poly->Attributes, "term", term);
					}
					else {
						pcb_line_t *line;
						line = pcb_line_new(dst_top_mask, via->X, via->Y, via->X, via->Y, mask, 0, pcb_no_flags());
						if (term != NULL)
							pcb_attribute_put(&line->Attributes, "term", term);
						line = pcb_line_new(dst_bottom_mask, via->X, via->Y, via->X, via->Y, mask, 0, pcb_no_flags());
						if (term != NULL)
							pcb_attribute_put(&line->Attributes, "term", term);
					}
				}
			}
		}

		while((ps = padstacklist_first(&buffer->Data->padstack)) != NULL) {
			const pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
			padstacklist_remove(ps);
			padstacklist_append(&sc->data->padstack, ps);
			PCB_SET_PARENT(ps, data, sc->data);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, ps);
			ps->proto = pcb_pstk_proto_insert_dup(sc->data, proto, 1);
			ps->protoi = -1;
		}
	}

	for(n = 0; n < vtp0_len(&paste_pads); n++)
		move_pad_side_effect(paste_pads.array[n], dst_top_paste, dst_bottom_paste);
	for(n = 0; n < vtp0_len(&mask_pads); n++)
		move_pad_side_effect(mask_pads.array[n], dst_top_mask, dst_bottom_mask);

	vtp0_uninit(&mask_pads);
	vtp0_uninit(&paste_pads);

	/* create aux layer */
	{
		pcb_coord_t unit = PCB_MM_TO_COORD(1);
		pcb_layer_t *aux = pcb_layer_new_bound(sc->data, PCB_LYT_VIRTUAL | PCB_LYT_NOEXPORT | PCB_LYT_MISC | PCB_LYT_TOP, SUBC_AUX_NAME);

		add_aux_line(aux, "subc-role", "origin", buffer->X, buffer->Y, buffer->X, buffer->Y);
		add_aux_line(aux, "subc-role", "x", buffer->X, buffer->Y, buffer->X+unit, buffer->Y);
		add_aux_line(aux, "subc-role", "y", buffer->X, buffer->Y, buffer->X, buffer->Y+unit);
	}

	/* Add refdes */
	pcb_attribute_put(&sc->Attributes, "refdes", "U0");
	if (!has_refdes_text) {
		if (dst_top_silk == NULL)
			dst_top_silk = pcb_layer_new_bound(sc->data, PCB_LYT_TOP | PCB_LYT_SILK, "top-silk");
		if (dst_top_silk != NULL)
			pcb_text_new(dst_top_silk, pcb_font(PCB, 0, 0), buffer->X, buffer->Y, 0, 100, "%a.parent.refdes%", pcb_flag_make(PCB_FLAG_DYNTEXT | PCB_FLAG_FLOATER));
		else
			pcb_message(PCB_MSG_ERROR, "Error: can't create top silk layer in subc for placing the refdes\n");
	}


	return 0;
}

static void pcb_subc_draw_origin(pcb_subc_t *sc, pcb_coord_t DX, pcb_coord_t DY)
{
	pcb_line_t *origin;
	pcb_subc_cache_update(sc);

	origin = sc->aux_cache[PCB_SUBCH_ORIGIN];

	if (origin == NULL)
		return;

	DX += (origin->Point1.X + origin->Point2.X) / 2;
	DY += (origin->Point1.Y + origin->Point2.Y) / 2;

	pcb_gui->draw_line(pcb_crosshair.GC, DX - PCB_EMARK_SIZE, DY, DX, DY - PCB_EMARK_SIZE);
	pcb_gui->draw_line(pcb_crosshair.GC, DX + PCB_EMARK_SIZE, DY, DX, DY - PCB_EMARK_SIZE);
	pcb_gui->draw_line(pcb_crosshair.GC, DX - PCB_EMARK_SIZE, DY, DX, DY + PCB_EMARK_SIZE);
	pcb_gui->draw_line(pcb_crosshair.GC, DX + PCB_EMARK_SIZE, DY, DX, DY + PCB_EMARK_SIZE);
}

void XORDrawSubc(pcb_subc_t *sc, pcb_coord_t DX, pcb_coord_t DY)
{
	int n;

	/* draw per layer objects */
	for(n = 0; n < sc->data->LayerN; n++) {
		pcb_layer_t *ly = sc->data->Layer + n;
		pcb_line_t *line;
		pcb_text_t *text;
		pcb_poly_t *poly;
		pcb_arc_t *arc;
		gdl_iterator_t it;

		linelist_foreach(&ly->Line, &it, line)
			pcb_gui->draw_line(pcb_crosshair.GC, DX + line->Point1.X, DY + line->Point1.Y, DX + line->Point2.X, DY + line->Point2.Y);

		arclist_foreach(&ly->Arc, &it, arc)
			pcb_gui->draw_arc(pcb_crosshair.GC, DX + arc->X, DY + arc->Y, arc->Width, arc->Height, arc->StartAngle, arc->Delta);

		polylist_foreach(&ly->Polygon, &it, poly)
			XORPolygon(poly, DX, DY, 0);

		textlist_foreach(&ly->Text, &it, text)
			pcb_text_draw_xor(text, DX, DY);
	}

	/* draw global objects */
	{
		pcb_pin_t *via;
		pcb_pstk_t *ps;
		gdl_iterator_t it;

		pinlist_foreach(&sc->data->Via, &it, via) {
			pcb_coord_t ox, oy;
			ox = via->X;
			oy = via->Y;
			via->X += DX;
			via->Y += DY;
			pcb_gui->thindraw_pcb_pv(pcb_crosshair.GC, pcb_crosshair.GC, via, pcb_true, pcb_false);
			via->X = ox;
			via->Y = oy;
		}
		padstacklist_foreach(&sc->data->padstack, &it, ps) {
			pcb_coord_t ox, oy;
			ox = ps->x;
			oy = ps->y;
			ps->x += DX;
			ps->y += DY;
			pcb_pstk_thindraw(pcb_crosshair.GC, ps);
			ps->x = ox;
			ps->y = oy;
		}
	}

	pcb_subc_draw_origin(sc, DX, DY);
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
	PCB_SET_PARENT(sc->data, subc, sc);
	PCB_SET_PARENT(sc, data, dst);
	pcb_subclist_append(&dst->subc, sc);

	src_pcb = pcb_data_get_top(src->data);

	pcb_subc_copy_meta(sc, src);

	sc->BoundingBox.X1 = sc->BoundingBox.Y1 = PCB_MAX_COORD;
	sc->BoundingBox.X2 = sc->BoundingBox.Y2 = -PCB_MAX_COORD;

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
					pcb_message(PCB_MSG_WARNING, "Couldn't bind a layer %s of subcricuit while placing it\n", name);
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
				pcb_box_bump_box_noflt(&sc->BoundingBox, &nline->BoundingBox);
			}
		}

		arclist_foreach(&sl->Arc, &it, arc) {
			narc = pcb_arc_dup_at(dl, arc, dx, dy);
			MAYBE_KEEP_ID(narc, arc);
			if (narc != NULL) {
				PCB_SET_PARENT(narc, layer, dl);
				pcb_box_bump_box_noflt(&sc->BoundingBox, &narc->BoundingBox);
			}
		}

		textlist_foreach(&sl->Text, &it, text) {
			ntext = pcb_text_dup_at(dl, text, dx, dy);
			MAYBE_KEEP_ID(ntext, text);
			if (ntext != NULL) {
				PCB_SET_PARENT(ntext, layer, dl);
				pcb_box_bump_box_noflt(&sc->BoundingBox, &ntext->BoundingBox);
			}
		}

	}
	sc->data->LayerN = src->data->LayerN;

	/* bind the via rtree so that vias added in this subc show up on the board */
	if (pcb != NULL) {
		if (pcb->Data->via_tree == NULL)
			pcb->Data->via_tree = pcb_r_create_tree(NULL, 0, 0);
		sc->data->via_tree = pcb->Data->via_tree;
		if (pcb->Data->padstack_tree == NULL)
			pcb->Data->padstack_tree = pcb_r_create_tree(NULL, 0, 0);
		sc->data->padstack_tree = pcb->Data->padstack_tree;
	}

	{ /* make a copy of global data */
		pcb_pin_t *via, *nvia;
		pcb_pstk_t *ps, *nps;
		gdl_iterator_t it;

		pinlist_foreach(&src->data->Via, &it, via) {
			nvia = pcb_via_dup_at(sc->data, via, dx, dy);
			MAYBE_KEEP_ID(nvia, via);
			if (nvia != NULL)
				pcb_box_bump_box_noflt(&sc->BoundingBox, &nvia->BoundingBox);
		}

		padstacklist_foreach(&src->data->padstack, &it, ps) {
			pcb_cardinal_t pid = pcb_pstk_proto_insert_dup(sc->data, pcb_pstk_get_proto(ps), 1);
			nps = pcb_pstk_new(sc->data, pid, ps->x+dx, ps->y+dy, ps->Clearance, ps->Flags);
			pcb_pstk_copy_meta(nps, ps);
			MAYBE_KEEP_ID(nps, ps);
			if (nps != NULL)
				pcb_box_bump_box_noflt(&sc->BoundingBox, &nps->BoundingBox);
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
				pcb_box_bump_box_noflt(&sc->BoundingBox, &npoly->BoundingBox);
				pcb_poly_ppclear(npoly);
			}
		}
	}

	memcpy(&sc->Flags, &src->Flags, sizeof(sc->Flags));

	pcb_close_box(&sc->BoundingBox);

	if (pcb != NULL) {
		if (!dst->subc_tree)
			dst->subc_tree = pcb_r_create_tree(NULL, 0, 0);
		pcb_r_insert_entry(dst->subc_tree, (pcb_box_t *)sc, 0);
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
void *pcb_subc_op(pcb_data_t *Data, pcb_subc_t *sc, pcb_opfunc_t *opfunc, pcb_opctx_t *ctx)
{
	int n;

	sc->BoundingBox.X1 = sc->BoundingBox.Y1 = PCB_MAX_COORD;
	sc->BoundingBox.X2 = sc->BoundingBox.Y2 = -PCB_MAX_COORD;

	EraseSubc(sc);

	if (pcb_data_get_top(Data) != NULL) {
		if (Data->subc_tree != NULL)
			pcb_r_delete_entry(Data->subc_tree, (pcb_box_t *)sc);
		else
			Data->subc_tree = pcb_r_create_tree(NULL, 0, 0);
	}

	/* execute on layer locals */
	for(n = 0; n < sc->data->LayerN; n++) {
		pcb_layer_t *sl = sc->data->Layer + n;
		pcb_line_t *line;
		pcb_text_t *text;
		pcb_poly_t *poly;
		pcb_arc_t *arc;
		gdl_iterator_t it;


		linelist_foreach(&sl->Line, &it, line) {
			pcb_object_operation(opfunc, ctx, PCB_TYPE_LINE, sl, line, line);
			pcb_box_bump_box_noflt(&sc->BoundingBox, &line->BoundingBox);
		}

		arclist_foreach(&sl->Arc, &it, arc) {
			pcb_object_operation(opfunc, ctx, PCB_TYPE_ARC, sl, arc, arc);
			pcb_box_bump_box_noflt(&sc->BoundingBox, &arc->BoundingBox);
		}

		textlist_foreach(&sl->Text, &it, text) {
			pcb_object_operation(opfunc, ctx, PCB_TYPE_TEXT, sl, text, text);
			pcb_box_bump_box_noflt(&sc->BoundingBox, &text->BoundingBox);
		}

		polylist_foreach(&sl->Polygon, &it, poly) {
			pcb_object_operation(opfunc, ctx, PCB_TYPE_POLY, sl, poly, poly);
			pcb_box_bump_box_noflt(&sc->BoundingBox, &poly->BoundingBox);
		}

	}


	/* execute on globals */
	{
		pcb_pin_t *via;
		pcb_pstk_t *ps;
		gdl_iterator_t it;

		pinlist_foreach(&sc->data->Via, &it, via) {
			pcb_object_operation(opfunc, ctx, PCB_TYPE_VIA, via, via, via);
			pcb_box_bump_box_noflt(&sc->BoundingBox, &via->BoundingBox);
		}

		padstacklist_foreach(&sc->data->padstack, &it, ps) {
			pcb_object_operation(opfunc, ctx, PCB_TYPE_PSTK, ps, ps, ps);
			pcb_box_bump_box_noflt(&sc->BoundingBox, &ps->BoundingBox);
		}
	}

	pcb_close_box(&sc->BoundingBox);
	if (pcb_data_get_top(Data) != NULL)
		pcb_r_insert_entry(Data->subc_tree, (pcb_box_t *)sc, 0);
	DrawSubc(sc);
	pcb_draw();
	return sc;
}


/* copies a subcircuit onto the PCB (a.k.a "paste"). Then does a draw. */
void *pcb_subcop_copy(pcb_opctx_t *ctx, pcb_subc_t *src)
{
	pcb_subc_t *sc;

	sc = pcb_subc_dup_at(PCB, PCB->Data, src, ctx->copy.DeltaX, ctx->copy.DeltaY, pcb_false);

	pcb_undo_add_obj_to_create(PCB_TYPE_SUBC, sc, sc, sc);

#warning subc TODO: BUG: "move selected" is really a copy&paste and this will send the subc to the top side
	if (conf_core.editor.show_solder_side)
		pcb_subc_change_side(&sc, 2 * pcb_crosshair.Y - PCB->MaxHeight);

	pcb_text_dyn_bbox_update(sc->data);

	return (sc);
}

extern pcb_opfunc_t ClipFunctions, MoveFunctions, MoveFunctions_noclip, Rotate90Functions, RotateFunctions, ChgFlagFunctions, ChangeSizeFunctions, ChangeClearSizeFunctions, Change1stSizeFunctions, Change2ndSizeFunctions;
extern pcb_opfunc_t ChangeOctagonFunctions, SetOctagonFunctions, ClrOctagonFunctions;
extern pcb_opfunc_t ChangeSquareFunctions, SetSquareFunctions, ClrSquareFunctions;

/* drag&drop move when not selected */
void *pcb_subcop_move(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_board_t *pcb = pcb_data_get_top(sc->data);
	pcb_data_t *data = (pcb != NULL ? pcb->Data : NULL);
	pcb_opctx_t clip;

	/* restore all pins/pads at once, at the old location */
	clip.clip.restore = 1; clip.clip.clear = 0;
	pcb_subc_op(data, sc, &ClipFunctions, &clip);

	/* do the move without messing with the clipping */
	pcb_subc_op(data, sc, &MoveFunctions_noclip, ctx);

	/* clear all pins/pads at once, at the new location */
	clip.clip.restore = 0; clip.clip.clear = 1;
	pcb_subc_op(data, sc, &ClipFunctions, &clip);
	return sc;
}

void *pcb_subcop_rotate90(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_board_t *pcb = pcb_data_get_top(sc->data);
	return pcb_subc_op((pcb != NULL ? pcb->Data : NULL), sc, &Rotate90Functions, ctx);
}

void *pcb_subcop_rotate(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_data_t *data;

	ctx->rotate.pcb = pcb_data_get_top(sc->data);
	data = (ctx->rotate.pcb != NULL ? ctx->rotate.pcb->Data : NULL);
	return pcb_subc_op(data, sc, &RotateFunctions, ctx);
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


static int subc_relocate_layer_objs(pcb_layer_t *dl, pcb_data_t *src_data, pcb_layer_t *sl, int src_has_real_layer, int dst_is_pcb)
{
	pcb_line_t *line;
	pcb_text_t *text;
	pcb_poly_t *poly;
	pcb_arc_t *arc;
	gdl_iterator_t it;
	int chg = 0;

	linelist_foreach(&sl->Line, &it, line) {
		if (src_has_real_layer) {
			pcb_poly_restore_to_poly(src_data, PCB_TYPE_LINE, sl, line);
			pcb_r_delete_entry(sl->line_tree, (pcb_box_t *)line);
			chg++;
		}
		PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, line);
		if ((dl != NULL) && (dl->line_tree != NULL)) {
			pcb_r_insert_entry(dl->line_tree, (pcb_box_t *)line, 0);
			chg++;
		}
	}

	arclist_foreach(&sl->Arc, &it, arc) {
		if (src_has_real_layer) {
			pcb_poly_restore_to_poly(src_data, PCB_TYPE_ARC, sl, arc);
			pcb_r_delete_entry(sl->arc_tree, (pcb_box_t *)arc);
			chg++;
		}
		PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, arc);
		if ((dl != NULL) && (dl->arc_tree != NULL)) {
			pcb_r_insert_entry(dl->arc_tree, (pcb_box_t *)arc, 0);
			chg++;
		}
	}

	textlist_foreach(&sl->Text, &it, text) {
		if (src_has_real_layer) {
			pcb_poly_restore_to_poly(src_data, PCB_TYPE_LINE, sl, text);
			pcb_r_delete_entry(sl->text_tree, (pcb_box_t *)text);
			chg++;
		}
		PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, text);
		if ((dl != NULL) && (dl->text_tree != NULL)) {
			pcb_r_insert_entry(dl->text_tree, (pcb_box_t *)text, 0);
			chg++;
		}
	}

	polylist_foreach(&sl->Polygon, &it, poly) {
		pcb_poly_pprestore(poly);
		if (src_has_real_layer) {
			pcb_r_delete_entry(sl->polygon_tree, (pcb_box_t *)poly);
			chg++;
		}
		PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, poly);
		if ((dl != NULL) && (dl->polygon_tree != NULL)) {
			pcb_r_insert_entry(dl->polygon_tree, (pcb_box_t *)poly, 0);
			chg++;
		}
		if (dst_is_pcb)
			pcb_poly_ppclear(poly);
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

void *pcb_subcop_move_to_buffer(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	int n;
	pcb_board_t *dst_top = pcb_data_get_top(ctx->buffer.dst);
	int dst_is_pcb = ((dst_top != NULL) && (dst_top->Data == ctx->buffer.dst));

	EraseSubc(sc);

	/* move the subc */
	if ((ctx->buffer.pcb != NULL) && (ctx->buffer.pcb->Data->subc_tree != NULL))
		pcb_r_delete_entry(ctx->buffer.pcb->Data->subc_tree, (pcb_box_t *)sc);

	pcb_subclist_remove(sc);
	pcb_subclist_append(&ctx->buffer.dst->subc, sc);

	if (dst_is_pcb) {
		if (ctx->buffer.dst->subc_tree == NULL)
			ctx->buffer.dst->subc_tree = pcb_r_create_tree(NULL, 0, 0);
		pcb_r_insert_entry(ctx->buffer.dst->subc_tree, (pcb_box_t *)sc, 0);
	}

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
				if (sl->line_tree == NULL) sl->line_tree = pcb_r_create_tree(NULL, 0, 0);
				if (sl->arc_tree == NULL) sl->arc_tree = pcb_r_create_tree(NULL, 0, 0);
				if (sl->text_tree == NULL) sl->text_tree = pcb_r_create_tree(NULL, 0, 0);
				if (sl->polygon_tree == NULL) sl->polygon_tree = pcb_r_create_tree(NULL, 0, 0);
				pcb_message(PCB_MSG_ERROR, "Couldn't bind subc layer %s on buffer move\n", sl->name == NULL ? "<anonymous>" : sl->name);
			}
		}
		else
			dl = ctx->buffer.dst->Layer + n;

		subc_relocate_layer_objs(dl, ctx->buffer.src, sl, src_has_real_layer, dst_is_pcb);
	}

	/* move globals */
	{
		pcb_pin_t *via;
		pcb_pstk_t *ps;
		gdl_iterator_t it;

		pinlist_foreach(&sc->data->Via, &it, via) {
			if (sc->data->via_tree != NULL)
				pcb_r_delete_entry(sc->data->via_tree, (pcb_box_t *)via);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, via);
			if (ctx->buffer.dst->via_tree != NULL)
				pcb_r_insert_entry(ctx->buffer.dst->via_tree, (pcb_box_t *)via, 0);
		}

		padstacklist_foreach(&sc->data->padstack, &it, ps) {
			const pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
			if (sc->data->padstack_tree != NULL)
				pcb_r_delete_entry(sc->data->padstack_tree, (pcb_box_t *)ps);
			PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, ps);
			if (ctx->buffer.dst->padstack_tree != NULL)
				pcb_r_insert_entry(ctx->buffer.dst->padstack_tree, (pcb_box_t *)ps, 0);
			ps->proto = pcb_pstk_proto_insert_dup(ctx->buffer.dst, proto, 1);
			ps->protoi = -1;
		}
	}

	/* bind globals */
	if (dst_is_pcb) {
		if (ctx->buffer.dst->via_tree == NULL)
			ctx->buffer.dst->via_tree = pcb_r_create_tree(NULL, 0, 0);
		sc->data->via_tree = ctx->buffer.dst->via_tree;

		if (ctx->buffer.dst->padstack_tree == NULL)
			ctx->buffer.dst->padstack_tree = pcb_r_create_tree(NULL, 0, 0);
		sc->data->padstack_tree = ctx->buffer.dst->padstack_tree;
	}
	else {
		sc->data->via_tree = NULL;
		sc->data->padstack_tree = NULL;
	}

	PCB_FLAG_CLEAR(PCB_FLAG_WARN | PCB_FLAG_FOUND | PCB_FLAG_SELECTED, sc);
	PCB_SET_PARENT(sc, data, ctx->buffer.dst);
	return sc;
}

void *pcb_subcop_add_to_buffer(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_t *nsc;
	nsc = pcb_subc_dup_at(NULL, ctx->buffer.dst, sc, 0, 0, pcb_false);
	if (ctx->buffer.extraflg & PCB_FLAG_SELECTED)
		pcb_subc_select(NULL, nsc, PCB_CHGFLG_CLEAR, 0);
	return nsc;
}

/* break buffer subc into pieces */
pcb_bool pcb_subc_smash_buffer(pcb_buffer_t *buff)
{
	pcb_subc_t *subc;

	if (pcb_subclist_length(&buff->Data->subc) != 1)
		return (pcb_false);

	subc = pcb_subclist_first(&buff->Data->subc);
	pcb_subclist_remove(subc);

	pcb_data_free(buff->Data);
	buff->Data = subc->data;
	buff->Data->parent_type = PCB_PARENT_INVALID;
	buff->Data->parent.data = NULL;

	return (pcb_true);
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
			if (dl->line_tree == NULL) dl->line_tree = pcb_r_create_tree(NULL, 0, 0);
			if (dl->arc_tree == NULL) dl->arc_tree = pcb_r_create_tree(NULL, 0, 0);
			if (dl->text_tree == NULL) dl->text_tree = pcb_r_create_tree(NULL, 0, 0);
			if (dl->polygon_tree == NULL) dl->polygon_tree = pcb_r_create_tree(NULL, 0, 0);
		}

		if (subc_relocate_layer_objs(dl, pcb->Data, sl, src_has_real_layer, 1) > 0)
			chgly++;

		if (dl != NULL)
			pcb_layer_link_trees(sl, dl);
	}

	return chgly;
}

void *pcb_subcop_change_size(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &ChangeSizeFunctions, ctx);
	return sc;
}

void *pcb_subcop_change_clear_size(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &ChangeClearSizeFunctions, ctx);
	return sc;
}

void *pcb_subcop_change_1st_size(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &Change1stSizeFunctions, ctx);
	return sc;
}

void *pcb_subcop_change_2nd_size(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &Change2ndSizeFunctions, ctx);
	return sc;
}

void *pcb_subcop_change_nonetlist(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
#warning subc TODO: add undo
	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, sc))
		return (NULL);
	PCB_FLAG_TOGGLE(PCB_FLAG_NONETLIST, sc);
	return sc;
}

void *pcb_subcop_change_name(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_attribute_put(&sc->Attributes, "refdes", ctx->chgname.new_name);
	return sc;
}

void *pcb_subcop_destroy(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	if (ctx->remove.pcb->Data->subc_tree != NULL)
		pcb_r_delete_entry(ctx->remove.pcb->Data->subc_tree, (pcb_box_t *)sc);

	pcb_subclist_remove(sc);
	EraseSubc(sc);
	pcb_subc_free(sc);
	return NULL;
}

void *pcb_subcop_remove(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	EraseSubc(sc);
	if (!ctx->remove.bulk)
		pcb_draw();
	pcb_undo_move_obj_to_remove(PCB_TYPE_SUBC, sc, sc, sc);
	return NULL;
}

void *pcb_subc_remove(pcb_subc_t *sc)
{
	pcb_opctx_t ctx;

	ctx.remove.pcb = PCB;
	ctx.remove.bulk = pcb_false;
	ctx.remove.destroy_target = NULL;
/*	PCB_CLEAR_PARENT(subc);*/

	return pcb_subcop_remove(&ctx, sc);
}


void *pcb_subcop_clear_octagon(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &ClrOctagonFunctions, ctx);
	return sc;
}

void *pcb_subcop_set_octagon(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &SetOctagonFunctions, ctx);
	return sc;
}

void *pcb_subcop_change_octagon(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &ChangeOctagonFunctions, ctx);
	return sc;
}

void *pcb_subcop_clear_square(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &ClrSquareFunctions, ctx);
	return sc;
}

void *pcb_subcop_set_square(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &SetSquareFunctions, ctx);
	return sc;
}

void *pcb_subcop_change_square(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	pcb_subc_op(ctx->chgsize.pcb->Data, sc, &ChangeSquareFunctions, ctx);
	return sc;
}



void *pcb_subcop_change_flag(pcb_opctx_t *ctx, pcb_subc_t *sc)
{
	static pcb_flag_values_t pcb_subc_flags = 0;
	if (pcb_subc_flags == 0)
		pcb_subc_flags = pcb_obj_valid_flags(PCB_TYPE_SUBC);

	if (ctx->chgflag.flag == PCB_FLAG_FLOATER) {
#warning subc TODO: subc-in-subc: can a whole subc be a floater? - add undo!
		PCB_FLAG_CHANGE(ctx->chgflag.how, ctx->chgflag.flag, sc);
		return sc;
	}

	pcb_subc_op(ctx->chgflag.pcb->Data, sc, &ChgFlagFunctions, ctx);
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

	pcb_subc_op((pcb == NULL ? NULL : pcb->Data), sc, &ChgFlagFunctions, &ctx);
	PCB_FLAG_CHANGE(how, PCB_FLAG_SELECTED, sc);
	if (redraw)
		DrawSubc(sc);
}

/* mirrors the coordinates of a subcircuit; an additional offset is passed */
void pcb_subc_mirror(pcb_data_t *data, pcb_subc_t *subc, pcb_coord_t y_offs)
{
	if ((data != NULL) && (data->subc_tree != NULL))
		pcb_r_delete_entry(data->subc_tree, (pcb_box_t *)subc);

	pcb_data_mirror(subc->data, y_offs);
	pcb_subc_bbox(subc);

	if ((data != NULL) && (data->subc_tree != NULL))
		pcb_r_insert_entry(data->subc_tree, (pcb_box_t *)subc, 0);
}

pcb_bool pcb_subc_change_side(pcb_subc_t **subc, pcb_coord_t yoff)
{
	pcb_opctx_t ctx;
	pcb_subc_t *newsc, *newsc2;
	int n;
	pcb_board_t *pcb;
	pcb_data_t *data;

	if (PCB_FLAG_TEST(PCB_FLAG_LOCK, *subc))
		return (pcb_false);

	assert((*subc)->parent_type = PCB_PARENT_DATA);
	data = (*subc)->parent.data;
	pcb = pcb_data_get_top(data);

	/* move subc into a local "buffer" */
	memset(&ctx, 0, sizeof(ctx));
	ctx.buffer.pcb = pcb_data_get_top((*subc)->data);
	ctx.buffer.dst = pcb_data_new(NULL);
	ctx.buffer.src = data;
	newsc = pcb_subcop_move_to_buffer(&ctx, *subc);


	/* mirror object geometry and stackup */
	pcb_subc_mirror(NULL, newsc, yoff);
	for(n = 0; n < newsc->data->LayerN; n++) {
		pcb_layer_t *ly = newsc->data->Layer + n;
		if (ly->meta.bound.type & PCB_LYT_TOP)
			ly->meta.bound.type = (ly->meta.bound.type & ~PCB_LYT_TOP) | PCB_LYT_BOTTOM;
		else if (ly->meta.bound.type & PCB_LYT_BOTTOM)
			ly->meta.bound.type = (ly->meta.bound.type & ~PCB_LYT_BOTTOM) | PCB_LYT_TOP;
	}

	/* place the new subc */
	newsc2 = pcb_subc_dup_at(pcb, data, newsc, 0, 0, pcb_true);
	newsc2->ID = newsc->ID;
	PCB_SET_PARENT(newsc2, data, data);
	pcb_undo_add_subc_to_otherside(PCB_TYPE_SUBC, newsc2, newsc2, newsc2, yoff);

	*subc = newsc2;
	pcb_subc_free(newsc);
	pcb_data_free(ctx.buffer.dst);
	return pcb_true;
}


#include "conf_core.h"
#include "draw.h"
pcb_r_dir_t draw_subc_mark_callback(const pcb_box_t *b, void *cl)
{
	pcb_subc_t *subc = (pcb_subc_t *) b;
	pcb_box_t *bb = &subc->BoundingBox;
	int selected = PCB_FLAG_TEST(PCB_FLAG_SELECTED, subc);

	pcb_gui->set_color(Output.fgGC, conf_core.appearance.color.element);
	pcb_gui->set_line_cap(Output.fgGC, Trace_Cap);
	pcb_gui->set_line_width(Output.fgGC, 0);
	pcb_gui->set_draw_xor(Output.fgGC, 1);
	pcb_subc_draw_origin(subc, 0, 0);
	pcb_gui->set_draw_xor(Output.fgGC, 0);

	pcb_gui->set_color(Output.fgGC, selected ? conf_core.appearance.color.subc_selected : conf_core.appearance.color.subc);
	pcb_gui->set_line_width(Output.fgGC, 0);
	pcb_gui->set_draw_xor(Output.fgGC, 1);
	pcb_draw_dashed_line(Output.fgGC, bb->X1, bb->Y1, bb->X2, bb->Y1);
	pcb_draw_dashed_line(Output.fgGC, bb->X1, bb->Y1, bb->X1, bb->Y2);
	pcb_draw_dashed_line(Output.fgGC, bb->X2, bb->Y2, bb->X2, bb->Y1);
	pcb_draw_dashed_line(Output.fgGC, bb->X2, bb->Y2, bb->X1, bb->Y2);
	pcb_gui->set_draw_xor(Output.fgGC, 0);

	if (subc->refdes != NULL)
		pcb_term_label_draw(bb->X1, bb->Y1, 50.0, 0, 0, subc->refdes, subc->intconn);

	return PCB_R_DIR_FOUND_CONTINUE;
}

void pcb_subc_draw_preview(const pcb_subc_t *sc, pcb_box_t *drawn_area)
{
	int n;
	pcb_pstk_draw_t ctx;

	for(n = 0; n < sc->data->LayerN; n++) {
		pcb_layer_t *layer = &sc->data->Layer[n];
		if (layer->meta.bound.type & (PCB_LYT_COPPER | PCB_LYT_SILK | PCB_LYT_OUTLINE))
			pcb_draw_layer(layer, drawn_area);
	}

	ctx.pcb = NULL;
	ctx.gid = -1;
	ctx.is_current = 1;
	ctx.comb = 0;
	pcb_r_search(sc->data->padstack_tree, drawn_area, NULL, pcb_pstk_draw_callback, &ctx, NULL);
}


pcb_subc_t *pcb_subc_by_refdes(pcb_data_t *base, const char *name)
{
#warning subc TODO: hierarchy
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
#warning subc TODO: hierarchy
	PCB_SUBC_LOOP(base);
	{
		if (subc->ID == ID)
			return subc;
	}
	PCB_END_LOOP;
	return NULL;
}
