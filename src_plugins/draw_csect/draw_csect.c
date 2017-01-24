/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

/* draw cross section */
#include "config.h"


#include "board.h"
#include "data.h"
#include "draw.h"
#include "plugins.h"
#include "stub_draw_csect.h"

#include "obj_text_draw.h"
#include "obj_line_draw.h"

static const char *COLOR_ANNOT = "#000000";
static const char *COLOR_BG = "#f0f0f0";

static const char *COLOR_COPPER = "#C05020";
static const char *COLOR_SUBSTRATE = "#E0D090";
static const char *COLOR_SILK = "#000000";
static const char *COLOR_MASK = "#30d030";
static const char *COLOR_PASTE = "#60e0e0";
static const char *COLOR_MISC = "#e0e000";
static const char *COLOR_OUTLINE = "#000000";

static pcb_layer_id_t drag_lid= -1;

/* Draw a text at x;y sized scale percentage */
static pcb_text_t *dtext(int x, int y, int scale, int dir, const char *txt)
{
	static pcb_text_t t;

	t.X = PCB_MM_TO_COORD(x);
	t.Y = PCB_MM_TO_COORD(y);
	t.TextString = (char *)txt;
	t.Direction = dir;
	t.Scale = scale;
	t.Flags = pcb_no_flags();
	DrawTextLowLevel(&t, 0);
	return &t;
}

/* Draw a text at x;y sized scale percentage */
static pcb_text_t *dtext_(pcb_coord_t x, pcb_coord_t y, int scale, int dir, const char *txt, pcb_coord_t th)
{
	static pcb_text_t t;

	t.X = x;
	t.Y = y;
	t.TextString = (char *)txt;
	t.Direction = dir;
	t.Scale = scale;
	t.Flags = pcb_no_flags();
	DrawTextLowLevel(&t, th);
	return &t;
}

/* Draw a text at x;y with a background */
static pcb_text_t *dtext_bg(pcb_hid_gc_t gc, int x, int y, int scale, int dir, const char *txt, const char *bgcolor, const char *fgcolor)
{
	static pcb_text_t t;

	t.X = PCB_MM_TO_COORD(x);
	t.Y = PCB_MM_TO_COORD(y);
	t.TextString = (char *)txt;
	t.Direction = dir;
	t.Scale = scale;
	t.Flags = pcb_no_flags();

	pcb_gui->set_color(gc, bgcolor);
	DrawTextLowLevel(&t, 1000000);

	pcb_gui->set_color(gc, fgcolor);
	DrawTextLowLevel(&t, 0);

	return &t;
}

/* draw a line of a specific thickness */
static void dline(int x1, int y1, int x2, int y2, float thick)
{
	pcb_line_t l;
	l.Point1.X = PCB_MM_TO_COORD(x1);
	l.Point1.Y = PCB_MM_TO_COORD(y1);
	l.Point2.X = PCB_MM_TO_COORD(x2);
	l.Point2.Y = PCB_MM_TO_COORD(y2);
	l.Thickness = PCB_MM_TO_COORD(thick);
	_draw_line(&l);
}

/* draw a line of a specific thickness */
static void dline_(pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, float thick)
{
	pcb_line_t l;
	l.Point1.X = x1;
	l.Point1.Y = y1;
	l.Point2.X = x2;
	l.Point2.Y = y2;
	l.Thickness = PCB_MM_TO_COORD(thick);
	_draw_line(&l);
}


/* draw a line clipped with two imaginary vertical lines at cx1 and cx2 */
static void dline_vclip(int x1, int y1, int x2, int y2, float thick, int cx1, int cx2)
{
	if (cx2 < cx1) { /* make sure cx2 > cx1 */
		int tmp;
		tmp = cx1;
		cx1 = cx2;
		cx2 = tmp;
	}

	if (x2 == x1) { /* do not clip vertical lines */
		if ((x1 >= cx1) && (x1 <= cx2))
			dline(x1, y1, x2, y2, thick);
		return;
	}

	if (x2 < x1) { /* make sure x2 > x1 */
		int tmp;
		tmp = x1;
		x1 = x2;
		x2 = tmp;
		tmp = y1;
		y1 = y2;
		y2 = tmp;
	}

	/* clip */
	if (x2 > cx2) {
		y2 = (double)(cx2 - x1) / (double)(x2 - x1) * (double)(y2 - y1) + y1;
		x2 = cx2;
	}
	if (x1 < cx1) {
		y1 = (double)(cx1 - x1) / (double)(x2 - x1) * (double)(y2 - y1) + y1;
		x1 = cx1;
	}

	dline(x1, y1, x2, y2, thick);
}


