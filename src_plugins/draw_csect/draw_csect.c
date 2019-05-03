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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

/* draw cross section */
#include "config.h"


#include "board.h"
#include "data.h"
#include "draw.h"
#include "plugins.h"
#include "stub_draw.h"
#include "compat_misc.h"
#include "actions.h"
#include "event.h"
#include "layer_vis.h"

#include "obj_text_draw.h"
#include "obj_line_draw.h"

extern pcb_layergrp_id_t pcb_actd_EditGroup_gid;

static const char *COLOR_ANNOT_ = "#000000";
static const char *COLOR_BG_ = "#f0f0f0";

static const char *COLOR_COPPER_ = "#C05020";
static const char *COLOR_SUBSTRATE_ = "#E0D090";
static const char *COLOR_SILK_ = "#000000";
static const char *COLOR_MASK_ = "#30d030";
static const char *COLOR_PASTE_ = "#60e0e0";
static const char *COLOR_MISC_ = "#e0e000";
static const char *COLOR_OUTLINE_ = "#000000";


static pcb_color_t
	COLOR_ANNOT, COLOR_BG, COLOR_COPPER, COLOR_SUBSTRATE, COLOR_SILK, COLOR_MASK,
	COLOR_PASTE, COLOR_MISC, COLOR_OUTLINE;

static pcb_layer_id_t drag_lid = -1;
static pcb_layergrp_id_t drag_gid = -1, drag_gid_subst = -1;

#define GROUP_WIDTH_MM 75

/* Draw a text at x;y sized scale percentage */
static pcb_text_t *dtext(int x, int y, int scale, int dir, const char *txt)
{
	static pcb_text_t t;

	memset(&t, 0, sizeof(t));

	t.X = PCB_MM_TO_COORD(x);
	t.Y = PCB_MM_TO_COORD(y);
	t.TextString = (char *)txt;
	t.rot = 90.0*dir;
	t.Scale = scale;
	t.fid = 0; /* use the default font */
	t.Flags = pcb_no_flags();
	pcb_text_draw_(NULL, &t, 0, 0, PCB_TXT_TINY_ACCURATE);
	return &t;
}

/* Draw a text at x;y sized scale percentage */
static pcb_text_t *dtext_(pcb_coord_t x, pcb_coord_t y, int scale, int dir, const char *txt, pcb_coord_t th)
{
	static pcb_text_t t;

	memset(&t, 0, sizeof(t));

	t.X = x;
	t.Y = y;
	t.TextString = (char *)txt;
	t.rot = 90.0*dir;
	t.Scale = scale;
	t.fid = 0; /* use the default font */
	t.Flags = pcb_no_flags();
	pcb_text_draw_(NULL, &t, th, 0, PCB_TXT_TINY_ACCURATE);
	return &t;
}

