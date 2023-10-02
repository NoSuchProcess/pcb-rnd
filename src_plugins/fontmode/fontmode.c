/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 2006 DJ Delorie
 *  Copyright (C) 2022,2023 Tibor 'Igor2' Palinkas
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
#include <librnd/poly/rtree.h>
#include "flag_str.h"
#include "undo.h"
#include <librnd/core/rnd_printf.h>
#include <librnd/core/plugins.h>
#include <librnd/core/actions.h>
#include <librnd/core/compat_misc.h>
#include "event.h"
#include "polygon.h"
#include "obj_poly_draw.h"
#include "preview.h"

/* FIXME - we currently hardcode the grid and PCB size.  What we
   should do in the future is scan the font for its extents, and size
   the grid appropriately.  Also, when we convert back to a font, we
   should search the grid for the gridlines and use them to figure out
   where the symbols are. */

#define CELL_SIZE   ((rnd_coord_t)(RND_MIL_TO_COORD (100)))

#define XYtoSym(x,y) (((x) / CELL_SIZE - 1)  +  (16 * ((y) / CELL_SIZE - 1)))

static pcb_layer_t *make_layer(pcb_board_t *pcb, rnd_layergrp_id_t grp, const char *lname)
{
	rnd_layer_id_t lid;

	assert(grp >= 0);
	lid = pcb_layer_create(pcb, grp, lname, 0);
	assert(lid >= 0);
	pcb->Data->Layer[lid].meta.real.vis = 1;
	pcb->Data->Layer[lid].meta.real.color = *pcb_layer_default_color(lid, 0);
	return &pcb->Data->Layer[lid];
}

/* Remember the original font we are editing; some metadata are not represented
   graphically and can not be converted back from graphics - those are loaded
   from here when converting the graphics back to a font for saving */
static rnd_font_t *fontedit_src;

static void font2editor_new(pcb_board_t *pcb, rnd_font_t *font, pcb_layer_t *lfont, pcb_layer_t *lorig, pcb_layer_t *lwidth, pcb_layer_t *lsilk)
{
	int s;
	long n;
	rnd_glyph_t *g;

	/* remember the font for fields not represente graphically */
	fontedit_src = font;

	for(s = 0, g = font->glyph; s <= RND_FONT_MAX_GLYPHS; s++,g++) {
		char txt[32];
		rnd_glyph_atom_t *a;

		rnd_coord_t ox = (s % 16 + 1) * CELL_SIZE;
		rnd_coord_t oy = (s / 16 + 1) * CELL_SIZE;
		rnd_coord_t w, miny, maxy, maxx = 0;

		miny = RND_MIL_TO_COORD(5);
		maxy = font->max_height;

		if ((s > 32) && (s < 127)) {
			sprintf(txt, "%c", s);
			pcb_text_new(lsilk, pcb_font(pcb, 0, 0), ox+CELL_SIZE-CELL_SIZE/3, oy+CELL_SIZE-CELL_SIZE/3, 0, 50, 0, txt, pcb_no_flags());
		}
		sprintf(txt, "%d", s);
		pcb_text_new(lsilk, pcb_font(pcb, 0, 0), ox+CELL_SIZE/20, oy+CELL_SIZE-CELL_SIZE/3, 0, 50, 0, txt, pcb_no_flags());

		for(n = 0, a = g->atoms.array; n < g->atoms.used; n++, a++) {
			pcb_poly_t *poly, *newpoly;
			pcb_arc_t *newarc;

			switch(a->type) {
				case RND_GLYPH_LINE:
					pcb_line_new_merge(lfont,
						a->line.x1 + ox, a->line.y1 + oy,
						a->line.x2 + ox, a->line.y2 + oy,
						a->line.thickness, 0, pcb_no_flags());
					pcb_line_new_merge(lorig,
						a->line.x1 + ox, a->line.y1 + oy,
						a->line.x2 + ox, a->line.y2 + oy,
						a->line.thickness, 0, pcb_no_flags());
					if (maxx < a->line.x1)
						maxx = a->line.x1;
					if (maxx < a->line.x2)
						maxx = a->line.x2;
					break;
				case RND_GLYPH_ARC:
					pcb_arc_new(lfont, a->arc.cx + ox, a->arc.cy + oy, a->arc.r, a->arc.r, a->arc.start, a->arc.delta, a->arc.thickness, 0, pcb_no_flags(), rnd_true);
					newarc = pcb_arc_new(lorig, a->arc.cx + ox, a->arc.cy + oy, a->arc.r, a->arc.r, a->arc.start, a->arc.delta, a->arc.thickness, 0, pcb_no_flags(), rnd_true);
					if (newarc != NULL) {
						if (maxx < newarc->BoundingBox.X2 - ox)
							maxx = newarc->BoundingBox.X2 - ox;
						if (maxy < newarc->BoundingBox.Y2 - oy)
							maxy = newarc->BoundingBox.Y2 - oy;
					}
					break;

				case RND_GLYPH_POLY:
				{
					int i, h = a->poly.pts.used / 2;
					rnd_coord_t *px = &a->poly.pts.array[0], *py = &a->poly.pts.array[h];

					poly = pcb_poly_alloc(lorig);
					newpoly = pcb_poly_alloc(lfont);
					pcb_poly_point_prealloc(poly, h);
					pcb_poly_point_prealloc(newpoly, h);

					poly->Flags = pcb_flag_make(PCB_FLAG_CLEARPOLY);
					newpoly->Flags = pcb_flag_make(PCB_FLAG_CLEARPOLY);

					for(i = 0; i < h; i++, px++, py++) {
						pcb_poly_point_new(poly, *px + ox, *py + oy);
						pcb_poly_point_new(newpoly, *px + ox, *py + oy);

						if (maxx < *px)
							maxx = *px;
						if (maxy < *py)
							maxy = *py;
					}

					pcb_add_poly_on_layer(lorig, poly);
					pcb_add_poly_on_layer(lfont, newpoly);
					pcb_poly_init_clip(pcb->Data, lorig, poly);
					pcb_poly_init_clip(pcb->Data, lfont, newpoly);
				}
				break;
			}
		}

		w = maxx + g->xdelta + ox;
		pcb_line_new_merge(lwidth, w, miny + oy, w, maxy + oy, RND_MIL_TO_COORD(1), RND_MIL_TO_COORD(1), pcb_no_flags());
	}
}

