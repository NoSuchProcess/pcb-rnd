/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Contact addresses for paper mail and Email:
 *  Thomas Nau, Schlehenweg 15, 88471 Baustetten, Germany
 *  Thomas.Nau@rz.uni-ulm.de
 *
 */


/* drawing routines
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
#include "obj_all.h"
#include "layer_ui.h"

#include "obj_pad_draw.h"
#include "obj_pinvia_draw.h"
#include "obj_elem_draw.h"
#include "obj_line_draw.h"
#include "obj_arc_draw.h"
#include "obj_rat_draw.h"
#include "obj_poly_draw.h"
#include "obj_text_draw.h"

#undef NDEBUG
#include <assert.h>

#define	SMALL_SMALL_TEXT_SIZE	0
#define	SMALL_TEXT_SIZE			1
#define	NORMAL_TEXT_SIZE		2
#define	LARGE_TEXT_SIZE			3
#define	N_TEXT_SIZES			4

pcb_output_t Output;							/* some widgets ... used for drawing */

/* ---------------------------------------------------------------------------
 * some local identifiers
 */

pcb_box_t pcb_draw_invalidated = { COORD_MAX, COORD_MAX, -COORD_MAX, -COORD_MAX };

int pcb_draw_doing_pinout = 0;
pcb_bool pcb_draw_doing_assy = pcb_false;
static vtp0_t delayed_labels;
pcb_bool delayed_labels_enabled = pcb_false;

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void DrawEverything(const pcb_box_t *);
static void DrawLayerGroup(int, const pcb_box_t *);
static void pcb_draw_obj_label(pcb_any_obj_t *obj);

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

