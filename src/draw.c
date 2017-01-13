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
#include "stub_draw_fab.h"
#include "stub_draw_csect.h"
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

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void DrawEverything(const pcb_box_t *);
static void DrawPPV(pcb_layergrp_id_t group, const pcb_box_t * drawn_area);
static void DrawLayerGroup(int, const pcb_box_t *);
static void DrawMask(int side, const pcb_box_t *);
static void DrawRats(const pcb_box_t *);
static void DrawSilk(unsigned int lyt_side, const pcb_box_t *);

#warning TODO: this should be cached
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

	pcb_r_search(PCB->Data->pin_tree, drawn_area, NULL, draw_hole_callback, &plated, NULL);
	pcb_r_search(PCB->Data->via_tree, drawn_area, NULL, draw_hole_callback, &plated, NULL);
}

/* ---------------------------------------------------------------------------
 * prints assembly drawing.
 */
static void PrintAssembly(unsigned int lyt_side, const pcb_box_t * drawn_area)
{
	pcb_layergrp_id_t side_group;

	if (pcb_layer_group_list(PCB_LYT_SILK | lyt_side, &side_group, 1) != 1)
		return;

	pcb_draw_doing_assy = pcb_true;
	pcb_gui->set_draw_faded(Output.fgGC, 1);
	DrawLayerGroup(side_group, drawn_area);
	pcb_gui->set_draw_faded(Output.fgGC, 0);

	/* draw package */
	DrawSilk(lyt_side, drawn_area);
	pcb_draw_doing_assy = pcb_false;
}

