/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *  Copyright (C) 1994,1995,1996, 2003, 2004 Thomas Nau
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

#include "conf_core.h"
#include "math_helper.h"
#include "board.h"
#include "data.h"
#include "draw.h"
#include "rotate.h"
#include "rtree.h"
#include "stub_draw.h"
#include "layer_ui.h"
#include "hid_inlines.h"

#include "obj_pstk_draw.h"
#include "obj_line_draw.h"
#include "obj_arc_draw.h"
#include "obj_rat_draw.h"
#include "obj_poly_draw.h"
#include "obj_text_draw.h"
#include "obj_subc_parent.h"

#undef NDEBUG
#include <assert.h>

#define	SMALL_SMALL_TEXT_SIZE	0
#define	SMALL_TEXT_SIZE			1
#define	NORMAL_TEXT_SIZE		2
#define	LARGE_TEXT_SIZE			3
#define	N_TEXT_SIZES			4

pcb_output_t pcb_draw_out; /* global context used for drawing */

pcb_box_t pcb_draw_invalidated = { COORD_MAX, COORD_MAX, -COORD_MAX, -COORD_MAX };

int pcb_draw_doing_pinout = 0;
pcb_bool pcb_draw_doing_assy = pcb_false;
static vtp0_t delayed_labels;
pcb_bool delayed_labels_enabled = pcb_false;

static void DrawEverything(const pcb_box_t *);
static void DrawLayerGroup(int, const pcb_box_t *, int);
static void pcb_draw_obj_label(pcb_layergrp_id_t gid, pcb_any_obj_t *obj);
static void pcb_draw_pstk_marks(const pcb_box_t *drawn_area);
static void pcb_draw_pstk_labels(const pcb_box_t *drawn_area);
static void pcb_draw_pstk_holes(pcb_layergrp_id_t group, const pcb_box_t *drawn_area, pcb_pstk_draw_hole_t holetype);

/* In draw_ly_spec.c: */
static void pcb_draw_paste(int side, const pcb_box_t *drawn_area);
static void pcb_draw_mask(int side, const pcb_box_t *screen);
static void pcb_draw_silk(unsigned long lyt_side, const pcb_box_t *drawn_area);
static void pcb_draw_rats(const pcb_box_t *);
static void pcb_draw_assembly(unsigned int lyt_side, const pcb_box_t *drawn_area);


void pcb_draw_delay_label_add(pcb_any_obj_t *obj)
{
	if (delayed_labels_enabled)
		vtp0_append(&delayed_labels, obj);
}


#warning cleanup TODO: this should be cached
void pcb_lighten_color(const char *orig, char buf[8], double factor)
{
	unsigned int r, g, b;

	if (orig[0] == '#') {
		sscanf(&orig[1], "%2x%2x%2x", &r, &g, &b);
		r = MIN(255, r * factor);
		g = MIN(255, g * factor);
		b = MIN(255, b * factor);
	}
	else {
		r = 0xff;
		g = 0xff;
		b = 0xff;
	}
	pcb_snprintf(buf, sizeof("#XXXXXX"), "#%02x%02x%02x", r, g, b);
}

void pcb_draw_dashed_line(pcb_hid_gc_t GC, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2, unsigned int segs, pcb_bool_t cheap)
{
/* TODO: we need a real geo lib... using double here is plain wrong */
	double dx = x2-x1, dy = y2-y1;
	double len_mnt = PCB_ABS(dx) + PCB_ABS(dy);
	int n;
	pcb_coord_t minlen = pcb_gui->coord_per_pix * 8;

	if (len_mnt < minlen*2) {
		/* line too short, just draw it */
		pcb_gui->draw_line(GC, x1, y1, x2, y2);
		return;
	}

	segs = (segs << 1) + 1; /* must be odd */

	if (cheap) {
		if ((segs > 3) && (len_mnt < minlen * segs))
			segs = 3;
		else if ((segs > 5) && (len_mnt < minlen * 2 * segs))
			segs = 5;
	}

	/* first seg is drawn from x1, y1 with no rounding error due to n-1 == 0 */
	for(n = 1; n < segs; n+=2)
		pcb_gui->draw_line(GC,
			x1 + (dx * (double)(n-1) / (double)segs), y1 + (dy * (double)(n-1) / (double)segs),
			x1 + (dx * (double)n / (double)segs), y1 + (dy * (double)n / (double)segs));


	/* make sure the last segment is drawn properly to x2 and y2, don't leave
	   room for rounding errors */
	pcb_gui->draw_line(GC,
		x2 - (dx / (double)segs), y2 - (dy / (double)segs),
		x2, y2);
}


/*
 * initiate the actual redrawing of the updated area
 */
pcb_cardinal_t pcb_draw_inhibit = 0;
void pcb_draw(void)
{
	if (pcb_draw_inhibit)
		return;
	if (pcb_draw_invalidated.X1 <= pcb_draw_invalidated.X2 && pcb_draw_invalidated.Y1 <= pcb_draw_invalidated.Y2)
		pcb_gui->invalidate_lr(pcb_draw_invalidated.X1, pcb_draw_invalidated.X2, pcb_draw_invalidated.Y1, pcb_draw_invalidated.Y2);

	/* shrink the update block */
	pcb_draw_invalidated.X1 = pcb_draw_invalidated.Y1 = COORD_MAX;
	pcb_draw_invalidated.X2 = pcb_draw_invalidated.Y2 = -COORD_MAX;
}