static void editor2font(pcb_board_t *pcb, rnd_font_t *font, const rnd_font_t *orig_font)
{
	rnd_glyph_t *g;
	int i;
	pcb_line_t *l;
	pcb_arc_t *a;
	pcb_poly_t *p;
	gdl_iterator_t it;
	pcb_layer_t *lfont, *lwidth;
	rnd_glyph_poly_t *gp;

	lfont = pcb->Data->Layer + 0;
	lwidth = pcb->Data->Layer + 2;

	for(i = 0; i <= RND_FONT_MAX_GLYPHS; i++)
		rnd_font_free_glyph(&font->glyph[i]);

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
		g = &font->glyph[s];

		x1 -= ox;
		y1 -= oy;
		x2 -= ox;
		y2 -= oy;

		if (g->width < x1)
			g->width = x1;
		if (g->width < x2)
			g->width = x2;
		g->valid = 1;

		rnd_font_new_line_in_glyph(g, x1, y1, x2, y2, l->Thickness);
	}

	/* pack arcs */
	arclist_foreach(&lfont->Arc, &it, a) {
		int cx = (a->BoundingBox.X1 + a->BoundingBox.X2)/2;
		int cy = (a->BoundingBox.Y1 + a->BoundingBox.Y2)/2;
		int ox, oy, s;

		s = XYtoSym(cx, cy);
		ox = (s % 16 + 1) * CELL_SIZE;
		oy = (s / 16 + 1) * CELL_SIZE;
		g = &font->glyph[s];

		cx -= ox;
		cy -= oy;

		pcb_arc_bbox(a);
		if (g->width < a->bbox_naked.X2 - ox - a->Thickness/2.0)
			g->width = rnd_round(a->bbox_naked.X2 - ox - a->Thickness/2.0);

		g->valid = 1;

		rnd_font_new_arc_in_glyph(g, a->X - ox, a->Y - oy, a->Width, a->StartAngle, a->Delta, a->Thickness);
	}

	/* pack polygons */
	polylist_foreach(&lfont->Polygon, &it, p) {
		rnd_coord_t x1 = p->Points[0].X;
		rnd_coord_t y1 = p->Points[0].Y;
		rnd_coord_t s, ox, oy;
		int n;

		s = XYtoSym(x1, y1);
		ox = (s % 16 + 1) * CELL_SIZE;
		oy = (s / 16 + 1) * CELL_SIZE;
		g = &font->glyph[s];
		gp = rnd_font_new_poly_in_glyph(g, p->PointN);

		for(n = 0; n < p->PointN; n++) {
			gp->pts.array[n] = p->Points[n].X - ox;
			gp->pts.array[n+p->PointN] = p->Points[n].Y - oy;
			if (g->width < gp->pts.array[n])
				g->width = gp->pts.array[n];
		}
	}

	/* recalc delta */
	linelist_foreach(&lwidth->Line, &it, l) {
		rnd_coord_t x1 = l->Point1.X;
		rnd_coord_t y1 = l->Point1.Y;
		rnd_coord_t ox, s;

		s = XYtoSym(x1, y1);
		ox = (s % 16 + 1) * CELL_SIZE;
		g = &font->glyph[s];

		x1 -= ox;

		g->xdelta = x1 - g->width;

		if (g->xdelta > 10) /* not 0 for rounding errors, just in case */
			g->valid = 1;
	}

