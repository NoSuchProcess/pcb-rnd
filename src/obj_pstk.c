/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2018 Tibor 'Igor2' Palinkas
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
#include "obj_hash.h"
#include "operation.h"
#include "search.h"
#include "undo.h"
#include "vtpadstack.h"
#include "event.h"
#include "hid_inlines.h"
#include "polygon_offs.h"

#define SQR(o) ((o)*(o))

static const char core_pstk_cookie[] = "padstack";

const pcb_proto_layer_t pcb_proto_layers[pcb_proto_num_layers] = {
	{"top paste",            PCB_LYT_TOP | PCB_LYT_PASTE,     PCB_LYC_AUTO,             {2,-1}, 0},
	{"top mask",             PCB_LYT_TOP | PCB_LYT_MASK,      PCB_LYC_SUB|PCB_LYC_AUTO, {2,-1}, PCB_PROTO_MASK_BLOAT},
	{"top copper",           PCB_LYT_TOP | PCB_LYT_COPPER,    0,                        {4,3},  0},
	{"any internal copper",  PCB_LYT_INTERN | PCB_LYT_COPPER, 0,                        {2,4},  0},
	{"bottom copper",        PCB_LYT_BOTTOM | PCB_LYT_COPPER, 0,                        {2,3},  0},
	{"bottom mask",          PCB_LYT_BOTTOM | PCB_LYT_MASK,   PCB_LYC_SUB|PCB_LYC_AUTO, {4,-1}, PCB_PROTO_MASK_BLOAT},
	{"bottom paste",         PCB_LYT_BOTTOM | PCB_LYT_PASTE,  PCB_LYC_AUTO,             {4,-1}, 0},
	{"slot",                 PCB_LYT_MECH,                    PCB_LYC_AUTO,             {-1,-1},0}
};


void pcb_pstk_reg(pcb_data_t *data, pcb_pstk_t *pstk)
{
	padstacklist_append(&data->padstack, pstk);
	pcb_obj_id_reg(data, pstk);
	PCB_SET_PARENT(pstk, data, data);
}

void pcb_pstk_unreg(pcb_pstk_t *pstk)
{
	pcb_data_t *data = pstk->parent.data;
	assert(pstk->parent_type == PCB_PARENT_DATA);
	padstacklist_remove(pstk);
	pcb_obj_id_del(data, pstk);
	PCB_CLEAR_PARENT(pstk);
}

pcb_pstk_t *pcb_pstk_alloc_id(pcb_data_t *data, long int id)
{
	pcb_pstk_t *ps;

	ps = calloc(sizeof(pcb_pstk_t), 1);
	ps->ID = id;
	ps->protoi = -1;
	ps->type = PCB_OBJ_PSTK;
	ps->Attributes.post_change = pcb_obj_attrib_post_change;

	pcb_pstk_reg(data, ps);

	return ps;
}

pcb_pstk_t *pcb_pstk_alloc(pcb_data_t *data)
{
 return pcb_pstk_alloc_id(data, pcb_create_ID_get());
}


void pcb_pstk_free(pcb_pstk_t *ps)
{
	pcb_pstk_unreg(ps);
	free(ps->thermals.shape);
	free(ps);
}

pcb_pstk_t *pcb_pstk_new_tr(pcb_data_t *data, long int id, pcb_cardinal_t proto, pcb_coord_t x, pcb_coord_t y, pcb_coord_t clearance, pcb_flag_t Flags, double rot, int xmirror, int smirror)
{
	pcb_pstk_t *ps;

	if (proto >= pcb_vtpadstack_proto_len(&data->ps_protos))
		return NULL;

	if (id <= 0)
		id = pcb_create_ID_get();

	ps = pcb_pstk_alloc_id(data, id);

	/* copy values */
	ps->proto = proto;
	ps->x = x;
	ps->y = y;
	ps->Clearance = clearance;
	ps->Flags = Flags;
	ps->rot = rot;
	ps->xmirror = xmirror;
	ps->smirror = smirror;
	pcb_pstk_add(data, ps);
	pcb_event(&PCB->hidlib, PCB_EVENT_NEW_PSTK, "p", ps);
	pcb_poly_clear_from_poly(data, PCB_OBJ_PSTK, NULL, ps);
	return ps;
}

pcb_pstk_t *pcb_pstk_new(pcb_data_t *data, long int id, pcb_cardinal_t proto, pcb_coord_t x, pcb_coord_t y, pcb_coord_t clearance, pcb_flag_t Flags)
{
	return pcb_pstk_new_tr(data, id, proto, x, y, clearance, Flags, 0, 0, 0);
}


void pcb_pstk_add(pcb_data_t *data, pcb_pstk_t *ps)
{
	pcb_pstk_bbox(ps);
	if (!data->padstack_tree)
		data->padstack_tree = pcb_r_create_tree();
	pcb_r_insert_entry(data->padstack_tree, (pcb_box_t *)ps);
	PCB_SET_PARENT(ps, data, data);
}

static void pcb_pstk_bbox_(pcb_box_t *dst, pcb_pstk_t *ps, pcb_bool copper_only)
{
	int n, sn;
	pcb_line_t line;
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
	pcb_pstk_tshape_t *ts = pcb_pstk_get_tshape(ps);
	assert(proto != NULL);

	dst->X1 = dst->X2 = ps->x;
	dst->Y1 = dst->Y2 = ps->y;

	if (ts != NULL)
	for(sn = 0; sn < ts->len; sn++) {
		pcb_pstk_shape_t *shape = ts->shape + sn;
		switch(shape->shape) {
			case PCB_PSSH_POLY:
				for(n = 0; n < shape->data.poly.len; n++)
					pcb_box_bump_point(dst, shape->data.poly.x[n] + ps->x, shape->data.poly.y[n] + ps->y);
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
				pcb_box_bump_box(dst, &line.BoundingBox);
				break;
			case PCB_PSSH_CIRC:
				pcb_box_bump_point(dst, ps->x - shape->data.circ.dia/2, ps->y - shape->data.circ.dia/2);
				pcb_box_bump_point(dst, ps->x + shape->data.circ.dia/2, ps->y + shape->data.circ.dia/2);
				break;
			case PCB_PSSH_HSHADOW:
				break;
		}
	}

	if (!copper_only && PCB_NONPOLY_HAS_CLEARANCE(ps)) {
		dst->X1 -= ps->Clearance;
		dst->Y1 -= ps->Clearance;
		dst->X2 += ps->Clearance;
		dst->Y2 += ps->Clearance;
	}

	if (proto->hdia != 0) {
		/* corner case: no copper around the hole all 360 deg - let the hole stick out */
		pcb_box_bump_point(dst, ps->x - proto->hdia/2, ps->y - proto->hdia/2);
		pcb_box_bump_point(dst, ps->x + proto->hdia/2, ps->y + proto->hdia/2);
	}

	pcb_close_box(dst);
}