/* ----------------------------------------------------------------------
 * redraws all the data by the event handlers
 */
void pcb_redraw(void)
{
	pcb_gui->invalidate_all();
}

static void DrawEverything_holes(pcb_layergrp_id_t gid, const pcb_box_t *drawn_area)
{
	int plated, unplated;
	pcb_board_count_holes(PCB, &plated, &unplated, drawn_area);

	if (plated && pcb_layer_gui_set_vlayer(PCB, PCB_VLY_PLATED_DRILL, 0)) {
		pcb_draw_pstk_holes(gid, drawn_area, PCB_PHOLE_PLATED);
		pcb_gui->end_layer();
	}

	if (unplated && pcb_layer_gui_set_vlayer(PCB, PCB_VLY_UNPLATED_DRILL, 0)) {
		pcb_draw_pstk_holes(gid, drawn_area, PCB_PHOLE_UNPLATED);
		pcb_gui->end_layer();
	}
}

static void draw_virtual_layers(const pcb_box_t *drawn_area)
{
	pcb_hid_expose_ctx_t hid_exp;

	hid_exp.view = *drawn_area;
	hid_exp.force = 0;

	if (pcb_layer_gui_set_vlayer(PCB, PCB_VLY_TOP_ASSY, 0)) {
		pcb_draw_assembly(PCB_LYT_TOP, drawn_area);
		pcb_gui->end_layer();
	}

	if (pcb_layer_gui_set_vlayer(PCB, PCB_VLY_BOTTOM_ASSY, 0)) {
		pcb_draw_assembly(PCB_LYT_BOTTOM, drawn_area);
		pcb_gui->end_layer();
	}

	if (pcb_layer_gui_set_vlayer(PCB, PCB_VLY_FAB, 0)) {
		pcb_stub_draw_fab(pcb_draw_out.fgGC, &hid_exp);
		pcb_gui->end_layer();
	}

	if (pcb_layer_gui_set_vlayer(PCB, PCB_VLY_CSECT, 0)) {
		pcb_stub_draw_csect(pcb_draw_out.fgGC, &hid_exp);
		pcb_gui->end_layer();
	}
}

static void draw_ui_layers(const pcb_box_t *drawn_area)
{
	int i;
	pcb_layer_t *first;

	/* find the first ui layer in use */
	first = NULL;
	for(i = 0; i < vtlayer_len(&pcb_uilayer); i++) {
		if (pcb_uilayer.array[i].meta.real.cookie != NULL) {
			first = pcb_uilayer.array+i;
			break;
		}
	}

	/* if there's any UI layer, try to draw them */
	if ((first != NULL) && pcb_layer_gui_set_g_ui(first, 0)) {
		int have_canvas = 0;
		for(i = 0; i < vtlayer_len(&pcb_uilayer); i++)
			if ((pcb_uilayer.array[i].meta.real.cookie != NULL) && (pcb_uilayer.array[i].meta.real.vis)) {
				if (!have_canvas) {
					pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, pcb_draw_out.direct, drawn_area);
					pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, drawn_area);
					have_canvas = 1;
				}
				pcb_draw_layer(pcb_uilayer.array+i, drawn_area);
			}
		pcb_gui->end_layer();
		if (have_canvas)
			pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, pcb_draw_out.direct, drawn_area);
	}
}

/* Draw subc and padstack marks in xor mode */
static void draw_xor_marks(const pcb_box_t *drawn_area)
{
	pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, pcb_draw_out.direct, drawn_area);
	pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE_XOR, pcb_draw_out.direct, drawn_area);

	pcb_hid_set_line_cap(pcb_draw_out.fgGC, pcb_cap_round);
	pcb_hid_set_line_width(pcb_draw_out.fgGC, 0);
	pcb_hid_set_draw_xor(pcb_draw_out.fgGC, 1);

	if (PCB->SubcOn)
		pcb_r_search(PCB->Data->subc_tree, drawn_area, NULL, draw_subc_mark_callback, NULL, NULL);

	if ((PCB->padstack_mark_on) && (conf_core.appearance.padstack.cross_thick > 0)) {
		pcb_hid_set_line_width(pcb_draw_out.fgGC, -conf_core.appearance.padstack.cross_thick);
		pcb_draw_pstk_marks(drawn_area);
	}

	pcb_hid_set_draw_xor(pcb_draw_out.fgGC, 0);
	pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, pcb_draw_out.direct, drawn_area);
}

static void draw_rats(const pcb_box_t *drawn_area)
{
	if (pcb_layer_gui_set_vlayer(PCB, PCB_VLY_RATS, 0)) {
		pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, pcb_draw_out.direct, drawn_area);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, drawn_area);
		pcb_draw_rats(drawn_area);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, pcb_draw_out.direct, drawn_area);
		pcb_gui->end_layer();
	}
}