/* Draw a 45-deg hactched box wihtout the rectangle;
   if step > 0 use \ hatching, else use / */
static void hatch_box(int x1, int y1, int x2, int y2, float thick, int step)
{
	int n, h = y2 - y1;

	if (step > 0)
		for(n = x1 - h; n <= x2; n += step)
			dline_vclip(n, y1, n+h, y2, thick,  x1, x2);
	else
		for(n = x2; n >= x1-h; n += step)
			dline_vclip(n+h, y1, n, y2, thick,  x1, x2);
}

enum {
	OMIT_NONE = 0,
	OMIT_TOP = 1,
	OMIT_BOTTOM = 2,
	OMIT_LEFT = 4,
	OMIT_RIGHT = 8
};

/* draw a hatched rectangle; to turn off hatching in a directon set the
   corresponding step to 0 */
static void dhrect(int x1, int y1, int x2, int y2, float thick_rect, float thick_hatch, int step_fwd, int step_back, unsigned omit)
{
	if (!(omit & OMIT_TOP))
		dline(x1, y1, x2, y1, thick_rect);
	if (!(omit & OMIT_RIGHT))
		dline(x2, y1, x2, y2, thick_rect);
	if (!(omit & OMIT_BOTTOM))
		dline(x1, y2, x2, y2, thick_rect);
	if (!(omit & OMIT_LEFT))
		dline(x1, y1, x1, y2, thick_rect);

	if (step_fwd > 0)
		hatch_box(x1, y1, x2, y2, thick_hatch, +step_fwd);

	if (step_back > 0)
		hatch_box(x1, y1, x2, y2, thick_hatch, -step_back);
}

static pcb_box_t layer_crd[PCB_MAX_LAYER];
static pcb_box_t group_crd[PCB_MAX_LAYERGRP];
static char layer_valid[PCB_MAX_LAYER];
static char group_valid[PCB_MAX_LAYERGRP];

static void reg_layer_coords(pcb_layer_id_t lid, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	if ((lid < 0) || (lid >= PCB_MAX_LAYER))
		return;
	layer_crd[lid].X1 = x1;
	layer_crd[lid].Y1 = y1;
	layer_crd[lid].X2 = x2;
	layer_crd[lid].Y2 = y2;
	layer_valid[lid] = 1;
}

static void reg_group_coords(pcb_layergrp_id_t gid, pcb_coord_t y1, pcb_coord_t y2)
{
	if ((gid < 0) || (gid >= PCB_MAX_LAYER))
		return;
	group_crd[gid].Y1 = y1;
	group_crd[gid].Y2 = y2;
	group_valid[gid] = 1;
}

static void reset_layer_coords(void)
{
	memset(layer_valid, 0, sizeof(layer_valid));
	memset(group_valid, 0, sizeof(group_valid));
}

static pcb_layer_id_t get_layer_coords(pcb_coord_t x, pcb_coord_t y)
{
	pcb_layer_id_t n;

	for(n = 0; n < PCB_MAX_LAYER; n++) {
		if (!layer_valid[n]) continue;
		if ((layer_crd[n].X1 <= x) && (layer_crd[n].Y1 <= y) &&
		    (layer_crd[n].X2 >= x) && (layer_crd[n].Y2 >= y))
			return n;
	}
	return -1;
}

static pcb_layergrp_id_t get_group_coords(pcb_coord_t y, pcb_coord_t *y1, pcb_coord_t *y2)
{
	pcb_layergrp_id_t n;

	for(n = 0; n < PCB_MAX_LAYERGRP; n++) {
		if (!group_valid[n]) continue;
		if ((group_crd[n].Y1 <= y) && (group_crd[n].Y2 >= y)) {
			*y1 = group_crd[n].Y1;
			*y2 = group_crd[n].Y2;
			return n;
		}
	}
	return -1;
}


static pcb_hid_gc_t csect_gc;

