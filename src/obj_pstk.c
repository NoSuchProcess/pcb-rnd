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

#include <assert.h>

#include "board.h"
#include "conf_core.h"
#include "data.h"
#include "data_list.h"
#include "draw.h"
#include "draw_wireframe.h"
#include "flag.h"
#include "obj_pstk.h"
#include "obj_pstk_draw.h"
#include "obj_pstk_list.h"
#include "obj_pstk_inlines.h"
#include "obj_pstk_op.h"
#include "obj_subc_parent.h"
#include "operation.h"
#include "search.h"
#include "undo.h"
#include "vtpadstack.h"

#define PS_CROSS_SIZE PCB_MM_TO_COORD(1)

#define SQR(o) ((o)*(o))

static const char core_pstk_cookie[] = "padstack";

pcb_pstk_t *pcb_pstk_alloc(pcb_data_t *data)
{
	pcb_pstk_t *ps;

	ps = calloc(sizeof(pcb_pstk_t), 1);
	ps->protoi = -1;
	ps->type = PCB_OBJ_PSTK;
	ps->Attributes.post_change = pcb_obj_attrib_post_change;
	PCB_SET_PARENT(ps, data, data);

	padstacklist_append(&data->padstack, ps);

	return ps;
}

void pcb_pstk_free(pcb_pstk_t *ps)
{
	padstacklist_remove(ps);
	free(ps->thermals.shape);
	free(ps);
}

pcb_pstk_t *pcb_pstk_new(pcb_data_t *data, pcb_cardinal_t proto, pcb_coord_t x, pcb_coord_t y, pcb_coord_t clearance, pcb_flag_t Flags)
{
	pcb_pstk_t *ps;

	if (proto >= pcb_vtpadstack_proto_len(&data->ps_protos))
		return NULL;

	ps = pcb_pstk_alloc(data);

	/* copy values */
	ps->proto = proto;
	ps->x = x;
	ps->y = y;
	ps->Clearance = clearance;
	ps->Flags = Flags;
	ps->ID = pcb_create_ID_get();
	pcb_pstk_add(data, ps);
	pcb_poly_clear_from_poly(data, PCB_TYPE_PSTK, NULL, ps);
	return ps;
}

void pcb_pstk_add(pcb_data_t *data, pcb_pstk_t *ps)
{
	pcb_pstk_bbox(ps);
	if (!data->padstack_tree)
		data->padstack_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(data->padstack_tree, (pcb_box_t *)ps, 0);
	PCB_SET_PARENT(ps, data, data);
}

void pcb_pstk_bbox(pcb_pstk_t *ps)
{
	int n, sn;
	pcb_line_t line;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
	pcb_pstk_tshape_t *ts = pcb_pstk_get_tshape(ps);
	assert(proto != NULL);

	ps->BoundingBox.X1 = ps->BoundingBox.X2 = ps->x;
	ps->BoundingBox.Y1 = ps->BoundingBox.Y2 = ps->y;

	for(sn = 0; sn < ts->len; sn++) {
		pcb_pstk_shape_t *shape = ts->shape + sn;
		switch(shape->shape) {
			case PCB_PSSH_POLY:
				for(n = 0; n < shape->data.poly.len; n++)
					pcb_box_bump_point(&ps->BoundingBox, shape->data.poly.x[n] + ps->x, shape->data.poly.y[n] + ps->y);
				break;
			case PCB_PSSH_LINE:
				line.Point1.X = shape->data.line.x1 + ps->x;
				line.Point1.Y = shape->data.line.y1 + ps->y;
				line.Point2.X = shape->data.line.x2 + ps->x;
				line.Point2.Y = shape->data.line.y2 + ps->y;
				line.Thickness = shape->data.line.thickness;
				line.Clearance = 0;
				line.Flags = pcb_flag_make(shape->data.line.square ? PCB_FLAG_SQUARE : 0);
				pcb_line_bbox(&line);
				pcb_box_bump_box(&ps->BoundingBox, &line.BoundingBox);
				break;
			case PCB_PSSH_CIRC:
				pcb_box_bump_point(&ps->BoundingBox, ps->x - shape->data.circ.dia/2, ps->y - shape->data.circ.dia/2);
				pcb_box_bump_point(&ps->BoundingBox, ps->x + shape->data.circ.dia/2, ps->y + shape->data.circ.dia/2);
				break;
		}
	}

	if (PCB_NONPOLY_HAS_CLEARANCE(ps)) {
		ps->BoundingBox.X1 -= ps->Clearance;
		ps->BoundingBox.Y1 -= ps->Clearance;
		ps->BoundingBox.X2 += ps->Clearance;
		ps->BoundingBox.Y2 += ps->Clearance;
	}

	if (proto->hdia != 0) {
		/* corner case: no copper around the hole all 360 deg - let the hole stick out */
		pcb_box_bump_point(&ps->BoundingBox, ps->x - proto->hdia/2, ps->y - proto->hdia/2);
		pcb_box_bump_point(&ps->BoundingBox, ps->x + proto->hdia/2, ps->y + proto->hdia/2);
	}

	pcb_close_box(&ps->BoundingBox);
}