static void draw_pins_and_pads(const pcb_box_t *drawn_area, pcb_layergrp_id_t component, pcb_layergrp_id_t solder)
{
	pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, pcb_draw_out.direct, drawn_area);
	pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, drawn_area);

	/* Draw pins' and pads' names */
	pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, pcb_draw_out.direct, drawn_area);
	pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, drawn_area);
	pcb_hid_set_line_cap(pcb_draw_out.fgGC, pcb_cap_round);
	pcb_hid_set_line_width(pcb_draw_out.fgGC, 0);
	if (PCB->SubcOn)
		pcb_r_search(PCB->Data->subc_tree, drawn_area, NULL, draw_subc_label_callback, NULL, NULL);
	if (PCB->padstack_mark_on) {
		pcb_hid_set_line_width(pcb_draw_out.fgGC, -conf_core.appearance.padstack.cross_thick);
		pcb_draw_pstk_labels(drawn_area);
	}
	pcb_draw_pstk_names(conf_core.editor.show_solder_side ? solder : component, drawn_area);
	pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, pcb_draw_out.direct, drawn_area);
}


/* ---------------------------------------------------------------------------
 * initializes some identifiers for a new zoom factor and redraws whole screen
 */
static void DrawEverything(const pcb_box_t *drawn_area)
{
	int i, ngroups, slk_len;
	pcb_layergrp_id_t component, solder, slk[16], gid, side_copper_grp;
	/* This is the list of layer groups we will draw.  */
	char do_group[PCB_MAX_LAYERGRP];
	/* This is the reverse of the order in which we draw them.  */
	pcb_layergrp_id_t drawn_groups[PCB_MAX_LAYERGRP];
	pcb_bool paste_empty;

	PCB->Data->SILKLAYER.meta.real.color = conf_core.appearance.color.element;
	PCB->Data->BACKSILKLAYER.meta.real.color = conf_core.appearance.color.invisible_objects;

	pcb_gui->render_burst(PCB_HID_BURST_START, drawn_area);

	memset(do_group, 0, sizeof(do_group));
	for (ngroups = 0, i = 0; i < pcb_max_layer; i++) {
		pcb_layer_t *l = LAYER_ON_STACK(i);
		pcb_layergrp_id_t group = pcb_layer_get_group(PCB, pcb_layer_stack[i]);
		unsigned int gflg = pcb_layergrp_flags(PCB, group);

		if ((gflg & PCB_LYT_SILK) || (gflg & PCB_LYT_MASK) || (gflg & PCB_LYT_PASTE)) /* do not draw silk, mask and paste here, they'll be drawn separately */
			continue;

		if (l->meta.real.vis && !do_group[group]) {
			do_group[group] = 1;
			drawn_groups[ngroups++] = group;
		}
	}

	solder = component = -1;
	pcb_layergrp_list(PCB, PCB_LYT_BOTTOM | PCB_LYT_COPPER, &solder, 1);
	pcb_layergrp_list(PCB, PCB_LYT_TOP | PCB_LYT_COPPER, &component, 1);
	side_copper_grp = conf_core.editor.show_solder_side ? solder : component;


	/*
	 * first draw all 'invisible' stuff
	 */
	if (!conf_core.editor.check_planes && pcb_layer_gui_set_vlayer(PCB, PCB_VLY_INVISIBLE, 0)) {
		pcb_draw_silk(PCB_LYT_INVISIBLE_SIDE(), drawn_area);

		pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, pcb_draw_out.direct, drawn_area);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, drawn_area);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, pcb_draw_out.direct, drawn_area);

		pcb_gui->end_layer();
	}

	/* draw all layers in layerstack order */
	for (i = ngroups - 1; i >= 0; i--) {
		pcb_layergrp_id_t group = drawn_groups[i];

		if (pcb_layer_gui_set_glayer(PCB, group, 0)) {
			int is_current = 0;
			pcb_layergrp_id_t cgrp = CURRENT->meta.real.grp;

			if ((cgrp == solder) || (cgrp == component)) {
				/* current group is top or bottom: visibility depends on side we are looking at */
				if (group == side_copper_grp)
					is_current = 1;
			}
			else {
				/* internal layer displayed on top: current group is solid, others are "invisible" */
				if (group == cgrp)
					is_current = 1;
			}

			DrawLayerGroup(group, drawn_area, is_current);
			pcb_gui->end_layer();
		}
	}

	if (conf_core.editor.check_planes && pcb_gui->gui)
		goto finish;

	/* Draw padstacks below silk */
	pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, pcb_draw_out.direct, drawn_area);
	pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, drawn_area);
	if (pcb_gui->gui)
		pcb_draw_ppv(conf_core.editor.show_solder_side ? solder : component, drawn_area);
	else if (!pcb_gui->holes_after)
		DrawEverything_holes(side_copper_grp, drawn_area);
	pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, pcb_draw_out.direct, drawn_area);

	/* Draw the solder mask if turned on */
	gid = pcb_layergrp_get_top_mask();
	if ((gid >= 0) && (pcb_layer_gui_set_glayer(PCB, gid, 0))) {
		pcb_draw_mask(PCB_COMPONENT_SIDE, drawn_area);
		pcb_gui->end_layer();
	}

	gid = pcb_layergrp_get_bottom_mask();
	if ((gid >= 0) && (pcb_layer_gui_set_glayer(PCB, gid, 0))) {
		pcb_draw_mask(PCB_SOLDER_SIDE, drawn_area);
		pcb_gui->end_layer();
	}

	/* Draw silks */
	slk_len = pcb_layergrp_list(PCB, PCB_LYT_SILK, slk, sizeof(slk) / sizeof(slk[0]));
	for(i = 0; i < slk_len; i++) {
		if (pcb_layer_gui_set_glayer(PCB, slk[i], 0)) {
			unsigned int loc = pcb_layergrp_flags(PCB, slk[i]);
			pcb_draw_silk(loc & PCB_LYT_ANYWHERE, drawn_area);
			pcb_gui->end_layer();
		}
	}

	if (pcb_gui->holes_after)
	{
		pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, pcb_draw_out.direct, drawn_area); 
		pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, drawn_area);
		DrawEverything_holes(side_copper_grp, drawn_area);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, pcb_draw_out.direct, drawn_area);
	}

	gid = pcb_layergrp_get_top_paste();
	if (gid >= 0)
		paste_empty = pcb_layergrp_is_empty(PCB, gid);
	if ((gid >= 0) && (pcb_layer_gui_set_glayer(PCB, gid, paste_empty))) {
		pcb_draw_paste(PCB_COMPONENT_SIDE, drawn_area);
		pcb_gui->end_layer();
	}

	gid = pcb_layergrp_get_bottom_paste();
	if (gid >= 0)
		paste_empty = pcb_layergrp_is_empty(PCB, gid);
	if ((gid >= 0) && (pcb_layer_gui_set_glayer(PCB, gid, paste_empty))) {
		pcb_draw_paste(PCB_SOLDER_SIDE, drawn_area);
		pcb_gui->end_layer();
	}

	draw_virtual_layers(drawn_area);
	if (pcb_gui->gui) {
		draw_rats(drawn_area);
		draw_pins_and_pads(drawn_area, component, solder);
	}
	draw_ui_layers(drawn_area);

	if (pcb_gui->gui)
		draw_xor_marks(drawn_area);

	finish:;
	pcb_gui->render_burst(PCB_HID_BURST_END, drawn_area);
}