#ifdef PCB_WANT_FONT2
	/* copy metadata */
	rnd_font_copy_tables(font, orig_font);
	font->kerning_tbl_valid = orig_font->kerning_tbl_valid;
	font->entity_tbl_valid = orig_font->entity_tbl_valid;
	/* these numericals are not edited graphically */
	font->baseline = orig_font->baseline;
	font->tab_width = orig_font->tab_width;
	font->line_height = orig_font->line_height;
#endif

	rnd_font_normalize(font);
}

#include "brave.h"

static const char pcb_acts_FontEdit[] = "FontEdit()";
static const char pcb_acth_FontEdit[] = "Convert the current font to a PCB for editing.";
static fgw_error_t pcb_act_FontEdit(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	rnd_font_t *font;
	pcb_layer_t *lfont, *lorig, *lwidth, *lgrid, *lsilk, *lbaslin;
	rnd_layergrp_id_t grp[4];
	int l, inplace = 0;
	const char *opt = "";

	RND_ACT_MAY_CONVARG(1, FGW_STR, FontEdit, opt = argv[1].val.str);

	/* this undocumented feature is used by io_lihata to avoid creating a new
	   board from within loading a new board */
	if (strcmp(opt, "inplace") == 0)
		inplace = 1;

	if (pcb->Changed && (rnd_hid_message_box(RND_ACT_DESIGN, "warning", "Switching to fontedit", "OK to lose unsaved edits on this board?", "cancel", 0, "yes", 1, NULL) != 1)) {
		RND_ACT_IRES(-1);
		return 0;
	}

	font = pcb_font_unlink(pcb, conf_core.design.text_font_id);
	if (font == NULL) {
		rnd_message(RND_MSG_ERROR, "Can't fetch font id %d\n", conf_core.design.text_font_id);
		return 1;
	}

	if (!inplace) {
		/* don't ask for losing changes twice */
		pcb->Changed = 0;

		if (rnd_actionva(RND_ACT_DESIGN, "New", "Font", 0))
			return 1;

		pcb = PCB; /* our new board created above */
	}

	rnd_conf_set(RND_CFR_DESIGN, "editor/grid_unit", -1, "mil", RND_POL_OVERWRITE);
	rnd_conf_set_design("design/min_wid", "%s", "1");
	rnd_conf_set_design("design/min_slk", "%s", "1");
	rnd_conf_set_design("design/text_font_id", "%s", "0");


	pcb->hidlib.dwg.X2 = CELL_SIZE * 18;
	pcb->hidlib.dwg.Y2 = CELL_SIZE * ((PCB_MAX_FONTPOSITION + 15) / 16 + 2);
	pcb->hidlib.grid = RND_MIL_TO_COORD(5);

	/* create the layer stack and logical layers */
	pcb_layergrp_inhibit_inc();
	pcb_layers_reset(pcb);
	pcb_layer_group_setup_default(pcb);
	pcb_get_grp_new_intern(pcb, 1);
	pcb_get_grp_new_intern(pcb, 2);

	assert(pcb_layergrp_list(pcb, PCB_LYT_COPPER, grp, 4) == 4);
	lfont  = make_layer(pcb, grp[0], "Font");
	lorig  = make_layer(pcb, grp[1], "OrigFont");
	lwidth = make_layer(pcb, grp[2], "Width");
	lgrid  = make_layer(pcb, grp[3], "Grid");

#ifdef PCB_WANT_FONT2
	if (font->baseline > 0)
		lbaslin = make_layer(pcb, grp[3], "Baseline");
#endif

	assert(pcb_layergrp_list(pcb, PCB_LYT_SILK, grp, 2) == 2);
	make_layer(pcb, grp[0], "Silk");
	lsilk = make_layer(pcb, grp[1], "Silk");

	pcb_layergrp_inhibit_dec();

	/* Inform the rest about the board change (layer stack, size) */
	rnd_event(&pcb->hidlib, RND_EVENT_DESIGN_SET_CURRENT, "p", NULL);
	rnd_event(&pcb->hidlib, PCB_EVENT_LAYERS_CHANGED, NULL);

	font2editor_new(pcb, font, lfont, lorig, lwidth, lsilk);

	for (l = 0; l < 16; l++) {
		int x = (l + 1) * CELL_SIZE;
		pcb_line_new_merge(lgrid, x, 0, x, pcb->hidlib.dwg.Y2, RND_MIL_TO_COORD(1), RND_MIL_TO_COORD(1), pcb_no_flags());
	}
	for (l = 0; l <= PCB_MAX_FONTPOSITION / 16 + 1; l++) {
		int y = (l + 1) * CELL_SIZE;
		pcb_line_new_merge(lgrid, 0, y, pcb->hidlib.dwg.X2, y, RND_MIL_TO_COORD(1), RND_MIL_TO_COORD(1), pcb_no_flags());
	}


#ifdef PCB_WANT_FONT2
	if (font->baseline > 0) {
		for (l = 0; l <= PCB_MAX_FONTPOSITION / 16 + 1; l++) {
			int y = (l + 1) * CELL_SIZE + font->baseline;
			pcb_line_new_merge(lbaslin, 0, y, pcb->hidlib.dwg.X2, y, RND_MIL_TO_COORD(0.25), RND_MIL_TO_COORD(1), pcb_no_flags());
		}
	}
#endif

	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_FontSave[] = "FontSave([filename])";
static const char pcb_acth_FontSave[] = "Convert the current PCB back to a font.";
static fgw_error_t pcb_act_FontSave(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	rnd_font_t *font = pcb_font(pcb, 0, 1);
	char *fn = NULL;

	RND_ACT_MAY_CONVARG(1, FGW_STR, FontSave, fn = argv[1].val.str);

	editor2font(pcb, font, fontedit_src);
	rnd_actionva(RND_ACT_DESIGN, "SaveFontTo", fn, NULL);

	RND_ACT_IRES(0);
	return 0;
}

static void font_xform(pcb_board_t *pcb, pcb_xform_mx_t mx)
{
	pcb_line_t *l;
	pcb_arc_t *a;
	pcb_poly_t *p;
	gdl_iterator_t it;
	pcb_layer_t *lfont, *lwidth;
	rnd_coord_t s, ox, oy;
	vtp0_t todel = {0};

	lfont = pcb->Data->Layer + 0;
	lwidth = pcb->Data->Layer + 2;

	/* xform lines */
	linelist_foreach(&lfont->Line, &it, l) {
		rnd_coord_t x1, y1, x2, y2, nx1, ny1, nx2, ny2;
		s = XYtoSym(l->Point1.X, l->Point1.Y);
		ox = (s % 16 + 1) * CELL_SIZE;
		oy = (s / 16 + 1) * CELL_SIZE;

		x1 = l->Point1.X - ox; y1 = l->Point1.Y - oy;
		x2 = l->Point2.X - ox; y2 = l->Point2.Y - oy;

		nx1 = rnd_round(pcb_xform_x(mx, x1, y1));
		ny1 = rnd_round(pcb_xform_y(mx, x1, y1));
		nx2 = rnd_round(pcb_xform_x(mx, x2, y2));
		ny2 = rnd_round(pcb_xform_y(mx, x2, y2));

		if ((nx1 != x1) || (ny1 != y1) || (nx2 != x2) || (ny2 != y2)) {
			nx1 += ox; ny1 += oy;
			nx2 += ox; ny2 += oy;
			pcb_line_modify(l, &nx1, &ny1, &nx2, &ny2, NULL, NULL, 1);
		}
	}

	/* xform arcs */
	arclist_foreach(&lfont->Arc, &it, a) {
		int cx = (a->BoundingBox.X1 + a->BoundingBox.X2)/2;
		int cy = (a->BoundingBox.Y1 + a->BoundingBox.Y2)/2;
		rnd_coord_t x, y, nx, ny;

		s = XYtoSym(cx, cy);
		ox = (s % 16 + 1) * CELL_SIZE;
		oy = (s / 16 + 1) * CELL_SIZE;

		x = a->X - ox; y = a->Y - oy;

		nx = rnd_round(pcb_xform_x(mx, x, y));
		ny = rnd_round(pcb_xform_y(mx, x, y));

		if ((nx != x) || (ny != y))
			pcb_move_obj(PCB_OBJ_ARC, a->parent.layer, a, a, nx-x, ny-y);
	}

	/* xform polygons */
	polylist_foreach(&lfont->Polygon, &it, p) {
		rnd_coord_t x1 = p->Points[0].X;
		rnd_coord_t y1 = p->Points[0].Y;
		int n;
		pcb_poly_t *np;

		s = XYtoSym(x1, y1);
		ox = (s % 16 + 1) * CELL_SIZE;
		oy = (s / 16 + 1) * CELL_SIZE;

		/* since there's no undoable mass-point-change API in core, it's cheaper
		   to just create a new poly */
		np = pcb_poly_alloc(lfont);
		pcb_poly_point_prealloc(np, p->PointN);
		np->Flags = pcb_flag_make(PCB_FLAG_CLEARPOLY);

		for(n = 0; n < p->PointN; n++) {
			rnd_coord_t x = p->Points[n].X - ox, y = p->Points[n].Y - oy, nx, ny;
			nx = rnd_round(pcb_xform_x(mx, x, y));
			ny = rnd_round(pcb_xform_y(mx, x, y));
			pcb_poly_point_new(np, nx+ox, ny+oy);
		}
		pcb_add_poly_on_layer(lfont, np);
		pcb_poly_init_clip(pcb->Data, lfont, np);
		pcb_undo_add_obj_to_create(PCB_OBJ_POLY, lfont, np, np);

		vtp0_append(&todel, p);
	}

	/* remove original, pre-xform polys */
	{
		long n;
		for(n = 0; n < todel.used; n++)
			pcb_remove_object(PCB_OBJ_POLY, lfont, todel.array[n], todel.array[n]);
		vtp0_uninit(&todel);
	}

	/* xform delta */
	linelist_foreach(&lwidth->Line, &it, l) {
		rnd_coord_t x1, y1, x2, y2, nx1, ny1, nx2, ny2, nx;

		s = XYtoSym(l->Point1.X, l->Point1.Y);
		ox = (s % 16 + 1) * CELL_SIZE;
		oy = (s / 16 + 1) * CELL_SIZE;

		x1 = l->Point1.X - ox; y1 = l->Point1.Y - oy;
		x2 = l->Point2.X - ox; y2 = l->Point2.Y - oy;

		nx1 = rnd_round(pcb_xform_x(mx, x1, y1));
		ny1 = rnd_round(pcb_xform_y(mx, x1, y1));
		nx2 = rnd_round(pcb_xform_x(mx, x2, y2));
		ny2 = rnd_round(pcb_xform_y(mx, x2, y2));

		nx = RND_MAX(nx1, nx2);

		if ((nx != x1) || (ny1 != y1) || (nx != x2) || (ny2 != y2)) {
			ny1 += oy;
			ny2 += oy;
			nx += ox;
			pcb_line_modify(l, &nx, &ny1, &nx, &ny2, NULL, NULL, 1);
		}

	}
}

/* DOC: fontxform.html */
static const char pcb_acts_FontXform[] = "FontXform(xform1, params..., [xform2, params...], ...)";
static const char pcb_acth_FontXform[] = "Transform font graphics in fontmode (FontEdit)";
static fgw_error_t pcb_act_FontXform(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	int n;
	pcb_xform_mx_t mx = PCB_XFORM_MX_IDENT;

	for(n = 1; n < argc;) {
		const char *cmd;
		RND_ACT_CONVARG(n, FGW_STR, FontXform, cmd = argv[n].val.str);
		if (strcmp(cmd, "move") == 0) {
			rnd_coord_t dx, dy;
			RND_ACT_CONVARG(n+1, FGW_COORD, FontXform, dx = fgw_coord(&argv[n+1]));
			RND_ACT_CONVARG(n+2, FGW_COORD, FontXform, dy = fgw_coord(&argv[n+2]));

			pcb_xform_mx_translate(mx, dx, dy);
			n += 3;
		}
		else if (strcmp(cmd, "shear") == 0) {
			double sx, sy;
			RND_ACT_CONVARG(n+1, FGW_DOUBLE, FontXform, sx = argv[n+1].val.nat_double);
			RND_ACT_CONVARG(n+2, FGW_DOUBLE, FontXform, sy = argv[n+2].val.nat_double);
			pcb_xform_mx_shear(mx, sx, sy);
			n += 3;
		}
		else if (strcmp(cmd, "scale") == 0) {
			double sx, sy;
			RND_ACT_CONVARG(n+1, FGW_DOUBLE, FontXform, sx = argv[n+1].val.nat_double);
			RND_ACT_CONVARG(n+2, FGW_DOUBLE, FontXform, sy = argv[n+2].val.nat_double);
			pcb_xform_mx_scale(mx, sx, sy);
			n += 3;
		}
		else if (strcmp(cmd, "rotate") == 0) {
			double rdeg;
			RND_ACT_CONVARG(n+1, FGW_DOUBLE, FontXform, rdeg = argv[n+1].val.nat_double);
			pcb_xform_mx_rotate(mx, rdeg);
			n += 2;
		}
		else {
			rnd_message(RND_MSG_ERROR, "FontXform(): invalid transformation name '%s'\n", cmd);
			return FGW_ERR_ARG_CONV;
		}
	}

	pcb_undo_freeze_serial();
	font_xform(pcb, mx);
	pcb_undo_unfreeze_serial();
	pcb_undo_inc_serial();

	RND_ACT_IRES(0);
	return 0;
}

static void font_set_xdelta(pcb_board_t *pcb, rnd_coord_t xd)
{
	pcb_line_t *l;
	pcb_arc_t *a;
	pcb_poly_t *p;
	gdl_iterator_t it;
	pcb_layer_t *lfont, *lwidth;
	int s;
	rnd_coord_t ox;
	rnd_coord_t gw[RND_FONT_MAX_GLYPHS] = {0};

	lfont = pcb->Data->Layer + 0;
	lwidth = pcb->Data->Layer + 2;

	/* pick up lines */
	linelist_foreach(&lfont->Line, &it, l) {
		s = XYtoSym(l->Point1.X, l->Point1.Y);
		ox = (s % 16 + 1) * CELL_SIZE;

		gw[s] = RND_MAX(gw[s], (l->Point1.X - ox));
		gw[s] = RND_MAX(gw[s], (l->Point2.X - ox));
	}

	/* pick up arcs */
	arclist_foreach(&lfont->Arc, &it, a) {
		int cx = (a->BoundingBox.X1 + a->BoundingBox.X2)/2;
		int cy = (a->BoundingBox.Y1 + a->BoundingBox.Y2)/2;

		s = XYtoSym(cx, cy);
		ox = (s % 16 + 1) * CELL_SIZE;

		gw[s] = RND_MAX(gw[s], rnd_round(a->bbox_naked.X2 - ox - a->Thickness/2));
	}

	/* pick up polygons */
	polylist_foreach(&lfont->Polygon, &it, p) {
		rnd_coord_t x1 = p->Points[0].X;
		rnd_coord_t y1 = p->Points[0].Y;
		long n;

		s = XYtoSym(x1, y1);
		ox = (s % 16 + 1) * CELL_SIZE;

		for(n = 0; n < p->PointN; n++)
			gw[s] = RND_MAX(gw[s], (p->Points[n].X - ox));
	}

	/* xform delta */
	linelist_foreach(&lwidth->Line, &it, l) {
		rnd_coord_t newx;
		s = XYtoSym(l->Point1.X, l->Point1.Y);
		ox = (s % 16 + 1) * CELL_SIZE;

		if (l->Point1.X-ox == 0) continue;

		newx = gw[s] + xd + ox;

		pcb_line_modify(l, &newx, NULL, &newx, NULL, NULL, NULL, 1);
	}
}

static const char pcb_acts_FontSetXdelta[] = "FontSetXdelta(xd)";
static const char pcb_acth_FontSetXdelta[] = "Calculate the right side of each glyph and place xdelta xd to the right (xd should be a distance: number and unit)";
static fgw_error_t pcb_act_FontSetXdelta(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	rnd_coord_t xd;

	RND_ACT_CONVARG(1, FGW_COORD, FontSetXdelta, xd = fgw_coord(&argv[1]));

	font_set_xdelta(pcb, xd);

	RND_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_FontNormalize[] = "FontNormalize()";
static const char pcb_acth_FontNormalize[] = "Normalize all glyphs (left justify)";
static fgw_error_t pcb_act_FontNormalize(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	pcb_board_t *pcb = PCB_ACT_BOARD;
	rnd_font_t *font = pcb_font(pcb, 0, 1);

	editor2font(pcb, font, fontedit_src);
	rnd_font_normalize(font);

	return pcb_act_FontEdit(res, argc, argv);
}

static const char pcb_acts_FontBaseline[] = "FontBaseline(+-delta)";
static const char pcb_acth_FontBaseline[] = "Change the baseline value and redraw. If there is no baseline, add baseline first.";
static fgw_error_t pcb_act_FontBaseline(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
#ifdef PCB_WANT_FONT2
	pcb_board_t *pcb = PCB_ACT_BOARD;
	rnd_font_t *font = pcb_font(pcb, 0, 1);
	rnd_coord_t delta = 0;

	RND_ACT_MAY_CONVARG(1, FGW_COORD, FontBaseline, delta = fgw_coord(&argv[1]));

	pcb->Changed = 0;
	editor2font(pcb, font, fontedit_src);
	if (font->baseline == 0)
		font->baseline = RND_MIL_TO_COORD(50); /* default value that works with the default font */
	font->baseline += delta;
	return pcb_act_FontEdit(res, argc, argv);
#else
	rnd_message(RND_MSG_ERROR, "FontBaseline() is not implemented until librnd 4.1.0 (font v2 support)\n");
	RND_ACT_IRES(-1);
	return 0;
#endif
}

rnd_action_t fontmode_action_list[] = {
	{"FontEdit", pcb_act_FontEdit, pcb_acth_FontEdit, pcb_acts_FontEdit},
	{"FontSave", pcb_act_FontSave, pcb_acth_FontSave, pcb_acts_FontSave},
	{"FontXform", pcb_act_FontXform, pcb_acth_FontXform, pcb_acts_FontXform},
	{"FontSetXdelta", pcb_act_FontSetXdelta, pcb_acth_FontSetXdelta, pcb_acts_FontSetXdelta},
	{"FontNormalize", pcb_act_FontNormalize, pcb_acth_FontNormalize, pcb_acts_FontNormalize},
	{"FontBaseline", pcb_act_FontBaseline, pcb_acth_FontBaseline, pcb_acts_FontBaseline},
	{"FontModePreview", pcb_act_FontModePreview, pcb_acth_FontModePreview, pcb_acts_FontModePreview}
};

static const char *fontmode_cookie = "fontmode plugin";

int pplg_check_ver_fontmode(int ver_needed) { return 0; }

void pplg_uninit_fontmode(void)
{
	rnd_remove_actions_by_cookie(fontmode_cookie);
}

int pplg_init_fontmode(void)
{
	RND_API_CHK_VER;
	RND_REGISTER_ACTIONS(fontmode_action_list, fontmode_cookie)
	return 0;
}