/*** utils ***/

static pcb_pstk_t *pcb_pstk_copy_meta(pcb_pstk_t *dst, pcb_pstk_t *src)
{
	if (dst == NULL)
		return NULL;
	pcb_attribute_copy_all(&dst->Attributes, &src->Attributes);
	dst->thermals.used = src->thermals.used;
	if (dst->thermals.used > 0) {
		dst->thermals.shape = malloc(dst->thermals.used * sizeof(dst->thermals.shape[0]));
		memcpy(dst->thermals.shape, src->thermals.shape, src->thermals.used * sizeof(src->thermals.shape[0]));
	}
	else
		dst->thermals.shape = 0;
	memcpy(&dst->Flags, &src->Flags, sizeof(dst->Flags));
	return dst;
}


void pcb_pstk_move(pcb_pstk_t *ps, pcb_coord_t dx, pcb_coord_t dy)
{
	ps->x += dx;
	ps->y += dy;
	ps->BoundingBox.X1 += dx;
	ps->BoundingBox.Y1 += dy;
	ps->BoundingBox.X2 += dx;
	ps->BoundingBox.Y2 += dy;
}

pcb_pstk_t *pcb_pstk_by_id(pcb_data_t *base, long int ID)
{
	pcb_box_t *ps;
	pcb_rtree_it_t it;

	for(ps = pcb_r_first(base->padstack_tree, &it); ps != NULL; ps = pcb_r_next(&it)) {
		if (((pcb_pstk_t *)ps)->ID == ID) {
			pcb_r_end(&it);
			return (pcb_pstk_t *)ps;
		}
	}

	pcb_r_end(&it);
	return NULL;
}

/*** draw ***/

static void set_ps_color(pcb_pstk_t *ps, int is_current, pcb_layer_type_t lyt)
{
	char *color, *layer_color = NULL;
	char buf[sizeof("#XXXXXX")];

	if (lyt & PCB_LYT_PASTE)
		layer_color = conf_core.appearance.color.paste;

	if (ps->term == NULL) {
		/* normal via, not a terminal */
		if (!pcb_draw_doing_pinout && PCB_FLAG_TEST(PCB_FLAG_WARN | PCB_FLAG_SELECTED | PCB_FLAG_FOUND, ps)) {
			if (PCB_FLAG_TEST(PCB_FLAG_WARN, ps))
				color = conf_core.appearance.color.warn;
			else if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, ps))
				color = conf_core.appearance.color.via_selected;
			else
				color = conf_core.appearance.color.connected;

			if (PCB_FLAG_TEST(PCB_FLAG_ONPOINT, ps)) {
				assert(color != NULL);
				pcb_lighten_color(color, buf, 1.75);
				color = buf;
			}
		}
		else {
			if (layer_color != NULL)
				color = layer_color;
			else if (is_current)
				color = conf_core.appearance.color.via;
			else
				color = conf_core.appearance.color.invisible_objects;
		}
	}
	else {
		/* terminal */
		if (!pcb_draw_doing_pinout && PCB_FLAG_TEST(PCB_FLAG_WARN | PCB_FLAG_SELECTED | PCB_FLAG_FOUND, ps)) {
			if (PCB_FLAG_TEST(PCB_FLAG_WARN, ps))
				color = conf_core.appearance.color.warn;
			else if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, ps))
				color = conf_core.appearance.color.pin_selected;
			else
				color = conf_core.appearance.color.connected;

			if (PCB_FLAG_TEST(PCB_FLAG_ONPOINT, ps)) {
				assert(color != NULL);
				pcb_lighten_color(color, buf, 1.75);
				color = buf;
			}
		}
		else
			if (layer_color != NULL)
				color = layer_color;
			else if (is_current)
				color = conf_core.appearance.color.pin;
			else
				color = conf_core.appearance.color.invisible_objects;

	}

	pcb_gui->set_color(Output.fgGC, color);
}