static void pcb_draw_pstks(pcb_layergrp_id_t group, const pcb_box_t *drawn_area, int is_current, pcb_layer_combining_t comb)
{
	pcb_pstk_draw_t ctx;
	ctx.pcb = PCB;
	ctx.gid = group;
	ctx.is_current = is_current;
	ctx.comb = comb;
	pcb_r_search(PCB->Data->padstack_tree, drawn_area, NULL, pcb_pstk_draw_callback, &ctx, NULL);
}

static void pcb_draw_pstk_marks(const pcb_box_t *drawn_area)
{
	pcb_pstk_draw_t ctx;
	ctx.pcb = PCB;
	pcb_r_search(PCB->Data->padstack_tree, drawn_area, NULL, pcb_pstk_draw_mark_callback, &ctx, NULL);
}

static void pcb_draw_pstk_labels(const pcb_box_t *drawn_area)
{
	pcb_pstk_draw_t ctx;
	ctx.pcb = PCB;
	pcb_r_search(PCB->Data->padstack_tree, drawn_area, NULL, pcb_pstk_draw_label_callback, &ctx, NULL);
}

static void pcb_draw_pstk_holes(pcb_layergrp_id_t group, const pcb_box_t *drawn_area, pcb_pstk_draw_hole_t holetype)
{
	pcb_pstk_draw_t ctx;

	if (!PCB->hole_on)
		return;

	ctx.pcb = PCB;
	ctx.gid = group;
	ctx.holetype = holetype;
	pcb_r_search(PCB->Data->padstack_tree, drawn_area, NULL, pcb_pstk_draw_hole_callback, &ctx, NULL);
}

/* ---------------------------------------------------------------------------
 * Draws padstacks - Always draws for non-gui HIDs,
 * otherwise drawing depends on PCB->pstk_on
 */
void pcb_draw_ppv(pcb_layergrp_id_t group, const pcb_box_t * drawn_area)
{
	/* draw padstack holes - copper is drawn with each group */
	if (PCB->pstk_on || !pcb_gui->gui) {
		pcb_draw_pstk_holes(group, drawn_area, PCB_PHOLE_PLATED | PCB_PHOLE_UNPLATED | PCB_PHOLE_BB);
	}
}

/* ---------------------------------------------------------------------------
 * Draws padstacks' names - Always draws for non-gui HIDs,
 * otherwise drawing depends on PCB->pstk_on
 */
void pcb_draw_pstk_names(pcb_layergrp_id_t group, const pcb_box_t *drawn_area)
{
	if (PCB->pstk_on || !pcb_gui->gui) {
		size_t n;
		for(n = 0; n < delayed_labels.used; n++)
			pcb_draw_obj_label(group, delayed_labels.array[n]);
	}
	delayed_labels.used = 0;
}

#include "draw_composite.c"
#include "draw_ly_spec.c"

