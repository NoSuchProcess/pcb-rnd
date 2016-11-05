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
#include "misc.h"
#include "rotate.h"
#include "rtree.h"
#include "stub_draw_fab.h"
#include "obj_all.h"

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

OutputType Output;							/* some widgets ... used for drawing */

/* ---------------------------------------------------------------------------
 * some local identifiers
 */
static BoxType Block = { MAXINT, MAXINT, -MAXINT, -MAXINT };

static int doing_pinout = 0;
static pcb_bool doing_assy = pcb_false;

/* ---------------------------------------------------------------------------
 * some local prototypes
 */
static void DrawEverything(const BoxType *);
static void DrawPPV(int group, const BoxType *);
static void DrawLayerGroup(int, const BoxType *);
static void AddPart(void *);
static void SetPVColor(PinTypePtr, int);
static void DrawEMark(ElementTypePtr, Coord, Coord, pcb_bool);
static void DrawMask(int side, const BoxType *);
static void DrawPaste(int side, const BoxType *);
static void DrawRats(const BoxType *);
static void DrawSilk(int side, const BoxType *);

static void LightenColor(const char *orig, char buf[8], double factor)
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


/*--------------------------------------------------------------------------------------
 * setup color for pin or via
 */
static void SetPVColor(PinTypePtr Pin, int Type)
{
	char *color;
	char buf[sizeof("#XXXXXX")];

	if (Type == PCB_TYPE_VIA) {
		if (!doing_pinout && TEST_FLAG(PCB_FLAG_WARN | PCB_FLAG_SELECTED | PCB_FLAG_FOUND, Pin)) {
			if (TEST_FLAG(PCB_FLAG_WARN, Pin))
				color = PCB->WarnColor;
			else if (TEST_FLAG(PCB_FLAG_SELECTED, Pin))
				color = PCB->ViaSelectedColor;
			else
				color = PCB->ConnectedColor;

			if (TEST_FLAG(PCB_FLAG_ONPOINT, Pin)) {
				assert(color != NULL);
				LightenColor(color, buf, 1.75);
				color = buf;
			}
		}
		else
			color = PCB->ViaColor;
	}
	else {
		if (!doing_pinout && TEST_FLAG(PCB_FLAG_WARN | PCB_FLAG_SELECTED | PCB_FLAG_FOUND, Pin)) {
			if (TEST_FLAG(PCB_FLAG_WARN, Pin))
				color = PCB->WarnColor;
			else if (TEST_FLAG(PCB_FLAG_SELECTED, Pin))
				color = PCB->PinSelectedColor;
			else
				color = PCB->ConnectedColor;

			if (TEST_FLAG(PCB_FLAG_ONPOINT, Pin)) {
				assert(color != NULL);
				LightenColor(color, buf, 1.75);
				color = buf;
			}
		}
		else
			color = PCB->PinColor;
	}

	gui->set_color(Output.fgGC, color);
}

/*---------------------------------------------------------------------------
 *  Adds the update rect to the update region
 */
static void AddPart(void *b)
{
	BoxType *box = (BoxType *) b;

	Block.X1 = MIN(Block.X1, box->X1);
	Block.X2 = MAX(Block.X2, box->X2);
	Block.Y1 = MIN(Block.Y1, box->Y1);
	Block.Y2 = MAX(Block.Y2, box->Y2);
}

/*
 * initiate the actual redrawing of the updated area
 */
pcb_cardinal_t pcb_draw_inhibit = 0;
void Draw(void)
{
	if (pcb_draw_inhibit)
		return;
	if (Block.X1 <= Block.X2 && Block.Y1 <= Block.Y2)
		gui->invalidate_lr(Block.X1, Block.X2, Block.Y1, Block.Y2);

	/* shrink the update block */
	Block.X1 = Block.Y1 = MAXINT;
	Block.X2 = Block.Y2 = -MAXINT;
}

/* ----------------------------------------------------------------------
 * redraws all the data by the event handlers
 */
void Redraw(void)
{
	gui->invalidate_all();
}

static void _draw_pv_name(PinType * pv)
{
	BoxType box;
	pcb_bool vert;
	TextType text;
	char buff[128];
	const char *pn;

	if (!pv->Name || !pv->Name[0])
		pn = EMPTY(pv->Number);
	else
		pn = EMPTY(conf_core.editor.show_number ? pv->Number : pv->Name);

	if (GET_INTCONN(pv) > 0)
		pcb_snprintf(buff, sizeof(buff), "%s[%d]", pn, GET_INTCONN(pv));
	else
		strcpy(buff, pn);
	text.TextString = buff;

	vert = TEST_FLAG(PCB_FLAG_EDGE2, pv);

	if (vert) {
		box.X1 = pv->X - pv->Thickness / 2 + conf_core.appearance.pinout.text_offset_y;
		box.Y1 = pv->Y - pv->DrillingHole / 2 - conf_core.appearance.pinout.text_offset_x;
	}
	else {
		box.X1 = pv->X + pv->DrillingHole / 2 + conf_core.appearance.pinout.text_offset_x;
		box.Y1 = pv->Y - pv->Thickness / 2 + conf_core.appearance.pinout.text_offset_y;
	}

	gui->set_color(Output.fgGC, PCB->PinNameColor);

	text.Flags = NoFlags();
	/* Set font height to approx 56% of pin thickness */
	text.Scale = 56 * pv->Thickness / FONT_CAPHEIGHT;
	text.X = box.X1;
	text.Y = box.Y1;
	text.Direction = vert ? 1 : 0;

	if (gui->gui)
		doing_pinout++;
	DrawTextLowLevel(&text, 0);
	if (gui->gui)
		doing_pinout--;
}

static void _draw_pv(PinTypePtr pv, pcb_bool draw_hole)
{
	if (conf_core.editor.thin_draw)
		gui->thindraw_pcb_pv(Output.fgGC, Output.fgGC, pv, draw_hole, pcb_false);
	else
		gui->fill_pcb_pv(Output.fgGC, Output.bgGC, pv, draw_hole, pcb_false);

	if (!TEST_FLAG(PCB_FLAG_HOLE, pv) && TEST_FLAG(PCB_FLAG_DISPLAYNAME, pv))
		_draw_pv_name(pv);
}

static void draw_pin(PinTypePtr pin, pcb_bool draw_hole)
{
	SetPVColor(pin, PCB_TYPE_PIN);
	_draw_pv(pin, draw_hole);
}

