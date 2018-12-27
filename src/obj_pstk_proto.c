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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"

#include <assert.h>

#include "board.h"
#include "buffer.h"
#include "compat_misc.h"
#include "conf_core.h"
#include "data.h"
#include "data_list.h"
#include "obj_pstk.h"
#include "obj_pstk_inlines.h"
#include "rotate.h"
#include "undo.h"
#include "vtpadstack_t.h"
#include "obj_hash.h"
#include "funchash_core.h"
#include "polygon_offs.h"

static const char core_proto_cookie[] = "padstack prototypes";


void pcb_pstk_proto_free_fields(pcb_pstk_proto_t *dst)
{
TODO("do a full field free here")
	dst->in_use = 0;
}

void pcb_pstk_proto_update(pcb_pstk_proto_t *dst)
{
	pcb_pstk_tshape_t *ts = &dst->tr.array[0];
	unsigned int n;

	dst->hash = pcb_pstk_proto_hash(dst);
	dst->mech_idx = -1;

	if (ts != NULL) {
		for(n = 0; n < ts->len; n++) {
			if (ts->shape[n].layer_mask & PCB_LYT_MECH) {
				dst->mech_idx = n;
				break;
			}
		}
	}
}

void pcb_pstk_shape_alloc_poly(pcb_pstk_poly_t *poly, int len)
{
	poly->x = malloc(sizeof(poly->x[0]) * len * 2);
	poly->y = poly->x + len;
	poly->len = len;
	poly->pa = NULL;
}

void pcb_pstk_shape_free_poly(pcb_pstk_poly_t *poly)
{
	if (poly->pa != NULL)
		pcb_polyarea_free(&poly->pa);
	free(poly->x);
	poly->len = 0;
}

void pcb_pstk_shape_copy_poly(pcb_pstk_poly_t *dst, const pcb_pstk_poly_t *src)
{
	memcpy(dst->x, src->x, sizeof(src->x[0]) * src->len * 2);
	pcb_pstk_shape_update_pa(dst);
}

int pcb_pstk_get_shape_idx(pcb_pstk_tshape_t *ts, pcb_layer_type_t lyt, pcb_layer_combining_t comb)
{
	int n;
	for(n = 0; n < ts->len; n++)
		if ((lyt == ts->shape[n].layer_mask) && (comb == ts->shape[n].comb))
			return n;
	return -1;
}

pcb_pstk_shape_t *pcb_pstk_alloc_append_shape(pcb_pstk_tshape_t *ts)
{
	int idx = ts->len;

	ts->len++;
	ts->shape = realloc(ts->shape, ts->len * sizeof(pcb_pstk_shape_t));

	return &ts->shape[idx];
}

static void append_circle(pcb_pstk_tshape_t *ts, pcb_layer_type_t lyt, pcb_layer_combining_t comb, pcb_coord_t dia)
{
	int idx = ts->len;

	ts->len++;
	ts->shape = realloc(ts->shape, ts->len * sizeof(pcb_pstk_shape_t));

	ts->shape[idx].shape = PCB_PSSH_CIRC;
	ts->shape[idx].data.circ.x = ts->shape[idx].data.circ.y = 0;
	ts->shape[idx].data.circ.dia = dia;
	ts->shape[idx].layer_mask = lyt;
	ts->shape[idx].comb = comb;
}

static void append_tshape(pcb_pstk_tshape_t *ts, pcb_pstk_tshape_t *src, int srci)
{
	int idx = ts->len;

	ts->len++;
	ts->shape = realloc(ts->shape, ts->len * sizeof(pcb_pstk_shape_t));
	memcpy(&ts->shape[idx], &src->shape[srci], sizeof(ts->shape[idx]));
	switch(src->shape[srci].shape) {
		case PCB_PSSH_LINE:
		case PCB_PSSH_CIRC:
		case PCB_PSSH_HSHADOW:
			break; /* do nothing, all fields are copied already by the memcpy */
		case PCB_PSSH_POLY:
			pcb_pstk_shape_alloc_poly(&ts->shape[idx].data.poly, src->shape[srci].data.poly.len);
			pcb_pstk_shape_copy_poly(&ts->shape[idx].data.poly, &src->shape[srci].data.poly);
			break;
	}
}

