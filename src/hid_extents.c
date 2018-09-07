/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 2004 harry eaton
 *  Copyright (C) 2016..2018 Tibor 'Igor2' Palinkas
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

#include "config.h"
#include "board.h"
#include "data.h"
#include "hid_draw_helpers.h"
#include "hid_extents.h"
#include "funchash_core.h"

static pcb_box_t box;

typedef struct hid_gc_s {
	int width;
} hid_gc_s;

static int extents_set_layer_group(pcb_layergrp_id_t group, const char *purpose, int purpi, pcb_layer_id_t layer, unsigned int flags, int is_empty)
{
	if (PCB_LAYER_IS_DRILL(flags, purpi))
		return 1;
	switch (flags & PCB_LYT_ANYTHING) {
		case PCB_LYT_COPPER:
		case PCB_LYT_BOUNDARY:
		case PCB_LYT_MECH:
		case PCB_LYT_SILK:
			return 1;
		default:
			return 0;
	}
	return 0;
}

static pcb_hid_gc_t extents_make_gc(void)
{
	pcb_hid_gc_t rv = (pcb_hid_gc_t) malloc(sizeof(hid_gc_s));
	memset(rv, 0, sizeof(hid_gc_s));
	return rv;
}

static void extents_destroy_gc(pcb_hid_gc_t gc)
{
	free(gc);
}

static void extents_set_drawing_mode(pcb_composite_op_t op, pcb_bool direct, const pcb_box_t *screen)
{
}

static void extents_render_burst(pcb_burst_op_t op, const pcb_box_t *screen)
{
}

static void extents_set_color(pcb_hid_gc_t gc, const char *name)
{
}

static void extents_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
}

static void extents_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
{
	gc->width = width;
}

static void extents_set_draw_xor(pcb_hid_gc_t gc, int xor_)
{
}

#define PEX(x,w) if (box.X1 > (x)-(w)) box.X1 = (x)-(w); \
	if (box.X2 < (x)+(w)) box.X2 = (x)+(w)
#define PEY(y,w) if (box.Y1 > (y)-(w)) box.Y1 = (y)-(w); \
	if (box.Y2 < (y)+(w)) box.Y2 = (y)+(w)

static void extents_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	PEX(x1, gc->width);
	PEY(y1, gc->width);
	PEX(x2, gc->width);
	PEY(y2, gc->width);
}

static void extents_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t start_angle, pcb_angle_t end_angle)
{
	/* Naive but good enough.  */
	PEX(cx, width + gc->width);
	PEY(cy, height + gc->width);
}

static void extents_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	PEX(x1, gc->width);
	PEY(y1, gc->width);
	PEX(x2, gc->width);
	PEY(y2, gc->width);
}

static void extents_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
	PEX(cx, radius);
	PEY(cy, radius);
}

static void extents_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t * x, pcb_coord_t * y)
{
	int i;
	for (i = 0; i < n_coords; i++) {
		PEX(x[i], 0);
		PEY(y[i], 0);
	}
}

static void extents_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	PEX(x1, 0);
	PEY(y1, 0);
	PEX(x2, 0);
	PEY(y2, 0);
}

static pcb_hid_t extents_hid;

void hid_extents_init(void)
{
	static pcb_bool initialised = pcb_false;

	if (initialised)
		return;

	memset(&extents_hid, 0, sizeof(pcb_hid_t));

	pcb_dhlp_draw_helpers_init(&extents_hid);

	extents_hid.struct_size = sizeof(pcb_hid_t);
	extents_hid.name = "extents-extents";
	extents_hid.description = "used to calculate extents";

	extents_hid.set_layer_group = extents_set_layer_group;
	extents_hid.make_gc = extents_make_gc;
	extents_hid.destroy_gc = extents_destroy_gc;
	extents_hid.set_drawing_mode = extents_set_drawing_mode;
	extents_hid.render_burst = extents_render_burst;
	extents_hid.set_color = extents_set_color;
	extents_hid.set_line_cap = extents_set_line_cap;
	extents_hid.set_line_width = extents_set_line_width;
	extents_hid.set_draw_xor = extents_set_draw_xor;
	extents_hid.draw_line = extents_draw_line;
	extents_hid.draw_arc = extents_draw_arc;
	extents_hid.draw_rect = extents_draw_rect;
	extents_hid.fill_circle = extents_fill_circle;
	extents_hid.fill_polygon = extents_fill_polygon;
	extents_hid.fill_rect = extents_fill_rect;

	initialised = pcb_true;
}

pcb_box_t *pcb_hid_get_extents_pinout(void *item)
{
	/* As this isn't a real "HID", we need to ensure we are initialised. */
	hid_extents_init();

	box.X1 = COORD_MAX;
	box.Y1 = COORD_MAX;
	box.X2 = -COORD_MAX;
	box.Y2 = -COORD_MAX;

	pcb_hid_expose_pinout(&extents_hid, item);

	return &box;
}