void pcb_pstk_bbox(pcb_pstk_t *ps)
{
	pcb_pstk_bbox_(&ps->BoundingBox, ps, pcb_false);
	pcb_pstk_bbox_(&ps->bbox_naked, ps, pcb_true);
}

void pcb_pstk_copper_bbox(pcb_box_t *dst, pcb_pstk_t *ps)
{
	pcb_pstk_bbox_(dst, ps, pcb_true);
}

/* hash */
int pcb_pstk_eq(const pcb_host_trans_t *tr1, const pcb_pstk_t *p1, const pcb_host_trans_t *tr2, const pcb_pstk_t *p2)
{
	double rotdir1 = tr1->on_bottom ? -1.0 : 1.0, rotdir2 = tr2->on_bottom ? -1.0 : 1.0;

TODO("padstack: should compare shape by shape: a 180 deg rotated or mirrored rectangle is still just the same rectangle!")

	if ((p1->smirror ^ tr1->on_bottom) != (p2->smirror ^ tr2->on_bottom)) return 0;
	if ((p1->xmirror ^ tr1->on_bottom) != (p2->xmirror ^ tr2->on_bottom)) return 0;
	if (floor(fmod((p1->rot * rotdir1) + tr1->rot, 360.0)*10000) != floor(fmod((p2->rot * rotdir2) + tr2->rot, 360.0)*10000)) return 0;

	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, p1) && !PCB_FLAG_TEST(PCB_FLAG_FLOATER, p2)) {
		pcb_coord_t x1, y1, x2, y2;
		
		pcb_hash_tr_coords(tr1, &x1, &y1, p1->x, p1->y);
		pcb_hash_tr_coords(tr2, &x2, &y2, p2->x, p2->y);
		if ((x1 != x2) || (y1 != y2)) return 0;
	}

	if (pcb_neqs(p1->term, p2->term)) return 0;
	return 1;
}

unsigned int pcb_pstk_hash(const pcb_host_trans_t *tr, const pcb_pstk_t *p)
{
	unsigned int crd = 0;
	double rotdir = tr->on_bottom ? -1.0 : 1.0;

	if (!PCB_FLAG_TEST(PCB_FLAG_FLOATER, p)) {
		pcb_coord_t x, y;
		
		pcb_hash_tr_coords(tr, &x, &y, p->x, p->y);
		crd = pcb_hash_coord(x) ^ pcb_hash_coord(y);
	}

	return pcb_hash_angle(tr, p->rot * rotdir) ^ pcb_hash_coord(p->Clearance) ^
		pcb_hash_str(p->term) ^ crd ^
		murmurhash(&p->smirror, sizeof(p->smirror)) ^
		murmurhash(&p->xmirror, sizeof(p->xmirror));
}

/*** utils ***/

pcb_pstk_t *pcb_pstk_copy_meta(pcb_pstk_t *dst, pcb_pstk_t *src)
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

pcb_pstk_t *pcb_pstk_copy_orient(pcb_pstk_t *dst, pcb_pstk_t *src)
{
	if (dst == NULL)
		return NULL;
	dst->rot = src->rot;
	dst->xmirror = src->xmirror;
	dst->smirror = src->smirror;
	dst->protoi = -1; /* invalidate the transformed index to get it recalculated */
	return dst;
}

void pcb_pstk_move_(pcb_pstk_t *ps, pcb_coord_t dx, pcb_coord_t dy)
{
	ps->x += dx;
	ps->y += dy;
	ps->BoundingBox.X1 += dx;
	ps->BoundingBox.Y1 += dy;
	ps->BoundingBox.X2 += dx;
	ps->BoundingBox.Y2 += dy;
	ps->bbox_naked.X1 += dx;
	ps->bbox_naked.Y1 += dy;
	ps->bbox_naked.X2 += dx;
	ps->bbox_naked.Y2 += dy;
}

void pcb_pstk_move(pcb_pstk_t *ps, pcb_coord_t dx, pcb_coord_t dy, pcb_bool more_to_come)
{
	pcb_opctx_t ctx;
	ctx.move.pcb = NULL;
	ctx.move.dx = dx;
	ctx.move.dy = dy;
	ctx.move.more_to_come = more_to_come;
	pcb_pstkop_move(&ctx, ps);
}

pcb_pstk_t *pcb_pstk_by_id(pcb_data_t *base, long int ID)
{
	/* We can not have an rtree based search here: we are often called
	   in the middle of an operation, after the pstk got already removed
	   from the rtree. It happens in e.g. undoable padstack operations
	   where the padstack tries to look up its parent subc by ID, while
	   the subc is being sent to the other side.
	
	   The solution will be the ID hash. */
	PCB_PADSTACK_LOOP(base);
	{
		if (padstack->ID == ID)
			return padstack;
	}
	PCB_END_LOOP;

	return NULL;
}

/*** draw ***/

