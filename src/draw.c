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
#include "obj_all.h"

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

#ifndef MAXINT
#define MAXINT (((unsigned int)(~0))>>1)
#endif

#define	SMALL_SMALL_TEXT_SIZE	0
#define	SMALL_TEXT_SIZE			1
#define	NORMAL_TEXT_SIZE		2
#define	LARGE_TEXT_SIZE			3
#define	N_TEXT_SIZES			4

pcb_output_t Output;							/* some widgets ... used for drawing */

/* ---------------------------------------------------------------------------
 * some local identifiers
 */

pcb_box_t pcb_draw_invalidated = { MAXINT, MAXINT, -MAXINT, -MAXINT };

int pcb_draw_doing_pinout = 0;
pcb_bool pcb_draw_doing_assy = pcb_false;

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void DrawEverything(const pcb_box_t *);
static void DrawPPV(int group, const pcb_box_t *);
static void DrawLayerGroup(int, const pcb_box_t *);
static void DrawMask(int side, const pcb_box_t *);
static void DrawRats(const pcb_box_t *);
static void DrawSilk(int side, const pcb_box_t *);

#warning TODO: this should be cached
void LightenColor(const char *orig, char buf[8], double factor)
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
void Draw(void)
{
	if (pcb_draw_inhibit)
		return;
	if (pcb_draw_invalidated.X1 <= pcb_draw_invalidated.X2 && pcb_draw_invalidated.Y1 <= pcb_draw_invalidated.Y2)
		gui->invalidate_lr(pcb_draw_invalidated.X1, pcb_draw_invalidated.X2, pcb_draw_invalidated.Y1, pcb_draw_invalidated.Y2);

	/* shrink the update block */
	pcb_draw_invalidated.X1 = pcb_draw_invalidated.Y1 = MAXINT;
	pcb_draw_invalidated.X2 = pcb_draw_invalidated.Y2 = -MAXINT;
}

/* ----------------------------------------------------------------------
 * redraws all the data by the event handlers
 */
void Redraw(void)
{
	gui->invalidate_all();
}

static void DrawHoles(pcb_bool draw_plated, pcb_bool draw_unplated, const pcb_box_t * drawn_area)
{
	int plated = -1;

	if (draw_plated && !draw_unplated)
		plated = 1;
	if (!draw_plated && draw_unplated)
		plated = 0;

	r_search(PCB->Data->pin_tree, drawn_area, NULL, draw_hole_callback, &plated, NULL);
	r_search(PCB->Data->via_tree, drawn_area, NULL, draw_hole_callback, &plated, NULL);
}

/* ---------------------------------------------------------------------------
 * prints assembly drawing.
 */
static void PrintAssembly(int side, const pcb_box_t * drawn_area)
{
	int side_group = GetLayerGroupNumberByNumber(max_copper_layer + side);

	pcb_draw_doing_assy = pcb_true;
	gui->set_draw_faded(Output.fgGC, 1);
	DrawLayerGroup(side_group, drawn_area);
	gui->set_draw_faded(Output.fgGC, 0);

	/* draw package */
	DrawSilk(side, drawn_area);
	pcb_draw_doing_assy = pcb_false;
}

static void DrawEverything_holes(const pcb_box_t * drawn_area)
{
	int plated, unplated;
	CountHoles(&plated, &unplated, drawn_area);

	if (plated && gui->set_layer("plated-drill", SL(PDRILL, 0), 0)) {
		DrawHoles(pcb_true, pcb_false, drawn_area);
		gui->end_layer();
	}

	if (unplated && gui->set_layer("unplated-drill", SL(UDRILL, 0), 0)) {
		DrawHoles(pcb_false, pcb_true, drawn_area);
		gui->end_layer();
	}
}

/* ---------------------------------------------------------------------------
 * initializes some identifiers for a new zoom factor and redraws whole screen
 */
