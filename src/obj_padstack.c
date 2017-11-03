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
#include "obj_padstack.h"
#include "obj_padstack_draw.h"
#include "obj_padstack_list.h"
#include "obj_padstack_inlines.h"
#include "operation.h"
#include "search.h"
#include "undo.h"
#include "vtpadstack.h"

#define PS_CROSS_SIZE PCB_MM_TO_COORD(1)

static const char core_padstack_cookie[] = "padstack";

pcb_padstack_t *pcb_padstack_alloc(pcb_data_t *data)
{
	pcb_padstack_t *ps;

	ps = calloc(sizeof(pcb_padstack_t), 1);
	ps->protoi = -1;
	ps->type = PCB_OBJ_PADSTACK;
	ps->Attributes.post_change = pcb_obj_attrib_post_change;
	PCB_SET_PARENT(ps, data, data);

	padstacklist_append(&data->padstack, ps);

	return ps;
}

void pcb_padstack_free(pcb_padstack_t *ps)
{
	padstacklist_remove(ps);
	free(ps->thermals.shape);
	free(ps);
}

pcb_padstack_t *pcb_padstack_new(pcb_data_t *data, pcb_cardinal_t proto, pcb_coord_t x, pcb_coord_t y, pcb_coord_t clearance, pcb_flag_t Flags)
{
	pcb_padstack_t *ps;

	if (proto >= pcb_vtpadstack_proto_len(&data->ps_protos))
		return NULL;

	ps = pcb_padstack_alloc(data);

	/* copy values */
	ps->proto = proto;
	ps->x = x;
	ps->y = y;
	ps->Clearance = clearance;
	ps->Flags = Flags;
	ps->ID = pcb_create_ID_get();
	pcb_padstack_add(data, ps);
	return ps;
}

void pcb_padstack_add(pcb_data_t *data, pcb_padstack_t *ps)
{
	pcb_padstack_bbox(ps);
	if (!data->padstack_tree)
		data->padstack_tree = pcb_r_create_tree(NULL, 0, 0);
	pcb_r_insert_entry(data->padstack_tree, (pcb_box_t *)ps, 0);
	PCB_SET_PARENT(ps, data, data);
}

void pcb_padstack_bbox(pcb_padstack_t *ps)
{
	int n, sn;
	pcb_line_t line;
	pcb_padstack_proto_t *proto = pcb_padstack_get_proto(ps);
	pcb_padstack_tshape_t *ts = pcb_padstack_get_tshape(ps);
	assert(proto != NULL);

	ps->BoundingBox.X1 = ps->BoundingBox.X2 = ps->x;
	ps->BoundingBox.Y1 = ps->BoundingBox.Y2 = ps->y;

	for(sn = 0; sn < ts->len; sn++) {
		pcb_padstack_shape_t *shape = ts->shape + sn;
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

static pcb_padstack_t *pcb_padstack_copy_meta(pcb_padstack_t *dst, pcb_padstack_t *src)
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
	return dst;
}


void pcb_padstack_move(pcb_padstack_t *ps, pcb_coord_t dx, pcb_coord_t dy)
{
	ps->x += dx;
	ps->y += dy;
	ps->BoundingBox.X1 += dx;
	ps->BoundingBox.Y1 += dy;
	ps->BoundingBox.X2 += dx;
	ps->BoundingBox.Y2 += dy;
}

pcb_padstack_t *pcb_padstack_by_id(pcb_data_t *base, long int ID)
{
	pcb_box_t *ps;
	pcb_rtree_it_t it;

	for(ps = pcb_r_first(base->padstack_tree, &it); ps != NULL; ps = pcb_r_next(&it)) {
		if (((pcb_padstack_t *)ps)->ID == ID) {
			pcb_r_end(&it);
			return (pcb_padstack_t *)ps;
		}
	}

	pcb_r_end(&it);
	return NULL;
}

/*** draw ***/

static void set_ps_color(pcb_padstack_t *ps, int is_current)
{
	char *color;
	char buf[sizeof("#XXXXXX")];

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
			if (is_current)
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
			if (is_current)
				color = conf_core.appearance.color.pin;
			else
				color = conf_core.appearance.color.invisible_objects;

	}

	pcb_gui->set_color(Output.fgGC, color);
}