void pcb_draw_dashed_line(pcb_hid_gc_t GC, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
/* TODO: we need a real geo lib... using double here is plain wrong */
	double dx = x2-x1, dy = y2-y1;
	double len_squared = dx*dx + dy*dy;
	int n;
	const int segs = 11; /* must be odd */

	if (len_squared < 1000000) {
		/* line too short, just draw it - TODO: magic value; with a proper geo lib this would be gone anyway */
		pcb_gui->draw_line(GC, x1, y1, x2, y2);
		return;
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

static void DrawHoles(pcb_bool draw_plated, pcb_bool draw_unplated, const pcb_box_t * drawn_area)
{
	int plated = -1;

	if (draw_plated && !draw_unplated)
		plated = 1;
	if (!draw_plated && draw_unplated)
		plated = 0;

	pcb_r_search(PCB->Data->pin_tree, drawn_area, NULL, pcb_hole_draw_callback, &plated, NULL);
	pcb_r_search(PCB->Data->via_tree, drawn_area, NULL, pcb_hole_draw_callback, &plated, NULL);
}

static void DrawEverything_holes(const pcb_box_t * drawn_area)
{
	int plated, unplated;
	pcb_board_count_holes(&plated, &unplated, drawn_area);

	if (plated && pcb_layer_gui_set_vlayer(PCB, PCB_VLY_PLATED_DRILL, 0)) {
		DrawHoles(pcb_true, pcb_false, drawn_area);
		pcb_gui->end_layer();
	}

	if (unplated && pcb_layer_gui_set_vlayer(PCB, PCB_VLY_UNPLATED_DRILL, 0)) {
		DrawHoles(pcb_false, pcb_true, drawn_area);
		pcb_gui->end_layer();
	}
}

/* ---------------------------------------------------------------------------
 * initializes some identifiers for a new zoom factor and redraws whole screen
 */
static void DrawEverything(const pcb_box_t * drawn_area)
{
	int i, ngroups, side, slk_len;
	pcb_layergrp_id_t component, solder, slk[16], gid;
	/* This is the list of layer groups we will draw.  */
	pcb_layergrp_id_t do_group[PCB_MAX_LAYERGRP];
	/* This is the reverse of the order in which we draw them.  */
	pcb_layergrp_id_t drawn_groups[PCB_MAX_LAYERGRP];
	pcb_layer_t *first;
	pcb_hid_expose_ctx_t  hid_exp;
	pcb_bool paste_empty;

	hid_exp.view = *drawn_area;
	hid_exp.force = 0;

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

	/*
	 * first draw all 'invisible' stuff
	 */
	if (!conf_core.editor.check_planes && pcb_layer_gui_set_vlayer(PCB, PCB_VLY_INVISIBLE, 0)) {
		side = PCB_SWAP_IDENT ? PCB_COMPONENT_SIDE : PCB_SOLDER_SIDE;
		pcb_draw_silk(PCB_LYT_INVISIBLE_SIDE(), drawn_area);
		pcb_r_search(PCB->Data->pad_tree, drawn_area, NULL, pcb_pad_draw_callback, &side, NULL);
		pcb_gui->end_layer();
	}

	/* draw all layers in layerstack order */
	for (i = ngroups - 1; i >= 0; i--) {
		pcb_layergrp_id_t group = drawn_groups[i];

		if (pcb_layer_gui_set_glayer(PCB, group, 0)) {
			DrawLayerGroup(group, drawn_area);
			pcb_gui->end_layer();
		}
	}

	if (conf_core.editor.check_planes && pcb_gui->gui)
		goto finish;

	/* Draw pins, pads, vias below silk */
	pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, Output.direct, drawn_area);
	pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, Output.direct, drawn_area);
	if (pcb_gui->gui)
		pcb_draw_ppv(PCB_SWAP_IDENT ? solder : component, drawn_area);
	else if (!pcb_gui->holes_after)
		DrawEverything_holes(drawn_area);
	pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, Output.direct, drawn_area);

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
		pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, Output.direct, drawn_area); 
		pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, Output.direct, drawn_area);
		DrawEverything_holes(drawn_area);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, Output.direct, drawn_area);
	}

	if (pcb_gui->gui) {
		/* Draw pins' and pads' names */
		pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, Output.direct, drawn_area);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, Output.direct, drawn_area);
		pcb_draw_ppv_names(PCB_SWAP_IDENT ? solder : component, drawn_area);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, Output.direct, drawn_area);
		
		/* Draw element Marks */
		pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, Output.direct, drawn_area);
		pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, Output.direct, drawn_area);
	
		if (PCB->PinOn)
			pcb_r_search(PCB->Data->element_tree, drawn_area, NULL, pcb_elem_mark_draw_callback, NULL, NULL);

		if (PCB->SubcOn)
			pcb_r_search(PCB->Data->subc_tree, drawn_area, NULL, draw_subc_mark_callback, NULL, NULL);
		
		pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, Output.direct, drawn_area);

		/* Draw rat lines on top */
		if (pcb_layer_gui_set_vlayer(PCB, PCB_VLY_RATS, 0)) {
			pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, Output.direct, drawn_area);
			pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, Output.direct, drawn_area);
			pcb_draw_rats(drawn_area);
			pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, Output.direct, drawn_area);
			pcb_gui->end_layer();
		}
	}

	paste_empty = pcb_layer_is_paste_empty(PCB, PCB_COMPONENT_SIDE);
	gid = pcb_layergrp_get_top_paste();
	if ((gid >= 0) && (pcb_layer_gui_set_glayer(PCB, gid, paste_empty))) {
		pcb_draw_paste(PCB_COMPONENT_SIDE, drawn_area);
		pcb_gui->end_layer();
	}

	paste_empty = pcb_layer_is_paste_empty(PCB, PCB_SOLDER_SIDE);
	gid = pcb_layergrp_get_bottom_paste();
	if ((gid >= 0) && (pcb_layer_gui_set_glayer(PCB, gid, paste_empty))) {
		pcb_draw_paste(PCB_SOLDER_SIDE, drawn_area);
		pcb_gui->end_layer();
	}

	if (pcb_layer_gui_set_vlayer(PCB, PCB_VLY_TOP_ASSY, 0)) {
		pcb_draw_assembly(PCB_LYT_TOP, drawn_area);
		pcb_gui->end_layer();
	}

	if (pcb_layer_gui_set_vlayer(PCB, PCB_VLY_BOTTOM_ASSY, 0)) {
		pcb_draw_assembly(PCB_LYT_BOTTOM, drawn_area);
		pcb_gui->end_layer();
	}

	if (pcb_layer_gui_set_vlayer(PCB, PCB_VLY_FAB, 0)) {
		pcb_stub_draw_fab(Output.fgGC, &hid_exp);
		pcb_gui->end_layer();
	}

	if (pcb_layer_gui_set_vlayer(PCB, PCB_VLY_CSECT, 0)) {
		pcb_stub_draw_csect(Output.fgGC, &hid_exp);
		pcb_gui->end_layer();
	}

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
					pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, Output.direct, drawn_area);
					pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, Output.direct, drawn_area);
					have_canvas = 1;
				}
				pcb_draw_layer(pcb_uilayer.array+i, drawn_area);
			}
		pcb_gui->end_layer();
		if (have_canvas)
			pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, Output.direct, drawn_area);
	}


	finish:;
	pcb_gui->render_burst(PCB_HID_BURST_END, drawn_area);
}