static void DrawEverything(const pcb_box_t * drawn_area)
{
	int i, ngroups, side;
	int component, solder;
	/* This is the list of layer groups we will draw.  */
	int do_group[MAX_LAYER];
	/* This is the reverse of the order in which we draw them.  */
	int drawn_groups[MAX_LAYER];

	pcb_bool paste_empty;

	PCB->Data->SILKLAYER.Color = PCB->ElementColor;
	PCB->Data->BACKSILKLAYER.Color = PCB->InvisibleObjectsColor;

	memset(do_group, 0, sizeof(do_group));
	for (ngroups = 0, i = 0; i < max_copper_layer; i++) {
		pcb_layer_t *l = LAYER_ON_STACK(i);
		int group = GetLayerGroupNumberByNumber(LayerStack[i]);
		if (l->On && !do_group[group]) {
			do_group[group] = 1;
			drawn_groups[ngroups++] = group;
		}
	}

	component = GetLayerGroupNumberByNumber(component_silk_layer);
	solder = GetLayerGroupNumberByNumber(solder_silk_layer);

	/*
	 * first draw all 'invisible' stuff
	 */
	if (!conf_core.editor.check_planes
			&& gui->set_layer("invisible", SL(INVISIBLE, 0), 0)) {
		side = SWAP_IDENT ? COMPONENT_LAYER : SOLDER_LAYER;
		if (PCB->ElementOn) {
			r_search(PCB->Data->element_tree, drawn_area, NULL, draw_element_callback, &side, NULL);
			r_search(PCB->Data->name_tree[NAME_INDEX()], drawn_area, NULL, draw_element_name_callback, &side, NULL);
			DrawLayer(&(PCB->Data->Layer[max_copper_layer + side]), drawn_area);
		}
		r_search(PCB->Data->pad_tree, drawn_area, NULL, draw_pad_callback, &side, NULL);
		gui->end_layer();
	}

	/* draw all layers in layerstack order */
	for (i = ngroups - 1; i >= 0; i--) {
		int group = drawn_groups[i];

		if (gui->set_layer(0, group, 0)) {
			DrawLayerGroup(group, drawn_area);
			gui->end_layer();
		}
	}

	if (conf_core.editor.check_planes && gui->gui)
		return;

	/* Draw pins, pads, vias below silk */
	if (gui->gui)
		DrawPPV(SWAP_IDENT ? solder : component, drawn_area);
	else if (!gui->holes_after)
		DrawEverything_holes(drawn_area);

	/* Draw the solder mask if turned on */
	if (gui->set_layer("componentmask", SL(MASK, TOP), 0)) {
		DrawMask(COMPONENT_LAYER, drawn_area);
		gui->end_layer();
	}

	if (gui->set_layer("soldermask", SL(MASK, BOTTOM), 0)) {
		DrawMask(SOLDER_LAYER, drawn_area);
		gui->end_layer();
	}

	if (gui->set_layer("topsilk", SL(SILK, TOP), 0)) {
		DrawSilk(COMPONENT_LAYER, drawn_area);
		gui->end_layer();
	}

	if (gui->set_layer("bottomsilk", SL(SILK, BOTTOM), 0)) {
		DrawSilk(SOLDER_LAYER, drawn_area);
		gui->end_layer();
	}

	if (gui->holes_after)
		DrawEverything_holes(drawn_area);

	if (gui->gui) {
		/* Draw element Marks */
		if (PCB->PinOn)
			r_search(PCB->Data->element_tree, drawn_area, NULL, draw_element_mark_callback, NULL, NULL);
		/* Draw rat lines on top */
		if (gui->set_layer("rats", SL(RATS, 0), 0)) {
			DrawRats(drawn_area);
			gui->end_layer();
		}
	}

	paste_empty = IsPasteEmpty(COMPONENT_LAYER);
	if (gui->set_layer("toppaste", SL(PASTE, TOP), paste_empty)) {
		DrawPaste(COMPONENT_LAYER, drawn_area);
		gui->end_layer();
	}

	paste_empty = IsPasteEmpty(SOLDER_LAYER);
	if (gui->set_layer("bottompaste", SL(PASTE, BOTTOM), paste_empty)) {
		DrawPaste(SOLDER_LAYER, drawn_area);
		gui->end_layer();
	}

	if (gui->set_layer("topassembly", SL(ASSY, TOP), 0)) {
		PrintAssembly(COMPONENT_LAYER, drawn_area);
		gui->end_layer();
	}

	if (gui->set_layer("bottomassembly", SL(ASSY, BOTTOM), 0)) {
		PrintAssembly(SOLDER_LAYER, drawn_area);
		gui->end_layer();
	}

	if (gui->set_layer("fab", SL(FAB, 0), 0)) {
		stub_DrawFab(Output.fgGC);
		gui->end_layer();
	}
}

/* ---------------------------------------------------------------------------
 * Draws pins pads and vias - Always draws for non-gui HIDs,
 * otherwise drawing depends on PCB->PinOn and PCB->ViaOn
 */