static void set_ps_annot_color(pcb_hid_gc_t gc, pcb_padstack_t *ps)
{
	pcb_gui->set_color(Output.fgGC, PCB_FLAG_TEST(PCB_FLAG_SELECTED, ps) ?
		conf_core.appearance.color.subc_selected : conf_core.appearance.color.padstackmark);
}

pcb_r_dir_t pcb_padstack_draw_callback(const pcb_box_t *b, void *cl)
{
	pcb_padstack_draw_t *ctx = cl;
	pcb_padstack_t *ps = (pcb_padstack_t *)b;
	pcb_padstack_shape_t *shape;
	pcb_coord_t mark;
	pcb_padstack_proto_t *proto;

	shape = pcb_padstack_shape_gid(ctx->pcb, ps, ctx->gid, (ctx->comb & ~PCB_LYC_AUTO));
	if (shape != NULL) {
		pcb_gui->set_draw_xor(Output.fgGC, 0);
		switch(shape->shape) {
			case PCB_PSSH_POLY:
				set_ps_color(ps, ctx->is_current);
				pcb_gui->fill_polygon_offs(Output.fgGC, shape->data.poly.len, shape->data.poly.x, shape->data.poly.y, ps->x, ps->y);
				break;
			case PCB_PSSH_LINE:
				set_ps_color(ps, ctx->is_current);
				pcb_gui->set_line_cap(Output.fgGC, shape->data.line.square ? Square_Cap : Round_Cap);
				pcb_gui->set_line_width(Output.fgGC, shape->data.line.thickness);
				pcb_gui->draw_line(Output.fgGC, ps->x + shape->data.line.x1, ps->y + shape->data.line.y1, ps->x + shape->data.line.x2, ps->y + shape->data.line.y2);
				break;
			case PCB_PSSH_CIRC:
				set_ps_color(ps, ctx->is_current);
				pcb_gui->fill_circle(Output.fgGC, ps->x + shape->data.circ.x, ps->y + shape->data.circ.y, shape->data.circ.dia/2);
				break;
		}
	}

	mark = PS_CROSS_SIZE/2;
	proto = pcb_padstack_get_proto(ps);
	if (proto != NULL)
		mark += proto->hdia/2;

	set_ps_annot_color(Output.fgGC, ps);
	pcb_gui->set_line_width(Output.fgGC, -3);
	pcb_gui->set_draw_xor(Output.fgGC, 1);
	pcb_gui->draw_line(Output.fgGC, ps->x-mark, ps->y, ps->x+mark, ps->y);
	pcb_gui->draw_line(Output.fgGC, ps->x, ps->y-mark, ps->x, ps->y+mark);
	pcb_gui->set_draw_xor(Output.fgGC, 0);

	return PCB_R_DIR_FOUND_CONTINUE;
}