/* Draw a text at x;y with a background */
static pcb_text_t *dtext_bg(pcb_hid_gc_t gc, int x, int y, int scale, int dir, const char *txt, const pcb_color_t *bgcolor, const pcb_color_t *fgcolor)
{
	static pcb_text_t t;

	memset(&t, 0, sizeof(t));

	t.X = PCB_MM_TO_COORD(x);
	t.Y = PCB_MM_TO_COORD(y);
	t.TextString = (char *)txt;
	t.rot = 90.0 * dir;
	t.Scale = scale;
	t.fid = 0; /* use the default font */
	t.Flags = pcb_no_flags();

	if (pcb_gui->gui) { /* it is unreadable on black&white and on most exporters */
		pcb_gui->set_color(gc, bgcolor);
		pcb_text_draw_(NULL, &t, 1000000, 0, PCB_TXT_TINY_ACCURATE);
	}

	pcb_gui->set_color(gc, fgcolor);
	pcb_text_draw_(NULL, &t, 0, 0, PCB_TXT_TINY_ACCURATE);

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
	if (l.Thickness == 0)
		l.Thickness = 1;
	pcb_line_draw_(NULL, &l, 0);
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
	if (l.Thickness == 0)
		l.Thickness = 1;
	pcb_line_draw_(NULL, &l, 0);
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
	OMIT_RIGHT = 8,
	OMIT_ALL = 1|2|4|8
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

static pcb_box_t btn_addgrp, btn_delgrp, btn_addlayer, btn_dellayer, btn_addoutline;
static pcb_box_t layer_crd[PCB_MAX_LAYER];
static pcb_box_t group_crd[PCB_MAX_LAYERGRP];
static pcb_box_t outline_crd;
static char layer_valid[PCB_MAX_LAYER];
static char group_valid[PCB_MAX_LAYERGRP];
static char outline_valid;

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

static void reg_outline_coords(pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	outline_crd.X1 = x1;
	outline_crd.Y1 = y1;
	outline_crd.X2 = x2;
	outline_crd.Y2 = y2;
	outline_valid = 1;
}

static void reset_layer_coords(void)
{
	memset(layer_valid, 0, sizeof(layer_valid));
	memset(group_valid, 0, sizeof(group_valid));
	outline_valid = 0;
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

static pcb_coord_t create_button(pcb_hid_gc_t gc, int x, int y, const char *label, pcb_box_t *box)
{
	pcb_text_t *t;
	t = dtext_bg(gc, x, y, 200, 0, label, &COLOR_BG, &COLOR_ANNOT);
	pcb_text_bbox(pcb_font(PCB, 0, 1), t);
	dhrect(PCB_COORD_TO_MM(t->BoundingBox.X1), y, PCB_COORD_TO_MM(t->BoundingBox.X2)+1, y+4, 0.25, 0, 0, 0, OMIT_NONE);
	box->X1 = t->BoundingBox.X1;
	box->Y1 = PCB_MM_TO_COORD(y);
	box->X2 = t->BoundingBox.X2+PCB_MM_TO_COORD(1);
	box->Y2 = PCB_MM_TO_COORD(y+4);
	return PCB_COORD_TO_MM(box->X2);
}

static int is_button(int x, int y, const pcb_box_t *box)
{
	return (x >= box->X1) && (x <= box->X2) && (y >= box->Y1) && (y <= box->Y2);
}

static pcb_hid_gc_t csect_gc;

static pcb_coord_t ox, oy, cx, cy;
static int drag_addgrp, drag_delgrp, drag_addlayer, drag_dellayer, drag_addoutline;
static pcb_layergrp_id_t gactive = -1;
static pcb_layergrp_id_t outline_gactive = -1;
static pcb_layer_id_t lactive = -1;
int lactive_idx = -1;

/* true if we are dragging somehting */
#define DRAGGING ((drag_lid >= 0) || (drag_addgrp | drag_delgrp | drag_addlayer | drag_dellayer) || (drag_gid >= 0))

typedef enum {
	MARK_GRP_FRAME,
	MARK_GRP_MIDDLE,
	MARK_GRP_TOP
} mark_grp_loc_t;

static void mark_grp(pcb_coord_t y, unsigned int accept_mask, mark_grp_loc_t loc)
{
	pcb_coord_t y1, y2, x0 = -PCB_MM_TO_COORD(5);
	pcb_layergrp_id_t g;

	g = get_group_coords(y, &y1, &y2);

	if ((g >= 0) && ((pcb_layergrp_flags(PCB, g) & accept_mask) & accept_mask)) {
		gactive = g;
		switch(loc) {
			case MARK_GRP_FRAME:
				dline_(x0, y1, PCB_MM_TO_COORD(200), y1, 0.1);
				dline_(x0, y2, PCB_MM_TO_COORD(200), y2, 0.1);
				break;
			case MARK_GRP_MIDDLE:
				dline_(x0, (y1+y2)/2, PCB_MM_TO_COORD(200), (y1+y2)/2, 0.1);
				break;
			case MARK_GRP_TOP:
				dline_(x0, y1, PCB_MM_TO_COORD(200), y1, 0.1);
				break;
		}
	}
	else
		gactive = -1;
}

static void mark_outline_grp(pcb_coord_t x, pcb_coord_t y, pcb_layergrp_id_t gid)
{
	if (outline_valid &&
			(outline_crd.X1 <= x) && (outline_crd.Y1 <= y) &&
			(outline_crd.X2 >= x) && (outline_crd.Y2 >= y)) {
		outline_gactive = gid;
	}
	else
		outline_gactive = -1;
}

static void mark_layer(pcb_coord_t x, pcb_coord_t y)
{
	lactive = get_layer_coords(x, y);
	if (lactive >= 0) {
		dhrect(
			PCB_COORD_TO_MM(layer_crd[lactive].X1)-1, PCB_COORD_TO_MM(layer_crd[lactive].Y1)-1,
			PCB_COORD_TO_MM(layer_crd[lactive].X2)+1, PCB_COORD_TO_MM(layer_crd[lactive].Y2)+1,
			0.1, 0.1, 3, 3, 0
		);
	}
}

static void mark_layer_order(pcb_coord_t x)
{
	pcb_layergrp_t *g;
	pcb_coord_t tx, ty1, ty2;
	lactive_idx = -1;

	if ((gactive < 0) || (PCB->LayerGroups.grp[gactive].len < 1))
		return;

	g = &PCB->LayerGroups.grp[gactive];

	tx = layer_crd[g->lid[0]].X1 - PCB_MM_TO_COORD(1.5);
	ty1 = layer_crd[g->lid[0]].Y1;
	ty2 = layer_crd[g->lid[0]].Y2;

	for(lactive_idx = g->len-1; lactive_idx >= 0; lactive_idx--) {
		pcb_layer_id_t lid = g->lid[lactive_idx];
		if (x > (layer_crd[lid].X1 + layer_crd[lid].X2)/2) {
			tx = layer_crd[lid].X2;
			break;
		}
	}
	dline_(tx, ty1-PCB_MM_TO_COORD(1.5), tx-PCB_MM_TO_COORD(0.75), ty2+PCB_MM_TO_COORD(1.5), 0.1);
	dline_(tx, ty1-PCB_MM_TO_COORD(1.5), tx+PCB_MM_TO_COORD(0.75), ty2+PCB_MM_TO_COORD(1.5), 0.1);
	lactive_idx++;
}

static void draw_hover_label(const char *str)
{
	int x0 = PCB_MM_TO_COORD(2.5); /* compensate for the mouse cursor (sort of random) */
	dtext_(cx+x0, cy, 250, 0, str, PCB_MM_TO_COORD(0.01));
}

/* Draw the cross-section layer */
static void draw_csect(pcb_hid_gc_t gc, const pcb_hid_expose_ctx_t *e)
{
	pcb_layergrp_id_t gid, outline_gid = -1;
	int ystart = 10, x, y, last_copper_step = 5;

	reset_layer_coords();
	csect_gc = gc;

	pcb_gui->set_color(gc, &COLOR_ANNOT);
	dtext(0, 0, 500, 0, "Board cross section");

	/* draw physical layers */
	y = ystart;
	for(gid = 0; gid < pcb_max_group(PCB); gid++) {
		int i, stepf = 0, stepb = 0, th;
		pcb_layergrp_t *g = PCB->LayerGroups.grp + gid;
		const pcb_color_t *color;

		if ((!g->valid) || (gid == drag_gid)  || (gid == drag_gid_subst))
			continue;
		else if (g->ltype & PCB_LYT_COPPER) {
			last_copper_step = -last_copper_step;
			if (last_copper_step > 0)
				stepf = last_copper_step;
			else
				stepb = -last_copper_step;
			th = 5;
			color = &COLOR_COPPER;
		}
		else if (g->ltype & PCB_LYT_SUBSTRATE) {
			stepf = stepb = 7;
			th = 10;
			color = &COLOR_SUBSTRATE;
		}
		else if (g->ltype & PCB_LYT_SILK) {
			th = 5;
			color = &COLOR_SILK;
			stepb = 3;
		}
		else if (g->ltype & PCB_LYT_MASK) {
			th = 5;
			color = &COLOR_MASK;
			stepb = 9;
		}
		else if (g->ltype & PCB_LYT_PASTE) {
			th = 5;
			color = &COLOR_PASTE;
			stepf = 9;
		}
		else if (g->ltype & PCB_LYT_MISC) {
			th = 5;
			color = &COLOR_MISC;
			stepf = 3;
		}
		else if (g->ltype & PCB_LYT_BOUNDARY) {
TODO("layer: handle multiple outline layers")
			outline_gid = gid;
			continue;
		}
		else
			continue;

		pcb_gui->set_color(gc, color);
		dhrect(0, y, GROUP_WIDTH_MM, y+th,  1, 0.5,  stepf, stepb, OMIT_LEFT | OMIT_RIGHT);
		dtext_bg(gc, 5, y, 200, 0, g->name, &COLOR_BG, &COLOR_ANNOT);
		reg_group_coords(gid, PCB_MM_TO_COORD(y), PCB_MM_TO_COORD(y+th));


		/* draw layer names */
		{
			x = GROUP_WIDTH_MM + 3;
			for(i = 0; i < g->len; i++) {
				pcb_text_t *t;
				pcb_layer_id_t lid = g->lid[i];
				pcb_layer_t *l = &PCB->Data->Layer[lid];
				int redraw_text = 0;

				if (lid == drag_lid)
					continue;
				t = dtext_bg(gc, x, y, 200, 0, l->name, &COLOR_BG, &l->meta.real.color);
				pcb_text_bbox(pcb_font(PCB, 0, 1), t);
				if (l->comb & PCB_LYC_SUB) {
					dhrect(PCB_COORD_TO_MM(t->BoundingBox.X1), y, PCB_COORD_TO_MM(t->BoundingBox.X2)+1, y+4, 1.2, 0, 0, 0, OMIT_NONE);
					redraw_text = 1;
				}

				if (redraw_text)
					t = dtext_bg(gc, x, y, 200, 0, l->name, &COLOR_BG, &l->meta.real.color);
				else
					dhrect(PCB_COORD_TO_MM(t->BoundingBox.X1), y, PCB_COORD_TO_MM(t->BoundingBox.X2)+1, y+4, 0.25, 0, 0, 0, OMIT_NONE);

				if (l->comb & PCB_LYC_AUTO)
					dhrect(PCB_COORD_TO_MM(t->BoundingBox.X1), y, PCB_COORD_TO_MM(t->BoundingBox.X2)+1, y+4, 0.0, 0, 3, 0, OMIT_ALL);

				reg_layer_coords(lid, t->BoundingBox.X1, PCB_MM_TO_COORD(y), t->BoundingBox.X2+PCB_MM_TO_COORD(1), PCB_MM_TO_COORD(y+4));
				x += PCB_COORD_TO_MM(t->BoundingBox.X2 - t->BoundingBox.X1) + 3;
			}
		}

		/* increment y */
		y += th + 1;
	}

	/* draw global/special layers */
	y+=7;
	if (outline_gid >= 0) {
		pcb_layergrp_t *g = PCB->LayerGroups.grp + outline_gid;
		pcb_text_t *t;
		if (g->len > 0) {
			pcb_layer_t *l = &PCB->Data->Layer[g->lid[0]];
			pcb_gui->set_color(gc, &l->meta.real.color);
			t = dtext_bg(gc, 1, y, 200, 0, l->name, &COLOR_BG, &l->meta.real.color);
			pcb_text_bbox(pcb_font(PCB, 0, 1), t);
			dhrect(PCB_COORD_TO_MM(t->BoundingBox.X1), y, PCB_COORD_TO_MM(t->BoundingBox.X2)+1, y+4, 1, 0, 0, 0, OMIT_NONE);
			dtext_bg(gc, 1, y, 200, 0, l->name, &COLOR_BG, &l->meta.real.color);
			reg_layer_coords(g->lid[0], t->BoundingBox.X1, PCB_MM_TO_COORD(y), t->BoundingBox.X2+PCB_MM_TO_COORD(1), PCB_MM_TO_COORD(y+4));
		}
		else {
			pcb_gui->set_color(gc, &COLOR_OUTLINE);
			t = dtext_bg(gc, 1, y, 200, 0, "<outline>", &COLOR_BG, &COLOR_OUTLINE);
			pcb_text_bbox(pcb_font(PCB, 0, 1), t);
			dhrect(PCB_COORD_TO_MM(t->BoundingBox.X1), y, PCB_COORD_TO_MM(t->BoundingBox.X2)+1, y+4, 1, 0, 0, 0, OMIT_NONE);
			dtext_bg(gc, 1, y, 200, 0, "<outline>", &COLOR_BG, &COLOR_OUTLINE);
		}
		dline(0, ystart, 0, y+4, 1);
		reg_outline_coords(t->BoundingBox.X1, PCB_MM_TO_COORD(y), t->BoundingBox.X2, PCB_MM_TO_COORD(y+4));
	}
	else {
		create_button(gc, 2, y, "Create outline", &btn_addoutline);
	}

	x=30;
	x = 2 + create_button(gc, x, y, "Add copper group", &btn_addgrp);
	x = 2 + create_button(gc, x, y, "Del copper group", &btn_delgrp);

	x += 2;
	x = 2 + create_button(gc, x, y, "Add logical layer", &btn_addlayer);
	x = 2 + create_button(gc, x, y, "Del logical layer", &btn_dellayer);


	if (DRAGGING) {
		pcb_gui->set_color(gc, pcb_color_black);

		/* draw the actual operation */
		if (drag_addgrp) {
			mark_grp(cy, PCB_LYT_SUBSTRATE, MARK_GRP_MIDDLE);
			draw_hover_label("INSERT GRP");
		}
		if (drag_delgrp) {
			mark_grp(cy, PCB_LYT_COPPER | PCB_LYT_INTERN, MARK_GRP_FRAME);
			draw_hover_label("DEL GROUP");
		}
		if (drag_addlayer) {
			mark_grp(cy, PCB_LYT_COPPER | PCB_LYT_MASK | PCB_LYT_PASTE | PCB_LYT_SILK, MARK_GRP_FRAME);
			mark_outline_grp(cx, cy, outline_gid);
			draw_hover_label("ADD LAYER");
		}
		if (drag_dellayer) {
			mark_layer(cx, cy);
			draw_hover_label("DEL LAYER");
		}
		else if (drag_lid >= 0) {
			pcb_layer_t *l = &PCB->Data->Layer[drag_lid];
			draw_hover_label(l->name);
			mark_grp(cy, PCB_LYT_COPPER | PCB_LYT_MASK | PCB_LYT_PASTE | PCB_LYT_SILK, MARK_GRP_FRAME);
			mark_outline_grp(cx, cy, outline_gid);
			mark_layer_order(cx);
		}
		else if (drag_gid >= 0) {
			pcb_layergrp_t *g = &PCB->LayerGroups.grp[drag_gid];
			const char *name = g->name == NULL ? "<unnamed group>" : g->name;
			draw_hover_label(name);
			mark_grp(cy, PCB_LYT_COPPER | PCB_LYT_INTERN, MARK_GRP_TOP);
			if (gactive < 0)
				mark_grp(cy, PCB_LYT_COPPER | PCB_LYT_BOTTOM, MARK_GRP_TOP);
			if (gactive < 0)
				mark_grp(cy, PCB_LYT_SUBSTRATE, MARK_GRP_TOP);
		}
	}
}

/* Returns 0 if gactive can be removed from its current group */
static int check_layer_del(pcb_layer_id_t lid)
{
	pcb_layergrp_t *grp;
	unsigned int tflg;

	tflg = pcb_layer_flags(PCB, lid);
	grp = pcb_get_layergrp(PCB, pcb_layer_get_group(PCB, lid));

	if (grp == NULL) {
		pcb_message(PCB_MSG_ERROR, "Invalid source group.\n");
		return -1;
	}

	if ((tflg & PCB_LYT_SILK) && (grp->len == 1)) {
		pcb_message(PCB_MSG_ERROR, "Can not remove the last layer of this group because this group must have at least one layer.\n");
		return -1;
	}

	return 0;
}

static void do_move_grp()
{
	unsigned int tflg;

	if ((gactive < 0) || (gactive == drag_gid+1))
		return;

	tflg = pcb_layergrp_flags(PCB, gactive);

	pcb_layergrp_move(PCB, drag_gid, gactive);

	if (drag_gid_subst >= 0) {
		if ((drag_gid < drag_gid_subst) && (gactive > drag_gid))
			drag_gid_subst--; /* the move shifted this down one slot */

		if (gactive < drag_gid_subst)
			drag_gid_subst++; /* the move shifted this up one slot */

		if (tflg & PCB_LYT_COPPER) {
			if (tflg & PCB_LYT_BOTTOM)
				pcb_layergrp_move(PCB, drag_gid_subst, gactive);
			else
				pcb_layergrp_move(PCB, drag_gid_subst, gactive+1);
		}
		else if (tflg & PCB_LYT_SUBSTRATE) {
			if (gactive < drag_gid)
				pcb_layergrp_move(PCB, drag_gid_subst, gactive);
			else
				pcb_layergrp_move(PCB, drag_gid_subst, gactive-1);
		}
	}
}


static pcb_bool mouse_csect(pcb_hid_mouse_ev_t kind, pcb_coord_t x, pcb_coord_t y)
{
	pcb_bool res = 0;
	pcb_layer_id_t lid;

	switch(kind) {
		case PCB_HID_MOUSE_PRESS:
			if (is_button(x, y, &btn_addoutline)) {
				drag_addoutline = 1;
				res = 1;
				break;
			}
			
			if (is_button(x, y, &btn_addgrp)) {
				drag_addgrp = 1;
				res = 1;
				break;
			}

			if (is_button(x, y, &btn_delgrp)) {
				drag_delgrp = 1;
				res = 1;
				break;
			}

			if (is_button(x, y, &btn_addlayer)) {
				drag_addlayer = 1;
				res = 1;
				break;
			}

			if (is_button(x, y, &btn_dellayer)) {
				drag_dellayer = 1;
				res = 1;
				break;
			}

			drag_lid = get_layer_coords(x, y);
			if (drag_lid >= 0) {
				ox = x;
				oy = y;
				res = 1;
				break;
			}
			
			if ((x > 0) && (x < PCB_MM_TO_COORD(GROUP_WIDTH_MM))) {
				pcb_coord_t tmp;
				pcb_layergrp_id_t gid;
				gid = get_group_coords(y, &tmp, &tmp);
				if ((gid >= 0) && (pcb_layergrp_flags(PCB, gid) & PCB_LYT_COPPER) && (pcb_layergrp_flags(PCB, gid) & PCB_LYT_INTERN)) {
					drag_gid = gid;
					/* temporary workaround for the restricted setup */
					if (pcb_layergrp_flags(PCB, gid - 1) & PCB_LYT_SUBSTRATE)
						drag_gid_subst = gid - 1;
					else if ((pcb_layergrp_flags(PCB, gid - 1) & PCB_LYT_BOUNDARY) && (pcb_layergrp_flags(PCB, gid - 2) & PCB_LYT_SUBSTRATE))
						drag_gid_subst = gid - 2;
					res = 1;
				}
			}
			break;
		case PCB_HID_MOUSE_RELEASE:
			if (drag_addoutline) {
				if (is_button(x, y, &btn_addoutline)) {
					pcb_layergrp_t *g = pcb_get_grp_new_misc(PCB);
					g->name = pcb_strdup("global_outline");
					g->ltype = PCB_LYT_BOUNDARY;
					g->purpose = pcb_strdup("uroute");
					g->valid = 1;
					g->open = 1;

					outline_gactive = pcb_layergrp_id(PCB, g);
					pcb_layer_create(PCB, outline_gactive, "outline");
					pcb_event(&PCB->hidlib, PCB_EVENT_LAYERS_CHANGED, NULL);
				}
				drag_addoutline = 0;
				gactive = -1;
				res = 1;
			}
			else if (drag_addgrp) {
				if (gactive >= 0) {
					pcb_layergrp_t *g;
					g = pcb_layergrp_insert_after(PCB, gactive);
					g->name = NULL;
					g->ltype = PCB_LYT_INTERN | PCB_LYT_SUBSTRATE;
					g->valid = 1;
					g = pcb_layergrp_insert_after(PCB, gactive);
					g->name = pcb_strdup("Intern");
					g->ltype = PCB_LYT_INTERN | PCB_LYT_COPPER;
					g->valid = 1;
				}
				drag_addgrp = 0;
				gactive = -1;
				res = 1;
			}
			else if (drag_delgrp) {
				if (gactive >= 0) {
					pcb_layergrp_del(PCB, gactive, 1);
					if (pcb_layergrp_flags(PCB, gactive) & PCB_LYT_SUBSTRATE)
						pcb_layergrp_del(PCB, gactive, 1);
					else if (pcb_layergrp_flags(PCB, gactive-1) & PCB_LYT_SUBSTRATE)
						pcb_layergrp_del(PCB, gactive-1, 1);
				}
				drag_delgrp = 0;
				gactive = -1;
				res = 1;
			}
			else if (drag_addlayer) {
				if (gactive >= 0) {
					pcb_layer_create(PCB, gactive, "New Layer");
					pcb_event(&PCB->hidlib, PCB_EVENT_LAYERS_CHANGED, NULL);
				}
				else if (outline_gactive >= 0 && PCB->LayerGroups.grp[outline_gactive].len == 0) {
					pcb_layer_create(PCB, outline_gactive, "outline");
					pcb_event(&PCB->hidlib, PCB_EVENT_LAYERS_CHANGED, NULL);
				}
				drag_addlayer = 0;
				gactive = -1;
				res = 1;
			}
			else if (drag_dellayer) {
				if (lactive >= 0) {
					char tmp[32];
					sprintf(tmp, "%ld", lactive);
					pcb_actionl("MoveLayer", tmp, "-1", NULL);
				}
				drag_dellayer = 0;
				lactive = -1;
				res = 1;
			}
			else if (drag_lid >= 0) {
				if (gactive >= 0) {
					pcb_layer_t *l = &PCB->Data->Layer[drag_lid];
					pcb_layergrp_t *g;
					if (l->meta.real.grp == gactive) { /* move within the group */
						int d, s, at;
						g = &PCB->LayerGroups.grp[gactive];

						/* move drag_lid to the end of the list within the group */
						for(s = d = 0; s < g->len; s++) {
							if (g->lid[s] != drag_lid) {
								g->lid[d] = g->lid[s];
								d++;
							}
							else
								at = s;
						}
						g->lid[g->len-1] = drag_lid;
						if (lactive_idx >= at) /* because lactive_idx was calculated with this layer in the middle */
							lactive_idx--;
						goto move_layer_to_its_place;
					}
					else if (check_layer_del(drag_lid) == 0) {
						g = &PCB->LayerGroups.grp[gactive];
						pcb_layer_move_to_group(PCB, drag_lid, gactive);
						pcb_message(PCB_MSG_INFO, "moved layer %s to group %d\n", l->name, gactive);
						move_layer_to_its_place:;
						if (lactive_idx < g->len-1) {
							memmove(g->lid + lactive_idx + 1, g->lid + lactive_idx, (g->len - 1 - lactive_idx) * sizeof(pcb_layer_id_t));
							g->lid[lactive_idx] = drag_lid;
						}
						pcb_event(&PCB->hidlib, PCB_EVENT_LAYERS_CHANGED, NULL);
					}
				}
				else if (outline_gactive >= 0 && PCB->LayerGroups.grp[outline_gactive].len == 0) {
					pcb_layer_t *l = &PCB->Data->Layer[drag_lid];
					pcb_layer_move_to_group(PCB, drag_lid, outline_gactive);
					pcb_message(PCB_MSG_INFO, "moved layer %s to group %d\n", l->name, outline_gactive);
				}
				else
					pcb_message(PCB_MSG_ERROR, "Can not move layer into that layer group\n");
				res = 1;
				drag_lid = -1;
				gactive = -1;
			}
			else if (drag_gid > 0) {
				do_move_grp();
				res = 1;
				drag_gid = -1;
				drag_gid_subst = -1;
			}
			break;
		case PCB_HID_MOUSE_MOTION:
			cx = x;
			cy = y;
			res = DRAGGING;
			break;
		case PCB_HID_MOUSE_POPUP:
			lid = get_layer_coords(x, y);
			if (lid >= 0) {
				pcb_layervis_change_group_vis(lid, 1, 1);
				pcb_actionl("Popup", "layer", NULL);
			}
			else if ((x > 0) && (x < PCB_MM_TO_COORD(GROUP_WIDTH_MM))) {
				pcb_coord_t tmp;
				pcb_actd_EditGroup_gid = get_group_coords(y, &tmp, &tmp);
				if (pcb_actd_EditGroup_gid >= 0)
					pcb_actionl("Popup", "group", NULL);
			}
			break;

	}
	return res;
}




static const char pcb_acts_dump_csect[] = "DumpCsect()";
static const char pcb_acth_dump_csect[] = "Print the cross-section of the board (layer stack)";

static fgw_error_t pcb_act_dump_csect(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_layergrp_id_t gid;

	for(gid = 0; gid < pcb_max_group(PCB); gid++) {
		int i;
		const char *type_gfx;
		pcb_layergrp_t *g = PCB->LayerGroups.grp + gid;

		if (!g->valid) {
			if (g->len <= 0)
				continue;
			type_gfx = "old";
		}
		else if (g->ltype & PCB_LYT_VIRTUAL) continue;
		else if (g->ltype & PCB_LYT_COPPER) type_gfx = "====";
		else if (g->ltype & PCB_LYT_SUBSTRATE) type_gfx = "xxxx";
		else if (g->ltype & PCB_LYT_SILK) type_gfx = "silk";
		else if (g->ltype & PCB_LYT_MASK) type_gfx = "mask";
		else if (g->ltype & PCB_LYT_PASTE) type_gfx = "pst.";
		else if (g->ltype & PCB_LYT_MISC) type_gfx = "misc";
		else if (g->ltype & PCB_LYT_MECH) type_gfx = "mech";
		else if (g->ltype & PCB_LYT_DOC) type_gfx = "doc.";
		else if (g->ltype & PCB_LYT_BOUNDARY) type_gfx = "||||";
		else type_gfx = "????";

		printf("%s {%ld} %s\n", type_gfx, gid, g->name);
		for(i = 0; i < g->len; i++) {
			pcb_layer_id_t lid = g->lid[i];
			pcb_layer_t *l = &PCB->Data->Layer[lid];
			printf("      [%ld] %s comb=", lid, l->name);
			if (l->comb & PCB_LYC_SUB) printf(" sub");
			if (l->comb & PCB_LYC_AUTO) printf(" auto");
			printf("\n");
			if (l->meta.real.grp != gid)
				printf("         *** broken layer-to-group cross reference: %ld\n", l->meta.real.grp);
		}
	}
	PCB_ACT_IRES(0);
	return 0;
}

static const char *draw_csect_cookie = "draw_csect";

pcb_action_t draw_csect_action_list[] = {
	{"DumpCsect", pcb_act_dump_csect, pcb_acth_dump_csect, pcb_acts_dump_csect}
};


PCB_REGISTER_ACTIONS(draw_csect_action_list, draw_csect_cookie)

int pplg_check_ver_draw_csect(int ver_needed) { return 0; }

void pplg_uninit_draw_csect(void)
{
	pcb_remove_actions_by_cookie(draw_csect_cookie);
}

#include "dolists.h"

int pplg_init_draw_csect(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(draw_csect_action_list, draw_csect_cookie)

	pcb_color_load_str(&COLOR_ANNOT,     COLOR_ANNOT_);
	pcb_color_load_str(&COLOR_BG,        COLOR_BG_);
	pcb_color_load_str(&COLOR_COPPER,    COLOR_COPPER_);
	pcb_color_load_str(&COLOR_SUBSTRATE, COLOR_SUBSTRATE_);
	pcb_color_load_str(&COLOR_SILK,      COLOR_SILK_);
	pcb_color_load_str(&COLOR_MASK,      COLOR_MASK_);
	pcb_color_load_str(&COLOR_PASTE,     COLOR_PASTE_);
	pcb_color_load_str(&COLOR_MISC,      COLOR_MISC_);
	pcb_color_load_str(&COLOR_OUTLINE,   COLOR_OUTLINE_);

	pcb_stub_draw_csect = draw_csect;
	pcb_stub_draw_csect_mouse_ev = mouse_csect;

	return 0;
}