int pcb_pstk_proto_conv(pcb_data_t *data, pcb_pstk_proto_t *dst, int quiet, vtp0_t *objs, pcb_coord_t ox, pcb_coord_t oy)
{
	int ret = -1, n, m, i, extra_obj = 0, has_slot = 0, has_hole = 0;
	pcb_any_obj_t **o;
	pcb_pstk_tshape_t *ts, *ts_src;
	pcb_pstk_t *pstk = NULL;
	pcb_pstk_proto_t *prt;

	dst->in_use = 1;
	dst->name = NULL;
	pcb_vtpadstack_tshape_init(&dst->tr);
	dst->hdia = 0;
	dst->htop = dst->hbottom = 0;

	/* allocate shapes on the canonical tshape (tr[0]) */
	ts = pcb_vtpadstack_tshape_alloc_append(&dst->tr, 1);
	ts->rot = 0.0;
	ts->xmirror = 0;
	ts->smirror = 0;
	ts->len = 0;
	for(n = 0, o = (pcb_any_obj_t **)objs->array; n < vtp0_len(objs); n++,o++) {
		pcb_layer_t *ly = (*o)->parent.layer;
		pcb_layer_type_t lyt = pcb_layer_flags_(ly);

		if (lyt & PCB_LYT_MECH) {
			int purpi;
			if (has_slot) {
				if (!quiet)
					pcb_message(PCB_MSG_ERROR, "Padstack conversion: multiple mechanical objects (slots) are not allowed\n");
				goto quit;
			}
			has_slot++;
			purpi = pcb_layer_purpose_(ly, NULL);
			dst->hplated = !!PCB_LAYER_IS_PROUTE(lyt, purpi);
		}

		switch((*o)->type) {
			case PCB_OBJ_LINE:
			case PCB_OBJ_POLY:
				ts->len++;
				break;
			case PCB_OBJ_PSTK:
				if (pstk != NULL) {
					if (!quiet)
						pcb_message(PCB_MSG_ERROR, "Padstack conversion: multiple vias/padstacks\n");
					goto quit;
				}
				pstk = *(pcb_pstk_t **)o;
				prt = pcb_pstk_get_proto(pstk);
				if (prt == NULL) {
					if (!quiet)
						pcb_message(PCB_MSG_ERROR, "Padstack conversion: invalid input padstacks proto\n");
					goto quit;
				}
				if (prt->hdia > 0)
					has_hole = 1;
				dst->hdia = prt->hdia;
				dst->hplated = prt->hplated;
				if ((ox != pstk->x) || (oy != pstk->y)) {
					pcb_message(PCB_MSG_INFO, "Padstack conversion: adjusting origin to padstack hole\n");
					ox = pstk->x;
					oy = pstk->y;
				}
				extra_obj++;
				break;
			default:;
				if (!quiet)
					pcb_message(PCB_MSG_ERROR, "Padstack conversion: invalid object type (%x) selected; must be via, padstack, line or polygon\n", (*o)->type);
				goto quit;
		}
	}

	if ((vtp0_len(objs) - extra_obj) > data->LayerN) {
		if (!quiet)
			pcb_message(PCB_MSG_ERROR, "Padstack conversion: too many objects selected\n");
		goto quit;
	}

	if ((ts->len == 0) && (pstk == NULL)) {
		if (!quiet)
			pcb_message(PCB_MSG_ERROR, "Padstack conversion: there are no shapes and there is no via/padstack participating in the conversion; can not create empty padstack\n");
		goto quit;
	}

	ts->shape = malloc(ts->len * sizeof(pcb_pstk_shape_t));

	/* convert local (line/poly) objects */
	for(i = 0, n = -1, o = (pcb_any_obj_t **)objs->array; i < vtp0_len(objs); o++,i++) {
		pcb_layer_t *ly;
		switch((*o)->type) {
			case PCB_OBJ_LINE:
				{
					pcb_line_t *line = (*(pcb_line_t **)o);

					n++;
					if ((line->Point1.X != line->Point2.X) || (line->Point1.Y != line->Point2.Y)) {
						ts->shape[n].shape = PCB_PSSH_LINE;
						ts->shape[n].data.line.x1 = line->Point1.X - ox;
						ts->shape[n].data.line.y1 = line->Point1.Y - oy;
						ts->shape[n].data.line.x2 = line->Point2.X - ox;
						ts->shape[n].data.line.y2 = line->Point2.Y - oy;
						ts->shape[n].data.line.thickness = line->Thickness;
						ts->shape[n].data.line.square = 0;
					}
					else { /* a zero-long line is really a circle - padstacks have a specific shape for that */
						ts->shape[n].shape = PCB_PSSH_CIRC;
						ts->shape[n].data.circ.x = line->Point1.X - ox;
						ts->shape[n].data.circ.y = line->Point1.Y - oy;
						ts->shape[n].data.circ.dia = line->Thickness;
					}
					ts->shape[n].clearance = line->Clearance;
				}
				break;
			case PCB_OBJ_POLY:
				{
					pcb_cardinal_t p, len, maxlen = (1L << ((sizeof(int)*8)-1));
					pcb_poly_t *poly = *(pcb_poly_t **)o;

					len = poly->PointN;
					n++;
					if (poly->HoleIndexN != 0) {
						if (!quiet)
							pcb_message(PCB_MSG_ERROR, "Padstack conversion: can not convert polygon with holes\n");
						goto quit;
					}
					if (len >= maxlen) {
						if (!quiet)
							pcb_message(PCB_MSG_ERROR, "Padstack conversion: polygon has too many points (%ld >= %ld)\n", (long)len, (long)maxlen);
						goto quit;
					}
					pcb_pstk_shape_alloc_poly(&ts->shape[n].data.poly, len);
					for(p = 0; p < len; p++) {
						ts->shape[n].data.poly.x[p] = poly->Points[p].X - ox;
						ts->shape[n].data.poly.y[p] = poly->Points[p].Y - oy;
					}
					pcb_pstk_shape_update_pa(&ts->shape[n].data.poly);
					ts->shape[n].shape = PCB_PSSH_POLY;
					ts->shape[n].clearance = (*(pcb_poly_t **)o)->Clearance;
				}
				break;
			default: continue;
		}
		assert((*o)->parent_type == PCB_PARENT_LAYER);
		ly = (*o)->parent.layer;
		ts->shape[n].layer_mask = pcb_layer_flags_(ly);
		ts->shape[n].comb = ly->comb;

		if (ts->shape[n].layer_mask & PCB_LYT_BOUNDARY) {
			ts->shape[n].layer_mask &= ~PCB_LYT_BOUNDARY;
			ts->shape[n].layer_mask |= PCB_LYT_MECH;
		}
		if (ts->shape[n].layer_mask & (PCB_LYT_PASTE | PCB_LYT_MASK | PCB_LYT_MECH))
			ts->shape[n].comb |= PCB_LYC_AUTO;

		for(m = 0; m < n; m++) {
			if ((ts->shape[n].layer_mask == ts->shape[m].layer_mask) && (ts->shape[n].comb == ts->shape[m].comb)) {
				if (!quiet)
					pcb_message(PCB_MSG_ERROR, "Padstack conversion: multiple objects on the same layer\n");
				goto quit;
			}
		}
		if ((ts->shape[n].layer_mask & PCB_LYT_COPPER) && (ts->shape[n].layer_mask & PCB_LYT_INTERN) && (has_hole) && (!has_slot)) {
			if (!quiet)
				pcb_message(PCB_MSG_ERROR, "Padstack conversion: can not have internal copper shape if there is no hole\n");
			goto quit;
		}
	}

	if (has_hole && has_slot) {
		if (!quiet)
			pcb_message(PCB_MSG_ERROR, "Padstack conversion: can not have both hole (padstack) and slot \n");
		goto quit;
	}

	/* if there was a padstack, use the padstack's shape on layers that are not specified */
	if (pstk != NULL) {
		int srci;
		ts_src = pcb_pstk_get_tshape(pstk);
		if (ts_src != NULL) {
#			define MAYBE_COPY(mask, comb) \
				if ((pcb_pstk_get_shape_idx(ts, mask, comb) == -1) && ((srci = pcb_pstk_get_shape_idx(ts_src, mask, comb)) != -1)) \
					append_tshape(ts, ts_src, srci);
			MAYBE_COPY(PCB_LYT_COPPER | PCB_LYT_TOP, 0);
			MAYBE_COPY(PCB_LYT_COPPER | PCB_LYT_INTERN, 0);
			MAYBE_COPY(PCB_LYT_COPPER | PCB_LYT_BOTTOM, 0);
			MAYBE_COPY(PCB_LYT_MASK | PCB_LYT_BOTTOM, PCB_LYC_SUB | PCB_LYC_AUTO);
			MAYBE_COPY(PCB_LYT_MASK | PCB_LYT_TOP, PCB_LYC_SUB | PCB_LYC_AUTO);
			MAYBE_COPY(PCB_LYT_PASTE | PCB_LYT_BOTTOM, PCB_LYC_AUTO);
			MAYBE_COPY(PCB_LYT_PASTE | PCB_LYT_TOP, PCB_LYC_AUTO);
			MAYBE_COPY(PCB_LYT_MECH, PCB_LYC_AUTO);
#			undef MAYBE_COPY
		}
	}

	/* all went fine */
	pcb_pstk_proto_update(dst);
	ret = 0;

	quit:;
	if (ret != 0)
		pcb_pstk_proto_free_fields(dst);
	return ret;
}