/* ---------------------------------------------------------------------------
 * Draws pins pads and vias - Always draws for non-gui HIDs,
 * otherwise drawing depends on PCB->PinOn and PCB->ViaOn
 */
void pcb_draw_ppv(pcb_layergrp_id_t group, const pcb_box_t * drawn_area)
{
	int side;
	unsigned int gflg = pcb_layergrp_flags(PCB, group);

	if (PCB->PinOn || !pcb_gui->gui) {
		/* draw element pins */
		pcb_r_search(PCB->Data->pin_tree, drawn_area, NULL, pcb_pin_draw_callback, NULL, NULL);

		/* draw element pads */
		if (gflg & PCB_LYT_TOP) {
			side = PCB_COMPONENT_SIDE;
			pcb_r_search(PCB->Data->pad_tree, drawn_area, NULL, pcb_pad_draw_callback, &side, NULL);
		}

		if (gflg & PCB_LYT_BOTTOM) {
			side = PCB_SOLDER_SIDE;
			pcb_r_search(PCB->Data->pad_tree, drawn_area, NULL, pcb_pad_draw_callback, &side, NULL);
		}
	}

	/* draw vias */
	if (PCB->ViaOn || !pcb_gui->gui) {
		pcb_r_search(PCB->Data->via_tree, drawn_area, NULL, pcb_via_draw_callback, NULL, NULL);
		pcb_r_search(PCB->Data->via_tree, drawn_area, NULL, pcb_hole_draw_callback, NULL, NULL);
	}
	if (PCB->PinOn || pcb_draw_doing_assy)
		pcb_r_search(PCB->Data->pin_tree, drawn_area, NULL, pcb_hole_draw_callback, NULL, NULL);
}

/* ---------------------------------------------------------------------------
 * Draws pins' and pads' names - Always draws for non-gui HIDs,
 * otherwise drawing depends on PCB->PinOn and PCB->ViaOn
 */