pcb_r_dir_t pcb_padstack_draw_hole_callback(const pcb_box_t *b, void *cl)
{
	pcb_padstack_draw_t *ctx = cl;
	pcb_padstack_t *ps = (pcb_padstack_t *)b;
	pcb_padstack_proto_t *proto;

	if (!pcb_padstack_bb_drills(ctx->pcb, ps, ctx->gid, &proto))
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

void pcb_padstack_thindraw(pcb_hid_gc_t gc, pcb_padstack_t *ps)
{
	pcb_padstack_shape_t *shape = NULL;
	pcb_board_t *pcb;
	pcb_layergrp_id_t gid = CURRENT->meta.real.grp;
	int n;

	pcb = pcb_data_get_top(ps->parent.data);
	if (pcb != NULL) {
		shape = pcb_padstack_shape_gid(pcb, ps, gid, 0);
		if (shape == NULL)
			shape = pcb_padstack_shape_gid(pcb, ps, gid, PCB_LYC_SUB);
	}
	else { /* no pcb means buffer - take the first shape, whichever layer it is for */
		pcb_padstack_tshape_t *ts = pcb_padstack_get_tshape(ps);
		if (ts != NULL)
			shape = ts->shape;
	}

	if (shape != NULL) {
		pcb_gui->set_draw_xor(gc, 0);
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
}

void pcb_padstack_invalidate_erase(pcb_padstack_t *ps)
{
	pcb_draw_invalidate(ps);
}

void pcb_padstack_invalidate_draw(pcb_padstack_t *ps)
{
	pcb_draw_invalidate(ps);
}


int pcb_padstack_near_box(pcb_padstack_t *ps, pcb_box_t *box)
{
#warning padstack TODO: refine this: consider the shapes on the layers that are visible
	return (PCB_IS_BOX_NEGATIVE(box) ? PCB_BOX_TOUCHES_BOX(&ps->BoundingBox,box) : PCB_BOX_IN_BOX(&ps->BoundingBox,box));
}

int pcb_is_point_in_padstack(pcb_coord_t x, pcb_coord_t y, pcb_coord_t radius, pcb_padstack_t *ps)
{
#warning padstack TODO: refine this: consider the shapes on the layers that are visible
	return (x >= ps->BoundingBox.X1) && (y >= ps->BoundingBox.Y1) && (x <= ps->BoundingBox.X2) && (y <= ps->BoundingBox.Y2);
}

int pcb_padstack_drc_check_clearance(pcb_padstack_t *ps, pcb_poly_t *polygon, pcb_coord_t min_clr)
{
#warning padstack TODO
	return 0;
}


int pcb_padstack_drc_check_and_warn(pcb_padstack_t *ps)
{
#warning padstack TODO
	return 0;
}

unsigned char *pcb_padstack_get_thermal(pcb_padstack_t *ps, unsigned long lid, pcb_bool_t alloc)
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

void pcb_padstack_set_thermal(pcb_padstack_t *ps, unsigned long lid, unsigned char shape)
{
	unsigned char *th = pcb_padstack_get_thermal(ps, lid, 1);
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
	pcb_padstack_t *ps;

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

	ps = pcb_padstack_by_id(data, u->ID);
	if (ps == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't undo padstack change: padstack ID #%ld is not available\n", u->ID);
		return -1;
	}

	swap(ps->proto,      u->proto,     pcb_cardinal_t);
	swap(ps->Clearance,  u->clearance, pcb_coord_t);
	swap(ps->rot,        u->rot,       double);
	swap(ps->xmirror,    u->xmirror,   int);

	/* force re-render the prototype */
	ps->protoi = -1;
	pcb_padstack_get_tshape(ps);

	return 0;
}

static void undo_change_instance_print(void *udata, char *dst, size_t dst_len)
{
	padstack_change_instance_t *u = udata;
	pcb_snprintf(dst, dst_len, "padstack change: clearance=%$mm rot=%.2f xmirror=%d\n", u->clearance, u->rot, u->xmirror);
}

static const uundo_oper_t undo_padstack_change_instance = {
	core_padstack_cookie,
	NULL, /* free */
	undo_change_instance_swap,
	undo_change_instance_swap,
	undo_change_instance_print
};

int pcb_padstack_change_instance(pcb_padstack_t *ps, pcb_cardinal_t *proto, const pcb_coord_t *clearance, double *rot, int *xmirror)
{
	padstack_change_instance_t *u;
	long int parent_ID;

	switch(ps->parent.data->parent_type) {
		case PCB_PARENT_BOARD: parent_ID = -1; break;
		case PCB_PARENT_SUBC: parent_ID = ps->parent.data->parent.subc->ID; break;
		default: return -1;
	}

	u = pcb_undo_alloc(PCB, &undo_padstack_change_instance, sizeof(padstack_change_instance_t));
	u->parent_ID = parent_ID;
	u->ID = ps->ID;
	u->proto = proto ? *proto : ps->proto;
	u->clearance = clearance ? *clearance : ps->Clearance;
	u->rot = rot ? *rot : ps->rot;
	u->xmirror = xmirror ? *xmirror : ps->xmirror;

	undo_change_instance_swap(u);

	pcb_undo_inc_serial();
	return 0;
}

#include "obj_padstack_op.c"