int pcb_pstk_proto_breakup(pcb_data_t *dst, pcb_pstk_t *src, pcb_bool remove_src)
{
	pcb_pstk_tshape_t *ts = pcb_pstk_get_tshape(src);
	int n, i;
	pcb_layer_type_t lyt, filt = (PCB_LYT_ANYTHING | PCB_LYT_ANYWHERE);
	pcb_any_obj_t *no;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(src);

	if (ts == NULL)
		return -1;

	for(n = 0; n < ts->len; n++) {
		pcb_pstk_shape_t *shp = &ts->shape[n];
		pcb_layer_id_t lid;
		pcb_layer_t *ly, *ly1 = NULL, *ly2 = NULL, *ly3 = NULL;
		pcb_coord_t clr;
		pcb_poly_t *p;

		if ((shp->layer_mask & PCB_LYT_ANYTHING) == 0)
			pcb_message(PCB_MSG_ERROR, "ERROR: breaking up padstack prototype: shape %d has invalid layer type, placing it on a random layer\nTHIS PADSTACK PROTOTYPE MUST BE FIXED.\n");

		/* look up the best layer type */
		for(lid = 0; lid < dst->LayerN; lid++) {
			ly = &dst->Layer[lid];
			lyt = pcb_layer_flags_(ly);

TODO("layer: make a real scoring mechanism here instead of ly1, ly2, ly3")
			/* cheat: pretend boundary layers are mech layers because padstack layer types are interested only in mech layers */
			if (lyt & PCB_LYT_BOUNDARY) {
				lyt &= ~PCB_LYT_BOUNDARY;
				lyt |= PCB_LYT_MECH;
			}

			if ((lyt & shp->layer_mask) == shp->layer_mask) {
				int comb_match = (shp->comb == ly->comb);
				int plate_match = 1;

				if (lyt & PCB_LYT_MECH) { /* check plating only for mech layers */
					int purpi = pcb_layer_purpose_(ly, NULL);
					int ly_plated = !!PCB_LAYER_IS_PROUTE(lyt, purpi);
					plate_match = (ly_plated == proto->hplated);
				}

				if (comb_match && plate_match) {
					ly1 = ly;
					break;
				}
				else
					ly2 = ly;
			}
			else if ((lyt & shp->layer_mask & filt) == (shp->layer_mask & filt))
				ly3 = ly;
		}

		ly = ly1;
		if (ly == NULL) ly = ly2;
		if (ly == NULL) ly = ly3;
		if (ly == NULL) {
			const char *locs, *mats;
			locs = pcb_layer_type_bit2str(lyt & PCB_LYT_ANYWHERE);
			mats = pcb_layer_type_bit2str(lyt & PCB_LYT_ANYTHING);
			if (locs == NULL) locs = "<unknown loc>";
			if (mats == NULL) mats = "<unknown material>";
			pcb_message(PCB_MSG_WARNING, "Can not create shape on %s %s\n", locs, mats);
			continue;
		}

		clr = src->Clearance == 0 ? shp->clearance : src->Clearance;
		switch(shp->shape) {
			case PCB_PSSH_CIRC:
				no = (pcb_any_obj_t *)pcb_line_new(ly,
					src->x + shp->data.circ.x, src->y + shp->data.circ.y,
					src->x + shp->data.circ.x, src->y + shp->data.circ.y,
					shp->data.circ.dia, clr, pcb_flag_make(PCB_FLAG_CLEARLINE));
					break;
			case PCB_PSSH_LINE:
				no = (pcb_any_obj_t *)pcb_line_new(ly,
					src->x + shp->data.line.x1, src->y + shp->data.line.y1,
					src->x + shp->data.line.x2, src->y + shp->data.line.y2,
					shp->data.line.thickness, clr, pcb_flag_make(PCB_FLAG_CLEARLINE));
					break;
			case PCB_PSSH_POLY:
				p = pcb_poly_new(ly, clr, pcb_flag_make(PCB_FLAG_CLEARPOLYPOLY));
				for(i = 0; i < shp->data.poly.len; i++)
					pcb_poly_point_new(p, src->x + shp->data.poly.x[i], src->y + shp->data.poly.y[i]);
				pcb_add_poly_on_layer(ly, p);
				no = (pcb_any_obj_t *)p;
				break;
			default:
				assert(!"invalid shape");
				continue;
		}
		if (src->term != NULL)
			pcb_attribute_put(&no->Attributes, "term", src->term);
	}

	if (remove_src) {
		if (src->parent.data->padstack_tree != NULL)
			pcb_r_delete_entry(src->parent.data->padstack_tree, (pcb_box_t *)src);
		pcb_pstk_free(src);
	}

	return -1;
}

int pcb_pstk_proto_conv_selection(pcb_board_t *pcb, pcb_pstk_proto_t *dst, int quiet, pcb_coord_t ox, pcb_coord_t oy)
{
	int ret;
	vtp0_t objs;

	vtp0_init(&objs);
	pcb_data_list_by_flag(pcb->Data, &objs, PCB_OBJ_CLASS_REAL, PCB_FLAG_SELECTED);
	ret = pcb_pstk_proto_conv(pcb->Data, dst, quiet, &objs, ox, oy);
	vtp0_uninit(&objs);

	return ret;
}


int pcb_pstk_proto_conv_buffer(pcb_pstk_proto_t *dst, int quiet)
{
	int ret;
	vtp0_t objs;
	pcb_coord_t ox, oy;
	pcb_box_t bb;

	pcb_data_bbox(&bb, PCB_PASTEBUFFER->Data, 0);

	ox = (bb.X1 + bb.X2) / 2;
	oy = (bb.Y1 + bb.Y2) / 2;

	vtp0_init(&objs);
	pcb_data_list_by_flag(PCB_PASTEBUFFER->Data, &objs, PCB_OBJ_CLASS_REAL, 0);
	ret = pcb_pstk_proto_conv(PCB_PASTEBUFFER->Data, dst, quiet, &objs, ox, oy);
	vtp0_uninit(&objs);

	return ret;
}