static r_dir_t pin_callback(const BoxType * b, void *cl)
{
	draw_pin((PinType *) b, pcb_false);
	return R_DIR_FOUND_CONTINUE;
}

static void draw_via(PinTypePtr via, pcb_bool draw_hole)
{
	SetPVColor(via, PCB_TYPE_VIA);
	_draw_pv(via, draw_hole);
}

static r_dir_t via_callback(const BoxType * b, void *cl)
{
	draw_via((PinType *) b, pcb_false);
	return R_DIR_FOUND_CONTINUE;
}

static void draw_pad_name(PadType * pad)
{
	BoxType box;
	pcb_bool vert;
	TextType text;
	char buff[128];
	const char *pn;

	if (!pad->Name || !pad->Name[0])
		pn = EMPTY(pad->Number);
	else
		pn = conf_core.editor.show_number ? pad->Number : pad->Name;

	if (GET_INTCONN(pad) > 0)
		pcb_snprintf(buff, sizeof(buff), "%s[%d]", pn, GET_INTCONN(pad));
	else
		strcpy(buff, pn);
	text.TextString = buff;

	/* should text be vertical ? */
	vert = (pad->Point1.X == pad->Point2.X);

	if (vert) {
		box.X1 = pad->Point1.X - pad->Thickness / 2;
		box.Y1 = MAX(pad->Point1.Y, pad->Point2.Y) + pad->Thickness / 2;
		box.X1 += conf_core.appearance.pinout.text_offset_y;
		box.Y1 -= conf_core.appearance.pinout.text_offset_x;
	}
	else {
		box.X1 = MIN(pad->Point1.X, pad->Point2.X) - pad->Thickness / 2;
		box.Y1 = pad->Point1.Y - pad->Thickness / 2;
		box.X1 += conf_core.appearance.pinout.text_offset_x;
		box.Y1 += conf_core.appearance.pinout.text_offset_y;
	}

	gui->set_color(Output.fgGC, PCB->PinNameColor);

	text.Flags = NoFlags();
	/* Set font height to approx 90% of pin thickness */
	text.Scale = 90 * pad->Thickness / FONT_CAPHEIGHT;
	text.X = box.X1;
	text.Y = box.Y1;
	text.Direction = vert ? 1 : 0;

	DrawTextLowLevel(&text, 0);
}

static void _draw_pad(hidGC gc, PadType * pad, pcb_bool clear, pcb_bool mask)
{
	if (clear && !mask && pad->Clearance <= 0)
		return;

	if (conf_core.editor.thin_draw || (clear && conf_core.editor.thin_draw_poly))
		gui->thindraw_pcb_pad(gc, pad, clear, mask);
	else
		gui->fill_pcb_pad(gc, pad, clear, mask);
}

static void draw_pad(PadType * pad)
{
	const char *color = NULL;
	char buf[sizeof("#XXXXXX")];

	if (doing_pinout)
		gui->set_color(Output.fgGC, PCB->PinColor);
	else if (TEST_FLAG(PCB_FLAG_WARN | PCB_FLAG_SELECTED | PCB_FLAG_FOUND, pad)) {
		if (TEST_FLAG(PCB_FLAG_WARN, pad))
			color = PCB->WarnColor;
		else if (TEST_FLAG(PCB_FLAG_SELECTED, pad))
			color = PCB->PinSelectedColor;
		else
			color = PCB->ConnectedColor;
	}
	else if (FRONT(pad))
		color = PCB->PinColor;
	else
		color = PCB->InvisibleObjectsColor;

	if (TEST_FLAG(PCB_FLAG_ONPOINT, pad)) {
		assert(color != NULL);
		LightenColor(color, buf, 1.75);
		color = buf;
	}

	if (color != NULL)
		gui->set_color(Output.fgGC, color);

	_draw_pad(Output.fgGC, pad, pcb_false, pcb_false);

	if (doing_pinout || TEST_FLAG(PCB_FLAG_DISPLAYNAME, pad))
		draw_pad_name(pad);
}

static r_dir_t pad_callback(const BoxType * b, void *cl)
{
	PadTypePtr pad = (PadTypePtr) b;
	int *side = cl;

	if (ON_SIDE(pad, *side))
		draw_pad(pad);
	return R_DIR_FOUND_CONTINUE;
}

static void draw_element_name(ElementType * element)
{
	if ((conf_core.editor.hide_names && gui->gui) || TEST_FLAG(PCB_FLAG_HIDENAME, element))
		return;
	if (doing_pinout || doing_assy)
		gui->set_color(Output.fgGC, PCB->ElementColor);
	else if (TEST_FLAG(PCB_FLAG_SELECTED, &ELEMENT_TEXT(PCB, element)))
		gui->set_color(Output.fgGC, PCB->ElementSelectedColor);
	else if (FRONT(element)) {
/* TODO: why do we test for Name's flag here? */
		if (TEST_FLAG(PCB_FLAG_NONETLIST, element))
			gui->set_color(Output.fgGC, PCB->ElementColor_nonetlist);
		else
			gui->set_color(Output.fgGC, PCB->ElementColor);
	}
	else
		gui->set_color(Output.fgGC, PCB->InvisibleObjectsColor);

	DrawTextLowLevel(&ELEMENT_TEXT(PCB, element), PCB->minSlk);

}

static r_dir_t name_callback(const BoxType * b, void *cl)
{
	TextTypePtr text = (TextTypePtr) b;
	ElementTypePtr element = (ElementTypePtr) text->Element;
	int *side = cl;

	if (TEST_FLAG(PCB_FLAG_HIDENAME, element))
		return R_DIR_NOT_FOUND;

	if (ON_SIDE(element, *side))
		draw_element_name(element);
	return R_DIR_NOT_FOUND;
}

static void draw_element_pins_and_pads(ElementType * element)
{
	PAD_LOOP(element);
	{
		if (doing_pinout || doing_assy || FRONT(pad) || PCB->InvisibleObjectsOn)
			draw_pad(pad);
	}
	END_LOOP;
	PIN_LOOP(element);
	{
		draw_pin(pin, pcb_true);
	}
	END_LOOP;
}

static r_dir_t EMark_callback(const BoxType * b, void *cl)
{
	ElementTypePtr element = (ElementTypePtr) b;

	DrawEMark(element, element->MarkX, element->MarkY, !FRONT(element));
	return R_DIR_FOUND_CONTINUE;
}