void pcb_draw_ppv_names(pcb_layergrp_id_t group, const pcb_box_t * drawn_area)
{
	int side;
	unsigned int gflg = pcb_layergrp_flags(PCB, group);

	if (PCB->PinOn || !pcb_gui->gui) {
		/* draw element pins' names */
		pcb_r_search(PCB->Data->pin_tree, drawn_area, NULL, pcb_pin_name_draw_callback, NULL, NULL);

		/* draw element pads' names */
		if (gflg & PCB_LYT_TOP) {
			side = PCB_COMPONENT_SIDE;
			pcb_r_search(PCB->Data->pad_tree, drawn_area, NULL, pcb_pad_name_draw_callback, &side, NULL);
		}

		if (gflg & PCB_LYT_BOTTOM) {
			side = PCB_SOLDER_SIDE;
			pcb_r_search(PCB->Data->pad_tree, drawn_area, NULL, pcb_pad_name_draw_callback, &side, NULL);
		}
	}

	if (PCB->ViaOn || !pcb_gui->gui) {
		/* draw element pins' names */
		pcb_r_search(PCB->Data->via_tree, drawn_area, NULL, pcb_pin_name_draw_callback, NULL, NULL);
	}

	if (PCB->PinOn || !pcb_gui->gui) {
		size_t n;
		for(n = 0; n < delayed_labels.used; n++)
			pcb_draw_obj_label(delayed_labels.array[n]);
	}
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
		Output.active_padGC = Output.padGC;
	else
		Output.active_padGC = Output.backpadGC;

	if (lflg & PCB_LYT_COPPER) {
		/* print the non-clearing polys */
		pcb_r_search(Layer->polygon_tree, screen, NULL, pcb_poly_draw_term_callback, &info, NULL);
	}
	else {
		/* print the non-clearing polys */
		pcb_r_search(Layer->polygon_tree, screen, NULL, pcb_poly_draw_callback, &info, NULL);
	}

	if (conf_core.editor.check_planes)
		goto out;

	if (lflg & PCB_LYT_COPPER) {
		/* draw all visible lines this layer - with terminal gfx */
		pcb_r_search(Layer->line_tree, screen, NULL, pcb_line_draw_term_callback, Layer, NULL);

		/* draw the layer arcs on screen */
		pcb_r_search(Layer->arc_tree, screen, NULL, pcb_arc_draw_term_callback, Layer, NULL);

		/* draw the layer text on screen */
		pcb_r_search(Layer->text_tree, screen, NULL, pcb_text_draw_term_callback, Layer, NULL);
	}
	else {
		/* draw all visible lines this layer */
		pcb_r_search(Layer->line_tree, screen, NULL, pcb_line_draw_callback, Layer, NULL);

		/* draw the layer arcs on screen */
		pcb_r_search(Layer->arc_tree, screen, NULL, pcb_arc_draw_callback, Layer, NULL);

		/* draw the layer text on screen */
		pcb_r_search(Layer->text_tree, screen, NULL, pcb_text_draw_callback, Layer, NULL);
	}

	/* The implicit outline rectangle (or automatic outline rectanlge).
	   We should check for pcb_gui->gui here, but it's kinda cool seeing the
	   auto-outline magically disappear when you first add something to
	   the outline layer.  */
	if ((lflg & PCB_LYT_OUTLINE) && pcb_layer_is_empty_(PCB, Layer)) {
		pcb_gui->set_color(Output.fgGC, Layer->meta.real.color);
		pcb_gui->set_line_width(Output.fgGC, PCB->minWid);
		pcb_gui->draw_rect(Output.fgGC, 0, 0, PCB->MaxWidth, PCB->MaxHeight);
	}

	out:;
		Output.active_padGC = NULL;
}

/* ---------------------------------------------------------------------------
 * draws one layer group.  If the exporter is not a GUI,
 * also draws the pins / pads / vias in this layer group.
 */
static void DrawLayerGroup(int group, const pcb_box_t * drawn_area)
{
	int i, rv = 1;
	pcb_layer_id_t layernum;
	pcb_layer_t *Layer;
	pcb_cardinal_t n_entries = PCB->LayerGroups.grp[group].len;
	pcb_layer_id_t *layers = PCB->LayerGroups.grp[group].lid;
	unsigned int gflg = pcb_layergrp_flags(PCB, group);

	pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, Output.direct, drawn_area);
	pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, Output.direct, drawn_area);

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

	if (rv && !pcb_gui->gui)
		pcb_draw_ppv(group, drawn_area);

	pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, Output.direct, drawn_area);
}