static void set_ps_annot_color(pcb_hid_gc_t gc, pcb_pstk_t *ps)
{
	pcb_gui->set_color(Output.fgGC, PCB_FLAG_TEST(PCB_FLAG_SELECTED, ps) ?
		conf_core.appearance.color.subc_selected : conf_core.appearance.color.padstackmark);
}

static void pcb_pstk_draw_shape_solid(pcb_hid_gc_t gc, pcb_pstk_t *ps, pcb_pstk_shape_t *shape)
{
	switch(shape->shape) {
		case PCB_PSSH_POLY:
			pcb_gui->fill_polygon_offs(Output.fgGC, shape->data.poly.len, shape->data.poly.x, shape->data.poly.y, ps->x, ps->y);
			break;
		case PCB_PSSH_LINE:
			pcb_gui->set_line_cap(Output.fgGC, shape->data.line.square ? Square_Cap : Round_Cap);
			pcb_gui->set_line_width(Output.fgGC, shape->data.line.thickness);
			pcb_gui->draw_line(Output.fgGC, ps->x + shape->data.line.x1, ps->y + shape->data.line.y1, ps->x + shape->data.line.x2, ps->y + shape->data.line.y2);
			break;
		case PCB_PSSH_CIRC:
			pcb_gui->fill_circle(Output.fgGC, ps->x + shape->data.circ.x, ps->y + shape->data.circ.y, shape->data.circ.dia/2);
			break;
	}
}

static void pcb_pstk_draw_shape_thin(pcb_hid_gc_t gc, pcb_pstk_t *ps, pcb_pstk_shape_t *shape)
{
	int n;
	switch(shape->shape) {
		case PCB_PSSH_POLY:
			for(n = 1; n < shape->data.poly.len; n++)
				pcb_gui->draw_line(gc, ps->x + shape->data.poly.x[n-1], ps->y + shape->data.poly.y[n-1], ps->x + shape->data.poly.x[n], ps->y + shape->data.poly.y[n]);
			pcb_gui->draw_line(gc, ps->x + shape->data.poly.x[n-1], ps->y + shape->data.poly.y[n-1], ps->x + shape->data.poly.x[0], ps->y + shape->data.poly.y[0]);
			break;
		case PCB_PSSH_LINE:
			pcb_draw_wireframe_line(gc, ps->x + shape->data.line.x1, ps->y + shape->data.line.y1, ps->x + shape->data.line.x2, ps->y + shape->data.line.y2, shape->data.line.thickness, shape->data.line.square);
			break;
		case PCB_PSSH_CIRC:
			pcb_gui->draw_arc(gc, ps->x + shape->data.circ.x, ps->y + shape->data.circ.y, shape->data.circ.dia/2, shape->data.circ.dia/2, 0, 360); 
			break;
	}
}

pcb_r_dir_t pcb_pstk_draw_callback(const pcb_box_t *b, void *cl)
{
	pcb_pstk_draw_t *ctx = cl;
	pcb_pstk_t *ps = (pcb_pstk_t *)b;
	pcb_pstk_shape_t *shape;
	pcb_layergrp_t *grp;

	if (!PCB->SubcPartsOn && pcb_gobj_parent_subc(ps->parent_type, &ps->parent))
		return PCB_R_DIR_NOT_FOUND;

	shape = pcb_pstk_shape_gid(ctx->pcb, ps, ctx->gid, ctx->comb, &grp);
	if (shape != NULL) {
		pcb_gui->set_draw_xor(Output.fgGC, 0);
		set_ps_color(ps, ctx->is_current, grp->type);
		if (conf_core.editor.thin_draw || conf_core.editor.wireframe_draw) {
			pcb_gui->set_line_width(Output.fgGC, 0);
			pcb_pstk_draw_shape_thin(Output.fgGC, ps, shape);
		}
		else
			pcb_pstk_draw_shape_solid(Output.fgGC, ps, shape);
	}

	return PCB_R_DIR_FOUND_CONTINUE;
}