static void set_ps_color(pcb_pstk_t *ps, int is_current, pcb_layer_type_t lyt, const pcb_layer_t *ly1)
{
	const pcb_color_t *color, *layer_color = NULL;
	pcb_color_t buf;

	if ((lyt & PCB_LYT_PASTE) || (lyt & PCB_LYT_MASK) || (lyt & PCB_LYT_SILK)) {
		if (ly1 == NULL)
			layer_color = pcb_layer_default_color(0, lyt);
		else
			layer_color = &ly1->meta.real.color;
	}

	if (conf_core.appearance.invis_other_groups && !is_current) {
		color = &conf_core.appearance.color.invisible_objects;
	}
	else if (conf_core.appearance.black_current_group && is_current) {
		color = pcb_color_black;
	}
	else if (ps->term == NULL) {
		/* normal via, not a terminal */
		if (!pcb_draw_force_termlab && PCB_FLAG_TEST(PCB_FLAG_WARN | PCB_FLAG_SELECTED | PCB_FLAG_FOUND, ps)) {
			if (PCB_FLAG_TEST(PCB_FLAG_WARN, ps))
				color = &conf_core.appearance.color.warn;
			else if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, ps))
				color = &conf_core.appearance.color.selected;
			else
				color = &conf_core.appearance.color.connected;

			if (PCB_FLAG_TEST(PCB_FLAG_ONPOINT, ps)) {
				pcb_lighten_color(color, &buf, 1.75);
				color = &buf;
			}
		}
		else {
			if (layer_color != NULL)
				color = layer_color;
			else if (PCB_HAS_COLOROVERRIDE(ps))
				color = ps->override_color;
			else if (is_current)
				color = &conf_core.appearance.color.via;
			else
				color = &conf_core.appearance.color.via_far;
		}
	}
	else {
		/* terminal */
		if (!pcb_draw_force_termlab && PCB_FLAG_TEST(PCB_FLAG_WARN | PCB_FLAG_SELECTED | PCB_FLAG_FOUND, ps)) {
			if (PCB_FLAG_TEST(PCB_FLAG_WARN, ps))
				color = &conf_core.appearance.color.warn;
			else if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, ps))
				color = &conf_core.appearance.color.selected;
			else
				color = &conf_core.appearance.color.connected;

			if (PCB_FLAG_TEST(PCB_FLAG_ONPOINT, ps)) {
				pcb_lighten_color(color, &buf, 1.75);
				color = &buf;
			}
		}
		else
			if (layer_color != NULL)
				color = layer_color;
			else if (PCB_HAS_COLOROVERRIDE(ps))
				color = ps->override_color;
			else if (is_current)
				color = &conf_core.appearance.color.pin;
			else
				color = &conf_core.appearance.color.pin_far;

	}

	pcb_gui->set_color(pcb_draw_out.fgGC, color);
}

static void set_ps_annot_color(pcb_hid_gc_t gc, pcb_pstk_t *ps)
{
	pcb_gui->set_color(pcb_draw_out.fgGC, PCB_FLAG_TEST(PCB_FLAG_SELECTED, ps) ?
		&conf_core.appearance.color.selected : &conf_core.appearance.color.padstackmark);
}

static void pcb_pstk_draw_poly(pcb_draw_info_t *info, pcb_hid_gc_t gc, pcb_pstk_t *ps, pcb_pstk_shape_t *shape, int fill, pcb_coord_t dthick)
{
	long n;

	if (shape->data.poly.pa == NULL)
		pcb_pstk_shape_update_pa(&shape->data.poly);
	pcb_hid_set_line_cap(gc, pcb_cap_round);
	if (dthick != 0) {
		/* slow - but would be used on export mostly, not on-screen drawing */
		pcb_polo_t *p, p_st[32];
		pcb_coord_t *x, *y, xy_st[64];
		double vl = pcb_round(-dthick/2);
		long n;

		if (shape->data.poly.inverted)
			dthick = -dthick;
		vl = pcb_round(dthick/2);
		
		if (shape->data.poly.len >= sizeof(p_st) / sizeof(p_st[0])) {
			p = malloc(sizeof(pcb_polo_t) * shape->data.poly.len);
			x = malloc(sizeof(pcb_coord_t) * shape->data.poly.len);
		}
		else {
			p = p_st;
			x = xy_st;
		}
		y = x + sizeof(p_st) / sizeof(p_st[0]);

		/* relative: move each point radially */
		for(n = 0; n < shape->data.poly.len; n++) {
			p[n].x = shape->data.poly.x[n];
			p[n].y = shape->data.poly.y[n];
		}
		pcb_polo_norms(p, shape->data.poly.len);
		pcb_polo_offs(vl, p, shape->data.poly.len);
		for(n = 0; n < shape->data.poly.len; n++) {
			x[n] = pcb_round(p[n].x);
			y[n] = pcb_round(p[n].y);
		}

		if (!fill) {
			for(n = 1; n < shape->data.poly.len; n++)
				pcb_gui->draw_line(gc, ps->x + x[n-1], ps->y + y[n-1], ps->x + x[n], ps->y + y[n]);
			pcb_gui->draw_line(gc, ps->x + x[n-1], ps->y + y[n-1], ps->x + x[0], ps->y + y[0]);
		}
		else
			pcb_gui->fill_polygon_offs(gc, shape->data.poly.len, x, y, ps->x, ps->y);
		if (p != p_st) {
			free(p);
			free(x);
		}
	}
	else {
		if (!fill) {
			for(n = 1; n < shape->data.poly.len; n++)
				pcb_gui->draw_line(gc, ps->x + shape->data.poly.x[n-1], ps->y + shape->data.poly.y[n-1], ps->x + shape->data.poly.x[n], ps->y + shape->data.poly.y[n]);
			pcb_gui->draw_line(gc, ps->x + shape->data.poly.x[n-1], ps->y + shape->data.poly.y[n-1], ps->x + shape->data.poly.x[0], ps->y + shape->data.poly.y[0]);
		}
		else
			pcb_gui->fill_polygon_offs(gc, shape->data.poly.len, shape->data.poly.x, shape->data.poly.y, ps->x, ps->y);
	}
}

static void pcb_pstk_draw_shape_solid(pcb_draw_info_t *info, pcb_hid_gc_t gc, pcb_pstk_t *ps, pcb_pstk_shape_t *shape)
{
	pcb_coord_t r, dthick = 0;
	if ((info != NULL) && (info->xform != NULL) && (info->xform->bloat != 0))
		dthick = info->xform->bloat;

	switch(shape->shape) {
		case PCB_PSSH_POLY:
			pcb_pstk_draw_poly(info, gc, ps, shape, 1, dthick);
			break;
		case PCB_PSSH_LINE:
			pcb_hid_set_line_cap(gc, shape->data.line.square ? pcb_cap_square : pcb_cap_round);
			pcb_hid_set_line_width(gc, MAX(shape->data.line.thickness + dthick, 1));
			pcb_gui->draw_line(gc, ps->x + shape->data.line.x1, ps->y + shape->data.line.y1, ps->x + shape->data.line.x2, ps->y + shape->data.line.y2);
			break;
		case PCB_PSSH_CIRC:
			r = MAX(shape->data.circ.dia/2 + dthick/2, 1);
			pcb_hid_set_line_cap(gc, pcb_cap_round);
			pcb_gui->fill_circle(gc, ps->x + shape->data.circ.x, ps->y + shape->data.circ.y, r);
			break;
		case PCB_PSSH_HSHADOW:
			break;
	}
}