/* Draw the cross-section layer */
static void draw_csect(pcb_hid_gc_t gc)
{
	pcb_layergrp_id_t gid;
	int ystart = 10, y, last_copper_step = 5, has_outline = 0;

	reset_layer_coords();
	csect_gc = gc;

	pcb_gui->set_color(gc, COLOR_ANNOT);
	dtext(0, 0, 500, 0, "Board cross section");

	/* draw physical layers */
	y = ystart;
	for(gid = 0; gid < pcb_max_group; gid++) {
		int x, i, stepf = 0, stepb = 0, th;
		pcb_layer_group_t *g = PCB->LayerGroups.grp + gid;
		const char *color = "#ff0000";

		if (!g->valid)
			continue;
		else if (g->type & PCB_LYT_COPPER) {
			last_copper_step = -last_copper_step;
			if (last_copper_step > 0)
				stepf = last_copper_step;
			else
				stepb = -last_copper_step;
			th = 5;
			color = COLOR_COPPER;
		}
		else if (g->type & PCB_LYT_SUBSTRATE) {
			stepf = stepb = 7;
			th = 10;
			color = COLOR_SUBSTRATE;
		}
		else if (g->type & PCB_LYT_SILK) {
			th = 5;
			color = COLOR_SILK;
			stepb = 3;
		}
		else if (g->type & PCB_LYT_MASK) {
			th = 5;
			color = COLOR_MASK;
			stepb = 9;
		}
		else if (g->type & PCB_LYT_PASTE) {
			th = 5;
			color = COLOR_PASTE;
			stepf = 9;
		}
		else if (g->type & PCB_LYT_MISC) {
			th = 5;
			color = COLOR_MISC;
			stepf = 3;
		}
		else if (g->type & PCB_LYT_OUTLINE) {
			has_outline = 1;
			continue;
		}
		else
			continue;

		pcb_gui->set_color(gc, color);
		dhrect(0, y, 75, y+th,  1, 0.5,  stepf, stepb, OMIT_LEFT | OMIT_RIGHT);
		dtext_bg(gc, 5, y, 200, 0, g->name, COLOR_BG, COLOR_ANNOT);
		reg_group_coords(gid, PCB_MM_TO_COORD(y), PCB_MM_TO_COORD(y+th));


		/* draw layer names */
		if (g->type & PCB_LYT_COPPER) {
			x = 78;
			for(i = 0; i < g->len; i++) {
				pcb_text_t *t;
				pcb_layer_id_t lid = g->lid[i];
				pcb_layer_t *l = &PCB->Data->Layer[lid];
				if (lid == drag_lid)
					continue;
				t = dtext_bg(gc, x, y, 200, 0, l->Name, COLOR_BG, l->Color);
				pcb_text_bbox(&PCB->Font, t);
				x += PCB_COORD_TO_MM(t->BoundingBox.X2 - t->BoundingBox.X1) + 3;
				dhrect(PCB_COORD_TO_MM(t->BoundingBox.X1), y, PCB_COORD_TO_MM(t->BoundingBox.X2)+1, y+4, 0.25, 0, 0, 0, OMIT_NONE);
				reg_layer_coords(lid, t->BoundingBox.X1, PCB_MM_TO_COORD(y), t->BoundingBox.X2+PCB_MM_TO_COORD(1), PCB_MM_TO_COORD(y+4));
			}
		}

		/* increment y */
		y += th + 1;
	}

	/* draw global/special layers */
	if (has_outline) {
		pcb_gui->set_color(gc, COLOR_OUTLINE);
		dline(0, ystart, 0, y+10, 1);
		dtext_bg(gc, 1, y+7, 200, 0, "outline", COLOR_BG, COLOR_ANNOT);
	}
}

static pcb_coord_t ox, oy, lx, ly, cx, cy, gy1, gy2;
static int lvalid, gvalid;
static pcb_layergrp_id_t gactive = -1;

static void mark_grp(pcb_coord_t y, unsigned int accept_mask)
{
	pcb_coord_t y1, y2, x0 = -PCB_MM_TO_COORD(5);
	pcb_layergrp_id_t g;

	if ((y1 == gy1) && (y2 == gy2) && gvalid)
		return;
	if (gvalid) {
		dline_(x0, gy1, PCB_MM_TO_COORD(200), gy1, 0.1);
		dline_(x0, gy2, PCB_MM_TO_COORD(200), gy2, 0.1);
		gvalid = 0;
	}
	g = get_group_coords(y, &y1, &y2);
	if ((g >= 0) && (pcb_layergrp_flags(g) & accept_mask)) {
		gy1 = y1;
		gy2 = y2;
		gactive = g;
		gvalid = 1;
		dline_(x0, y1, PCB_MM_TO_COORD(200), y1, 0.1);
		dline_(x0, y2, PCB_MM_TO_COORD(200), y2, 0.1);
	}
	else
		gactive = -1;
}