pcb_r_dir_t pcb_pstk_draw_mark_callback(const pcb_box_t *b, void *cl)
{
	pcb_pstk_t *ps = (pcb_pstk_t *)b;
	pcb_pstk_proto_t *proto;
	pcb_coord_t mark;

	mark = PS_CROSS_SIZE/2;
	proto = pcb_pstk_get_proto(ps);
	if (proto != NULL)
		mark += proto->hdia/2;

	set_ps_annot_color(Output.fgGC, ps);
	pcb_gui->set_line_width(Output.fgGC, -3);
	pcb_gui->set_draw_xor(Output.fgGC, 1);
	pcb_gui->draw_line(Output.fgGC, ps->x-mark, ps->y, ps->x+mark, ps->y);
	pcb_gui->draw_line(Output.fgGC, ps->x, ps->y-mark, ps->x, ps->y+mark);
	pcb_gui->set_draw_xor(Output.fgGC, 0);

	if (ps->term != NULL) {
		if ((pcb_draw_doing_pinout) || PCB_FLAG_TEST(PCB_FLAG_TERMNAME, ps))
			pcb_draw_delay_label_add((pcb_any_obj_t *)ps);
	}
}


pcb_r_dir_t pcb_pstk_draw_hole_callback(const pcb_box_t *b, void *cl)
{
	pcb_pstk_draw_t *ctx = cl;
	pcb_pstk_t *ps = (pcb_pstk_t *)b;
	pcb_pstk_proto_t *proto;

	if (!PCB->SubcPartsOn && pcb_gobj_parent_subc(ps->parent_type, &ps->parent))
		return PCB_R_DIR_NOT_FOUND;

	if (!pcb_pstk_bb_drills(ctx->pcb, ps, ctx->gid, &proto))
		return PCB_R_DIR_FOUND_CONTINUE;

	pcb_gui->fill_circle(Output.drillGC, ps->x, ps->y, proto->hdia / 2);
	if (!proto->hplated) {
		pcb_coord_t r = proto->hdia / 2;
		r += r/8; /* +12.5% */
		pcb_gui->set_color(Output.fgGC, PCB_FLAG_TEST(PCB_FLAG_SELECTED, ps) ? conf_core.appearance.color.subc_selected : conf_core.appearance.color.subc);
		pcb_gui->set_line_width(Output.fgGC, 0);
		pcb_gui->set_draw_xor(Output.fgGC, 1);
		pcb_gui->draw_arc(Output.fgGC, ps->x, ps->y, r, r, 20, 290);
		pcb_gui->set_draw_xor(Output.fgGC, 0);
	}

	return PCB_R_DIR_FOUND_CONTINUE;
}

void pcb_pstk_thindraw(pcb_hid_gc_t gc, pcb_pstk_t *ps)
{
	pcb_pstk_shape_t *shape = NULL;
	pcb_board_t *pcb;
	pcb_layergrp_id_t gid = CURRENT->meta.real.grp;

	pcb = pcb_data_get_top(ps->parent.data);
	if (pcb != NULL) {
		shape = pcb_pstk_shape_gid(pcb, ps, gid, 0, NULL);
		if (shape == NULL)
			shape = pcb_pstk_shape_gid(pcb, ps, gid, PCB_LYC_SUB, NULL);
	}
	else { /* no pcb means buffer - take the first shape, whichever layer it is for */
		pcb_pstk_tshape_t *ts = pcb_pstk_get_tshape(ps);
		if (ts != NULL)
			shape = ts->shape;
	}

	if (shape != NULL) {
		pcb_gui->set_draw_xor(gc, 0);
		pcb_pstk_draw_shape_thin(gc, ps, shape);
	}
}

void pcb_pstk_draw_label(pcb_pstk_t *ps)
{
	pcb_coord_t offs = 0;
	pcb_pstk_proto_t *proto;

	if (ps->term == NULL)
		return;

	proto = pcb_pstk_get_proto(ps);
	if ((proto != NULL) && (proto->hdia > 0))
		offs = proto->hdia/2;
	pcb_term_label_draw(ps->x + offs, ps->y, 100.0, 0, pcb_false, ps->term, ps->intconn);
}


void pcb_pstk_invalidate_erase(pcb_pstk_t *ps)
{
	pcb_draw_invalidate(ps);
}

void pcb_pstk_invalidate_draw(pcb_pstk_t *ps)
{
	pcb_draw_invalidate(ps);
}