static r_dir_t hole_callback(const BoxType * b, void *cl)
{
	PinTypePtr pv = (PinTypePtr) b;
	int plated = cl ? *(int *) cl : -1;
	const char *color;
	char buf[sizeof("#XXXXXX")];

	if ((plated == 0 && !TEST_FLAG(PCB_FLAG_HOLE, pv)) || (plated == 1 && TEST_FLAG(PCB_FLAG_HOLE, pv)))
		return R_DIR_FOUND_CONTINUE;

	if (conf_core.editor.thin_draw) {
		if (!TEST_FLAG(PCB_FLAG_HOLE, pv)) {
			gui->set_line_cap(Output.fgGC, Round_Cap);
			gui->set_line_width(Output.fgGC, 0);
			gui->draw_arc(Output.fgGC, pv->X, pv->Y, pv->DrillingHole / 2, pv->DrillingHole / 2, 0, 360);
		}
	}
	else
		gui->fill_circle(Output.bgGC, pv->X, pv->Y, pv->DrillingHole / 2);

	if (TEST_FLAG(PCB_FLAG_HOLE, pv)) {
		if (TEST_FLAG(PCB_FLAG_WARN, pv))
			color = PCB->WarnColor;
		else if (TEST_FLAG(PCB_FLAG_SELECTED, pv))
			color = PCB->ViaSelectedColor;
		else
			color = conf_core.appearance.color.black;

		if (TEST_FLAG(PCB_FLAG_ONPOINT, pv)) {
			assert(color != NULL);
			LightenColor(color, buf, 1.75);
			color = buf;
		}
		gui->set_color(Output.fgGC, color);

		gui->set_line_cap(Output.fgGC, Round_Cap);
		gui->set_line_width(Output.fgGC, 0);
		gui->draw_arc(Output.fgGC, pv->X, pv->Y, pv->DrillingHole / 2, pv->DrillingHole / 2, 0, 360);
	}
	return R_DIR_FOUND_CONTINUE;
}

static void DrawHoles(pcb_bool draw_plated, pcb_bool draw_unplated, const BoxType * drawn_area)
{
	int plated = -1;

	if (draw_plated && !draw_unplated)
		plated = 1;
	if (!draw_plated && draw_unplated)
		plated = 0;

	r_search(PCB->Data->pin_tree, drawn_area, NULL, hole_callback, &plated, NULL);
	r_search(PCB->Data->via_tree, drawn_area, NULL, hole_callback, &plated, NULL);
}

static void _draw_line(LineType * line)
{
	gui->set_line_cap(Output.fgGC, Trace_Cap);
	if (conf_core.editor.thin_draw)
		gui->set_line_width(Output.fgGC, 0);
	else
		gui->set_line_width(Output.fgGC, line->Thickness);

	gui->draw_line(Output.fgGC, line->Point1.X, line->Point1.Y, line->Point2.X, line->Point2.Y);
}

static void draw_line(LayerType * layer, LineType * line)
{
	const char *color;
	char buf[sizeof("#XXXXXX")];

	if (TEST_FLAG(PCB_FLAG_WARN, line))
		color = PCB->WarnColor;
	else if (TEST_FLAG(PCB_FLAG_SELECTED | PCB_FLAG_FOUND, line)) {
		if (TEST_FLAG(PCB_FLAG_SELECTED, line))
			color = layer->SelectedColor;
		else
			color = PCB->ConnectedColor;
	}
	else
		color = layer->Color;

	if (TEST_FLAG(PCB_FLAG_ONPOINT, line)) {
		assert(color != NULL);
		LightenColor(color, buf, 1.75);
		color = buf;
	}

	gui->set_color(Output.fgGC, color);
	_draw_line(line);
}

static r_dir_t line_callback(const BoxType * b, void *cl)
{
	draw_line((LayerType *) cl, (LineType *) b);
	return R_DIR_FOUND_CONTINUE;
}

static r_dir_t rat_callback(const BoxType * b, void *cl)
{
	RatType *rat = (RatType *) b;

	if (TEST_FLAG(PCB_FLAG_SELECTED | PCB_FLAG_FOUND, rat)) {
		if (TEST_FLAG(PCB_FLAG_SELECTED, rat))
			gui->set_color(Output.fgGC, PCB->RatSelectedColor);
		else
			gui->set_color(Output.fgGC, PCB->ConnectedColor);
	}
	else
		gui->set_color(Output.fgGC, PCB->RatColor);

	if (conf_core.appearance.rat_thickness < 20)
		rat->Thickness = pixel_slop * conf_core.appearance.rat_thickness;
	/* rats.c set PCB_FLAG_VIA if this rat goes to a containing poly: draw a donut */
	if (TEST_FLAG(PCB_FLAG_VIA, rat)) {
		int w = rat->Thickness;

		if (conf_core.editor.thin_draw)
			gui->set_line_width(Output.fgGC, 0);
		else
			gui->set_line_width(Output.fgGC, w);
		gui->draw_arc(Output.fgGC, rat->Point1.X, rat->Point1.Y, w * 2, w * 2, 0, 360);
	}
	else
		_draw_line((LineType *) rat);
	return R_DIR_FOUND_CONTINUE;
}

static void _draw_arc(ArcType * arc)
{
	if (!arc->Thickness)
		return;

	if (conf_core.editor.thin_draw)
		gui->set_line_width(Output.fgGC, 0);
	else
		gui->set_line_width(Output.fgGC, arc->Thickness);
	gui->set_line_cap(Output.fgGC, Trace_Cap);

	gui->draw_arc(Output.fgGC, arc->X, arc->Y, arc->Width, arc->Height, arc->StartAngle, arc->Delta);
}

static void draw_arc(LayerType * layer, ArcType * arc)
{
	const char *color;
	char buf[sizeof("#XXXXXX")];

	if (TEST_FLAG(PCB_FLAG_WARN, arc))
		color = PCB->WarnColor;
	else if (TEST_FLAG(PCB_FLAG_SELECTED | PCB_FLAG_FOUND, arc)) {
		if (TEST_FLAG(PCB_FLAG_SELECTED, arc))
			color = layer->SelectedColor;
		else
			color = PCB->ConnectedColor;
	}
	else
		color = layer->Color;

	if (TEST_FLAG(PCB_FLAG_ONPOINT, arc)) {
		assert(color != NULL);
		LightenColor(color, buf, 1.75);
		color = buf;
	}
	gui->set_color(Output.fgGC, color);
	_draw_arc(arc);
}