void pcb_pstk_shape_copy(pcb_pstk_shape_t *dst, pcb_pstk_shape_t *src)
{
	memcpy(dst, src, sizeof(pcb_pstk_shape_t));
	switch(src->shape) {
		case PCB_PSSH_LINE:
		case PCB_PSSH_CIRC:
		case PCB_PSSH_HSHADOW:
			break; /* do nothing, all fields are copied already by the memcpy */
		case PCB_PSSH_POLY:
			pcb_pstk_shape_alloc_poly(&dst->data.poly, src->data.poly.len);
			pcb_pstk_shape_copy_poly(&dst->data.poly, &src->data.poly);
			break;
	}
}

void pcb_pstk_shape_free(pcb_pstk_shape_t *s)
{
	switch(s->shape) {
		case PCB_PSSH_LINE:
		case PCB_PSSH_CIRC:
		case PCB_PSSH_HSHADOW:
			break; /* no allocation */
		case PCB_PSSH_POLY:
			pcb_pstk_shape_free_poly(&s->data.poly);
			break;
	}
}

void pcb_pstk_tshape_copy(pcb_pstk_tshape_t *ts_dst, pcb_pstk_tshape_t *ts_src)
{
	int n;

	ts_dst->rot = ts_src->rot;
	ts_dst->xmirror = ts_src->xmirror;
	ts_dst->smirror = ts_src->smirror;
	ts_dst->shape = malloc(sizeof(pcb_pstk_shape_t) * ts_src->len);
	ts_dst->len = ts_src->len;
	memcpy(ts_dst->shape, ts_src->shape, sizeof(pcb_pstk_shape_t) * ts_src->len);
	for(n = 0; n < ts_src->len; n++) {
		switch(ts_src->shape[n].shape) {
			case PCB_PSSH_LINE:
			case PCB_PSSH_CIRC:
			case PCB_PSSH_HSHADOW:
				break; /* do nothing, all fields are copied already by the memcpy */
			case PCB_PSSH_POLY:
				pcb_pstk_shape_alloc_poly(&ts_dst->shape[n].data.poly, ts_src->shape[n].data.poly.len);
				pcb_pstk_shape_copy_poly(&ts_dst->shape[n].data.poly, &ts_src->shape[n].data.poly);
				break;
		}
	}
}

void pcb_pstk_shape_rot(pcb_pstk_shape_t *sh, double sina, double cosa, double angle)
{
	int i;

	switch(sh->shape) {
		case PCB_PSSH_LINE:
			pcb_rotate(&sh->data.line.x1, &sh->data.line.y1, 0, 0, cosa, sina);
			pcb_rotate(&sh->data.line.x2, &sh->data.line.y2, 0, 0, cosa, sina);
			break;
		case PCB_PSSH_CIRC:
			pcb_rotate(&sh->data.circ.x, &sh->data.circ.y, 0, 0, cosa, sina);
			break;
		case PCB_PSSH_HSHADOW:
			break;
		case PCB_PSSH_POLY:
			if (sh->data.poly.pa != NULL)
				pcb_polyarea_free(&sh->data.poly.pa);
			for(i = 0; i < sh->data.poly.len; i++)
				pcb_rotate(&sh->data.poly.x[i], &sh->data.poly.y[i], 0, 0, cosa, sina);
			pcb_pstk_shape_update_pa(&sh->data.poly);
			break;
	}
}


void pcb_pstk_tshape_rot(pcb_pstk_tshape_t *ts, double angle)
{
	int n;
	double cosa = cos(angle / PCB_RAD_TO_DEG), sina = sin(angle / PCB_RAD_TO_DEG);

	for(n = 0; n < ts->len; n++)
		pcb_pstk_shape_rot(&ts->shape[n], sina, cosa, angle);
}

void pcb_pstk_tshape_xmirror(pcb_pstk_tshape_t *ts)
{
	int n, i;

	for(n = 0; n < ts->len; n++) {
		pcb_pstk_shape_t *sh = &ts->shape[n];
		switch(sh->shape) {
			case PCB_PSSH_LINE:
				sh->data.line.y1 = -sh->data.line.y1;
				sh->data.line.y2 = -sh->data.line.y2;
				break;
			case PCB_PSSH_CIRC:
				sh->data.circ.y = -sh->data.circ.y;
				break;
			case PCB_PSSH_HSHADOW:
				break;
			case PCB_PSSH_POLY:
				if (sh->data.poly.pa != NULL)
					pcb_polyarea_free(&sh->data.poly.pa);
				for(i = 0; i < sh->data.poly.len; i++)
					sh->data.poly.y[i] = -sh->data.poly.y[i];
				pcb_pstk_shape_update_pa(&sh->data.poly);
				break;
		}
	}
}

void pcb_pstk_tshape_smirror(pcb_pstk_tshape_t *ts)
{
	int n;

	for(n = 0; n < ts->len; n++) {
		pcb_pstk_shape_t *sh = &ts->shape[n];
		if (sh->layer_mask & PCB_LYT_TOP) {
			sh->layer_mask &= ~PCB_LYT_TOP;
			sh->layer_mask |= PCB_LYT_BOTTOM;
		}
		else if (sh->layer_mask & PCB_LYT_BOTTOM) {
			sh->layer_mask &= ~PCB_LYT_BOTTOM;
			sh->layer_mask |= PCB_LYT_TOP;
		}
	}
}

void pcb_pstk_proto_copy(pcb_pstk_proto_t *dst, const pcb_pstk_proto_t *src)
{
	pcb_pstk_tshape_t *ts_dst, *ts_src;

	memcpy(dst, src, sizeof(pcb_pstk_proto_t));
	pcb_vtpadstack_tshape_init(&dst->tr);

	if (src->tr.used > 0) {
		ts_src = &src->tr.array[0];

		/* allocate shapes on the canonical tshape (tr[0]) */
		ts_dst = pcb_vtpadstack_tshape_alloc_append(&dst->tr, 1);
		pcb_pstk_tshape_copy(ts_dst, ts_src);

		/* make sure it's the canonical form */
		ts_dst->rot = 0.0;
		ts_dst->xmirror = 0;
		ts_dst->smirror = 0;
	}

	dst->in_use = 1;
}


/* Matches proto against all protos in data's cache; returns
   PCB_PADSTACK_INVALID (and loads first_free_out) if not found */
static pcb_cardinal_t pcb_pstk_proto_insert_try(pcb_data_t *data, const pcb_pstk_proto_t *proto, pcb_cardinal_t *first_free_out)
{
	pcb_cardinal_t n, first_free = PCB_PADSTACK_INVALID;

	/* look for the first existing padstack that matches */
	for(n = 0; n < pcb_vtpadstack_proto_len(&data->ps_protos); n++) {
		if (!(data->ps_protos.array[n].in_use)) {
			if (first_free == PCB_PADSTACK_INVALID)
				first_free = n;
		}
		else if (data->ps_protos.array[n].hash == proto->hash) {
			if (pcb_pstk_proto_eq(&data->ps_protos.array[n], proto))
				return n;
		}
	}
	*first_free_out = first_free;
	return PCB_PADSTACK_INVALID;
}