static void draw_csect_overlay(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *ctx)
{
	if (drag_lid >= 0) {
		pcb_hid_t *old_gui = pcb_gui;
		pcb_layer_t *l = &PCB->Data->Layer[drag_lid];
		pcb_gui = hid;
		Output.fgGC = pcb_gui->make_gc();

		pcb_gui->set_color(Output.fgGC, "#000000");
		pcb_gui->set_draw_xor(Output.fgGC, 1);

		if ((lx != cx) && (ly != cy)) {
			if (lvalid) {
				dtext_(lx, ly, 250, 0, l->Name, PCB_MM_TO_COORD(0.01));
				lvalid = 0;
			}
			dtext_(cx, cy, 250, 0, l->Name, PCB_MM_TO_COORD(0.01));
			lx = cx;
			ly = cy;
			lvalid = 1;
		}
		mark_grp(cy, PCB_LYT_COPPER);
		pcb_gui->destroy_gc(Output.fgGC);
		pcb_gui = old_gui;
	}
}


static pcb_bool mouse_csect(void *widget, pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y)
{
	pcb_bool res = 0;
	switch(kind) {
		case PCB_HID_MOUSE_PRESS:
			drag_lid = get_layer_coords(x, y);
			ox = x;
			oy = y;
			lvalid = 0;
			printf("lid=%d\n", drag_lid);
			res = 1;
			break;
		case PCB_HID_MOUSE_RELEASE:
			if (drag_lid >= 0) {
				res = 1;
				drag_lid = -1;
				lvalid = gvalid = 0;
			}
			break;
		case PCB_HID_MOUSE_MOTION:
			cx = x;
			cy = y;
			break;
	}
	return res;
}




static const char pcb_acts_dump_csect[] = "DumpCsect()";
static const char pcb_acth_dump_csect[] = "Print the cross-section of the board (layer stack)";

static int pcb_act_dump_csect(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_layergrp_id_t gid;

	for(gid = 0; gid < pcb_max_group; gid++) {
		int i;
		const char *type_gfx;
		pcb_layer_group_t *g = PCB->LayerGroups.grp + gid;

		if (!g->valid) {
			if (g->len <= 0)
				continue;
			type_gfx = "old";
		}
		else if (g->type & PCB_LYT_VIRTUAL) continue;
		else if (g->type & PCB_LYT_COPPER) type_gfx = "====";
		else if (g->type & PCB_LYT_SUBSTRATE) type_gfx = "xxxx";
		else if (g->type & PCB_LYT_SILK) type_gfx = "silk";
		else if (g->type & PCB_LYT_MASK) type_gfx = "mask";
		else if (g->type & PCB_LYT_PASTE) type_gfx = "pst.";
		else if (g->type & PCB_LYT_MISC) type_gfx = "misc";
		else if (g->type & PCB_LYT_OUTLINE) type_gfx = "||||";
		else type_gfx = "????";

		printf("%s {%ld} %s\n", type_gfx, gid, g->name);
		for(i = 0; i < g->len; i++) {
			pcb_layer_id_t lid = g->lid[i];
			pcb_layer_t *l = &PCB->Data->Layer[lid];
			printf("      [%ld] %s\n", lid, l->Name);
			if (l->grp != gid)
				printf("         *** broken layer-to-group cross reference: %d\n", l->grp);
		}
	}
	return 0;
}

static const char *draw_csect_cookie = "draw_csect";

pcb_hid_action_t draw_csect_action_list[] = {
	{"DumpCsect", 0, pcb_act_dump_csect,
	 pcb_acth_dump_csect, pcb_acts_dump_csect}
};


PCB_REGISTER_ACTIONS(draw_csect_action_list, draw_csect_cookie)

static void hid_draw_csect_uninit(void)
{
	pcb_hid_remove_actions_by_cookie(draw_csect_cookie);
}

#include "dolists.h"

pcb_uninit_t hid_draw_csect_init(void)
{
	PCB_REGISTER_ACTIONS(draw_csect_action_list, draw_csect_cookie)

	pcb_stub_draw_csect = draw_csect;
	pcb_stub_draw_csect_mouse_ev = mouse_csect;
	pcb_stub_draw_csect_overlay = draw_csect_overlay;

	return hid_draw_csect_uninit;
}