static r_dir_t arc_callback(const BoxType * b, void *cl)
{
	draw_arc((LayerTypePtr) cl, (ArcTypePtr) b);
	return R_DIR_FOUND_CONTINUE;
}

static void draw_element_package(ElementType * element)
{
	/* set color and draw lines, arcs, text and pins */
	if (doing_pinout || doing_assy)
		gui->set_color(Output.fgGC, PCB->ElementColor);
	else if (TEST_FLAG(PCB_FLAG_SELECTED, element))
		gui->set_color(Output.fgGC, PCB->ElementSelectedColor);
	else if (FRONT(element))
		gui->set_color(Output.fgGC, PCB->ElementColor);
	else
		gui->set_color(Output.fgGC, PCB->InvisibleObjectsColor);

	/* draw lines, arcs, text and pins */
	ELEMENTLINE_LOOP(element);
	{
		_draw_line(line);
	}
	END_LOOP;
	ARC_LOOP(element);
	{
		_draw_arc(arc);
	}
	END_LOOP;
}

static r_dir_t element_callback(const BoxType * b, void *cl)
{
	ElementTypePtr element = (ElementTypePtr) b;
	int *side = cl;

	if (ON_SIDE(element, *side))
		draw_element_package(element);
	return R_DIR_FOUND_CONTINUE;
}

/* ---------------------------------------------------------------------------
 * prints assembly drawing.
 */

static void PrintAssembly(int side, const BoxType * drawn_area)
{
	int side_group = GetLayerGroupNumberByNumber(max_copper_layer + side);

	doing_assy = pcb_true;
	gui->set_draw_faded(Output.fgGC, 1);
	DrawLayerGroup(side_group, drawn_area);
	gui->set_draw_faded(Output.fgGC, 0);

	/* draw package */
	DrawSilk(side, drawn_area);
	doing_assy = pcb_false;
}

static void DrawEverything_holes(const BoxType * drawn_area)
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
static void DrawEverything(const BoxType * drawn_area)
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
		LayerType *l = LAYER_ON_STACK(i);
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
			r_search(PCB->Data->element_tree, drawn_area, NULL, element_callback, &side, NULL);
			r_search(PCB->Data->name_tree[NAME_INDEX()], drawn_area, NULL, name_callback, &side, NULL);
			DrawLayer(&(PCB->Data->Layer[max_copper_layer + side]), drawn_area);
		}
		r_search(PCB->Data->pad_tree, drawn_area, NULL, pad_callback, &side, NULL);
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
			r_search(PCB->Data->element_tree, drawn_area, NULL, EMark_callback, NULL, NULL);
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

static void DrawEMark(ElementTypePtr e, Coord X, Coord Y, pcb_bool invisible)
{
	Coord mark_size = EMARK_SIZE;
	if (!PCB->InvisibleObjectsOn && invisible)
		return;

	if (pinlist_length(&e->Pin) != 0) {
		PinType *pin0 = pinlist_first(&e->Pin);
		if (TEST_FLAG(PCB_FLAG_HOLE, pin0))
			mark_size = MIN(mark_size, pin0->DrillingHole / 2);
		else
			mark_size = MIN(mark_size, pin0->Thickness / 2);
	}

	if (padlist_length(&e->Pad) != 0) {
		PadType *pad0 = padlist_first(&e->Pad);
		mark_size = MIN(mark_size, pad0->Thickness / 2);
	}

	gui->set_color(Output.fgGC, invisible ? PCB->InvisibleMarkColor : PCB->ElementColor);
	gui->set_line_cap(Output.fgGC, Trace_Cap);
	gui->set_line_width(Output.fgGC, 0);
	gui->draw_line(Output.fgGC, X - mark_size, Y, X, Y - mark_size);
	gui->draw_line(Output.fgGC, X + mark_size, Y, X, Y - mark_size);
	gui->draw_line(Output.fgGC, X - mark_size, Y, X, Y + mark_size);
	gui->draw_line(Output.fgGC, X + mark_size, Y, X, Y + mark_size);

	/*
	 * If an element is locked, place a "L" on top of the "diamond".
	 * This provides a nice visual indication that it is locked that
	 * works even for color blind users.
	 */
	if (TEST_FLAG(PCB_FLAG_LOCK, e)) {
		gui->draw_line(Output.fgGC, X, Y, X + 2 * mark_size, Y);
		gui->draw_line(Output.fgGC, X, Y, X, Y - 4 * mark_size);
	}
}

/* ---------------------------------------------------------------------------
 * Draws pins pads and vias - Always draws for non-gui HIDs,
 * otherwise drawing depends on PCB->PinOn and PCB->ViaOn
 */
static void DrawPPV(int group, const BoxType * drawn_area)
{
	int component_group = GetLayerGroupNumberByNumber(component_silk_layer);
	int solder_group = GetLayerGroupNumberByNumber(solder_silk_layer);
	int side;

	if (PCB->PinOn || !gui->gui) {
		/* draw element pins */
		r_search(PCB->Data->pin_tree, drawn_area, NULL, pin_callback, NULL, NULL);

		/* draw element pads */
		if (group == component_group) {
			side = COMPONENT_LAYER;
			r_search(PCB->Data->pad_tree, drawn_area, NULL, pad_callback, &side, NULL);
		}

		if (group == solder_group) {
			side = SOLDER_LAYER;
			r_search(PCB->Data->pad_tree, drawn_area, NULL, pad_callback, &side, NULL);
		}
	}

	/* draw vias */
	if (PCB->ViaOn || !gui->gui) {
		r_search(PCB->Data->via_tree, drawn_area, NULL, via_callback, NULL, NULL);
		r_search(PCB->Data->via_tree, drawn_area, NULL, hole_callback, NULL, NULL);
	}
	if (PCB->PinOn || doing_assy)
		r_search(PCB->Data->pin_tree, drawn_area, NULL, hole_callback, NULL, NULL);
}

static r_dir_t clearPin_callback(const BoxType * b, void *cl)
{
	PinType *pin = (PinTypePtr) b;
	if (conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly)
		gui->thindraw_pcb_pv(Output.pmGC, Output.pmGC, pin, pcb_false, pcb_true);
	else
		gui->fill_pcb_pv(Output.pmGC, Output.pmGC, pin, pcb_false, pcb_true);
	return R_DIR_FOUND_CONTINUE;
}

