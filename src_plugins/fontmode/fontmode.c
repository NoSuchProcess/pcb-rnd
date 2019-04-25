/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2006 DJ Delorie
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
 *
 *
 *  Old contact info:
 *  DJ Delorie, 334 North Road, Deerfield NH 03037-1110, USA
 *  dj@delorie.com
 *
 */

#include "config.h"
#include "conf_core.h"

#include "board.h"

#include <math.h>
#include <memory.h>
#include <limits.h>
#include <assert.h>

#include "data.h"
#include "draw.h"
#include "flag.h"
#include "layer.h"
#include "move.h"
#include "remove.h"
#include "rtree.h"
#include "flag_str.h"
#include "undo.h"
#include "pcb-printf.h"
#include "plugins.h"
#include "actions.h"
#include "compat_misc.h"
#include "event.h"
#include "polygon.h"
#include "obj_poly_draw.h"

/* FIXME - we currently hardcode the grid and PCB size.  What we
   should do in the future is scan the font for its extents, and size
   the grid appropriately.  Also, when we convert back to a font, we
   should search the grid for the gridlines and use them to figure out
   where the symbols are. */

#define CELL_SIZE   ((pcb_coord_t)(PCB_MIL_TO_COORD (100)))

#define XYtoSym(x,y) (((x) / CELL_SIZE - 1)  +  (16 * ((y) / CELL_SIZE - 1)))

static const char pcb_acts_fontedit[] = "FontEdit()";
static const char pcb_acth_fontedit[] = "Convert the current font to a PCB for editing.";
static pcb_layer_t *make_layer(pcb_layergrp_id_t grp, const char *lname)
{
	pcb_layer_id_t lid;

	assert(grp >= 0);
	lid = pcb_layer_create(PCB, grp, lname);
	assert(lid >= 0);
	PCB->Data->Layer[lid].meta.real.vis = 1;
	PCB->Data->Layer[lid].meta.real.color = *pcb_layer_default_color(lid, 0);
	return &PCB->Data->Layer[lid];
}

static void add_poly(pcb_layer_t *layer, pcb_poly_t *poly, pcb_coord_t ox, pcb_coord_t oy)
{
	pcb_poly_t *np;
	
	/* alloc */
	np = pcb_poly_new(layer, 0, pcb_no_flags());
	pcb_poly_copy(np, poly, ox, oy);

	/* add */
	pcb_add_poly_on_layer(layer, np);
	pcb_poly_init_clip(PCB->Data, layer, np);
	pcb_poly_invalidate_draw(layer, np);
}