static void pcb_pstk_draw_shape_thin(pcb_draw_info_t *info, pcb_hid_gc_t gc, pcb_pstk_t *ps, pcb_pstk_shape_t *shape)
{
	pcb_coord_t r, dthick = 0;
	pcb_hid_set_line_cap(gc, pcb_cap_round);

	if ((info != NULL) && (info->xform != NULL) && (info->xform->bloat != 0))
		dthick = info->xform->bloat;

	switch(shape->shape) {
		case PCB_PSSH_POLY:
			pcb_pstk_draw_poly(info, gc, ps, shape, 0, dthick);
			break;
		case PCB_PSSH_LINE:
			pcb_draw_wireframe_line(gc, ps->x + shape->data.line.x1, ps->y + shape->data.line.y1, ps->x + shape->data.line.x2, ps->y + shape->data.line.y2, MAX(shape->data.line.thickness + dthick, 1), shape->data.line.square);
			break;
		case PCB_PSSH_CIRC:
			r = MAX(shape->data.circ.dia/2 + dthick/2, 1);
			pcb_gui->draw_arc(gc, ps->x + shape->data.circ.x, ps->y + shape->data.circ.y, r, r, 0, 360);
			break;
		case PCB_PSSH_HSHADOW:
			break;
	}
}

pcb_r_dir_t pcb_pstk_draw_callback(const pcb_box_t *b, void *cl)
{
	pcb_draw_info_t *info = cl;
	pcb_pstk_t *ps = (pcb_pstk_t *)b;
	pcb_pstk_shape_t *shape;
	pcb_layergrp_t *grp = NULL;

	if (pcb_hidden_floater((pcb_any_obj_t*)b))
		return PCB_R_DIR_FOUND_CONTINUE;

	if (!PCB->SubcPartsOn && pcb_gobj_parent_subc(ps->parent_type, &ps->parent))
		return PCB_R_DIR_NOT_FOUND;

	if (info->objcb.pstk.gid < 0) {
		if (info->objcb.pstk.shape_mask != 0)
			shape = pcb_pstk_shape(ps, info->objcb.pstk.shape_mask, info->objcb.pstk.comb);
		else
			return PCB_R_DIR_NOT_FOUND;
	}
	else {
		int n;
		pcb_layer_id_t *lid;
		shape = pcb_pstk_shape_gid(info->pcb, ps, info->objcb.pstk.gid, info->objcb.pstk.comb, &grp);
		for(n = 0, lid = grp->lid; n < grp->len; n++,lid++) {
			if (*lid < ps->thermals.used) {
				if ((ps->thermals.shape[*lid] & PCB_THERMAL_ON) && ((ps->thermals.shape[*lid] & 3) == PCB_THERMAL_NOSHAPE))
					return PCB_R_DIR_NOT_FOUND;
			}
		}
	}

	if (shape != NULL) {
		if (grp == NULL)
			set_ps_color(ps, info->objcb.pstk.is_current, info->objcb.pstk.shape_mask, info->objcb.pstk.layer1);
		else
			set_ps_color(ps, info->objcb.pstk.is_current, grp->ltype, info->objcb.pstk.layer1);
		if (conf_core.editor.thin_draw || conf_core.editor.wireframe_draw) {
			pcb_hid_set_line_width(pcb_draw_out.fgGC, 0);
			pcb_pstk_draw_shape_thin(info, pcb_draw_out.fgGC, ps, shape);
		}
		else
			pcb_pstk_draw_shape_solid(info, pcb_draw_out.fgGC, ps, shape);
	}

	return PCB_R_DIR_FOUND_CONTINUE;
}

pcb_r_dir_t pcb_pstk_draw_mark_callback(const pcb_box_t *b, void *cl)
{
	pcb_pstk_t *ps = (pcb_pstk_t *)b;
	pcb_pstk_proto_t *proto;
	pcb_coord_t mark, mark2;

	/* mark is a cross in the middle, right on the hole;
	   cross size should extend beyond the hole */
	pcb_hid_set_line_cap(pcb_draw_out.fgGC, pcb_cap_round);
	mark = conf_core.appearance.padstack.cross_size;
	proto = pcb_pstk_get_proto(ps);
	if (proto != NULL)
		mark += proto->hdia/2;

	mark2 = mark*2;
	if (mark2 < pcb_gui->coord_per_pix)
		return PCB_R_DIR_FOUND_CONTINUE;

	/* draw the cross using xor */
	set_ps_annot_color(pcb_draw_out.fgGC, ps);
	pcb_hid_set_line_width(pcb_draw_out.fgGC, 0);
	if (mark2 > pcb_gui->coord_per_pix*3) {
		pcb_gui->draw_line(pcb_draw_out.fgGC, ps->x-mark, ps->y, ps->x+mark, ps->y);
		pcb_gui->draw_line(pcb_draw_out.fgGC, ps->x, ps->y-mark, ps->x, ps->y+mark);
	}
	else
		pcb_gui->draw_line(pcb_draw_out.fgGC, ps->x-pcb_gui->coord_per_pix, ps->y, ps->x+pcb_gui->coord_per_pix, ps->y);

	return PCB_R_DIR_FOUND_CONTINUE;
}

pcb_r_dir_t pcb_pstk_draw_label_callback(const pcb_box_t *b, void *cl)
{
	pcb_draw_info_t *info = cl;
	pcb_pstk_t *ps = (pcb_pstk_t *)b;

	/* draw the label if enabled, after everything else is drawn */
	if (ps->term != NULL) {
		if ((pcb_draw_force_termlab) || PCB_FLAG_TEST(PCB_FLAG_TERMNAME, ps))
			pcb_pstk_draw_label(info, ps);
	}
	return PCB_R_DIR_FOUND_CONTINUE;
}