struct poly_info {
	const BoxType *drawn_area;
	LayerType *layer;
};

static r_dir_t poly_callback(const BoxType * b, void *cl)
{
	struct poly_info *i = cl;
	PolygonType *polygon = (PolygonType *) b;
	static const char *color;
	char buf[sizeof("#XXXXXX")];

	if (!polygon->Clipped)
		return R_DIR_NOT_FOUND;

	if (TEST_FLAG(PCB_FLAG_WARN, polygon))
		color = PCB->WarnColor;
	else if (TEST_FLAG(PCB_FLAG_SELECTED, polygon))
		color = i->layer->SelectedColor;
	else if (TEST_FLAG(PCB_FLAG_FOUND, polygon))
		color = PCB->ConnectedColor;
	else if (TEST_FLAG(PCB_FLAG_ONPOINT, polygon)) {
		assert(color != NULL);
		LightenColor(color, buf, 1.75);
		color = buf;
	}
	else
		color = i->layer->Color;
	gui->set_color(Output.fgGC, color);

	if ((gui->thindraw_pcb_polygon != NULL) && (conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly))
		gui->thindraw_pcb_polygon(Output.fgGC, polygon, i->drawn_area);
	else
		gui->fill_pcb_polygon(Output.fgGC, polygon, i->drawn_area);

	/* If checking planes, thin-draw any pieces which have been clipped away */
	if (gui->thindraw_pcb_polygon != NULL && conf_core.editor.check_planes && !TEST_FLAG(PCB_FLAG_FULLPOLY, polygon)) {
		PolygonType poly = *polygon;

		for (poly.Clipped = polygon->Clipped->f; poly.Clipped != polygon->Clipped; poly.Clipped = poly.Clipped->f)
			gui->thindraw_pcb_polygon(Output.fgGC, &poly, i->drawn_area);
	}

	return R_DIR_FOUND_CONTINUE;
}

static r_dir_t clearPad_callback(const BoxType * b, void *cl)
{
	PadTypePtr pad = (PadTypePtr) b;
	int *side = cl;
	if (ON_SIDE(pad, *side) && pad->Mask)
		_draw_pad(Output.pmGC, pad, pcb_true, pcb_true);
	return R_DIR_FOUND_CONTINUE;
}

/* ---------------------------------------------------------------------------
 * Draws silk layer.
 */

static void DrawSilk(int side, const BoxType * drawn_area)
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
		r_search(PCB->Data->element_tree, drawn_area, NULL, element_callback, &side, NULL);
		r_search(PCB->Data->name_tree[NAME_INDEX()], drawn_area, NULL, name_callback, &side, NULL);
#if 0
	}

	gui->use_mask(HID_MASK_CLEAR);
	r_search(PCB->Data->pin_tree, drawn_area, NULL, clearPin_callback, NULL, NULL);
	r_search(PCB->Data->via_tree, drawn_area, NULL, clearPin_callback, NULL, NULL);
	r_search(PCB->Data->pad_tree, drawn_area, NULL, clearPad_callback, &side, NULL);

	if (gui->poly_after) {
		gui->use_mask(HID_MASK_AFTER);
		DrawLayer(LAYER_PTR(max_copper_layer + layer), drawn_area);
		/* draw package */
		r_search(PCB->Data->element_tree, drawn_area, NULL, element_callback, &side, NULL);
		r_search(PCB->Data->name_tree[NAME_INDEX()], drawn_area, NULL, name_callback, &side, NULL);
	}
	gui->use_mask(HID_MASK_OFF);
#endif
}


static void DrawMaskBoardArea(int mask_type, const BoxType * drawn_area)
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
static void DrawMask(int side, const BoxType * screen)
{
	int thin = conf_core.editor.thin_draw || conf_core.editor.thin_draw_poly;

	if (thin)
		gui->set_color(Output.pmGC, PCB->MaskColor);
	else {
		DrawMaskBoardArea(HID_MASK_BEFORE, screen);
		gui->use_mask(HID_MASK_CLEAR);
	}

	r_search(PCB->Data->pin_tree, screen, NULL, clearPin_callback, NULL, NULL);
	r_search(PCB->Data->via_tree, screen, NULL, clearPin_callback, NULL, NULL);
	r_search(PCB->Data->pad_tree, screen, NULL, clearPad_callback, &side, NULL);

	if (thin)
		gui->set_color(Output.pmGC, "erase");
	else {
		DrawMaskBoardArea(HID_MASK_AFTER, screen);
		gui->use_mask(HID_MASK_OFF);
	}
}

/* ---------------------------------------------------------------------------
 * draws solder paste layer for a given side of the board
 */
static void DrawPaste(int side, const BoxType * drawn_area)
{
	gui->set_color(Output.fgGC, PCB->ElementColor);
	ALLPAD_LOOP(PCB->Data);
	{
		if (ON_SIDE(pad, side) && !TEST_FLAG(PCB_FLAG_NOPASTE, pad) && pad->Mask > 0) {
			if (pad->Mask < pad->Thickness)
				_draw_pad(Output.fgGC, pad, pcb_true, pcb_true);
			else
				_draw_pad(Output.fgGC, pad, pcb_false, pcb_false);
		}
	}
	ENDALL_LOOP;
}

static void DrawRats(const BoxType * drawn_area)
{
	/*
	 * XXX lesstif allows positive AND negative drawing in HID_MASK_CLEAR.
	 * XXX gtk only allows negative drawing.
	 * XXX using the mask here is to get rat transparency
	 */
	int can_mask = strcmp(gui->name, "lesstif") == 0;

	if (can_mask)
		gui->use_mask(HID_MASK_CLEAR);
	r_search(PCB->Data->rat_tree, drawn_area, NULL, rat_callback, NULL, NULL);
	if (can_mask)
		gui->use_mask(HID_MASK_OFF);
}

static r_dir_t text_callback(const BoxType * b, void *cl)
{
	LayerType *layer = cl;
	TextType *text = (TextType *) b;
	int min_silk_line;

	if (TEST_FLAG(PCB_FLAG_SELECTED, text))
		gui->set_color(Output.fgGC, layer->SelectedColor);
	else
		gui->set_color(Output.fgGC, layer->Color);
	if (layer == &PCB->Data->SILKLAYER || layer == &PCB->Data->BACKSILKLAYER)
		min_silk_line = PCB->minSlk;
	else
		min_silk_line = PCB->minWid;
	DrawTextLowLevel(text, min_silk_line);
	return R_DIR_FOUND_CONTINUE;
}