void pcb_erase_obj(int type, void *lptr, void *ptr)
{
	switch (type) {
	case PCB_TYPE_VIA:
	case PCB_TYPE_PIN:
		pcb_pin_invalidate_erase((pcb_pin_t *) ptr);
		break;
	case PCB_TYPE_TEXT:
	case PCB_TYPE_ELEMENT_NAME:
		pcb_text_invalidate_erase((pcb_layer_t *) lptr, (pcb_text_t *) ptr);
		break;
	case PCB_TYPE_POLYGON:
		pcb_poly_invalidate_erase((pcb_polygon_t *) ptr);
		break;
	case PCB_TYPE_ELEMENT:
		pcb_elem_invalidate_erase((pcb_element_t *) ptr);
		break;
	case PCB_TYPE_SUBC:
		EraseSubc((pcb_subc_t *)ptr);
		break;
	case PCB_TYPE_LINE:
	case PCB_TYPE_ELEMENT_LINE:
	case PCB_TYPE_RATLINE:
		pcb_line_invalidate_erase((pcb_line_t *) ptr);
		break;
	case PCB_TYPE_PAD:
		pcb_pad_invalidate_erase((pcb_pad_t *) ptr);
		break;
	case PCB_TYPE_ARC:
	case PCB_TYPE_ELEMENT_ARC:
		pcb_arc_invalidate_erase((pcb_arc_t *) ptr);
		break;
	default:
		pcb_message(PCB_MSG_ERROR, "hace: Internal ERROR, trying to erase an unknown type\n");
	}
}


void pcb_draw_obj(pcb_any_obj_t *obj)
{
	switch (obj->type) {
	case PCB_OBJ_VIA:
		if (PCB->ViaOn)
			pcb_via_invalidate_draw((pcb_pin_t *)obj);
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
	case PCB_OBJ_POLYGON:
		if (obj->parent.layer->meta.real.vis)
			pcb_poly_invalidate_draw(obj->parent.layer, (pcb_polygon_t *)obj);
		break;
	case PCB_OBJ_ELEMENT:
		if (pcb_silk_on(PCB) && (PCB_FRONT((pcb_element_t *)obj) || PCB->InvisibleObjectsOn))
			pcb_elem_invalidate_draw((pcb_element_t *)obj);
		break;
	case PCB_OBJ_RAT:
		if (PCB->RatOn)
			pcb_rat_invalidate_draw((pcb_rat_t *)obj);
		break;
	case PCB_OBJ_PIN:
		if (PCB->PinOn)
			pcb_pin_invalidate_draw((pcb_pin_t *)obj);
		break;
	case PCB_OBJ_PAD:
		if (PCB->PinOn)
			pcb_pad_invalidate_draw((pcb_pad_t *)obj);
		break;
	case PCB_OBJ_ETEXT:
		if (pcb_silk_on(PCB) && (PCB_FRONT(obj->parent.element) || PCB->InvisibleObjectsOn))
			pcb_elem_name_invalidate_draw(obj->parent.element);
		break;
	case PCB_OBJ_POINT:
	case PCB_OBJ_SUBC:
	case PCB_OBJ_NET:
	case PCB_OBJ_LAYER:
	case PCB_OBJ_ELINE:
	case PCB_OBJ_EARC:
	case PCB_OBJ_CLASS_MASK:
	case PCB_OBJ_CLASS_OBJ:
	case PCB_OBJ_ANY:
		break;
	}
}