void pcb_draw_layer(pcb_layer_t *Layer, const pcb_box_t * screen)
{
	pcb_draw_info_t info;
	pcb_box_t scr2;
	unsigned int lflg = 0;

	if ((screen->X2 <= screen->X1) || (screen->Y2 <= screen->Y1)) {
		scr2 = *screen;
		screen = &scr2;
		if (scr2.X2 <= scr2.X1)
			scr2.X2 = scr2.X1+1;
		if (scr2.Y2 <= scr2.Y1)
			scr2.Y2 = scr2.Y1+1;
	}

	info.drawn_area = screen;
	info.layer = Layer;

	lflg = pcb_layer_flags_(Layer);
	if (PCB_LAYERFLG_ON_VISIBLE_SIDE(lflg))
		pcb_draw_out.active_padGC = pcb_draw_out.padGC;
	else
		pcb_draw_out.active_padGC = pcb_draw_out.backpadGC;

		/* print the non-clearing polys */
	if (lflg & PCB_LYT_COPPER) {
		pcb_hid_set_line_width(pcb_draw_out.fgGC, 1);
		pcb_hid_set_line_cap(pcb_draw_out.fgGC, pcb_cap_square);
		pcb_r_search(Layer->polygon_tree, screen, NULL, pcb_poly_draw_term_callback, &info, NULL);
	}
	else {
		pcb_r_search(Layer->polygon_tree, screen, NULL, pcb_poly_draw_callback, &info, NULL);
	}

	if (conf_core.editor.check_planes)
		goto out;

	/* draw all visible layer objects (with terminal gfx on copper) */
	if (lflg & PCB_LYT_COPPER) {
		pcb_r_search(Layer->line_tree, screen, NULL, pcb_line_draw_term_callback, Layer, NULL);
		pcb_r_search(Layer->arc_tree, screen, NULL, pcb_arc_draw_term_callback, Layer, NULL);
		pcb_r_search(Layer->text_tree, screen, NULL, pcb_text_draw_term_callback, Layer, NULL);
	}
	else {
		pcb_r_search(Layer->line_tree, screen, NULL, pcb_line_draw_callback, Layer, NULL);
		pcb_r_search(Layer->arc_tree, screen, NULL, pcb_arc_draw_callback, Layer, NULL);
		pcb_r_search(Layer->text_tree, screen, NULL, pcb_text_draw_callback, Layer, NULL);
	}

	/* The implicit outline rectangle (or automatic outline rectanlge).
	   We should check for pcb_gui->gui here, but it's kinda cool seeing the
	   auto-outline magically disappear when you first add something to
	   the outline layer.  */
	if ((lflg & PCB_LYT_OUTLINE) && pcb_layer_is_empty_(PCB, Layer)) {
		pcb_gui->set_color(pcb_draw_out.fgGC, Layer->meta.real.color);
		pcb_hid_set_line_cap(pcb_draw_out.fgGC, pcb_cap_round);
		pcb_hid_set_line_width(pcb_draw_out.fgGC, conf_core.design.min_wid);
		pcb_gui->draw_rect(pcb_draw_out.fgGC, 0, 0, PCB->MaxWidth, PCB->MaxHeight);
	}

	out:;
		pcb_draw_out.active_padGC = NULL;
}

/* This version is about 1% slower and used rarely, thus it's all dupped
   from pcb_draw_layer() to keep the original speed there */
void pcb_draw_layer_under(pcb_layer_t *Layer, const pcb_box_t *screen, pcb_data_t *data)
{
	pcb_draw_info_t info;
	pcb_box_t scr2;
	unsigned int lflg = 0;
	pcb_rtree_it_t it;
	pcb_any_obj_t *o;

	if ((screen->X2 <= screen->X1) || (screen->Y2 <= screen->Y1)) {
		scr2 = *screen;
		screen = &scr2;
		if (scr2.X2 <= scr2.X1)
			scr2.X2 = scr2.X1+1;
		if (scr2.Y2 <= scr2.Y1)
			scr2.Y2 = scr2.Y1+1;
	}

	info.drawn_area = screen;
	info.layer = Layer;

	lflg = pcb_layer_flags_(Layer);
	if (PCB_LAYERFLG_ON_VISIBLE_SIDE(lflg))
		pcb_draw_out.active_padGC = pcb_draw_out.padGC;
	else
		pcb_draw_out.active_padGC = pcb_draw_out.backpadGC;

		/* print the non-clearing polys */
	if (Layer->polygon_tree != NULL) {
		if (lflg & PCB_LYT_COPPER) {
			for(o = pcb_rtree_first(&it, Layer->polygon_tree, (pcb_rtree_box_t *)screen); o != NULL; o = pcb_rtree_next(&it))
				if (pcb_obj_is_under(o, data))
					pcb_poly_draw_term_callback((pcb_box_t *)o, &info);
		}
		else {
			for(o = pcb_rtree_first(&it, Layer->polygon_tree, (pcb_rtree_box_t *)screen); o != NULL; o = pcb_rtree_next(&it))
				if (pcb_obj_is_under(o, data))
					pcb_poly_draw_callback((pcb_box_t *)o, &info);
		}
	}

	if (conf_core.editor.check_planes)
		goto out;

	/* draw all visible layer objects (with terminal gfx on copper) */
	if (lflg & PCB_LYT_COPPER) {
		if (Layer->line_tree != NULL)
			for(o = pcb_rtree_first(&it, Layer->line_tree, (pcb_rtree_box_t *)screen); o != NULL; o = pcb_rtree_next(&it))
				if (pcb_obj_is_under(o, data))
					pcb_line_draw_term_callback((pcb_box_t *)o, Layer);
		if (Layer->arc_tree != NULL)
			for(o = pcb_rtree_first(&it, Layer->arc_tree, (pcb_rtree_box_t *)screen); o != NULL; o = pcb_rtree_next(&it))
				if (pcb_obj_is_under(o, data))
					pcb_arc_draw_term_callback((pcb_box_t *)o, Layer);
		if (Layer->text_tree != NULL)
			for(o = pcb_rtree_first(&it, Layer->text_tree, (pcb_rtree_box_t *)screen); o != NULL; o = pcb_rtree_next(&it))
				if (pcb_obj_is_under(o, data))
					pcb_text_draw_term_callback((pcb_box_t *)o, Layer);
	}
	else {
		if (Layer->line_tree != NULL)
			for(o = pcb_rtree_first(&it, Layer->line_tree, (pcb_rtree_box_t *)screen); o != NULL; o = pcb_rtree_next(&it))
				if (pcb_obj_is_under(o, data))
					pcb_line_draw_callback((pcb_box_t *)o, Layer);
		if (Layer->arc_tree != NULL)
			for(o = pcb_rtree_first(&it, Layer->arc_tree, (pcb_rtree_box_t *)screen); o != NULL; o = pcb_rtree_next(&it))
				if (pcb_obj_is_under(o, data))
					pcb_arc_draw_callback((pcb_box_t *)o, Layer);
		if (Layer->text_tree != NULL)
			for(o = pcb_rtree_first(&it, Layer->text_tree, (pcb_rtree_box_t *)screen); o != NULL; o = pcb_rtree_next(&it))
				if (pcb_obj_is_under(o, data))
					pcb_text_draw_callback((pcb_box_t *)o, Layer);
	}

	out:;
		pcb_draw_out.active_padGC = NULL;
}