static void DrawEverything_holes(const pcb_box_t * drawn_area)
{
	int plated, unplated;
	pcb_board_count_holes(&plated, &unplated, drawn_area);

	if (plated && pcb_layer_gui_set_vlayer(PCB_VLY_PLATED_DRILL, 0)) {
		DrawHoles(pcb_true, pcb_false, drawn_area);
		pcb_gui->end_layer();
	}

	if (unplated && pcb_layer_gui_set_vlayer(PCB_VLY_UNPLATED_DRILL, 0)) {
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
	pcb_layergrp_id_t component, solder, slk[16];
	/* This is the list of layer groups we will draw.  */
	pcb_layergrp_id_t do_group[PCB_MAX_LAYERGRP];
	/* This is the reverse of the order in which we draw them.  */
	pcb_layergrp_id_t drawn_groups[PCB_MAX_LAYERGRP];
	pcb_layer_t *first;

	pcb_bool paste_empty;

	PCB->Data->SILKLAYER.Color = PCB->ElementColor;
	PCB->Data->BACKSILKLAYER.Color = PCB->InvisibleObjectsColor;

	memset(do_group, 0, sizeof(do_group));
	for (ngroups = 0, i = 0; i < pcb_max_layer; i++) {
		pcb_layer_t *l = LAYER_ON_STACK(i);
		pcb_layergrp_id_t group = pcb_layer_get_group(pcb_layer_stack[i]);
		if (l->On && !do_group[group]) {
			do_group[group] = 1;
			drawn_groups[ngroups++] = group;
		}
	}

	solder = component = -1;
	pcb_layer_group_list(PCB_LYT_BOTTOM | PCB_LYT_COPPER, &solder, 1);
	pcb_layer_group_list(PCB_LYT_TOP | PCB_LYT_COPPER, &component, 1);

	/*
	 * first draw all 'invisible' stuff
	 */
	if (!conf_core.editor.check_planes && pcb_layer_gui_set_vlayer(PCB_VLY_INVISIBLE, 0)) {
		side = PCB_SWAP_IDENT ? PCB_COMPONENT_SIDE : PCB_SOLDER_SIDE;
		if (PCB->ElementOn) {
			pcb_layer_id_t lsilk;
			pcb_r_search(PCB->Data->element_tree, drawn_area, NULL, draw_element_callback, &side, NULL);
			pcb_r_search(PCB->Data->name_tree[PCB_ELEMNAME_IDX_VISIBLE()], drawn_area, NULL, draw_element_name_callback, &side, NULL);
			if (pcb_layer_list(PCB_LYT_INVISIBLE_SIDE() | PCB_LYT_SILK, &lsilk, 1) > 0)
				pcb_draw_layer(&(PCB->Data->Layer[lsilk]), drawn_area);
		}
		pcb_r_search(PCB->Data->pad_tree, drawn_area, NULL, draw_pad_callback, &side, NULL);
		pcb_gui->end_layer();
	}

	/* draw all layers in layerstack order */
	for (i = ngroups - 1; i >= 0; i--) {
		pcb_layergrp_id_t group = drawn_groups[i];

		if (pcb_layer_gui_set_glayer(group, 0)) {
			DrawLayerGroup(group, drawn_area);
			pcb_gui->end_layer();
		}
	}

	if (conf_core.editor.check_planes && pcb_gui->gui)
		return;

	/* Draw pins, pads, vias below silk */
	if (pcb_gui->gui)
		DrawPPV(PCB_SWAP_IDENT ? solder : component, drawn_area);
	else if (!pcb_gui->holes_after)
		DrawEverything_holes(drawn_area);

	/* Draw the solder mask if turned on */
	if (pcb_layer_gui_set_vlayer(PCB_VLY_TOP_MASK, 0)) {
		DrawMask(PCB_COMPONENT_SIDE, drawn_area);
		pcb_gui->end_layer();
	}

	if (pcb_layer_gui_set_vlayer(PCB_VLY_BOTTOM_MASK, 0)) {
		DrawMask(PCB_SOLDER_SIDE, drawn_area);
		pcb_gui->end_layer();
	}

	/* Draw silks */
	slk_len = pcb_layer_group_list(PCB_LYT_SILK, slk, sizeof(slk) / sizeof(slk[0]));
	for(i = 0; i < slk_len; i++) {
		if (pcb_layer_gui_set_glayer(slk[i], 0)) {
			unsigned int loc = pcb_layergrp_flags(slk[i]);
			DrawSilk(loc & PCB_LYT_ANYWHERE, drawn_area);
			pcb_gui->end_layer();
		}
	}

	if (pcb_gui->holes_after)
		DrawEverything_holes(drawn_area);

	if (pcb_gui->gui) {
		/* Draw element Marks */
		if (PCB->PinOn)
			pcb_r_search(PCB->Data->element_tree, drawn_area, NULL, draw_element_mark_callback, NULL, NULL);
		/* Draw rat lines on top */
		if (pcb_layer_gui_set_vlayer(PCB_VLY_RATS, 0)) {
			DrawRats(drawn_area);
			pcb_gui->end_layer();
		}
	}

	paste_empty = pcb_layer_is_paste_empty(PCB_COMPONENT_SIDE);
	if (pcb_layer_gui_set_vlayer(PCB_VLY_TOP_PASTE, paste_empty)) {
		DrawPaste(PCB_COMPONENT_SIDE, drawn_area);
		pcb_gui->end_layer();
	}

	paste_empty = pcb_layer_is_paste_empty(PCB_SOLDER_SIDE);
	if (pcb_layer_gui_set_vlayer(PCB_VLY_BOTTOM_PASTE, paste_empty)) {
		DrawPaste(PCB_SOLDER_SIDE, drawn_area);
		pcb_gui->end_layer();
	}

	if (pcb_layer_gui_set_vlayer(PCB_VLY_TOP_ASSY, 0)) {
		PrintAssembly(PCB_LYT_TOP, drawn_area);
		pcb_gui->end_layer();
	}

	if (pcb_layer_gui_set_vlayer(PCB_VLY_BOTTOM_ASSY, 0)) {
		PrintAssembly(PCB_LYT_BOTTOM, drawn_area);
		pcb_gui->end_layer();
	}

	if (pcb_layer_gui_set_vlayer(PCB_VLY_FAB, 0)) {
		pcb_stub_draw_fab(Output.fgGC);
		pcb_gui->end_layer();
	}

	if (pcb_layer_gui_set_vlayer(PCB_VLY_CSECT, 0)) {
		pcb_stub_draw_csect(Output.fgGC);
		pcb_gui->end_layer();
	}

	/* find the first ui layer in use */
	first = NULL;
	for(i = 0; i < vtlayer_len(&pcb_uilayer); i++) {
		if (pcb_uilayer.array[i].cookie != NULL) {
			first = pcb_uilayer.array+i;
			break;
		}
	}

	/* if there's any UI layer, try to draw them */
	if ((first != NULL) && pcb_layer_gui_set_g_ui(first, 0)) {
		for(i = 0; i < vtlayer_len(&pcb_uilayer); i++)
			if ((pcb_uilayer.array[i].cookie != NULL) && (pcb_uilayer.array[i].On))
				pcb_draw_layer(pcb_uilayer.array+i, drawn_area);
		pcb_gui->end_layer();
	}

}

/* ---------------------------------------------------------------------------
 * Draws pins pads and vias - Always draws for non-gui HIDs,
 * otherwise drawing depends on PCB->PinOn and PCB->ViaOn
 */
static void DrawPPV(pcb_layergrp_id_t group, const pcb_box_t * drawn_area)
{
	int side;
	unsigned int gflg = pcb_layergrp_flags(group);

	if (PCB->PinOn || !pcb_gui->gui) {
		/* draw element pins */
		pcb_r_search(PCB->Data->pin_tree, drawn_area, NULL, draw_pin_callback, NULL, NULL);

		/* draw element pads */
		if (gflg & PCB_LYT_TOP) {
			side = PCB_COMPONENT_SIDE;
			pcb_r_search(PCB->Data->pad_tree, drawn_area, NULL, draw_pad_callback, &side, NULL);
		}

		if (gflg & PCB_LYT_BOTTOM) {
			side = PCB_SOLDER_SIDE;
			pcb_r_search(PCB->Data->pad_tree, drawn_area, NULL, draw_pad_callback, &side, NULL);
		}
	}

	/* draw vias */
	if (PCB->ViaOn || !pcb_gui->gui) {
		pcb_r_search(PCB->Data->via_tree, drawn_area, NULL, draw_via_callback, NULL, NULL);
		pcb_r_search(PCB->Data->via_tree, drawn_area, NULL, draw_hole_callback, NULL, NULL);
	}
	if (PCB->PinOn || pcb_draw_doing_assy)
		pcb_r_search(PCB->Data->pin_tree, drawn_area, NULL, draw_hole_callback, NULL, NULL);
}

/* ---------------------------------------------------------------------------
 * Draws silk layer.
 */

static void DrawSilk(unsigned int lyt_side, const pcb_box_t * drawn_area)
{
#if 0
	/* This code is used when you want to mask silk to avoid exposed
	   pins and pads.  We decided it was a bad idea to do this
	   unconditionally, but the code remains.  */
#endif

#if 0
	if (pcb_gui->poly_before) {
		pcb_gui->use_mask(HID_MASK_BEFORE);
#endif
		pcb_layer_id_t lid;
		int side = lyt_side == PCB_LYT_TOP ? PCB_COMPONENT_SIDE : PCB_SOLDER_SIDE;

		if (pcb_layer_list(PCB_LYT_SILK | lyt_side, &lid, 1) == 0)
			return;

		pcb_draw_layer(LAYER_PTR(lid), drawn_area);
		/* draw package */
		pcb_r_search(PCB->Data->element_tree, drawn_area, NULL, draw_element_callback, &side, NULL);
		pcb_r_search(PCB->Data->name_tree[PCB_ELEMNAME_IDX_VISIBLE()], drawn_area, NULL, draw_element_name_callback, &side, NULL);

#if 0
	}

	pcb_gui->use_mask(HID_MASK_CLEAR);
	pcb_r_search(PCB->Data->pin_tree, drawn_area, NULL, clear_pin_callback, NULL, NULL);
	pcb_r_search(PCB->Data->via_tree, drawn_area, NULL, clear_pin_callback, NULL, NULL);
	pcb_r_search(PCB->Data->pad_tree, drawn_area, NULL, clear_pad_callback, &side, NULL);

	if (pcb_gui->poly_after) {
		pcb_layer_id_t lsilk;
		pcb_gui->use_mask(HID_MASK_AFTER);
		if (pcb_layer_list(PCB_LYT_VISIBLE_SIDE() | PCB_LYT_SILK, &lsilk, 1) > 0)
			pcb_draw_layer(LAYER_PTR(lsilk), drawn_area);
		/* draw package */
		pcb_r_search(PCB->Data->element_tree, drawn_area, NULL, draw_element_callback, &side, NULL);
		pcb_r_search(PCB->Data->name_tree[PCB_ELEMNAME_IDX_VISIBLE()], drawn_area, NULL, draw_element_name_callback, &side, NULL);
	}
	pcb_gui->use_mask(HID_MASK_OFF);
#endif
}


static void DrawMaskBoardArea(int mask_type, const pcb_box_t * drawn_area)
{
	/* Skip the mask drawing if the GUI doesn't want this type */
	if ((mask_type == HID_MASK_BEFORE && !pcb_gui->poly_before) || (mask_type == HID_MASK_AFTER && !pcb_gui->poly_after))
		return;

	pcb_gui->use_mask(mask_type);
	pcb_gui->set_color(Output.fgGC, PCB->MaskColor);
	if (drawn_area == NULL)
		pcb_gui->fill_rect(Output.fgGC, 0, 0, PCB->MaxWidth, PCB->MaxHeight);
	else
		pcb_gui->fill_rect(Output.fgGC, drawn_area->X1, drawn_area->Y1, drawn_area->X2, drawn_area->Y2);
}

/* ---------------------------------------------------------------------------
 * draws solder mask layer - this will cover nearly everything
 */
static void DrawMask(int side, const pcb_box_t * screen)
{
	int thin = conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly;

	if (thin)
		pcb_gui->set_color(Output.pmGC, PCB->MaskColor);
	else {
		DrawMaskBoardArea(HID_MASK_BEFORE, screen);
		pcb_gui->use_mask(HID_MASK_CLEAR);
	}

	pcb_r_search(PCB->Data->pin_tree, screen, NULL, clear_pin_callback, NULL, NULL);
	pcb_r_search(PCB->Data->via_tree, screen, NULL, clear_pin_callback, NULL, NULL);
	pcb_r_search(PCB->Data->pad_tree, screen, NULL, clear_pad_callback, &side, NULL);

	if (thin)
		pcb_gui->set_color(Output.pmGC, "erase");
	else {
		DrawMaskBoardArea(HID_MASK_AFTER, screen);
		pcb_gui->use_mask(HID_MASK_OFF);
	}
}

static void DrawRats(const pcb_box_t * drawn_area)
{
	/*
	 * lesstif allows positive AND negative drawing in HID_MASK_CLEAR.
	 * gtk only allows negative drawing.
	 * using the mask here is to get rat transparency
	 */
	if (pcb_gui->can_mask_clear_rats)
		pcb_gui->use_mask(HID_MASK_CLEAR);
	pcb_r_search(PCB->Data->rat_tree, drawn_area, NULL, draw_rat_callback, NULL, NULL);
	if (pcb_gui->can_mask_clear_rats)
		pcb_gui->use_mask(HID_MASK_OFF);
}

void pcb_draw_layer(pcb_layer_t *Layer, const pcb_box_t * screen)
{
	struct pcb_draw_poly_info_s info;
	pcb_box_t scr2;
	pcb_layer_id_t lid;
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


	/* print the non-clearing polys */
	pcb_r_search(Layer->polygon_tree, screen, NULL, draw_poly_callback, &info, NULL);

	if (conf_core.editor.check_planes)
		return;

	/* draw all visible lines this layer */
	pcb_r_search(Layer->line_tree, screen, NULL, draw_line_callback, Layer, NULL);

	/* draw the layer arcs on screen */
	pcb_r_search(Layer->arc_tree, screen, NULL, draw_arc_callback, Layer, NULL);

	/* draw the layer text on screen */
	pcb_r_search(Layer->text_tree, screen, NULL, draw_text_callback, Layer, NULL);

	lid = pcb_layer_id(PCB->Data, Layer);
	if (lid >= 0)
		lflg = pcb_layer_flags(lid);

	/* The implicit outline rectangle (or automatic outline rectanlge).
	   We should check for pcb_gui->gui here, but it's kinda cool seeing the
	   auto-outline magically disappear when you first add something to
	   the outline layer.  */
	if ((lflg & PCB_LYT_OUTLINE) && pcb_layer_is_empty_(Layer)) {
		pcb_gui->set_color(Output.fgGC, Layer->Color);
		pcb_gui->set_line_width(Output.fgGC, PCB->minWid);
		pcb_gui->draw_rect(Output.fgGC, 0, 0, PCB->MaxWidth, PCB->MaxHeight);
	}
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

	for (i = n_entries - 1; i >= 0; i--) {
		layernum = layers[i];
		Layer = PCB->Data->Layer + layernum;
		if (pcb_layer_flags(layernum) & PCB_LYT_OUTLINE)
			rv = 0;
#warning layer TODO: probably enough to check this on the group
		if (!(pcb_layer_flags(layernum) & PCB_LYT_SILK) && Layer->On)
			pcb_draw_layer(Layer, drawn_area);
	}
	if (n_entries > 1)
		rv = 1;

	if (rv && !pcb_gui->gui)
		DrawPPV(group, drawn_area);
}

void pcb_erase_obj(int type, void *lptr, void *ptr)
{
	switch (type) {
	case PCB_TYPE_VIA:
	case PCB_TYPE_PIN:
		ErasePin((pcb_pin_t *) ptr);
		break;
	case PCB_TYPE_TEXT:
	case PCB_TYPE_ELEMENT_NAME:
		EraseText((pcb_layer_t *) lptr, (pcb_text_t *) ptr);
		break;
	case PCB_TYPE_POLYGON:
		ErasePolygon((pcb_polygon_t *) ptr);
		break;
	case PCB_TYPE_ELEMENT:
		EraseElement((pcb_element_t *) ptr);
		break;
	case PCB_TYPE_LINE:
	case PCB_TYPE_ELEMENT_LINE:
	case PCB_TYPE_RATLINE:
		EraseLine((pcb_line_t *) ptr);
		break;
	case PCB_TYPE_PAD:
		ErasePad((pcb_pad_t *) ptr);
		break;
	case PCB_TYPE_ARC:
	case PCB_TYPE_ELEMENT_ARC:
		EraseArc((pcb_arc_t *) ptr);
		break;
	default:
		pcb_message(PCB_MSG_ERROR, "hace: Internal ERROR, trying to erase an unknown type\n");
	}
}


void pcb_draw_obj(int type, void *ptr1, void *ptr2)
{
	switch (type) {
	case PCB_TYPE_VIA:
		if (PCB->ViaOn)
			DrawVia((pcb_pin_t *) ptr2);
		break;
	case PCB_TYPE_LINE:
		if (((pcb_layer_t *) ptr1)->On)
			DrawLine((pcb_layer_t *) ptr1, (pcb_line_t *) ptr2);
		break;
	case PCB_TYPE_ARC:
		if (((pcb_layer_t *) ptr1)->On)
			DrawArc((pcb_layer_t *) ptr1, (pcb_arc_t *) ptr2);
		break;
	case PCB_TYPE_TEXT:
		if (((pcb_layer_t *) ptr1)->On)
			DrawText((pcb_layer_t *) ptr1, (pcb_text_t *) ptr2);
		break;
	case PCB_TYPE_POLYGON:
		if (((pcb_layer_t *) ptr1)->On)
			DrawPolygon((pcb_layer_t *) ptr1, (pcb_polygon_t *) ptr2);
		break;
	case PCB_TYPE_ELEMENT:
		if (PCB->ElementOn && (PCB_FRONT((pcb_element_t *) ptr2) || PCB->InvisibleObjectsOn))
			DrawElement((pcb_element_t *) ptr2);
		break;
	case PCB_TYPE_RATLINE:
		if (PCB->RatOn)
			DrawRat((pcb_rat_t *) ptr2);
		break;
	case PCB_TYPE_PIN:
		if (PCB->PinOn)
			DrawPin((pcb_pin_t *) ptr2);
		break;
	case PCB_TYPE_PAD:
		if (PCB->PinOn)
			DrawPad((pcb_pad_t *) ptr2);
		break;
	case PCB_TYPE_ELEMENT_NAME:
		if (PCB->ElementOn && (PCB_FRONT((pcb_element_t *) ptr2) || PCB->InvisibleObjectsOn))
			DrawElementName((pcb_element_t *) ptr1);
		break;
	}
}

/* ---------------------------------------------------------------------------
 * HID drawing callback.
 */

static pcb_hid_t *expose_begin(pcb_hid_t *hid)
{
	pcb_hid_t *old_gui = pcb_gui;

	pcb_gui = hid;
	Output.fgGC = pcb_gui->make_gc();
	Output.bgGC = pcb_gui->make_gc();
	Output.pmGC = pcb_gui->make_gc();

	hid->set_color(Output.pmGC, "erase");
	hid->set_color(Output.bgGC, "drill");
	return old_gui;
}

static void *expose_end(pcb_hid_t *old_gui)
{
	pcb_gui->destroy_gc(Output.fgGC);
	pcb_gui->destroy_gc(Output.bgGC);
	pcb_gui->destroy_gc(Output.pmGC);
	pcb_gui = old_gui;
}

void pcb_hid_expose_all(pcb_hid_t * hid, void *region)
{
	if (!pcb_draw_inhibit) {
		pcb_hid_t *old_gui = expose_begin(hid);
		DrawEverything((pcb_box_t *)region);
		expose_end(old_gui);
	}
}

void pcb_hid_expose_pinout(pcb_hid_t * hid, void *element)
{
	pcb_hid_t *old_gui = expose_begin(hid);

	pcb_draw_doing_pinout = pcb_true;
	draw_element((pcb_element_t *)element);
	pcb_draw_doing_pinout = pcb_false;

	expose_end(old_gui);
}