static void DrawPPV(int group, const pcb_box_t * drawn_area)
{
	int component_group = GetLayerGroupNumberByNumber(component_silk_layer);
	int solder_group = GetLayerGroupNumberByNumber(solder_silk_layer);
	int side;

	if (PCB->PinOn || !gui->gui) {
		/* draw element pins */
		r_search(PCB->Data->pin_tree, drawn_area, NULL, draw_pin_callback, NULL, NULL);

		/* draw element pads */
		if (group == component_group) {
			side = COMPONENT_LAYER;
			r_search(PCB->Data->pad_tree, drawn_area, NULL, draw_pad_callback, &side, NULL);
		}

		if (group == solder_group) {
			side = SOLDER_LAYER;
			r_search(PCB->Data->pad_tree, drawn_area, NULL, draw_pad_callback, &side, NULL);
		}
	}

	/* draw vias */
	if (PCB->ViaOn || !gui->gui) {
		r_search(PCB->Data->via_tree, drawn_area, NULL, draw_via_callback, NULL, NULL);
		r_search(PCB->Data->via_tree, drawn_area, NULL, draw_hole_callback, NULL, NULL);
	}
	if (PCB->PinOn || pcb_draw_doing_assy)
		r_search(PCB->Data->pin_tree, drawn_area, NULL, draw_hole_callback, NULL, NULL);
}

/* ---------------------------------------------------------------------------
 * Draws silk layer.
 */

static void DrawSilk(int side, const pcb_box_t * drawn_area)
{
#if 0
	/* This code is used when you want to mask silk to avoid exposed
	   pins and pads.  We decided it was a bad idea to do this
	   unconditionally, but the code remains.  */
#endif

#if 0
	if (gui->poly_before) {
		gui->use_mask(HID_MASK_BEFORE);
#endif
		DrawLayer(LAYER_PTR(max_copper_layer + side), drawn_area);
		/* draw package */
		r_search(PCB->Data->element_tree, drawn_area, NULL, draw_element_callback, &side, NULL);
		r_search(PCB->Data->name_tree[NAME_INDEX()], drawn_area, NULL, draw_element_name_callback, &side, NULL);
#if 0
	}

	gui->use_mask(HID_MASK_CLEAR);
	r_search(PCB->Data->pin_tree, drawn_area, NULL, clear_pin_callback, NULL, NULL);
	r_search(PCB->Data->via_tree, drawn_area, NULL, clear_pin_callback, NULL, NULL);
	r_search(PCB->Data->pad_tree, drawn_area, NULL, clear_pad_callback, &side, NULL);

	if (gui->poly_after) {
		gui->use_mask(HID_MASK_AFTER);
		DrawLayer(LAYER_PTR(max_copper_layer + layer), drawn_area);
		/* draw package */
		r_search(PCB->Data->element_tree, drawn_area, NULL, draw_element_callback, &side, NULL);
		r_search(PCB->Data->name_tree[NAME_INDEX()], drawn_area, NULL, draw_element_name_callback, &side, NULL);
	}
	gui->use_mask(HID_MASK_OFF);
#endif
}


static void DrawMaskBoardArea(int mask_type, const pcb_box_t * drawn_area)
{
	/* Skip the mask drawing if the GUI doesn't want this type */
	if ((mask_type == HID_MASK_BEFORE && !gui->poly_before) || (mask_type == HID_MASK_AFTER && !gui->poly_after))
		return;

	gui->use_mask(mask_type);
	gui->set_color(Output.fgGC, PCB->MaskColor);
	if (drawn_area == NULL)
		gui->fill_rect(Output.fgGC, 0, 0, PCB->MaxWidth, PCB->MaxHeight);
	else
		gui->fill_rect(Output.fgGC, drawn_area->X1, drawn_area->Y1, drawn_area->X2, drawn_area->Y2);
}

/* ---------------------------------------------------------------------------
 * draws solder mask layer - this will cover nearly everything
 */
static void DrawMask(int side, const pcb_box_t * screen)
{
	int thin = conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly;

	if (thin)
		gui->set_color(Output.pmGC, PCB->MaskColor);
	else {
		DrawMaskBoardArea(HID_MASK_BEFORE, screen);
		gui->use_mask(HID_MASK_CLEAR);
	}

	r_search(PCB->Data->pin_tree, screen, NULL, clear_pin_callback, NULL, NULL);
	r_search(PCB->Data->via_tree, screen, NULL, clear_pin_callback, NULL, NULL);
	r_search(PCB->Data->pad_tree, screen, NULL, clear_pad_callback, &side, NULL);

	if (thin)
		gui->set_color(Output.pmGC, "erase");
	else {
		DrawMaskBoardArea(HID_MASK_AFTER, screen);
		gui->use_mask(HID_MASK_OFF);
	}
}