static int pcb_pstk_near_box_(pcb_pstk_t *ps, pcb_box_t *box, pcb_pstk_shape_t *shape)
{
	pcb_pad_t pad;
	pcb_vector_t v;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
	int isneg = PCB_IS_BOX_NEGATIVE(box), is_in, n;

	/* cheap check: hole */
	if (proto->hdia > 0) {
		/* in negative case, touching the hole means hit */
		if (isneg)
			if (PCB_CIRCLE_TOUCHES_BOX(ps->x, ps->y, proto->hdia/2, box))
				return 1;
		
		/* in positive case, not including the hole means bad */
		if (!isneg)
			if (!PCB_POINT_IN_BOX(ps->x-proto->hdia/2,ps->y-proto->hdia/2, box) ||
				!PCB_POINT_IN_BOX(ps->x+proto->hdia/2,ps->y+proto->hdia/2, box))
					return 0;
	}

	switch(shape->shape) {
		case PCB_PSSH_CIRC:
		if (isneg)
			return PCB_CIRCLE_TOUCHES_BOX(ps->x + shape->data.circ.x, ps->y + shape->data.circ.y, shape->data.circ.dia/2, box);
		return
			PCB_POINT_IN_BOX(ps->x+shape->data.circ.x-proto->hdia/2,ps->y+shape->data.circ.y-shape->data.circ.dia/2, box) &&
			PCB_POINT_IN_BOX(ps->x+shape->data.circ.x+proto->hdia/2,ps->y+shape->data.circ.y+shape->data.circ.dia/2, box);
		case PCB_PSSH_LINE:
			pad.Point1.X = shape->data.line.x1 + ps->x;
			pad.Point1.Y = shape->data.line.y1 + ps->y;
			pad.Point2.X = shape->data.line.x2 + ps->x;
			pad.Point2.Y = shape->data.line.y2 + ps->y;
			pad.Thickness = shape->data.line.thickness;
			pad.Clearance = 0;
			pad.Flags = pcb_flag_make(shape->data.line.square ? PCB_FLAG_SQUARE : 0);
			return isneg ? PCB_PAD_TOUCHES_BOX(&pad, box) : PCB_PAD_IN_BOX(&pad, box);
		case PCB_PSSH_POLY:
			if (shape->data.poly.pa == NULL)
				pcb_pstk_shape_update_pa(&shape->data.poly);

			if (isneg) {
				/* shortcut: if any point is in our box in negative, we are done */
				for(n = 0; n < shape->data.poly.len; n++) {
					int pib = PCB_POINT_IN_BOX(ps->x + shape->data.poly.x[n], ps->x + shape->data.poly.y[n], box);
					if (pib)
						return 1;
				}
			}

			/* fully within */
			is_in = 1;
			for(n = 0; n < shape->data.poly.len; n++) {
				int pib = PCB_POINT_IN_BOX(ps->x + shape->data.poly.x[n], ps->x + shape->data.poly.y[n], box);
				if (!pib) {
					is_in = 0;
					break;
				}
			}

			if (!isneg) /* normal box - accept only if we are in */
				return is_in;

			/* negative box, not fully within, no poly point in the box, check whether
			   the box is fully within the poly - if any of it's corners is in */
			v[0] = box->X1 - ps->x;
			v[1] = box->Y1 - ps->y;
			return pcb_polyarea_contour_inside(shape->data.poly.pa, v);


			/* negative box, not fully within, no poly point in the box, check whether
			   box lines intersect poly lines
			   We expect shapes to be simple so linear search is faster than
			   being clever */
			for(n = 1; n < shape->data.poly.len; n++) {
				if (PCB_XYLINE_ISECTS_BOX(ps->x+shape->data.poly.x[n-1],ps->y+shape->data.poly.y[n-1],ps->x+shape->data.poly.x[n],ps->y+shape->data.poly.y[n],  box))
					return 1;
			}

			n = shape->data.poly.len-1;
			if (PCB_XYLINE_ISECTS_BOX(ps->x+shape->data.poly.x[n-1],ps->y+shape->data.poly.y[n-1],ps->x+shape->data.poly.x[0],ps->y+shape->data.poly.y[0],  box))
				return 1;
	}

	return 0;
}