pcb_cardinal_t pcb_pstk_proto_insert_or_free(pcb_data_t *data, pcb_pstk_proto_t *proto, int quiet)
{
	pcb_cardinal_t n, first_free;

	n = pcb_pstk_proto_insert_try(data, proto, &first_free);
	if (n != PCB_PADSTACK_INVALID) {
		pcb_pstk_proto_free_fields(proto);
		return n; /* already in cache */
	}

	/* no match, have to register a new one */
	if (first_free == PCB_PADSTACK_INVALID) {
		pcb_pstk_proto_t *np;
		n = pcb_vtpadstack_proto_len(&data->ps_protos);
		pcb_vtpadstack_proto_append(&data->ps_protos, *proto);
		np = pcb_vtpadstack_proto_get(&data->ps_protos, n, 0);
		np->parent = data;
		pcb_pstk_proto_update(np);
	}
	else {
		memcpy(data->ps_protos.array+first_free, proto, sizeof(pcb_pstk_proto_t));
		data->ps_protos.array[first_free].in_use = 1;
		data->ps_protos.array[first_free].parent = data;
		pcb_pstk_proto_update(data->ps_protos.array+first_free);
	}
	memset(proto, 0, sizeof(pcb_pstk_proto_t)); /* make sure a subsequent free() won't do any harm */
	return n;
}

static pcb_cardinal_t pcb_pstk_proto_insert_dup_(pcb_data_t *data, const pcb_pstk_proto_t *proto, int quiet, int forcedup)
{
	pcb_cardinal_t n, first_free = PCB_PADSTACK_INVALID;

	n = pcb_pstk_proto_insert_try(data, proto, &first_free);
	if ((n != PCB_PADSTACK_INVALID) && (!forcedup))
		return n; /* already in cache */

	/* no match, have to register a new one, which is a dup of the original */
	if (first_free == PCB_PADSTACK_INVALID) {
		pcb_pstk_proto_t *nproto;
		n = pcb_vtpadstack_proto_len(&data->ps_protos);
		nproto = pcb_vtpadstack_proto_alloc_append(&data->ps_protos, 1);
		pcb_pstk_proto_copy(nproto, proto);
		nproto->parent = data;
		pcb_pstk_proto_update(nproto);
	}
	else {
		pcb_pstk_proto_copy(data->ps_protos.array+first_free, proto);
		data->ps_protos.array[first_free].in_use = 1;
		data->ps_protos.array[first_free].parent = data;
		pcb_pstk_proto_update(data->ps_protos.array+first_free);
	}
	return n;
}

pcb_cardinal_t pcb_pstk_proto_insert_dup(pcb_data_t *data, const pcb_pstk_proto_t *proto, int quiet)
{
	return pcb_pstk_proto_insert_dup_(data, proto, quiet, 0);
}

pcb_cardinal_t pcb_pstk_proto_insert_forcedup(pcb_data_t *data, const pcb_pstk_proto_t *proto, int quiet)
{
	return pcb_pstk_proto_insert_dup_(data, proto, quiet, 1);
}


pcb_cardinal_t pcb_pstk_proto_replace(pcb_data_t *data, pcb_cardinal_t proto_id, const pcb_pstk_proto_t *src)
{
	pcb_pstk_proto_t *dst;
	if ((proto_id < 0) || (proto_id >= pcb_vtpadstack_proto_len(&data->ps_protos)))
		return PCB_PADSTACK_INVALID;

	dst = &data->ps_protos.array[proto_id];
	pcb_pstk_proto_free_fields(dst);

	pcb_pstk_proto_copy(dst, src);
	dst->in_use = 1;
	dst->parent = data;
	pcb_pstk_proto_update(dst);
	return proto_id;
}


pcb_cardinal_t pcb_pstk_conv_selection(pcb_board_t *pcb, int quiet, pcb_coord_t ox, pcb_coord_t oy)
{
	pcb_pstk_proto_t proto;

	if (pcb_pstk_proto_conv_selection(pcb, &proto, quiet, ox, oy) != 0)
		return -1;

	return pcb_pstk_proto_insert_or_free(pcb->Data, &proto, quiet);
}

pcb_cardinal_t pcb_pstk_conv_buffer(int quiet)
{
	pcb_pstk_proto_t proto;

	if (pcb_pstk_proto_conv_buffer(&proto, quiet) != 0)
		return -1;

	return pcb_pstk_proto_insert_or_free(PCB_PASTEBUFFER->Data, &proto, quiet);
}


void pcb_pstk_shape_update_pa(pcb_pstk_poly_t *poly)
{
	int n;
	pcb_vector_t v;
	pcb_pline_t *pl;

	v[0] = poly->x[0]; v[1] = poly->y[0];
	pl = pcb_poly_contour_new(v);
	for(n = 1; n < poly->len; n++) {
		v[0] = poly->x[n]; v[1] = poly->y[n];
		pcb_poly_vertex_include(pl->head.prev, pcb_poly_node_create(v));
	}
	pcb_poly_contour_pre(pl, 1);

	poly->pa = pcb_polyarea_create();
	pcb_polyarea_contour_include(poly->pa, pl);

	if (!pcb_poly_valid(poly->pa)) {
		poly->pa->contours = NULL; /* keep pl safe from pcb_polyarea_free() */
		pcb_polyarea_free(&poly->pa);
		
		poly->pa = pcb_polyarea_create();
		pcb_poly_contour_inv(pl);
		pcb_polyarea_contour_include(poly->pa, pl);
		poly->inverted = 1;
	}
	else
		poly->inverted = 0;
}

/*** Undoable hole change ***/

typedef struct {
	long int parent_ID; /* -1 for pcb, positive for a subc */
	pcb_cardinal_t proto;

	int hplated;
	pcb_coord_t hdia;
	int htop, hbottom;
} padstack_proto_change_hole_t;

#define swap(a,b,type) \
	do { \
		type tmp = a; \
		a = b; \
		b = tmp; \
	} while(0)