/* ---------------------------------------------------------------------------
 * draws one layer group.  If the exporter is not a GUI,
 * also draws the padstacks in this layer group.
 */
static void DrawLayerGroup(int group, const pcb_box_t *drawn_area, int is_current)
{
	int i, rv = 1;
	pcb_layer_id_t layernum;
	pcb_layer_t *Layer;
	pcb_cardinal_t n_entries = PCB->LayerGroups.grp[group].len;
	pcb_layer_id_t *layers = PCB->LayerGroups.grp[group].lid;
	unsigned int gflg = pcb_layergrp_flags(PCB, group);

	pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, pcb_draw_out.direct, drawn_area);
	pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, pcb_draw_out.direct, drawn_area);

	if (gflg & PCB_LYT_OUTLINE)
		rv = 0;

	for (i = n_entries - 1; i >= 0; i--) {
		layernum = layers[i];
		Layer = PCB->Data->Layer + layernum;
		if (!(gflg & PCB_LYT_SILK) && Layer->meta.real.vis)
			pcb_draw_layer(Layer, drawn_area);
	}
	if (n_entries > 1)
		rv = 1;

	if ((gflg & PCB_LYT_COPPER) && (PCB->pstk_on))
		pcb_draw_pstks(group, drawn_area, (CURRENT->meta.real.grp == group), 0);

	/* this draws the holes - must be the last, so holes are drawn over everything else */
	if (rv && !pcb_gui->gui)
		pcb_draw_ppv(group, drawn_area);

	pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, pcb_draw_out.direct, drawn_area);
}

void pcb_erase_obj(int type, void *lptr, void *ptr)
{
	if (pcb_hidden_floater((pcb_any_obj_t *)ptr))
		return;

	switch (type) {
	case PCB_OBJ_PSTK:
		pcb_pstk_invalidate_erase((pcb_pstk_t *) ptr);
		break;

	case PCB_OBJ_TEXT:
		pcb_text_invalidate_erase((pcb_layer_t *) lptr, (pcb_text_t *) ptr);
		break;
	case PCB_OBJ_POLY:
		pcb_poly_invalidate_erase((pcb_poly_t *) ptr);
		break;
	case PCB_OBJ_SUBC:
		EraseSubc((pcb_subc_t *)ptr);
		break;
	case PCB_OBJ_LINE:
	case PCB_OBJ_RAT:
		pcb_line_invalidate_erase((pcb_line_t *) ptr);
		break;
	case PCB_OBJ_ARC:
		pcb_arc_invalidate_erase((pcb_arc_t *) ptr);
		break;
	default:
		pcb_message(PCB_MSG_ERROR, "hace: Internal ERROR, trying to erase an unknown type\n");
	}
}


void pcb_draw_obj(pcb_any_obj_t *obj)
{
	if (pcb_hidden_floater(obj))
		return;

	switch (obj->type) {
	case PCB_OBJ_PSTK:
		if (PCB->pstk_on)
			pcb_pstk_invalidate_draw((pcb_pstk_t *)obj);
		break;
	case PCB_OBJ_LINE:
		if (obj->parent.layer->meta.real.vis)
			pcb_line_invalidate_draw(obj->parent.layer, (pcb_line_t *)obj);
		break;
	case PCB_OBJ_ARC:
		if (obj->parent.layer->meta.real.vis)
			pcb_arc_invalidate_draw(obj->parent.layer, (pcb_arc_t *)obj);
		break;
	case PCB_OBJ_TEXT:
		if (obj->parent.layer->meta.real.vis)
			pcb_text_invalidate_draw(obj->parent.layer, (pcb_text_t *)obj);
		break;
	case PCB_OBJ_POLY:
		if (obj->parent.layer->meta.real.vis)
			pcb_poly_invalidate_draw(obj->parent.layer, (pcb_poly_t *)obj);
		break;
	case PCB_OBJ_RAT:
		if (PCB->RatOn)
			pcb_rat_invalidate_draw((pcb_rat_t *)obj);
		break;
	case PCB_OBJ_SUBC:
	case PCB_OBJ_NET:
	case PCB_OBJ_LAYER:
	case PCB_OBJ_LAYERGRP:
	case PCB_OBJ_VOID:
		break;
	}
}

