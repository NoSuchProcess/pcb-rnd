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

/* Draw a text at x;y sized scale percentage */
static void dtext(int x, int y, int scale, int dir, const char *txt)
{
	pcb_text_t t;

	t.X = PCB_MM_TO_COORD(x);
	t.Y = PCB_MM_TO_COORD(y);
	t.TextString = txt;
	t.Direction = dir;
	t.Scale = scale;
	t.Flags = pcb_no_flags();
	DrawTextLowLevel(&t, 0);
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

/* draw a hatched rectangle; to turn off hatching in a directon set the
   corresponding step to 0 */
static void dhrect(int x1, int y1, int x2, int y2, float thick_rect, float thick_hatch, int step_fwd, int step_back)
{

	dline(x1, y1, x2, y1, thick_rect);
	dline(x2, y1, x2, y2, thick_rect);
	dline(x1, y2, x2, y2, thick_rect);
	dline(x1, y1, x1, y2, thick_rect);

	if (step_fwd > 0)
		hatch_box(x1, y1, x2, y2, thick_hatch, +step_fwd);

	if (step_back > 0)
		hatch_box(x1, y1, x2, y2, thick_hatch, -step_back);
}


/* Draw the cross-section layer */
static void draw_csect(pcb_hid_gc_t gc)
{
	pcb_gui->set_color(gc, PCB->ElementColor);

	dtext(0, 0, 500, 0, "Board cross section");
	dhrect(0, 20, 100, 40,  1, 0.5,  5, 5);
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
	return hid_draw_csect_uninit;
}