int pcb_pstk_near_box(pcb_pstk_t *ps, pcb_box_t *box, pcb_layer_t *layer)
{
	pcb_pstk_shape_t *shp;

	if (layer == NULL) {
		int n;
		pcb_pstk_tshape_t *tshp = pcb_pstk_get_tshape(ps);

		if (tshp == NULL)
			return 0;

		for(n = 0; n < tshp->len; n++)
			if (pcb_pstk_near_box_(ps, box, &tshp->shape[n]) != 0)
				return 1;
		return 0;
	}

	shp = pcb_pstk_shape(ps, pcb_layer_flags_(layer), layer->comb);
	if (shp == NULL)
		return 0;
	return pcb_pstk_near_box_(ps, box, shp);
}

static int pcb_is_point_in_pstk_(pcb_pstk_t *ps, pcb_coord_t x, pcb_coord_t y, pcb_coord_t radius, pcb_pstk_shape_t *shape)
{
	pcb_pad_t pad;
	pcb_vector_t v;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);

	/* cheap check: hole clicked (also necessary for a mounting hole too) */
	if (proto->hdia > 0) {
		double dist2 = SQR((double)x - (double)ps->x) + SQR((double)y - (double)ps->y);
		if (dist2 <= SQR((double)radius + (double)proto->hdia/2.0))
			return 1;
	}

	switch(shape->shape) {
		case PCB_PSSH_CIRC:
			return PCB_POINT_IN_CIRCLE(x, y, ps->x + shape->data.circ.x, ps->y + shape->data.circ.y, shape->data.circ.dia/2 + radius);
		case PCB_PSSH_LINE:
			pad.Point1.X = shape->data.line.x1 + ps->x;
			pad.Point1.Y = shape->data.line.y1 + ps->y;
			pad.Point2.X = shape->data.line.x2 + ps->x;
			pad.Point2.Y = shape->data.line.y2 + ps->y;
			pad.Thickness = shape->data.line.thickness;
			pad.Clearance = 0;
			pad.Flags = pcb_flag_make(shape->data.line.square ? PCB_FLAG_SQUARE : 0);
			return pcb_is_point_in_pad(x, y, radius, &pad);
		case PCB_PSSH_POLY:
			if (shape->data.poly.pa == NULL)
				pcb_pstk_shape_update_pa(&shape->data.poly);
			v[0] = x - ps->x;
			v[1] = y - ps->y;
			return pcb_polyarea_contour_inside(shape->data.poly.pa, v);
	}

	return 0;
}

int pcb_is_point_in_pstk(pcb_coord_t x, pcb_coord_t y, pcb_coord_t radius, pcb_pstk_t *ps, pcb_layer_t *layer)
{
	pcb_pstk_shape_t *shp;

	if (layer == NULL) {
		int n;
		pcb_pstk_tshape_t *tshp = pcb_pstk_get_tshape(ps);

		if (tshp == NULL)
			return 0;

		for(n = 0; n < tshp->len; n++)
			if (pcb_is_point_in_pstk_(ps, x, y, radius, &tshp->shape[n]) != 0)
				return 1;
		return 0;
	}

	shp = pcb_pstk_shape(ps, pcb_layer_flags_(layer), layer->comb);
	if (shp == NULL)
		return 0;
	return pcb_is_point_in_pstk_(ps, x, y, radius, shp);
}

int pcb_pstk_drc_check_clearance(pcb_pstk_t *ps, pcb_poly_t *polygon, pcb_coord_t min_clr)
{
#warning padstack TODO
	return 0;
}


int pcb_pstk_drc_check_and_warn(pcb_pstk_t *ps)
{
#warning padstack TODO
	return 0;
}

unsigned char *pcb_pstk_get_thermal(pcb_pstk_t *ps, unsigned long lid, pcb_bool_t alloc)
{
	if (ps->thermals.used <= lid) {
		unsigned long oldu = ps->thermals.used;
		if (!alloc)
			return NULL;
		ps->thermals.used = lid+1;
		ps->thermals.shape = realloc(ps->thermals.shape, ps->thermals.used);
		memset(ps->thermals.shape + oldu, 0, ps->thermals.used - oldu);
	}
	return ps->thermals.shape + lid;
}

void pcb_pstk_set_thermal(pcb_pstk_t *ps, unsigned long lid, unsigned char shape)
{
	unsigned char *th = pcb_pstk_get_thermal(ps, lid, 1);
	if (th != NULL)
		*th = shape;
}

/*** Undoable instance parameter change ***/