void DrawLayer(LayerTypePtr Layer, const BoxType * screen)
{
	struct poly_info info;

	info.drawn_area = screen;
	info.layer = Layer;

	/* print the non-clearing polys */
	r_search(Layer->polygon_tree, screen, NULL, poly_callback, &info, NULL);

	if (conf_core.editor.check_planes)
		return;

	/* draw all visible lines this layer */
	r_search(Layer->line_tree, screen, NULL, line_callback, Layer, NULL);

	/* draw the layer arcs on screen */
	r_search(Layer->arc_tree, screen, NULL, arc_callback, Layer, NULL);

	/* draw the layer text on screen */
	r_search(Layer->text_tree, screen, NULL, text_callback, Layer, NULL);

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
static void DrawLayerGroup(int group, const BoxType * drawn_area)
{
	int i, rv = 1;
	int layernum;
	LayerTypePtr Layer;
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

static void GatherPVName(PinTypePtr Ptr)
{
	BoxType box;
	pcb_bool vert = TEST_FLAG(PCB_FLAG_EDGE2, Ptr);

	if (vert) {
		box.X1 = Ptr->X - Ptr->Thickness / 2 + conf_core.appearance.pinout.text_offset_y;
		box.Y1 = Ptr->Y - Ptr->DrillingHole / 2 - conf_core.appearance.pinout.text_offset_x;
	}
	else {
		box.X1 = Ptr->X + Ptr->DrillingHole / 2 + conf_core.appearance.pinout.text_offset_x;
		box.Y1 = Ptr->Y - Ptr->Thickness / 2 + conf_core.appearance.pinout.text_offset_y;
	}

	if (vert) {
		box.X2 = box.X1;
		box.Y2 = box.Y1;
	}
	else {
		box.X2 = box.X1;
		box.Y2 = box.Y1;
	}
	AddPart(&box);
}

static void GatherPadName(PadTypePtr Pad)
{
	BoxType box;
	pcb_bool vert;

	/* should text be vertical ? */
	vert = (Pad->Point1.X == Pad->Point2.X);

	if (vert) {
		box.X1 = Pad->Point1.X - Pad->Thickness / 2;
		box.Y1 = MAX(Pad->Point1.Y, Pad->Point2.Y) + Pad->Thickness / 2;
		box.X1 += conf_core.appearance.pinout.text_offset_y;
		box.Y1 -= conf_core.appearance.pinout.text_offset_x;
		box.X2 = box.X1;
		box.Y2 = box.Y1;
	}
	else {
		box.X1 = MIN(Pad->Point1.X, Pad->Point2.X) - Pad->Thickness / 2;
		box.Y1 = Pad->Point1.Y - Pad->Thickness / 2;
		box.X1 += conf_core.appearance.pinout.text_offset_x;
		box.Y1 += conf_core.appearance.pinout.text_offset_y;
		box.X2 = box.X1;
		box.Y2 = box.Y1;
	}

	AddPart(&box);
	return;
}

/* ---------------------------------------------------------------------------
 * lowlevel drawing routine for text objects
 */
void DrawTextLowLevel(TextTypePtr Text, Coord min_line_width)
{
	Coord x = 0;
	unsigned char *string = (unsigned char *) Text->TextString;
	pcb_cardinal_t n;
	FontTypePtr font = &PCB->Font;

	while (string && *string) {
		/* draw lines if symbol is valid and data is present */
		if (*string <= MAX_FONTPOSITION && font->Symbol[*string].Valid) {
			LineTypePtr line = font->Symbol[*string].Line;
			LineType newline;

			for (n = font->Symbol[*string].LineN; n; n--, line++) {
				/* create one line, scale, move, rotate and swap it */
				newline = *line;
				newline.Point1.X = PCB_SCALE_TEXT(newline.Point1.X + x, Text->Scale);
				newline.Point1.Y = PCB_SCALE_TEXT(newline.Point1.Y, Text->Scale);
				newline.Point2.X = PCB_SCALE_TEXT(newline.Point2.X + x, Text->Scale);
				newline.Point2.Y = PCB_SCALE_TEXT(newline.Point2.Y, Text->Scale);
				newline.Thickness = PCB_SCALE_TEXT(newline.Thickness, Text->Scale / 2);
				if (newline.Thickness < min_line_width)
					newline.Thickness = min_line_width;

				RotateLineLowLevel(&newline, 0, 0, Text->Direction);

				/* the labels of SMD objects on the bottom
				 * side haven't been swapped yet, only their offset
				 */
				if (TEST_FLAG(PCB_FLAG_ONSOLDER, Text)) {
					newline.Point1.X = SWAP_SIGN_X(newline.Point1.X);
					newline.Point1.Y = SWAP_SIGN_Y(newline.Point1.Y);
					newline.Point2.X = SWAP_SIGN_X(newline.Point2.X);
					newline.Point2.Y = SWAP_SIGN_Y(newline.Point2.Y);
				}
				/* add offset and draw line */
				newline.Point1.X += Text->X;
				newline.Point1.Y += Text->Y;
				newline.Point2.X += Text->X;
				newline.Point2.Y += Text->Y;
				_draw_line(&newline);
			}

			/* move on to next cursor position */
			x += (font->Symbol[*string].Width + font->Symbol[*string].Delta);
		}
		else {
			/* the default symbol is a filled box */
			BoxType defaultsymbol = PCB->Font.DefaultSymbol;
			Coord size = (defaultsymbol.X2 - defaultsymbol.X1) * 6 / 5;

			defaultsymbol.X1 = PCB_SCALE_TEXT(defaultsymbol.X1 + x, Text->Scale);
			defaultsymbol.Y1 = PCB_SCALE_TEXT(defaultsymbol.Y1, Text->Scale);
			defaultsymbol.X2 = PCB_SCALE_TEXT(defaultsymbol.X2 + x, Text->Scale);
			defaultsymbol.Y2 = PCB_SCALE_TEXT(defaultsymbol.Y2, Text->Scale);

			RotateBoxLowLevel(&defaultsymbol, 0, 0, Text->Direction);

			/* add offset and draw box */
			defaultsymbol.X1 += Text->X;
			defaultsymbol.Y1 += Text->Y;
			defaultsymbol.X2 += Text->X;
			defaultsymbol.Y2 += Text->Y;
			gui->fill_rect(Output.fgGC, defaultsymbol.X1, defaultsymbol.Y1, defaultsymbol.X2, defaultsymbol.Y2);

			/* move on to next cursor position */
			x += size;
		}
		string++;
	}
}

/* ---------------------------------------------------------------------------
 * draw a via object
 */
void DrawVia(PinTypePtr Via)
{
	AddPart(Via);
	if (!TEST_FLAG(PCB_FLAG_HOLE, Via) && TEST_FLAG(PCB_FLAG_DISPLAYNAME, Via))
		DrawViaName(Via);
}

/* ---------------------------------------------------------------------------
 * draws the name of a via
 */
void DrawViaName(PinTypePtr Via)
{
	GatherPVName(Via);
}

/* ---------------------------------------------------------------------------
 * draw a pin object
 */
void DrawPin(PinTypePtr Pin)
{
	AddPart(Pin);
	if ((!TEST_FLAG(PCB_FLAG_HOLE, Pin) && TEST_FLAG(PCB_FLAG_DISPLAYNAME, Pin))
			|| doing_pinout)
		DrawPinName(Pin);
}

/* ---------------------------------------------------------------------------
 * draws the name of a pin
 */
void DrawPinName(PinTypePtr Pin)
{
	GatherPVName(Pin);
}

/* ---------------------------------------------------------------------------
 * draw a pad object
 */
void DrawPad(PadTypePtr Pad)
{
	AddPart(Pad);
	if (doing_pinout || TEST_FLAG(PCB_FLAG_DISPLAYNAME, Pad))
		DrawPadName(Pad);
}

/* ---------------------------------------------------------------------------
 * draws the name of a pad
 */
void DrawPadName(PadTypePtr Pad)
{
	GatherPadName(Pad);
}

/* ---------------------------------------------------------------------------
 * draws a line on a layer
 */
void DrawLine(LayerTypePtr Layer, LineTypePtr Line)
{
	AddPart(Line);
}

/* ---------------------------------------------------------------------------
 * draws a ratline
 */
void DrawRat(RatTypePtr Rat)
{
	if (conf_core.appearance.rat_thickness < 20)
		Rat->Thickness = pixel_slop * conf_core.appearance.rat_thickness;
	/* rats.c set PCB_FLAG_VIA if this rat goes to a containing poly: draw a donut */
	if (TEST_FLAG(PCB_FLAG_VIA, Rat)) {
		Coord w = Rat->Thickness;

		BoxType b;

		b.X1 = Rat->Point1.X - w * 2 - w / 2;
		b.X2 = Rat->Point1.X + w * 2 + w / 2;
		b.Y1 = Rat->Point1.Y - w * 2 - w / 2;
		b.Y2 = Rat->Point1.Y + w * 2 + w / 2;
		AddPart(&b);
	}
	else
		DrawLine(NULL, (LineType *) Rat);
}

/* ---------------------------------------------------------------------------
 * draws an arc on a layer
 */
void DrawArc(LayerTypePtr Layer, ArcTypePtr Arc)
{
	AddPart(Arc);
}

/* ---------------------------------------------------------------------------
 * draws a text on a layer
 */
void DrawText(LayerTypePtr Layer, TextTypePtr Text)
{
	AddPart(Text);
}


/* ---------------------------------------------------------------------------
 * draws a polygon on a layer
 */
void DrawPolygon(LayerTypePtr Layer, PolygonTypePtr Polygon)
{
	AddPart(Polygon);
}

/* ---------------------------------------------------------------------------
 * draws an element
 */
void DrawElement(ElementTypePtr Element)
{
	DrawElementPackage(Element);
	DrawElementName(Element);
	DrawElementPinsAndPads(Element);
}

/* ---------------------------------------------------------------------------
 * draws the name of an element
 */
void DrawElementName(ElementTypePtr Element)
{
	if (TEST_FLAG(PCB_FLAG_HIDENAME, Element))
		return;
	DrawText(NULL, &ELEMENT_TEXT(PCB, Element));
}

/* ---------------------------------------------------------------------------
 * draws the package of an element
 */
void DrawElementPackage(ElementTypePtr Element)
{
	ELEMENTLINE_LOOP(Element);
	{
		DrawLine(NULL, line);
	}
	END_LOOP;
	ARC_LOOP(Element);
	{
		DrawArc(NULL, arc);
	}
	END_LOOP;
}

/* ---------------------------------------------------------------------------
 * draw pins of an element
 */
void DrawElementPinsAndPads(ElementTypePtr Element)
{
	PAD_LOOP(Element);
	{
		if (doing_pinout || doing_assy || FRONT(pad) || PCB->InvisibleObjectsOn)
			DrawPad(pad);
	}
	END_LOOP;
	PIN_LOOP(Element);
	{
		DrawPin(pin);
	}
	END_LOOP;
}

/* ---------------------------------------------------------------------------
 * erase a via
 */
void EraseVia(PinTypePtr Via)
{
	AddPart(Via);
	if (TEST_FLAG(PCB_FLAG_DISPLAYNAME, Via))
		EraseViaName(Via);
	EraseFlags(&Via->Flags);
}

/* ---------------------------------------------------------------------------
 * erase a ratline
 */
void EraseRat(RatTypePtr Rat)
{
	if (TEST_FLAG(PCB_FLAG_VIA, Rat)) {
		Coord w = Rat->Thickness;

		BoxType b;

		b.X1 = Rat->Point1.X - w * 2 - w / 2;
		b.X2 = Rat->Point1.X + w * 2 + w / 2;
		b.Y1 = Rat->Point1.Y - w * 2 - w / 2;
		b.Y2 = Rat->Point1.Y + w * 2 + w / 2;
		AddPart(&b);
	}
	else
		EraseLine((LineType *) Rat);
	EraseFlags(&Rat->Flags);
}


/* ---------------------------------------------------------------------------
 * erase a via name
 */
void EraseViaName(PinTypePtr Via)
{
	GatherPVName(Via);
}

/* ---------------------------------------------------------------------------
 * erase a pad object
 */
void ErasePad(PadTypePtr Pad)
{
	AddPart(Pad);
	if (TEST_FLAG(PCB_FLAG_DISPLAYNAME, Pad))
		ErasePadName(Pad);
	EraseFlags(&Pad->Flags);
}

/* ---------------------------------------------------------------------------
 * erase a pad name
 */
void ErasePadName(PadTypePtr Pad)
{
	GatherPadName(Pad);
}

/* ---------------------------------------------------------------------------
 * erase a pin object
 */
void ErasePin(PinTypePtr Pin)
{
	AddPart(Pin);
	if (TEST_FLAG(PCB_FLAG_DISPLAYNAME, Pin))
		ErasePinName(Pin);
	EraseFlags(&Pin->Flags);
}

/* ---------------------------------------------------------------------------
 * erase a pin name
 */
void ErasePinName(PinTypePtr Pin)
{
	GatherPVName(Pin);
}

/* ---------------------------------------------------------------------------
 * erases a line on a layer
 */
void EraseLine(LineTypePtr Line)
{
	AddPart(Line);
	EraseFlags(&Line->Flags);
}

/* ---------------------------------------------------------------------------
 * erases an arc on a layer
 */
void EraseArc(ArcTypePtr Arc)
{
	if (!Arc->Thickness)
		return;
	AddPart(Arc);
	EraseFlags(&Arc->Flags);
}

/* ---------------------------------------------------------------------------
 * erases a text on a layer
 */
void EraseText(LayerTypePtr Layer, TextTypePtr Text)
{
	r_delete_entry(Layer->text_tree, (BoxTypePtr)Text);
	AddPart(Text);
}

/* ---------------------------------------------------------------------------
 * erases a polygon on a layer
 */
void ErasePolygon(PolygonTypePtr Polygon)
{
	AddPart(Polygon);
	EraseFlags(&Polygon->Flags);
}

/* ---------------------------------------------------------------------------
 * erases an element
 */
void EraseElement(ElementTypePtr Element)
{
	ELEMENTLINE_LOOP(Element);
	{
		EraseLine(line);
	}
	END_LOOP;
	ARC_LOOP(Element);
	{
		EraseArc(arc);
	}
	END_LOOP;
	EraseElementName(Element);
	EraseElementPinsAndPads(Element);
	EraseFlags(&Element->Flags);
}

/* ---------------------------------------------------------------------------
 * erases all pins and pads of an element
 */
void EraseElementPinsAndPads(ElementTypePtr Element)
{
	PIN_LOOP(Element);
	{
		ErasePin(pin);
	}
	END_LOOP;
	PAD_LOOP(Element);
	{
		ErasePad(pad);
	}
	END_LOOP;
}

/* ---------------------------------------------------------------------------
 * erases the name of an element
 */
void EraseElementName(ElementTypePtr Element)
{
	if (TEST_FLAG(PCB_FLAG_HIDENAME, Element)) {
		return;
	}
	DrawText(NULL, &ELEMENT_TEXT(PCB, Element));
}


void EraseObject(int type, void *lptr, void *ptr)
{
	switch (type) {
	case PCB_TYPE_VIA:
	case PCB_TYPE_PIN:
		ErasePin((PinTypePtr) ptr);
		break;
	case PCB_TYPE_TEXT:
	case PCB_TYPE_ELEMENT_NAME:
		EraseText((LayerTypePtr) lptr, (TextTypePtr) ptr);
		break;
	case PCB_TYPE_POLYGON:
		ErasePolygon((PolygonTypePtr) ptr);
		break;
	case PCB_TYPE_ELEMENT:
		EraseElement((ElementTypePtr) ptr);
		break;
	case PCB_TYPE_LINE:
	case PCB_TYPE_ELEMENT_LINE:
	case PCB_TYPE_RATLINE:
		EraseLine((LineTypePtr) ptr);
		break;
	case PCB_TYPE_PAD:
		ErasePad((PadTypePtr) ptr);
		break;
	case PCB_TYPE_ARC:
	case PCB_TYPE_ELEMENT_ARC:
		EraseArc((ArcTypePtr) ptr);
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
			DrawVia((PinTypePtr) ptr2);
		break;
	case PCB_TYPE_LINE:
		if (((LayerTypePtr) ptr1)->On)
			DrawLine((LayerTypePtr) ptr1, (LineTypePtr) ptr2);
		break;
	case PCB_TYPE_ARC:
		if (((LayerTypePtr) ptr1)->On)
			DrawArc((LayerTypePtr) ptr1, (ArcTypePtr) ptr2);
		break;
	case PCB_TYPE_TEXT:
		if (((LayerTypePtr) ptr1)->On)
			DrawText((LayerTypePtr) ptr1, (TextTypePtr) ptr2);
		break;
	case PCB_TYPE_POLYGON:
		if (((LayerTypePtr) ptr1)->On)
			DrawPolygon((LayerTypePtr) ptr1, (PolygonTypePtr) ptr2);
		break;
	case PCB_TYPE_ELEMENT:
		if (PCB->ElementOn && (FRONT((ElementTypePtr) ptr2) || PCB->InvisibleObjectsOn))
			DrawElement((ElementTypePtr) ptr2);
		break;
	case PCB_TYPE_RATLINE:
		if (PCB->RatOn)
			DrawRat((RatTypePtr) ptr2);
		break;
	case PCB_TYPE_PIN:
		if (PCB->PinOn)
			DrawPin((PinTypePtr) ptr2);
		break;
	case PCB_TYPE_PAD:
		if (PCB->PinOn)
			DrawPad((PadTypePtr) ptr2);
		break;
	case PCB_TYPE_ELEMENT_NAME:
		if (PCB->ElementOn && (FRONT((ElementTypePtr) ptr2) || PCB->InvisibleObjectsOn))
			DrawElementName((ElementTypePtr) ptr1);
		break;
	}
}

static void draw_element(ElementTypePtr element)
{
	draw_element_package(element);
	draw_element_name(element);
	draw_element_pins_and_pads(element);
}

/* ---------------------------------------------------------------------------
 * HID drawing callback.
 */

void hid_expose_callback(HID * hid, BoxType * region, void *item)
{
	HID *old_gui = gui;

	gui = hid;
	Output.fgGC = gui->make_gc();
	Output.bgGC = gui->make_gc();
	Output.pmGC = gui->make_gc();

	hid->set_color(Output.pmGC, "erase");
	hid->set_color(Output.bgGC, "drill");

	if (item) {
		doing_pinout = pcb_true;
		draw_element((ElementType *) item);
		doing_pinout = pcb_false;
	}
	else
		DrawEverything(region);

	gui->destroy_gc(Output.fgGC);
	gui->destroy_gc(Output.bgGC);
	gui->destroy_gc(Output.pmGC);
	gui = old_gui;
}