static int undo_change_hole_swap(void *udata)
{
	padstack_proto_change_hole_t *u = udata;
	pcb_data_t *data;
	pcb_pstk_proto_t *proto;

	if (u->parent_ID != -1) {
		pcb_subc_t *subc = pcb_subc_by_id(PCB->Data, u->parent_ID);
		if (subc == NULL) {
			pcb_message(PCB_MSG_ERROR, "Can't undo padstack prototype hole change: parent subc #%ld is not found\n", u->parent_ID);
			return -1;
		}
		data = subc->data;
	}
	else
		data = PCB->Data;

	proto = pcb_pstk_get_proto_(data, u->proto);
	if (proto == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't undo padstack prototype hole change: proto ID #%ld is not available\n", u->parent_ID);
		return -1;
	}

	swap(proto->hplated, u->hplated, int);
	swap(proto->hdia,    u->hdia,    pcb_coord_t);
	swap(proto->htop,    u->htop,    int);
	swap(proto->hbottom, u->hbottom, int);
	return 0;
}

static void undo_change_hole_print(void *udata, char *dst, size_t dst_len)
{
	padstack_proto_change_hole_t *u = udata;
	pcb_snprintf(dst, dst_len, "padstack proto hole change: plated=%d dia=%$mm top=%d bottom=%d\n", u->hplated, u->hdia, u->htop, u->hbottom);
}

static const uundo_oper_t undo_pstk_proto_change_hole = {
	core_proto_cookie,
	NULL, /* free */
	undo_change_hole_swap,
	undo_change_hole_swap,
	undo_change_hole_print
};

int pcb_pstk_proto_change_hole(pcb_pstk_proto_t *proto, const int *hplated, const pcb_coord_t *hdia, const int *htop, const int *hbottom)
{
	padstack_proto_change_hole_t *u;
	long int parent_ID;

	switch(proto->parent->parent_type) {
		case PCB_PARENT_BOARD: parent_ID = -1; break;
		case PCB_PARENT_SUBC: parent_ID = proto->parent->parent.subc->ID; break;
		default: return -1;
	}

	u = pcb_undo_alloc(PCB, &undo_pstk_proto_change_hole, sizeof(padstack_proto_change_hole_t));
	u->parent_ID = parent_ID;
	u->proto = pcb_pstk_get_proto_id(proto);
	u->hplated = hplated ? *hplated : proto->hplated;
	u->hdia = hdia ? *hdia : proto->hdia;
	u->htop = htop ? *htop : proto->htop;
	u->hbottom = hbottom ? *hbottom : proto->hbottom;
	undo_change_hole_swap(u);

	pcb_undo_inc_serial();
	return 0;
}

int pcb_pstk_proto_change_name(pcb_pstk_proto_t *proto, const char *new_name)
{
TODO("undo: make this undoable (check how pcb_pstk_proto_change_hole() does it)")
	free(proto->name);
	if ((new_name == NULL) || (*new_name == '\0'))
		proto->name = NULL;
	else
		proto->name = pcb_strdup(new_name);
	return 0;
}

#define TSHAPE_ANGLE_TOL 0.01
#define tshape_angle_eq(a1, a2) (((a1 - a2) >= -TSHAPE_ANGLE_TOL) && ((a1 - a2) <= TSHAPE_ANGLE_TOL))

pcb_pstk_tshape_t *pcb_pstk_make_tshape(pcb_data_t *data, pcb_pstk_proto_t *proto, double rot, int xmirror, int smirror, int *out_protoi)
{
	size_t n;
	pcb_pstk_tshape_t *ts;

	xmirror = !!xmirror;
	smirror = !!smirror;

	/* cheap case: canonical */
	if (tshape_angle_eq(rot, 0.0) && (xmirror == 0) && (smirror == 0)) {
		if (out_protoi != NULL) *out_protoi = 0;
		return &proto->tr.array[0];
	}

	/* search for an existing version in the cache - we expect only a few
	   transformations per padstack, the result is cached -> linear search. */
	for(n = 0; n < proto->tr.used; n++) {
		if (tshape_angle_eq(proto->tr.array[n].rot, rot) && (proto->tr.array[n].xmirror == xmirror) && (proto->tr.array[n].smirror == smirror)) {
			if (out_protoi != NULL) *out_protoi = n;
			return &proto->tr.array[n];
		}
	}

	/* allocate and render the transformed version for the cache */
	if (out_protoi != NULL) *out_protoi = proto->tr.used;
	ts = pcb_vtpadstack_tshape_alloc_append(&proto->tr, 1);

	/* first make a vanilla copy */
	pcb_pstk_tshape_copy(ts, &proto->tr.array[0]);

	if (!tshape_angle_eq(rot, 0.0))
		pcb_pstk_tshape_rot(ts, rot);

	if (xmirror)
		pcb_pstk_tshape_xmirror(ts);

	if (smirror)
		pcb_pstk_tshape_smirror(ts);

	ts->rot = rot;
	ts->xmirror = xmirror;
	ts->smirror = smirror;
	return ts;
}

static void pcb_pstk_poly_center(const pcb_pstk_poly_t *poly, pcb_coord_t *cx, pcb_coord_t *cy)
{
	double x = 0.0, y = 0.0;
	int n;

	for(n = 0; n < poly->len; n++) {
		x += (double)poly->x[n];
		y += (double)poly->y[n];
	}
	x /= (double)poly->len;
	y /= (double)poly->len;
	*cx = pcb_round(x);
	*cy = pcb_round(y);
}

void pcb_pstk_shape_grow(pcb_pstk_shape_t *shp, pcb_bool is_absolute, pcb_coord_t val)
{
	pcb_coord_t cx, cy;
	int n;

TODO("padstack: undo")

	switch(shp->shape) {
		case PCB_PSSH_LINE:
			if (is_absolute)
				shp->data.line.thickness = val;
			else
				shp->data.line.thickness += val;
			if (shp->data.line.thickness < 1)
				shp->data.line.thickness = 1;
			break;
		case PCB_PSSH_CIRC:
			if (is_absolute)
				shp->data.circ.dia = val;
			else
				shp->data.circ.dia += val;
			if (shp->data.circ.dia < 1)
				shp->data.circ.dia = 1;
			break;
		case PCB_PSSH_HSHADOW:
			break;
		case PCB_PSSH_POLY:
			pcb_pstk_poly_center(&shp->data.poly, &cx, &cy);
			pcb_polyarea_free(&shp->data.poly.pa);
			if (is_absolute) {
TODO("TODO")
			}
			else {
				pcb_polo_t *p, p_st[32];
				double vl = pcb_round(val/2);

				if (shp->data.poly.len >= sizeof(p_st) / sizeof(p_st[0]))
					p = malloc(sizeof(pcb_polo_t) * shp->data.poly.len);
				else
					p = p_st;

				/* relative: move each point radially */
				for(n = 0; n < shp->data.poly.len; n++) {
					p[n].x = shp->data.poly.x[n];
					p[n].y = shp->data.poly.y[n];
				}
				pcb_polo_norms(p, shp->data.poly.len);
				pcb_polo_offs(vl, p, shp->data.poly.len);
				for(n = 0; n < shp->data.poly.len; n++) {
					shp->data.poly.x[n] = p[n].x;
					shp->data.poly.y[n] = p[n].y;
				}
				if (p != p_st)
					free(p);
			}
			pcb_pstk_shape_update_pa(&shp->data.poly);
			break;
	}
}