pcb_r_dir_t pcb_pstk_draw_hole_callback(const pcb_box_t *b, void *cl)
{
	pcb_draw_info_t *info = cl;
	pcb_pstk_t *ps = (pcb_pstk_t *)b;
	pcb_pstk_proto_t *proto;

	/* hide subc parts if requested */
	if (!info->pcb->SubcPartsOn && pcb_gobj_parent_subc(ps->parent_type, &ps->parent))
		return PCB_R_DIR_NOT_FOUND;

	/* no hole in this layer group */
	if (info->objcb.pstk.gid >= 0) {
		if (!pcb_pstk_bb_drills(info->pcb, ps, info->objcb.pstk.gid, &proto))
			return PCB_R_DIR_FOUND_CONTINUE;
	}
	else
		proto = pcb_pstk_get_proto(ps);

	/* No hole at all */
	if ((proto == NULL) || (proto->hdia <= 0))
		return PCB_R_DIR_NOT_FOUND;

	/* hole is plated, but the caller doesn't want plated holes */
	if (proto->hplated && (!(info->objcb.pstk.holetype & PCB_PHOLE_PLATED)))
		return PCB_R_DIR_NOT_FOUND;

	/* hole is unplated, but the caller doesn't want unplated holes */
	if (!proto->hplated && (!(info->objcb.pstk.holetype & PCB_PHOLE_UNPLATED)))
		return PCB_R_DIR_NOT_FOUND;

	/* BBvia, but the caller doesn't want BBvias */
	if (((proto->htop != 0) || (proto->hbottom != 0)) && (!(info->objcb.pstk.holetype & PCB_PHOLE_BB)))
		return PCB_R_DIR_NOT_FOUND;

	/* actual hole */
	pcb_hid_set_line_width(pcb_draw_out.drillGC, 0);
	pcb_hid_set_line_cap(pcb_draw_out.drillGC, pcb_cap_round);
	pcb_gui->fill_circle(pcb_draw_out.drillGC, ps->x, ps->y, proto->hdia / 2);

	/* indicate unplated holes with an arc; unplated holes are more rare
	   than plated holes, thus unplated holes are indicated */
	if (!proto->hplated) {
		pcb_coord_t r = proto->hdia / 2;
		r -= r/8; /* +12.5% */
		pcb_gui->set_color(pcb_draw_out.fgGC, PCB_FLAG_TEST(PCB_FLAG_SELECTED, ps) ? &conf_core.appearance.color.selected : &conf_core.appearance.color.subc);
		pcb_hid_set_line_width(pcb_draw_out.fgGC, 0);
		pcb_gui->draw_arc(pcb_draw_out.fgGC, ps->x, ps->y, r, r, 20, 290);
	}

	return PCB_R_DIR_FOUND_CONTINUE;
}

pcb_r_dir_t pcb_pstk_draw_slot_callback(const pcb_box_t *b, void *cl)
{
	pcb_draw_info_t *info = cl;
	pcb_pstk_t *ps = (pcb_pstk_t *)b;
	pcb_pstk_proto_t *proto;
	pcb_pstk_shape_t *shape;

	/* hide subc parts if requested */
	if (!info->pcb->SubcPartsOn && pcb_gobj_parent_subc(ps->parent_type, &ps->parent))
		return PCB_R_DIR_NOT_FOUND;

	/* no slot in this layer group */
	if (info->objcb.pstk.gid >= 0) {
		if (!pcb_pstk_bb_drills(info->pcb, ps, info->objcb.pstk.gid, &proto))
			return PCB_R_DIR_FOUND_CONTINUE;
	}
	else
		proto = pcb_pstk_get_proto(ps);

	/* No slot at all */
	if (proto->mech_idx < 0)
		return PCB_R_DIR_NOT_FOUND;

	/* hole is plated, but the caller doesn't want plated holes */
	if (proto->hplated && (!(info->objcb.pstk.holetype & PCB_PHOLE_PLATED)))
		return PCB_R_DIR_NOT_FOUND;

	/* hole is unplated, but the caller doesn't want unplated holes */
	if (!proto->hplated && (!(info->objcb.pstk.holetype & PCB_PHOLE_UNPLATED)))
		return PCB_R_DIR_NOT_FOUND;

	/* BBslot, but the caller doesn't want BBslot */
	if (((proto->htop != 0) || (proto->hbottom != 0)) && (!(info->objcb.pstk.holetype & PCB_PHOLE_BB)))
		return PCB_R_DIR_NOT_FOUND;

	/* actual slot */
	shape = pcb_pstk_shape(ps, PCB_LYT_MECH, PCB_LYC_AUTO);
	if (shape != NULL) {
		if (conf_core.editor.thin_draw || conf_core.editor.wireframe_draw) {
			pcb_hid_set_line_width(pcb_draw_out.drillGC, 0);
			pcb_pstk_draw_shape_thin(info, pcb_draw_out.drillGC, ps, shape);
		}
		else
			pcb_pstk_draw_shape_solid(info, pcb_draw_out.drillGC, ps, shape);
	}

	return PCB_R_DIR_FOUND_CONTINUE;
}

void pcb_pstk_thindraw(pcb_draw_info_t *info, pcb_hid_gc_t gc, pcb_pstk_t *ps)
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
		if ((ts != NULL)  && (ts->len > 0))
			shape = ts->shape;
	}

	if (shape != NULL)
		pcb_pstk_draw_shape_thin(info, gc, ps, shape);
}

void pcb_pstk_draw_label(pcb_draw_info_t *info, pcb_pstk_t *ps)
{
	pcb_bool vert;
	pcb_coord_t dx, dy;

	if (ps->term == NULL)
		return;

	dx = ps->BoundingBox.X2 - ps->BoundingBox.X1;
	dy = ps->BoundingBox.Y2 - ps->BoundingBox.Y1;
	if ((dx == 0) && (dy == 0)) {
		pcb_pstk_bbox(ps);
		dx = ps->BoundingBox.X2 - ps->BoundingBox.X1;
		dy = ps->BoundingBox.Y2 - ps->BoundingBox.Y1;
	}

	vert = dx < dy;
#ifdef PCB_PSTK_LABEL_OFFCENTER
	pcb_coord_t offs = 0;
	pcb_pstk_proto_t *proto;
	proto = pcb_pstk_get_proto(ps);
	if ((proto != NULL) && (proto->hdia > 0))
		offs = proto->hdia/2;
	pcb_term_label_draw(info, ps->x + offs, ps->y, conf_core.appearance.term_label_size, vert, pcb_false, (pcb_any_obj_t *)ps);
#endif
	pcb_term_label_draw(info, ps->x, ps->y, conf_core.appearance.term_label_size, vert, pcb_true, (pcb_any_obj_t *)ps);
}