typedef struct {
	long int parent_ID; /* -1 for pcb, positive for a subc */
	long int ID;        /* ID of the padstack */

	pcb_cardinal_t proto;
	pcb_coord_t clearance;
	double rot;
	int xmirror;
} padstack_change_instance_t;

#define swap(a,b,type) \
	do { \
		type tmp = a; \
		a = b; \
		b = tmp; \
	} while(0)

static int undo_change_instance_swap(void *udata)
{
	padstack_change_instance_t *u = udata;
	pcb_data_t *data;
	pcb_pstk_t *ps;

	if (u->parent_ID != -1) {
		pcb_subc_t *subc = pcb_subc_by_id(PCB->Data, u->parent_ID);
		if (subc == NULL) {
			pcb_message(PCB_MSG_ERROR, "Can't undo padstack change: parent subc #%ld is not found\n", u->parent_ID);
			return -1;
		}
		data = subc->data;
	}
	else
		data = PCB->Data;

	ps = pcb_pstk_by_id(data, u->ID);
	if (ps == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't undo padstack change: padstack ID #%ld is not available\n", u->ID);
		return -1;
	}

	pcb_poly_restore_to_poly(ps->parent.data, PCB_TYPE_PSTK, NULL, ps);
	pcb_pstk_invalidate_erase(ps);

	swap(ps->proto,      u->proto,     pcb_cardinal_t);
	swap(ps->Clearance,  u->clearance, pcb_coord_t);
	swap(ps->rot,        u->rot,       double);
	swap(ps->xmirror,    u->xmirror,   int);

	/* force re-render the prototype */
	ps->protoi = -1;
	pcb_pstk_get_tshape(ps);

	pcb_poly_clear_from_poly(ps->parent.data, PCB_TYPE_PSTK, NULL, ps);
	pcb_pstk_invalidate_draw(ps);

	return 0;
}

static void undo_change_instance_print(void *udata, char *dst, size_t dst_len)
{
	padstack_change_instance_t *u = udata;
	pcb_snprintf(dst, dst_len, "padstack change: clearance=%$mm rot=%.2f xmirror=%d\n", u->clearance, u->rot, u->xmirror);
}

static const uundo_oper_t undo_pstk_change_instance = {
	core_pstk_cookie,
	NULL, /* free */
	undo_change_instance_swap,
	undo_change_instance_swap,
	undo_change_instance_print
};

int pcb_pstk_change_instance(pcb_pstk_t *ps, pcb_cardinal_t *proto, const pcb_coord_t *clearance, double *rot, int *xmirror)
{
	padstack_change_instance_t *u;
	long int parent_ID;
	pcb_opctx_t ctx;

	switch(ps->parent.data->parent_type) {
		case PCB_PARENT_BOARD: parent_ID = -1; break;
		case PCB_PARENT_SUBC: parent_ID = ps->parent.data->parent.subc->ID; break;
		default: return -1;
	}

	ctx.clip.clear = 0;
	ctx.clip.restore = 1;
	pcb_pstkop_clip(&ctx, ps);

	u = pcb_undo_alloc(PCB, &undo_pstk_change_instance, sizeof(padstack_change_instance_t));
	u->parent_ID = parent_ID;
	u->ID = ps->ID;
	u->proto = proto ? *proto : ps->proto;
	u->clearance = clearance ? *clearance : ps->Clearance;
	u->rot = rot ? *rot : ps->rot;
	u->xmirror = xmirror ? *xmirror : ps->xmirror;

	pcb_pstk_bbox(ps);
	ctx.clip.clear = 1;
	ctx.clip.restore = 0;
	pcb_pstkop_clip(&ctx, ps);


	undo_change_instance_swap(u);

	pcb_undo_inc_serial();
	return 0;
}

pcb_bool pcb_pstk_is_group_empty(pcb_board_t *pcb, pcb_layergrp_id_t gid)
{
	pcb_layergrp_t *g = &pcb->LayerGroups.grp[gid];

	PCB_PADSTACK_LOOP(pcb->Data); {
		pcb_cardinal_t n;
		for(n = 0; n < g->len; n++)
			if (pcb_pstk_shape_at(pcb, padstack, &pcb->Data->Layer[g->lid[n]]))
				return pcb_false;
	}
	PCB_END_LOOP;
	return pcb_true;
}


#include "obj_pstk_op.c"