void pcb_pstk_shape_scale(pcb_pstk_shape_t *shp, double sx, double sy)
{
	pcb_coord_t cx, cy;
	int n;

TODO("padstack: undo")

	switch(shp->shape) {
		case PCB_PSSH_LINE:
			shp->data.line.thickness = pcb_round(shp->data.line.thickness * ((sx+sy)/2.0));
			if (shp->data.line.thickness < 1)
				shp->data.line.thickness = 1;
			shp->data.line.x1 = pcb_round((double)shp->data.line.x1 * sx);
			shp->data.line.y1 = pcb_round((double)shp->data.line.y1 * sy);
			shp->data.line.x2 = pcb_round((double)shp->data.line.x2 * sx);
			shp->data.line.y2 = pcb_round((double)shp->data.line.y2 * sy);
			break;
		case PCB_PSSH_CIRC:
			shp->data.circ.dia = pcb_round(shp->data.circ.dia * ((sx+sy)/2.0));
			if (shp->data.circ.dia < 1)
				shp->data.circ.dia = 1;
			shp->data.circ.x = pcb_round((double)shp->data.circ.x * sx);
			shp->data.circ.y = pcb_round((double)shp->data.circ.y * sy);
			break;
		case PCB_PSSH_HSHADOW:
			break;
		case PCB_PSSH_POLY:
			pcb_pstk_poly_center(&shp->data.poly, &cx, &cy);
			pcb_polyarea_free(&shp->data.poly.pa);

			for(n = 0; n < shp->data.poly.len; n++) {
				shp->data.poly.x[n] = pcb_round((double)shp->data.poly.x[n] * sx);
				shp->data.poly.y[n] = pcb_round((double)shp->data.poly.y[n] * sy);
			}
			pcb_pstk_shape_update_pa(&shp->data.poly);
			break;
	}
}

void pcb_pstk_shape_clr_grow(pcb_pstk_shape_t *shp, pcb_bool is_absolute, pcb_coord_t val)
{
TODO("padstack: undo")
	if (is_absolute)
		shp->clearance = val;
	else
		shp->clearance += val;
}

void pcb_pstk_proto_grow(pcb_pstk_proto_t *proto, pcb_bool is_absolute, pcb_coord_t val)
{
	int n, i;

	/* do the same growth on all shapes of all transformed variants */
	for(n = 0; n < proto->tr.used; n++)
		for(i = 0; i < proto->tr.array[n].len; i++)
			pcb_pstk_shape_grow(&proto->tr.array[n].shape[i], is_absolute, val);
	pcb_pstk_proto_update(proto);
}

void pcb_pstk_shape_derive(pcb_pstk_proto_t *proto, int dst_idx, int src_idx, pcb_coord_t bloat, pcb_layer_type_t mask, pcb_layer_combining_t comb)
{
	int n;

	/* do the same copy on all shapes of all transformed variants */
	for(n = 0; n < proto->tr.used; n++) {
		int d = dst_idx;
		if (d < 0) {
			d = proto->tr.array[n].len;
			proto->tr.array[n].len++;
			proto->tr.array[n].shape = realloc(proto->tr.array[n].shape, proto->tr.array[n].len * sizeof(proto->tr.array[n].shape[0]));
			
		}
		else
			pcb_pstk_shape_free(&proto->tr.array[n].shape[d]);
		pcb_pstk_shape_copy(&proto->tr.array[n].shape[d], &proto->tr.array[n].shape[src_idx]);
		proto->tr.array[n].shape[d].layer_mask = mask;
		proto->tr.array[n].shape[d].comb = comb;
		if (bloat != 0)
			pcb_pstk_shape_grow(&proto->tr.array[n].shape[d], pcb_false, bloat);
	}
	pcb_pstk_proto_update(proto);
}

int pcb_pstk_shape_swap_layer(pcb_pstk_proto_t *proto, int idx1, int idx2)
{
	int n;
	pcb_layer_type_t lm;
	pcb_layer_combining_t lc;

	if ((idx1 < 0) || (idx1 > proto->tr.array[0].len))
		return -1;
	if ((idx2 < 0) || (idx2 > proto->tr.array[0].len))
		return -1;

	for(n = 0; n < proto->tr.used; n++) {
		lm = proto->tr.array[n].shape[idx1].layer_mask;
		lc = proto->tr.array[n].shape[idx1].comb;
		proto->tr.array[n].shape[idx1].layer_mask = proto->tr.array[n].shape[idx2].layer_mask;
		proto->tr.array[n].shape[idx1].comb = proto->tr.array[n].shape[idx2].comb;
		proto->tr.array[n].shape[idx2].layer_mask = lm;
		proto->tr.array[n].shape[idx2].comb = lc;
	}
	return 0;
}


void pcb_pstk_shape_add_hshadow(pcb_pstk_proto_t *proto, pcb_layer_type_t mask, pcb_layer_combining_t comb)
{
	int n;

	/* do the same on all shapes of all transformed variants */
	for(n = 0; n < proto->tr.used; n++) {
		int d = proto->tr.array[n].len;
		proto->tr.array[n].len++;
		proto->tr.array[n].shape = realloc(proto->tr.array[n].shape, proto->tr.array[n].len * sizeof(proto->tr.array[n].shape[0]));
		proto->tr.array[n].shape[d].shape = PCB_PSSH_HSHADOW;
		proto->tr.array[n].shape[d].layer_mask = mask;
		proto->tr.array[n].shape[d].comb = comb;
	}
	pcb_pstk_proto_update(proto);
}


static void pcb_pstk_tshape_del_idx(pcb_pstk_tshape_t *shp, int idx)
{
	int n;
	for(n = idx; n < shp->len-1; n++)
		shp->shape[n] = shp->shape[n+1];
	shp->len--;
}

void pcb_pstk_proto_del_shape_idx(pcb_pstk_proto_t *proto, int idx)
{
	int n;

	if ((proto->tr.used == 0) || (idx < 0) || (idx >= proto->tr.array[0].len))
		return;

	/* delete the shape from all transformed variants */
	for(n = 0; n < proto->tr.used; n++)
		pcb_pstk_tshape_del_idx(&proto->tr.array[n], idx);

	pcb_pstk_proto_update(proto);
}