static fgw_error_t pcb_act_FontEdit(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_font_t *font;
	pcb_symbol_t *symbol;
	pcb_layer_t *lfont, *lorig, *lwidth, *lgrid, *lsilk;
	pcb_layergrp_id_t grp[4];
	pcb_poly_t *poly;
	pcb_arc_t *arc, *newarc;
	int s, l;

	font = pcb_font_unlink(PCB, conf_core.design.text_font_id);
	if (font == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't fetch font id %d\n", conf_core.design.text_font_id);
		return 1;
	}

	if (pcb_actionl("New", "Font", 0))
		return 1;

	conf_set(CFR_DESIGN, "editor/grid_unit", -1, "mil", POL_OVERWRITE);
	conf_set_design("design/min_wid", "%s", "1");
	conf_set_design("design/min_slk", "%s", "1");
	conf_set_design("design/text_font_id", "%s", "0");


	PCB->hidlib.size_x = CELL_SIZE * 18;
	PCB->hidlib.size_y = CELL_SIZE * ((PCB_MAX_FONTPOSITION + 15) / 16 + 2);
	PCB->hidlib.grid = PCB_MIL_TO_COORD(5);

	/* create the layer stack and logical layers */
	pcb_layergrp_inhibit_inc();
	pcb_layers_reset(PCB);
	pcb_layer_group_setup_default(PCB);
	pcb_get_grp_new_intern(PCB, 1);
	pcb_get_grp_new_intern(PCB, 2);

	assert(pcb_layergrp_list(PCB, PCB_LYT_COPPER, grp, 4) == 4);
	lfont  = make_layer(grp[0], "Font");
	lorig  = make_layer(grp[1], "OrigFont");
	lwidth = make_layer(grp[2], "Width");
	lgrid  = make_layer(grp[3], "Grid");

	assert(pcb_layergrp_list(PCB, PCB_LYT_SILK, grp, 2) == 2);
	make_layer(grp[0], "Silk");
	lsilk = make_layer(grp[1], "Silk");

	pcb_layergrp_inhibit_dec();

	/* Inform the rest about the board change (layer stack, size) */
	pcb_event(PCB_EVENT_BOARD_CHANGED, NULL);
	pcb_event(PCB_EVENT_LAYERS_CHANGED, NULL);

	for (s = 0; s <= PCB_MAX_FONTPOSITION; s++) {
		char txt[32];
		pcb_coord_t ox = (s % 16 + 1) * CELL_SIZE;
		pcb_coord_t oy = (s / 16 + 1) * CELL_SIZE;
		pcb_coord_t w, miny, maxy, maxx = 0;

		symbol = &font->Symbol[s];

		miny = PCB_MIL_TO_COORD(5);
		maxy = font->MaxHeight;

		if ((s > 32) && (s < 127)) {
			sprintf(txt, "%c", s);
			pcb_text_new(lsilk, pcb_font(PCB, 0, 0), ox+CELL_SIZE-CELL_SIZE/3, oy+CELL_SIZE-CELL_SIZE/3, 0, 50, 0, txt, pcb_no_flags());
		}
		sprintf(txt, "%d", s);
		pcb_text_new(lsilk, pcb_font(PCB, 0, 0), ox+CELL_SIZE/20, oy+CELL_SIZE-CELL_SIZE/3, 0, 50, 0, txt, pcb_no_flags());

		for (l = 0; l < symbol->LineN; l++) {
			pcb_line_new_merge(lfont,
														 symbol->Line[l].Point1.X + ox,
														 symbol->Line[l].Point1.Y + oy,
														 symbol->Line[l].Point2.X + ox,
														 symbol->Line[l].Point2.Y + oy, symbol->Line[l].Thickness, symbol->Line[l].Thickness, pcb_no_flags());
			pcb_line_new_merge(lorig, symbol->Line[l].Point1.X + ox,
														 symbol->Line[l].Point1.Y + oy,
														 symbol->Line[l].Point2.X + ox,
														 symbol->Line[l].Point2.Y + oy, symbol->Line[l].Thickness, symbol->Line[l].Thickness, pcb_no_flags());
			if (maxx < symbol->Line[l].Point1.X)
				maxx = symbol->Line[l].Point1.X;
			if (maxx < symbol->Line[l].Point2.X)
				maxx = symbol->Line[l].Point2.X;
		}


		for(arc = arclist_first(&symbol->arcs); arc != NULL; arc = arclist_next(arc)) {
			pcb_arc_new(lfont, arc->X + ox, arc->Y + oy, arc->Width, arc->Height, arc->StartAngle, arc->Delta, arc->Thickness, 0, pcb_no_flags(), pcb_true);
			newarc = pcb_arc_new(lorig, arc->X + ox, arc->Y + oy, arc->Width, arc->Height, arc->StartAngle, arc->Delta, arc->Thickness, 0, pcb_no_flags(), pcb_true);
			if (newarc != NULL) {
				if (maxx < newarc->BoundingBox.X2 - ox)
					maxx = newarc->BoundingBox.X2 - ox;
				if (maxy < newarc->BoundingBox.Y2 - oy)
					maxy = newarc->BoundingBox.Y2 - oy;
			}
		}

		for(poly = polylist_first(&symbol->polys); poly != NULL; poly = polylist_next(poly)) {
			int n;
			pcb_point_t *pnt;

			add_poly(lfont, poly, ox, oy);
			add_poly(lorig, poly, ox, oy);

			for(n = 0, pnt = poly->Points; n < poly->PointN; n++,pnt++) {
				if (maxx < pnt->X)
					maxx = pnt->X;
				if (maxy < pnt->Y)
					maxy = pnt->Y;
			}
		}

		w = maxx + symbol->Delta + ox;
		pcb_line_new_merge(lwidth, w, miny + oy, w, maxy + oy, PCB_MIL_TO_COORD(1), PCB_MIL_TO_COORD(1), pcb_no_flags());
	}

	for (l = 0; l < 16; l++) {
		int x = (l + 1) * CELL_SIZE;
		pcb_line_new_merge(lgrid, x, 0, x, PCB->hidlib.size_y, PCB_MIL_TO_COORD(1), PCB_MIL_TO_COORD(1), pcb_no_flags());
	}
	for (l = 0; l <= PCB_MAX_FONTPOSITION / 16 + 1; l++) {
		int y = (l + 1) * CELL_SIZE;
		pcb_line_new_merge(lgrid, 0, y, PCB->hidlib.size_x, y, PCB_MIL_TO_COORD(1), PCB_MIL_TO_COORD(1), pcb_no_flags());
	}
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_fontsave[] = "FontSave()";
static const char pcb_acth_fontsave[] = "Convert the current PCB back to a font.";
static fgw_error_t pcb_act_FontSave(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_font_t *font;
	pcb_symbol_t *symbol;
	int i;
	pcb_line_t *l;
	pcb_arc_t *a;
	pcb_poly_t *p, *np;
	gdl_iterator_t it;
	pcb_layer_t *lfont, *lwidth;

	font = pcb_font(PCB, 0, 1);
	lfont = PCB->Data->Layer + 0;
	lwidth = PCB->Data->Layer + 2;

	for (i = 0; i <= PCB_MAX_FONTPOSITION; i++) {
		font->Symbol[i].LineN = 0;
		font->Symbol[i].Valid = 0;
		font->Symbol[i].Width = 0;
	}

	/* pack lines */
	linelist_foreach(&lfont->Line, &it, l) {
		int x1 = l->Point1.X;
		int y1 = l->Point1.Y;
		int x2 = l->Point2.X;
		int y2 = l->Point2.Y;
		int ox, oy, s;

		s = XYtoSym(x1, y1);
		ox = (s % 16 + 1) * CELL_SIZE;
		oy = (s / 16 + 1) * CELL_SIZE;
		symbol = &font->Symbol[s];

		x1 -= ox;
		y1 -= oy;
		x2 -= ox;
		y2 -= oy;

		if (symbol->Width < x1)
			symbol->Width = x1;
		if (symbol->Width < x2)
			symbol->Width = x2;
		symbol->Valid = 1;

		pcb_font_new_line_in_sym(symbol, x1, y1, x2, y2, l->Thickness);
	}

	/* pack arcs */
	arclist_foreach(&lfont->Arc, &it, a) {
		int cx = a->X;
		int cy = a->Y;
		int ox, oy, s;

		s = XYtoSym(cx, cy);
		ox = (s % 16 + 1) * CELL_SIZE;
		oy = (s / 16 + 1) * CELL_SIZE;
		symbol = &font->Symbol[s];

		cx -= ox;
		cy -= oy;

		pcb_arc_bbox(a);
		if (symbol->Width < a->BoundingBox.X2 - ox)
			symbol->Width = a->BoundingBox.X2 - ox;

		if (symbol->Width < cx)
			symbol->Width = cx;
		symbol->Valid = 1;

		pcb_font_new_arc_in_sym(symbol, cx, cy, a->Width, a->StartAngle, a->Delta, a->Thickness);
	}

	/* pack polygons */
	polylist_foreach(&lfont->Polygon, &it, p) {
		pcb_coord_t x1 = p->Points[0].X;
		pcb_coord_t y1 = p->Points[0].Y;
		pcb_coord_t s, ox, oy;
		int n;

		s = XYtoSym(x1, y1);
		ox = (s % 16 + 1) * CELL_SIZE;
		oy = (s / 16 + 1) * CELL_SIZE;
		symbol = &font->Symbol[s];

		np = pcb_font_new_poly_in_sym(symbol, p->PointN);

		for(n = 0; n < p->PointN; n++) {
			np->Points[n].X = p->Points[n].X - ox;
			np->Points[n].Y = p->Points[n].Y - oy;
			if (symbol->Width < np->Points[n].X)
				symbol->Width = np->Points[n].X;
		}
	}

	/* recalc delta */
	linelist_foreach(&lwidth->Line, &it, l) {
		pcb_coord_t x1 = l->Point1.X;
		pcb_coord_t y1 = l->Point1.Y;
		pcb_coord_t ox, s;

		s = XYtoSym(x1, y1);
		ox = (s % 16 + 1) * CELL_SIZE;
		symbol = &font->Symbol[s];

		x1 -= ox;

		symbol->Delta = x1 - symbol->Width;
	}

	pcb_font_set_info(font);
	pcb_actionl("SaveFontTo", NULL);

	PCB_ACT_IRES(0);
	return 0;
}

pcb_action_t fontmode_action_list[] = {
	{"FontEdit", pcb_act_FontEdit, pcb_acth_fontedit, pcb_acts_fontedit},
	{"FontSave", pcb_act_FontSave, pcb_acth_fontsave, pcb_acts_fontsave}
};

static const char *fontmode_cookie = "fontmode plugin";

PCB_REGISTER_ACTIONS(fontmode_action_list, fontmode_cookie)

int pplg_check_ver_fontmode(int ver_needed) { return 0; }

void pplg_uninit_fontmode(void)
{
	pcb_remove_actions_by_cookie(fontmode_cookie);
}

#include "dolists.h"
int pplg_init_fontmode(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(fontmode_action_list, fontmode_cookie)
	return 0;
}