static void pcb_draw_obj_label(pcb_any_obj_t *obj)
{
	switch(obj->type) {
		case PCB_OBJ_LINE:    pcb_line_draw_label((pcb_line_t *)obj); return;
		case PCB_OBJ_ARC:     pcb_arc_draw_label((pcb_arc_t *)obj); return;
		case PCB_OBJ_POLYGON: pcb_poly_draw_label((pcb_polygon_t *)obj); return;
		case PCB_OBJ_TEXT:    pcb_text_draw_label((pcb_text_t *)obj); return;
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
	Output.fgGC = pcb_gui->make_gc();
	Output.padGC = pcb_gui->make_gc();
	Output.backpadGC = pcb_gui->make_gc();
	Output.padselGC = pcb_gui->make_gc();
	Output.drillGC = pcb_gui->make_gc();
	Output.pmGC = pcb_gui->make_gc();
	Output.direct = 1;

	hid->set_color(Output.pmGC, "erase");
	hid->set_color(Output.drillGC, "drill");
	hid->set_color(Output.padGC, conf_core.appearance.color.pin);
	hid->set_color(Output.backpadGC, conf_core.appearance.color.invisible_objects);
	hid->set_color(Output.padselGC, conf_core.appearance.color.pin_selected);

	return old_gui;
}

static void expose_end(pcb_hid_t *old_gui)
{
	pcb_gui->destroy_gc(Output.fgGC);
	pcb_gui->destroy_gc(Output.padGC);
	pcb_gui->destroy_gc(Output.backpadGC);
	pcb_gui->destroy_gc(Output.padselGC);
	pcb_gui->destroy_gc(Output.drillGC);
	pcb_gui->destroy_gc(Output.pmGC);
	pcb_gui = old_gui;

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

	pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, 1, &ctx->view);
	pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, 1, &ctx->view);

	pcb_draw_doing_pinout = pcb_true;
	pcb_elem_draw(ctx->content.elem);
	pcb_draw_doing_pinout = pcb_false;
			
	pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, 1, &ctx->view);

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
			pcb_stub_draw_csect(Output.fgGC, e);
			pcb_gui->set_drawing_mode(PCB_HID_COMP_FLUSH, 1, &e->view);
			pcb_gui->end_layer();
		}
	}
	else if (lflg & PCB_LYT_DIALOG) {
		if ((pcb_layer_gui_set_vlayer(PCB, PCB_VLY_DIALOG, 0)) || (e->force)) {
			pcb_gui->set_drawing_mode(PCB_HID_COMP_RESET, 1, &e->view);
			pcb_gui->set_drawing_mode(PCB_HID_COMP_POSITIVE, 1, &e->view);
			e->dialog_draw(Output.fgGC, e);
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

static void pcb_term_label_setup_(pcb_text_t *text, pcb_coord_t x, pcb_coord_t y, double scale, pcb_bool vert, pcb_bool centered, const char *label)
{
	pcb_bool flip_x = conf_core.editor.view.flip_x;
	pcb_bool flip_y = conf_core.editor.view.flip_y;

	text->TextString = (char *)label;
	text->Flags = (flip_x ^ flip_y) ? pcb_flag_make(PCB_FLAG_ONSOLDER) : pcb_no_flags();
	text->X = x;
	text->Y = y;
	text->fid = 0;
	text->Scale = scale;
	text->Direction = (vert ? 1 : 0) + (flip_x ? 2 : 0);

	pcb_text_bbox(NULL, text);

	if (centered) {
		pcb_coord_t dx, dy;
		dx = (text->BoundingBox.X2 - text->BoundingBox.X1) / 2;
		dy = (text->BoundingBox.Y2 - text->BoundingBox.Y1) / 2;

		/* make sure the offset is in-line with the flip and our sick y coords for vertical */
		if ((vert) && (!(flip_x ^ flip_y)))
			dy = -dy;
		if (flip_x)
			dx = -dx;
		if (flip_y)
			dy = -dy;

		text->X -= dx;
		text->Y -= dy;
		text->BoundingBox.X1 -= dx;
		text->BoundingBox.X2 -= dx;
		text->BoundingBox.Y1 -= dy;
		text->BoundingBox.Y2 -= dy;
	}
}

void pcb_term_label_setup(pcb_text_t *text, pcb_coord_t x, pcb_coord_t y, double scale, pcb_bool vert, pcb_bool centered, const char *lab, int intconn)
{
	const char *label;
	char buff[128];

	label = lab_with_intconn(intconn, lab, buff, sizeof(buff));
	pcb_term_label_setup_(text, x, y, scale, vert, centered, label);
}


void pcb_term_label_draw(pcb_coord_t x, pcb_coord_t y, double scale, pcb_bool vert, pcb_bool centered, const char *lab, int intconn)
{
	pcb_text_t text;
	const char *label;
	char buff[128];
	label = lab_with_intconn(intconn, lab, buff, sizeof(buff));

	pcb_gui->set_color(Output.fgGC, conf_core.appearance.color.pin_name);

	pcb_term_label_setup_(&text, x, y, scale, vert, centered, label);

	if (pcb_gui->gui)
		pcb_draw_doing_pinout++;
	pcb_text_draw_(&text, 0, 0);
	if (pcb_gui->gui)
		pcb_draw_doing_pinout--;
}