void pcb_pstk_proto_del_shape(pcb_pstk_proto_t *proto, pcb_layer_type_t lyt, pcb_layer_combining_t comb)
{
	int idx;

	if (proto->tr.used == 0)
		return;

	/* search the 0th transformed, all other tshapes are the same */
	idx = pcb_pstk_get_shape_idx(&proto->tr.array[0], lyt, comb);
	pcb_pstk_proto_del_shape_idx(proto, idx);
}

void pcb_pstk_proto_del(pcb_data_t *data, pcb_cardinal_t proto_id)
{
	pcb_pstk_proto_t *proto = pcb_vtpadstack_proto_get(&data->ps_protos, proto_id, 0);
	if (proto == NULL)
		return;
	pcb_pstk_proto_free_fields(proto);
}

pcb_cardinal_t *pcb_pstk_proto_used_all(pcb_data_t *data, pcb_cardinal_t *len_out)
{
	pcb_cardinal_t len, *res;
	pcb_pstk_t *ps;

	len = data->ps_protos.used;
	if (len == 0) {
		*len_out = 0;
		return NULL;
	}

	res = calloc(sizeof(pcb_cardinal_t), len);
	for(ps = padstacklist_first(&data->padstack); ps != NULL; ps = padstacklist_next(ps)) {
		if ((ps->proto >= 0) && (ps->proto < len))
			res[ps->proto]++;
	}

	/* routing styles may also reference to prototypes if we are on a board */
	if (data->parent_type == PCB_PARENT_BOARD) {
		pcb_board_t *pcb = data->parent.board;
		int n;

		for(n = 0; n < pcb->RouteStyle.used; n++) {
			if (pcb->RouteStyle.array[n].via_proto_set) {
				pcb_cardinal_t pid = pcb->RouteStyle.array[n].via_proto;
				if ((pid >= 0) && (pid < len))
					res[pid]++;
			}
		}
	}

	*len_out = len;
	return res;
}

/*** hash ***/
static unsigned int pcb_pstk_shape_hash(const pcb_pstk_shape_t *sh)
{
	unsigned int n, ret = murmurhash32(sh->layer_mask) ^ murmurhash32(sh->comb) ^ pcb_hash_coord(sh->clearance);

	switch(sh->shape) {
		case PCB_PSSH_POLY:
			for(n = 0; n < sh->data.poly.len; n++)
				ret ^= pcb_hash_coord(sh->data.poly.x[n]) ^ pcb_hash_coord(sh->data.poly.y[n]);
			break;
		case PCB_PSSH_LINE:
			ret ^= pcb_hash_coord(sh->data.line.x1) ^ pcb_hash_coord(sh->data.line.x2) ^ pcb_hash_coord(sh->data.line.y1) ^ pcb_hash_coord(sh->data.line.y2);
			ret ^= pcb_hash_coord(sh->data.line.thickness);
			ret ^= sh->data.line.square;
			break;
		case PCB_PSSH_CIRC:
			ret ^= pcb_hash_coord(sh->data.circ.x) ^ pcb_hash_coord(sh->data.circ.y);
			ret ^= pcb_hash_coord(sh->data.circ.dia);
			break;
		case PCB_PSSH_HSHADOW:
			break;
	}

	return ret;
}

unsigned int pcb_pstk_proto_hash(const pcb_pstk_proto_t *p)
{
	pcb_pstk_tshape_t *ts = &p->tr.array[0];
	unsigned int n, ret = pcb_hash_coord(p->hdia) ^ pcb_hash_coord(p->htop) ^ pcb_hash_coord(p->hbottom) ^ pcb_hash_coord(p->hplated);
	if (ts != NULL) {
	 ret ^= pcb_hash_coord(ts->len);
		for(n = 0; n < ts->len; n++)
			ret ^= pcb_pstk_shape_hash(ts->shape + n);
	}
	return ret;
}

int pcb_pstk_shape_eq(const pcb_pstk_shape_t *sh1, const pcb_pstk_shape_t *sh2)
{
	int n;

	if (sh1->layer_mask != sh2->layer_mask) return 0;
	if (sh1->comb != sh2->comb) return 0;
	if (sh1->clearance != sh2->clearance) return 0;
	if (sh1->shape != sh2->shape) return 0;

	switch(sh1->shape) {
		case PCB_PSSH_POLY:
			if (sh1->data.poly.len != sh2->data.poly.len) return 0;
			for(n = 0; n < sh1->data.poly.len; n++) {
				if (sh1->data.poly.x[n] != sh2->data.poly.x[n]) return 0;
				if (sh1->data.poly.y[n] != sh2->data.poly.y[n]) return 0;
			}
			break;
		case PCB_PSSH_LINE:
			if (sh1->data.line.x1 != sh2->data.line.x1) return 0;
			if (sh1->data.line.x2 != sh2->data.line.x2) return 0;
			if (sh1->data.line.y1 != sh2->data.line.y1) return 0;
			if (sh1->data.line.y2 != sh2->data.line.y2) return 0;
			if (sh1->data.line.thickness != sh2->data.line.thickness) return 0;
			if (sh1->data.line.square != sh2->data.line.square) return 0;
			break;
		case PCB_PSSH_CIRC:
			if (sh1->data.circ.x != sh2->data.circ.x) return 0;
			if (sh1->data.circ.y != sh2->data.circ.y) return 0;
			if (sh1->data.circ.dia != sh2->data.circ.dia) return 0;
			break;
		case PCB_PSSH_HSHADOW:
			break;
	}

	return 1;
}

int pcb_pstk_proto_eq(const pcb_pstk_proto_t *p1, const pcb_pstk_proto_t *p2)
{
	pcb_pstk_tshape_t *ts1 = &p1->tr.array[0], *ts2 = &p2->tr.array[0];
	int n1, n2;

	if (p1->hdia != p2->hdia) return 0;
	if (p1->htop != p2->htop) return 0;
	if (p1->hbottom != p2->hbottom) return 0;
	if (p1->hplated != p2->hplated) return 0;
	if ((ts1 == NULL) && (ts2 != NULL)) return 0;
	if ((ts2 == NULL) && (ts1 != NULL)) return 0;

	if (ts1 != NULL) {
		if (ts1->len != ts2->len) return 0;
		for(n1 = 0; n1 < ts1->len; n1++) {
			for(n2 = 0; n2 < ts2->len; n2++)
				if (pcb_pstk_shape_eq(ts1->shape + n1, ts2->shape + n2))
					goto found;
			return 0;
			found:;
		}
	}

	return 1;
}