static void pcb_draw_obj_label(pcb_layergrp_id_t gid, pcb_any_obj_t *obj)
{
	if (pcb_hidden_floater(obj))
		return;

	/* do not show layer-object labels of the other side on non-pinout views */
	if ((!pcb_draw_doing_pinout) && (obj->parent_type == PCB_PARENT_LAYER)) {
		pcb_layer_t *ly = pcb_layer_get_real(obj->parent.layer);
		if ((ly != NULL) && (ly->meta.real.grp != gid))
			return;
	}

	switch(obj->type) {
		case PCB_OBJ_LINE:    pcb_line_draw_label((pcb_line_t *)obj); return;
		case PCB_OBJ_ARC:     pcb_arc_draw_label((pcb_arc_t *)obj); return;
		case PCB_OBJ_POLY:    pcb_poly_draw_label((pcb_poly_t *)obj); return;
		case PCB_OBJ_TEXT:    pcb_text_draw_label((pcb_text_t *)obj); return;
		case PCB_OBJ_PSTK:    pcb_pstk_draw_label((pcb_pstk_t *)obj); return;
		default: break;
	}
}

/* ---------------------------------------------------------------------------
 * HID drawing callback.
 */

static pcb_hid_t *expose_begin(pcb_hid_t *hid)
{
	pcb_hid_t *old_gui = pcb_gui;

	delayed_labels_enabled = pcb_true;
	vtp0_truncate(&delayed_labels, 0);

	pcb_gui = hid;
	pcb_draw_out.fgGC = pcb_hid_make_gc();
	pcb_draw_out.padGC = pcb_hid_make_gc();
	pcb_draw_out.backpadGC = pcb_hid_make_gc();
	pcb_draw_out.padselGC = pcb_hid_make_gc();
	pcb_draw_out.drillGC = pcb_hid_make_gc();
	pcb_draw_out.pmGC = pcb_hid_make_gc();

	if (hid->force_compositing)
		pcb_draw_out.direct = 0;
	else
		pcb_draw_out.direct = 1;

	hid->set_color(pcb_draw_out.pmGC, "erase");
	hid->set_color(pcb_draw_out.drillGC, "drill");
	hid->set_color(pcb_draw_out.padGC, conf_core.appearance.color.pin);
	hid->set_color(pcb_draw_out.backpadGC, conf_core.appearance.color.invisible_objects);
	hid->set_color(pcb_draw_out.padselGC, conf_core.appearance.color.pin_selected);
	pcb_hid_set_line_width(pcb_draw_out.backpadGC, -1);
	pcb_hid_set_line_cap(pcb_draw_out.backpadGC, pcb_cap_square);
	pcb_hid_set_line_width(pcb_draw_out.padselGC, -1);
	pcb_hid_set_line_cap(pcb_draw_out.padselGC, pcb_cap_square);
	pcb_hid_set_line_width(pcb_draw_out.padGC, -1);
	pcb_hid_set_line_cap(pcb_draw_out.padGC, pcb_cap_square);

	return old_gui;
}

static void expose_end(pcb_hid_t *old_gui)
{
	pcb_hid_destroy_gc(pcb_draw_out.fgGC);
	pcb_hid_destroy_gc(pcb_draw_out.padGC);
	pcb_hid_destroy_gc(pcb_draw_out.backpadGC);
	pcb_hid_destroy_gc(pcb_draw_out.padselGC);
	pcb_hid_destroy_gc(pcb_draw_out.drillGC);
	pcb_hid_destroy_gc(pcb_draw_out.pmGC);
	pcb_gui = old_gui;

	pcb_draw_out.fgGC = NULL;

	delayed_labels_enabled = pcb_false;
	vtp0_truncate(&delayed_labels, 0);
}

void pcb_hid_expose_all(pcb_hid_t * hid, const pcb_hid_expose_ctx_t *ctx)
{
	if (!pcb_draw_inhibit) {
		pcb_hid_t *old_gui = expose_begin(hid);
		DrawEverything(&ctx->view);
		expose_end(old_gui);
	}
}

void pcb_hid_expose_pinout(pcb_hid_t * hid, const pcb_hid_expose_ctx_t *ctx)
{
	pcb_hid_t *old_gui = expose_begin(hid);
	int fx, fy, fs;

	pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, 1, &ctx->view);
	pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, 1, &ctx->view);

	fx = conf_core.editor.view.flip_x;
	fy = conf_core.editor.view.flip_y;
	fs = conf_core.editor.show_solder_side;

	conf_force_set_bool(conf_core.editor.view.flip_x, 0);
	conf_force_set_bool(conf_core.editor.view.flip_y, 0);
	conf_force_set_bool(conf_core.editor.show_solder_side, 0);

	if (ctx->content.obj != NULL) {
		pcb_draw_doing_pinout = pcb_true;
		switch(ctx->content.obj->type) {
			case PCB_OBJ_SUBC:    pcb_subc_draw_preview((pcb_subc_t *)ctx->content.obj, &ctx->view); break;
			default:              pcb_message(PCB_MSG_ERROR, "pcb_hid_expose_pinout: unknown object type %x\n", ctx->content.obj->type);
		}
		pcb_draw_doing_pinout = pcb_false;
	}
	pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, 1, &ctx->view);

	conf_force_set_bool(conf_core.editor.view.flip_x, fx);
	conf_force_set_bool(conf_core.editor.view.flip_y, fy);
	conf_force_set_bool(conf_core.editor.show_solder_side, fs);

	expose_end(old_gui);
}