void pcb_pstk_draw_preview(pcb_board_t *pcb, const pcb_pstk_t *ps, char *layers, pcb_bool mark, pcb_bool label, const pcb_box_t *drawn_area)
{
	pcb_draw_info_t info;
	int n, draw_hole = 0;
	pcb_layer_id_t lid;

	info.pcb = pcb;
	info.drawn_area = drawn_area;
	info.xform_caller = info.xform_exporter = info.xform = NULL;
	info.layer = NULL;
	info.objcb.pstk.gid = -1;
	info.objcb.pstk.holetype = PCB_PHOLE_UNPLATED | PCB_PHOLE_PLATED;

	/* draw non-currents */
	info.objcb.pstk.is_current = 0;
	for(n = pcb_proto_num_layers-1; n >= 0; n--) {
		if ((layers == NULL) || (layers[n] == 1)) {
			info.objcb.pstk.shape_mask = pcb_proto_layers[n].mask;
			info.objcb.pstk.comb = pcb_proto_layers[n].comb;
			info.objcb.pstk.layer1 = info.layer = NULL;
			if (!(info.objcb.pstk.shape_mask & PCB_LYT_COPPER)) {
				if (pcb_layer_list(pcb, info.objcb.pstk.shape_mask, &lid, 1) == 1)
					info.objcb.pstk.layer1 = info.layer = pcb_get_layer(pcb->Data, lid);
			}
			if (info.objcb.pstk.shape_mask == PCB_LYT_MECH)
				draw_hole = 1;
			else
				pcb_pstk_draw_callback((pcb_box_t *)ps, &info);
		}
	}

	/* draw current with strong */
	info.objcb.pstk.is_current = 1;
	for(n = pcb_proto_num_layers-1; n >= 0; n--) {
		if ((layers == NULL) || (layers[n] > 1)) {
			info.objcb.pstk.shape_mask = pcb_proto_layers[n].mask;
			info.objcb.pstk.comb = pcb_proto_layers[n].comb;
			info.objcb.pstk.layer1 = info.layer = NULL;
			if (!(info.objcb.pstk.shape_mask & PCB_LYT_COPPER)) {
				if (pcb_layer_list(pcb, info.objcb.pstk.shape_mask, &lid, 1) == 1)
					info.objcb.pstk.layer1 = info.layer = pcb_get_layer(pcb->Data, lid);
			}
			if (info.objcb.pstk.shape_mask == PCB_LYT_MECH)
				draw_hole = 2;
			else
				pcb_pstk_draw_callback((pcb_box_t *)ps, &info);
		}
	}

	if (draw_hole) {
		info.objcb.pstk.shape_mask = PCB_LYT_MECH;
		info.objcb.pstk.is_current = (draw_hole > 1);
		pcb_pstk_draw_hole_callback((pcb_box_t *)ps, &info);
	}

	if (mark)
		pcb_pstk_draw_mark_callback((pcb_box_t *)ps, &info);

	if (label)
		pcb_pstk_draw_label_callback((pcb_box_t *)ps, &info);
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
	pcb_any_line_t pad;
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

		case PCB_PSSH_HSHADOW:
			return 0;

		case PCB_PSSH_POLY:
			if (shape->data.poly.pa == NULL)
				pcb_pstk_shape_update_pa(&shape->data.poly);

			if (isneg) {
				/* shortcut: if any point is in our box in negative, we are done */
				for(n = 0; n < shape->data.poly.len; n++) {
					int pib = PCB_POINT_IN_BOX(ps->x + shape->data.poly.x[n], ps->y + shape->data.poly.y[n], box);
					if (pib)
						return 1;
				}
			}

			/* fully within */
			is_in = 1;
			for(n = 0; n < shape->data.poly.len; n++) {
				int pib = PCB_POINT_IN_BOX(ps->x + shape->data.poly.x[n], ps->y + shape->data.poly.y[n], box);
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
			if (pcb_polyarea_contour_inside(shape->data.poly.pa, v))
				return 1;


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

#define	HOLE_IN_BOX(ps, dia, b) \
	( \
	PCB_POINT_IN_BOX((ps)->x - dia/2, (ps)->y - dia/2, (b)) && \
	PCB_POINT_IN_BOX((ps)->x + dia/2, (ps)->y + dia/2, (b)) \
	)

#define	HOLE_TOUCHES_BOX(ps, dia, b) \
	PCB_CIRCLE_TOUCHES_BOX((ps)->x, (ps)->y, dia/2, (b))

int pcb_pstk_near_box(pcb_pstk_t *ps, pcb_box_t *box, pcb_layer_t *layer)
{
	pcb_pstk_shape_t *shp;
	pcb_pstk_tshape_t *tshp = pcb_pstk_get_tshape(ps);

	/* special case: no-shape padstack is used for hole */
	if ((tshp == NULL) || (tshp->len == 0)) {
		pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);
		if (proto->hdia <= 0)
			return 0;
		return (PCB_IS_BOX_NEGATIVE(box) ? HOLE_TOUCHES_BOX(ps, proto->hdia, box) : HOLE_IN_BOX(ps, proto->hdia, box));
	}

	/* no layer means: "is any shape near?" */
	if (layer == NULL) {
		int n;

		if (tshp == NULL)
			return 0;

		for(n = 0; n < tshp->len; n++)
			if (pcb_pstk_near_box_(ps, box, &tshp->shape[n]) != 0)
				return 1;
		return 0;
	}

	/* else check only on the specific layer */
	shp = pcb_pstk_shape(ps, pcb_layer_flags_(layer), layer->comb);
	if (shp == NULL)
		return 0;
	return pcb_pstk_near_box_(ps, box, shp);
}

static int pcb_is_point_in_pstk_(pcb_pstk_t *ps, pcb_coord_t x, pcb_coord_t y, pcb_coord_t radius, pcb_pstk_shape_t *shape)
{
	pcb_any_line_t pad;
	pcb_vector_t v;

	switch(shape->shape) {
		case PCB_PSSH_HSHADOW:
			return 0;
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
			return pcb_is_point_in_line(x, y, radius, &pad);
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
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);

	/* cheap check: hole clicked (also necessary for a mounting hole too) */
	if (proto->hdia > 0) {
		double dist2 = SQR((double)x - (double)ps->x) + SQR((double)y - (double)ps->y);
		if (dist2 <= SQR((double)radius + (double)proto->hdia/2.0))
			return 1;
	}

	/* no layer means: "is point in any shape?" */
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

	/* else check only on the specific layer */
	shp = pcb_pstk_shape(ps, pcb_layer_flags_(layer), layer->comb);
	if (shp == NULL)
		return 0;
	return pcb_is_point_in_pstk_(ps, x, y, radius, shp);
}

int pcb_pstk_drc_check_clearance(pcb_pstk_t *ps, pcb_poly_t *polygon, pcb_coord_t min_clr)
{
	int n;
	pcb_pstk_tshape_t *ts;

	/* global clearance */
	if (ps->Clearance > 0)
		return ps->Clearance < 2 * conf_core.design.bloat;

	/* else check each shape; it's safest to run this check on the canonical
	   transformed shape, that's always available */
	ts = pcb_pstk_get_tshape_(ps->parent.data, ps->proto, 0);
	for(n = 0; n < ts->len; n++)
		if ((ts->shape[n].clearance > 0) && (ts->shape[n].clearance < 2 * conf_core.design.bloat))
			return 1;

	return 0;
}

/* Check the minimum distance between a hole's edge and a shape's edge and
   indicate error if it's smaller than min */
static pcb_bool pcb_pstk_shape_hole_break(pcb_pstk_shape_t *shp, pcb_coord_t hdia, pcb_coord_t min)
{
	double dist, neck, mindist2, dist2;
	pcb_line_t line;
	int n;

	switch(shp->shape) {
		case PCB_PSSH_HSHADOW:
			break;
		case PCB_PSSH_CIRC:
			dist = sqrt((double)shp->data.circ.x*(double)shp->data.circ.x + (double)shp->data.circ.y*(double)shp->data.circ.y);
			neck = (double)(shp->data.circ.dia - hdia) / 2.0 - dist;
			break;
		case PCB_PSSH_LINE:
			line.Point1.X = shp->data.line.x1;
			line.Point1.Y = shp->data.line.y1;
			line.Point2.X = shp->data.line.x2;
			line.Point2.Y = shp->data.line.y2;
			line.Thickness = shp->data.line.thickness;
			dist = sqrt(pcb_point_line_dist2(0, 0, &line));
			neck = (double)(shp->data.line.thickness - hdia) / 2.0 - dist;
			break;
		case PCB_PSSH_POLY:
			/* square of the minimal distance required for a neck */
			mindist2 = hdia/2 + min;
			mindist2 *= mindist2;

			/* cheapest test: if any corner is closer to the hole than min, we are doomed */
			for(n = 0; n < shp->data.poly.len; n++) {
				dist2 = (double)shp->data.poly.x[n] * (double)shp->data.poly.x[n] + (double)shp->data.poly.y[n] * (double)shp->data.poly.y[n];
				if (dist2 < mindist2)
					return 1;
			}
			
			/* more expensive: check each edge */
			line.Point1.X = shp->data.poly.x[shp->data.poly.len - 1];
			line.Point1.Y = shp->data.poly.y[shp->data.poly.len - 1];
			line.Thickness = 1;

			for(n = 0; n < shp->data.poly.len; n++) {
				line.Point2.X = shp->data.poly.x[n];
				line.Point2.Y = shp->data.poly.y[n];

				dist2 = pcb_point_line_dist2(0, 0, &line);
				if (dist2 < mindist2)
					return 1;

				/* shift coords for the next iteration */
				line.Point1.X = line.Point2.X;
				line.Point1.Y = line.Point2.Y;
			}
			return 0; /* survived all tests: we are fine! */
	}

	return neck < min;
}

void pcb_pstk_drc_check_and_warn(pcb_pstk_t *ps, pcb_coord_t *err_minring, pcb_coord_t *err_minhole)
{
	pcb_pstk_proto_t *proto = pcb_pstk_get_proto(ps);

	if (proto->hdia >= 0) {
		int n;
		pcb_pstk_tshape_t *ts = pcb_pstk_get_tshape_(ps->parent.data, ps->proto, 0);

		for(n = 0; n < ts->len; n++) {
			if (!(ts->shape[n].layer_mask & PCB_LYT_COPPER))
				continue; /* only copper shapes can break */
			if (pcb_pstk_shape_hole_break(&ts->shape[n], proto->hdia, conf_core.design.min_ring)) {
				(*err_minring)++;
				break;
			}
		}
	}

TODO("slot: check if slot breaks other shapes")

	if ((proto->hdia > 0) && (proto->hdia < conf_core.design.min_drill))
		*err_minhole = proto->hdia;
}

void pcb_pstk_mirror(pcb_pstk_t *ps, pcb_coord_t y_offs, int swap_side, int disable_xmirror)
{
	int xmirror = !ps->xmirror, smirror = (swap_side ? (!ps->smirror) : ps->smirror);


	/* change the mirror flag - this will automatically cause mirroring in
	   every aspect */
	if (disable_xmirror)
		pcb_pstk_change_instance(ps, NULL, NULL, NULL, NULL, &smirror);
	else
		pcb_pstk_change_instance(ps, NULL, NULL, NULL, &xmirror, &smirror);

	/* if mirror center is not 0, also move, to emulate that the mirror took
	   place around that point */
	if (((y_offs != 0) || (ps->y != 0)) && (y_offs != PCB_PSTK_DONT_MIRROR_COORDS)) {
		pcb_poly_restore_to_poly(ps->parent.data, PCB_OBJ_PSTK, NULL, ps);
		pcb_pstk_invalidate_erase(ps);
		if (ps->parent.data->padstack_tree != NULL)
			pcb_r_delete_entry(ps->parent.data->padstack_tree, (pcb_box_t *)ps);

		ps->y = PCB_SWAP_Y(ps->y) + y_offs;
		pcb_pstk_bbox(ps);

		if (ps->parent.data->padstack_tree != NULL)
			pcb_r_insert_entry(ps->parent.data->padstack_tree, (pcb_box_t *)ps);
		pcb_poly_clear_from_poly(ps->parent.data, PCB_OBJ_PSTK, NULL, ps);
		pcb_pstk_invalidate_draw(ps);
	}
}

void pcb_pstk_scale(pcb_pstk_t *ps, double sx, double sy)
{
	pcb_pstk_proto_t *prt;
	pcb_pstk_tshape_t *tshp;
	int n;

	if ((sx == 1.0) && (sy == 1.0))
		return;

	pcb_pstk_pre(ps);

	prt = malloc(sizeof(pcb_pstk_proto_t));
	pcb_pstk_proto_copy(prt, pcb_pstk_get_proto(ps));

	/* after the copy we have the canonical transformed shape only; scale each shape in it */
	tshp = &prt->tr.array[0];
	for(n = 0; n < tshp->len; n++)
		pcb_pstk_shape_scale(&tshp->shape[n], sx, sy);

	if (prt->hdia > 0.0)
		prt->hdia = pcb_round((double)prt->hdia * ((sx+sy)/2.0));

	pcb_pstk_proto_update(prt);
	ps->proto = pcb_pstk_proto_insert_or_free(ps->parent.data, prt, 1);

	if (sx != 1.0)
		ps->x = pcb_round((double)ps->x * sx);

	if (sy != 1.0)
		ps->y = pcb_round((double)ps->y * sy);

	pcb_pstk_post(ps);
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
	int xmirror, smirror;
} padstack_change_instance_t;

#define swap(a,b,type) \
	do { \
		type tmp = a; \
		a = b; \
		b = tmp; \
	} while(0)

pcb_data_t *pcb_pstk_data_hack = NULL;

static int undo_change_instance_swap(void *udata)
{
	padstack_change_instance_t *u = udata;
	pcb_data_t *data;
	pcb_pstk_t *ps;

	/* data is either parent subc's data or board's data */
	if (u->parent_ID != -1) {
		pcb_subc_t *subc;
		int n;

		/* Look up the parent subc by ID; on the board, in the hack-data and in all buffers */
		subc = pcb_subc_by_id(PCB->Data, u->parent_ID);
		if ((subc == NULL) && (pcb_pstk_data_hack != NULL))
			subc = pcb_subc_by_id(pcb_pstk_data_hack, u->parent_ID);
		for(n = 0; (subc == NULL) && (n < PCB_MAX_BUFFER); n++)
			subc = pcb_subc_by_id(pcb_buffers[n].Data, u->parent_ID);
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

	pcb_poly_restore_to_poly(ps->parent.data, PCB_OBJ_PSTK, NULL, ps);
	pcb_pstk_invalidate_erase(ps);
	if (ps->parent.data->padstack_tree != NULL)
		pcb_r_delete_entry(ps->parent.data->padstack_tree, (pcb_box_t *)ps);

	swap(ps->proto,      u->proto,     pcb_cardinal_t);
	swap(ps->Clearance,  u->clearance, pcb_coord_t);
	swap(ps->rot,        u->rot,       double);
	swap(ps->xmirror,    u->xmirror,   int);
	swap(ps->smirror,    u->smirror,   int);

	/* force re-render the prototype */
	ps->protoi = -1;
	pcb_pstk_get_tshape(ps);

	pcb_pstk_bbox(ps);
	if (ps->parent.data->padstack_tree != NULL)
		pcb_r_insert_entry(ps->parent.data->padstack_tree, (pcb_box_t *)ps);
	pcb_poly_clear_from_poly(ps->parent.data, PCB_OBJ_PSTK, NULL, ps);
	pcb_pstk_invalidate_draw(ps);

	return 0;
}

static void undo_change_instance_print(void *udata, char *dst, size_t dst_len)
{
	padstack_change_instance_t *u = udata;
	pcb_snprintf(dst, dst_len, "padstack change: clearance=%$mm rot=%.2f xmirror=%d smirror=%d\n", u->clearance, u->rot, u->xmirror, u->smirror);
}

static const uundo_oper_t undo_pstk_change_instance = {
	core_pstk_cookie,
	NULL, /* free */
	undo_change_instance_swap,
	undo_change_instance_swap,
	undo_change_instance_print
};

int pcb_pstk_change_instance(pcb_pstk_t *ps, pcb_cardinal_t *proto, const pcb_coord_t *clearance, double *rot, int *xmirror, int *smirror)
{
	padstack_change_instance_t *u;
	long int parent_ID;
	pcb_opctx_t ctx;

	if (ps->ID <= 0) { /* off-board padstack: undo and clipping disabled */
		if (proto != NULL) ps->proto = *proto;
		if (clearance != NULL) ps->Clearance = *clearance;
		if (rot != NULL) ps->rot = *rot;
		if (xmirror != NULL) ps->xmirror = *xmirror;
		if (smirror != NULL) ps->xmirror = *smirror;
		pcb_pstk_bbox(ps);
		return 0;
	}

	switch(ps->parent.data->parent_type) {
		case PCB_PARENT_INVALID:
			/* happens if parent is a buffer: do not undo operations done on the buffer */
			ps->proto = proto ? *proto : ps->proto;
			ps->Clearance = clearance ? *clearance : ps->Clearance;
			ps->rot = rot ? *rot : ps->rot;
			ps->xmirror = xmirror ? *xmirror : ps->xmirror;
			ps->smirror = smirror ? *smirror : ps->smirror;

			/* re-render the prototype so the new shape is generated and cached */
			ps->protoi = -1;
			pcb_pstk_get_tshape(ps);
			return 0;
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
	u->smirror = smirror ? *smirror : ps->smirror;

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

void pcb_pstk_rotate90(pcb_pstk_t *pstk, pcb_coord_t cx, pcb_coord_t cy, int steps)
{
	pcb_opctx_t ctx;

	ctx.rotate.center_x = cx;
	ctx.rotate.center_y = cy;
	ctx.rotate.number = steps;
	pcb_pstkop_rotate90(&ctx, pstk);
}

void pcb_pstk_rotate(pcb_pstk_t *pstk, pcb_coord_t cx, pcb_coord_t cy, double cosa, double sina, double angle)
{
	pcb_opctx_t ctx;

	ctx.rotate.center_x = cx;
	ctx.rotate.center_y = cy;
	ctx.rotate.angle = angle;
	ctx.rotate.cosa = cosa;
	ctx.rotate.sina = sina;
	pcb_pstkop_rotate(&ctx, pstk);
}

pcb_coord_t obj_pstk_get_clearance(pcb_board_t *pcb, pcb_pstk_t *ps, pcb_layer_t *layer)
{
	pcb_pstk_shape_t *shp;

	if (!PCB_FLAG_TEST(PCB_FLAG_CLEARLINE, ps))
		return 0;

	if (ps->Clearance > 0) /* global clarance overrides */
		return ps->Clearance;

	shp = pcb_pstk_shape_at(pcb, ps, layer);
	return shp->clearance;
}

void pcb_pstk_pre(pcb_pstk_t *pstk)
{
	pcb_data_t *data = pstk->parent.data;
	if (data->padstack_tree != NULL)
		pcb_r_delete_entry(data->padstack_tree, (pcb_box_t *)pstk);
	pcb_poly_restore_to_poly(data, PCB_OBJ_PSTK, NULL, pstk);
}

void pcb_pstk_post(pcb_pstk_t *pstk)
{
	pcb_data_t *data = pstk->parent.data;
	if (data->padstack_tree != NULL)
		pcb_r_insert_entry(data->padstack_tree, (pcb_box_t *)pstk);
	pcb_poly_clear_from_poly(data, PCB_OBJ_PSTK, NULL, pstk);
}

pcb_layer_id_t pcb_proto_board_layer_for(const pcb_data_t *data, pcb_layer_type_t mask, pcb_layer_combining_t comb)
{
	pcb_layer_id_t lid;
	const pcb_layer_t *ly;
	for(lid = 0, ly = data->Layer; lid < data->LayerN; lid++,ly++) {
		pcb_layer_type_t typ = pcb_layer_flags_(ly);
		if ((typ == mask) && (ly->comb == comb))
			return lid;
	}
	return -1;
}
