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
#include "flag.h"
#include "obj_padstack.h"
#include "obj_padstack_draw.h"
#include "obj_padstack_list.h"
#include "obj_padstack_inlines.h"
#include "search.h"
#include "vtpadstack.h"

#define PS_CROSS_SIZE PCB_MM_TO_COORD(1)

pcb_padstack_t *pcb_padstack_alloc(pcb_data_t *data)
{
	pcb_padstack_t *ps;

	ps = calloc(sizeof(pcb_padstack_t), 1);
	ps->type = PCB_OBJ_PADSTACK;
	ps->Attributes.post_change = pcb_obj_attrib_post_change;
	PCB_SET_PARENT(ps, data, data);

	padstacklist_append(&data->padstack, ps);

	return ps;
}

void pcb_padstack_free(pcb_padstack_t *ps)
{
	padstacklist_remove(ps);
	free(ps);
}

pcb_padstack_t *pcb_padstack_new(pcb_data_t *data, pcb_cardinal_t proto, pcb_coord_t x, pcb_coord_t y, pcb_flag_t Flags)
{
	pcb_padstack_t *ps;

	if (proto >= pcb_vtpadstack_proto_len(&data->ps_protos))
		return NULL;

	ps = pcb_padstack_alloc(data);

	/* copy values */
	ps->proto = proto;
	ps->x = x;
	ps->y = y;
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
	assert(proto != NULL);

	ps->BoundingBox.X1 = ps->BoundingBox.X2 = ps->x;
	ps->BoundingBox.Y1 = ps->BoundingBox.Y2 = ps->y;

	for(sn = 0; sn < proto->len; sn++) {
		pcb_padstack_shape_t *shape = proto->shape + sn;
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

	if (proto->hdia != 0) {
		/* corner case: no copper around the hole all 360 deg - let the hole stick out */
		pcb_box_bump_point(&ps->BoundingBox, ps->x - proto->hdia/2, ps->y - proto->hdia/2);
		pcb_box_bump_point(&ps->BoundingBox, ps->x + proto->hdia/2, ps->y + proto->hdia/2);
	}

	pcb_close_box(&ps->BoundingBox);
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



pcb_r_dir_t pcb_padstack_draw_callback(const pcb_box_t *b, void *cl)
{
	pcb_padstack_draw_t *ctx = cl;
	pcb_padstack_t *ps = (pcb_padstack_t *)b;
	pcb_padstack_shape_t *shape;

pcb_trace("DRAW %ld!\n", (long int)ctx->gid);

#warning padstack TODO: comb should not be 0 - draw both add and sub!
	shape = pcb_padstack_shape(ps, pcb_layergrp_flags(ctx->pcb, ctx->gid), 0);
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

#warning padstack TODO: have an own color instead of subc_*
	pcb_gui->set_color(Output.fgGC, PCB_FLAG_TEST(PCB_FLAG_SELECTED, ps) ? conf_core.appearance.color.subc_selected : conf_core.appearance.color.subc);
	pcb_gui->set_line_width(Output.fgGC, 0);
	pcb_gui->set_draw_xor(Output.fgGC, 1);
	pcb_gui->draw_line(Output.fgGC, ps->x-PS_CROSS_SIZE/2, ps->y, ps->x+PS_CROSS_SIZE/2, ps->y);
	pcb_gui->draw_line(Output.fgGC, ps->x, ps->y-PS_CROSS_SIZE/2, ps->x, ps->y+PS_CROSS_SIZE/2);
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


#warning padstack TODO: implement these
void pcb_padstack_invalidate_erase(pcb_padstack_t *ps)
{

}

void pcb_padstack_invalidate_draw(pcb_padstack_t *ps)
{

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