void pcb_hid_expose_layer(pcb_hid_t *hid, const pcb_hid_expose_ctx_t *e)
{
	pcb_hid_t *old_gui = expose_begin(hid);
	unsigned long lflg = pcb_layer_flags(PCB, e->content.layer_id);
	int fx, fy;

	if (lflg & PCB_LYT_LOGICAL) {
		fx = conf_core.editor.view.flip_x;
		fy = conf_core.editor.view.flip_y;
		conf_force_set_bool(conf_core.editor.view.flip_x, 0);
		conf_force_set_bool(conf_core.editor.view.flip_y, 0);
	}

	if (lflg & PCB_LYT_CSECT) {
		if ((pcb_layer_gui_set_vlayer(PCB, PCB_VLY_CSECT, 0)) || (e->force)) {
			pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, 1, &e->view);
			pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, 1, &e->view);
			pcb_stub_draw_csect(pcb_draw_out.fgGC, e);
			pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, 1, &e->view);
			pcb_gui->end_layer();
		}
	}
	else if (lflg & PCB_LYT_DIALOG) {
		if ((pcb_layer_gui_set_vlayer(PCB, PCB_VLY_DIALOG, 0)) || (e->force)) {
			pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, 1, &e->view);
			pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, 1, &e->view);
			e->dialog_draw(pcb_draw_out.fgGC, e);
			pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, 1, &e->view);
			pcb_gui->end_layer();
		}
	}
	else if ((e->content.layer_id >= 0) && (e->content.layer_id < pcb_max_layer)) {
		pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, 1, &e->view);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, 1, &e->view);
		pcb_draw_layer(&(PCB->Data->Layer[e->content.layer_id]), &e->view);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, 1, &e->view);
	}
	else
		pcb_message(PCB_MSG_ERROR, "Internal error: don't know how to draw layer %ld for preview; please report this bug.\n", e->content.layer_id);

	expose_end(old_gui);

	if (lflg & PCB_LYT_LOGICAL) {
		conf_force_set_bool(conf_core.editor.view.flip_x, fx);
		conf_force_set_bool(conf_core.editor.view.flip_y, fy);
	}
}

static const char *lab_with_intconn(int intconn, const char *lab, char *buff, int bufflen)
{
	if (intconn <= 0)
		return lab;
	pcb_snprintf(buff, bufflen, "%s[%d]", lab, intconn);
	return buff;
}

/* vert flip magic: make sure the offset is in-line with the flip and our sick y coords for vertical */
#define PCB_TERM_LABEL_SETUP \
	const unsigned char *label; \
	char buff[128]; \
	pcb_bool flip_x = conf_core.editor.view.flip_x; \
	pcb_bool flip_y = conf_core.editor.view.flip_y; \
	pcb_font_t *font = pcb_font(PCB, 0, 0); \
	pcb_coord_t w, h, dx, dy; \
	label = (const unsigned char *)lab_with_intconn(intconn, lab, buff, sizeof(buff)); \
	if (vert) { \
		h = pcb_text_width(font, scale, label); \
		w = pcb_text_height(font, scale, label); \
	} \
	else { \
		w = pcb_text_width(font, scale, label); \
		h = pcb_text_height(font, scale, label); \
	} \
	dx = w / 2; \
	dy = h / 2; \
	if ((vert) && (!(flip_x ^ flip_y))) \
		dy = -dy; \
	if (flip_x) \
		dx = -dx; \
	if (flip_y) \
		dy = -dy; \
	if (centered) { \
		x -= dx; \
		y -= dy; \
	}


void pcb_term_label_draw(pcb_coord_t x, pcb_coord_t y, double scale, pcb_bool vert, pcb_bool centered, const char *lab, int intconn)
{
	int mirror, direction;
	PCB_TERM_LABEL_SETUP;

	mirror = (flip_x ^ flip_y);
	direction = (vert ? 1 : 0) + (flip_x ? 2 : 0);

	pcb_gui->set_color(pcb_draw_out.fgGC, conf_core.appearance.color.pin_name);

	if (pcb_gui->gui)
		pcb_draw_doing_pinout++;
	pcb_text_draw_string(font, label, x, y, scale, direction, mirror, 1, 0, 0, 0, PCB_TXT_TINY_HIDE);
	if (pcb_gui->gui)
		pcb_draw_doing_pinout--;
}

void pcb_term_label_invalidate(pcb_coord_t x, pcb_coord_t y, double scale, pcb_bool vert, pcb_bool centered, const char *lab, int intconn)
{
	pcb_coord_t ox = x, oy = y, margin = 0;
	pcb_box_t b;
	PCB_TERM_LABEL_SETUP;

	dx = PCB_ABS(dx);
	dy = PCB_ABS(dy);
	b.X1 = ox - dx - margin;
	b.X2 = ox + dx + margin;
	b.Y1 = oy - dy - margin;
	b.Y2 = oy + dy + margin;

	pcb_draw_invalidate(&b);
}