static void DrawRats(const pcb_box_t * drawn_area)
{
	/*
	 * XXX lesstif allows positive AND negative drawing in HID_MASK_CLEAR.
	 * XXX gtk only allows negative drawing.
	 * XXX using the mask here is to get rat transparency
	 */
#warning TODO: make this a pcb_hid_t struct item instead
	int can_mask = strcmp(gui->name, "lesstif") == 0;

	if (can_mask)
		gui->use_mask(HID_MASK_CLEAR);
	r_search(PCB->Data->rat_tree, drawn_area, NULL, draw_rat_callback, NULL, NULL);
	if (can_mask)
		gui->use_mask(HID_MASK_OFF);
}

void DrawLayer(pcb_layer_t *Layer, const pcb_box_t * screen)
{
	struct pcb_draw_poly_info_s info;

	info.drawn_area = screen;
	info.layer = Layer;

	/* print the non-clearing polys */
	r_search(Layer->polygon_tree, screen, NULL, draw_poly_callback, &info, NULL);

	if (conf_core.editor.check_planes)
		return;

	/* draw all visible lines this layer */
	r_search(Layer->line_tree, screen, NULL, draw_line_callback, Layer, NULL);

	/* draw the layer arcs on screen */
	r_search(Layer->arc_tree, screen, NULL, draw_arc_callback, Layer, NULL);

	/* draw the layer text on screen */
	r_search(Layer->text_tree, screen, NULL, draw_text_callback, Layer, NULL);

	/* We should check for gui->gui here, but it's kinda cool seeing the
	   auto-outline magically disappear when you first add something to
	   the "outline" layer.  */
	if (IsLayerEmpty(Layer)
			&& (strcmp(Layer->Name, "outline") == 0 || strcmp(Layer->Name, "route") == 0)) {
		gui->set_color(Output.fgGC, Layer->Color);
		gui->set_line_width(Output.fgGC, PCB->minWid);
		gui->draw_rect(Output.fgGC, 0, 0, PCB->MaxWidth, PCB->MaxHeight);
	}
}

/* ---------------------------------------------------------------------------
 * draws one layer group.  If the exporter is not a GUI,
 * also draws the pins / pads / vias in this layer group.
 */
static void DrawLayerGroup(int group, const pcb_box_t * drawn_area)
{
	int i, rv = 1;
	int layernum;
	pcb_layer_t *Layer;
	int n_entries = PCB->LayerGroups.Number[group];
	pcb_cardinal_t *layers = PCB->LayerGroups.Entries[group];

	for (i = n_entries - 1; i >= 0; i--) {
		layernum = layers[i];
		Layer = PCB->Data->Layer + layers[i];
		if (strcmp(Layer->Name, "outline") == 0 || strcmp(Layer->Name, "route") == 0)
			rv = 0;
		if (layernum < max_copper_layer && Layer->On)
			DrawLayer(Layer, drawn_area);
	}
	if (n_entries > 1)
		rv = 1;

	if (rv && !gui->gui)
		DrawPPV(group, drawn_area);
}

void EraseObject(int type, void *lptr, void *ptr)
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
		Message(PCB_MSG_DEFAULT, "hace: Internal ERROR, trying to erase an unknown type\n");
	}
}


void DrawObject(int type, void *ptr1, void *ptr2)
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
		if (PCB->ElementOn && (FRONT((pcb_element_t *) ptr2) || PCB->InvisibleObjectsOn))
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
		if (PCB->ElementOn && (FRONT((pcb_element_t *) ptr2) || PCB->InvisibleObjectsOn))
			DrawElementName((pcb_element_t *) ptr1);
		break;
	}
}

/* ---------------------------------------------------------------------------
 * HID drawing callback.
 */

void hid_expose_callback(pcb_hid_t * hid, pcb_box_t * region, void *item)
{
	pcb_hid_t *old_gui = gui;

	gui = hid;
	Output.fgGC = gui->make_gc();
	Output.bgGC = gui->make_gc();
	Output.pmGC = gui->make_gc();

	hid->set_color(Output.pmGC, "erase");
	hid->set_color(Output.bgGC, "drill");

	if (item) {
		pcb_draw_doing_pinout = pcb_true;
		draw_element((pcb_element_t *) item);
		pcb_draw_doing_pinout = pcb_false;
	}
	else
		DrawEverything(region);

	gui->destroy_gc(Output.fgGC);
	gui->destroy_gc(Output.bgGC);
	gui->destroy_gc(Output.pmGC);
	gui = old_gui;
}
