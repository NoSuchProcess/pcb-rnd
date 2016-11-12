/*!
 * \file autocrop.c
 *
 * \brief Autocrop plug-in for PCB.
 * Reduce the board dimensions to just enclose the elements.
 *
 * \author Copyright (C) 2007 Ben Jackson <ben@ben.com> based on teardrops.c by
 * Copyright (C) 2006 DJ Delorie <dj@delorie.com>
 *
 * \copyright Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Ported to pcb-rnd by Tibor 'Igor2' Palinkas in 2016.
 *
 * From: Ben Jackson <bjj@saturn.home.ben.com>
 * To: geda-user@moria.seul.org
 * Date: Mon, 19 Feb 2007 13:30:58 -0800
 * Subject: Autocrop() plugin for PCB
 *
 * As a "learn PCB internals" project I've written an Autocrop() plugin.
 * It finds the extents of your existing board and resizes the board to
 * contain only your parts plus a margin.
 *
 * There are some issues that I can't resolve from a plugin:
 *
 * <ol>
 * <li> Board size has no Undo function, so while Undo will put your objects
 * back where they started, the board size has to be replaced manually.
 * <li> There is no 'edge clearance' DRC paramater, so I used 5*line spacing.
 * <li> Moving a layout with lots of objects and polygons is slow due to the
 * repeated clearing/unclearing of the polygons as things move.  Undo is
 * slower than moving because every individual move is drawn (instead of
 * one redraw at the end).
 * </ol>
 *
 * Original source was: http://ad7gd.net/geda/autocrop.c
 *
 * Run it by typing `:Autocrop()' in the gui, or by binding Autocrop() to a key.
 *
 * --
 * Ben Jackson AD7GD
 * <ben@ben.com>
 * http://www.ben.com/
 */

#include <stdio.h>
#include <math.h>

#include "config.h"
#include "board.h"
#include "data.h"
#include "hid.h"
#include "rtree.h"
#include "undo.h"
#include "move.h"
#include "draw.h"
#include "set.h"
#include "polygon.h"
#include "plugins.h"
#include "obj_all.h"
#include "box.h"
#include "hid_actions.h"

static void *MyMoveViaLowLevel(pcb_data_t * Data, pcb_pin_t * Via, Coord dx, Coord dy)
{
	if (Data) {
		RestoreToPolygon(Data, PCB_TYPE_VIA, Via, Via);
		r_delete_entry(Data->via_tree, (pcb_box_t *) Via);
	}
	MOVE_VIA_LOWLEVEL(Via, dx, dy);
	if (Data) {
		r_insert_entry(Data->via_tree, (pcb_box_t *) Via, 0);
		ClearFromPolygon(Data, PCB_TYPE_VIA, Via, Via);
	}
	return Via;
}

static void *MyMoveLineLowLevel(pcb_data_t * Data, pcb_layer_t * Layer, pcb_line_t * Line, Coord dx, Coord dy)
{
	if (Data) {
		RestoreToPolygon(Data, PCB_TYPE_LINE, Layer, Line);
		r_delete_entry(Layer->line_tree, (pcb_box_t *) Line);
	}
	MOVE_LINE_LOWLEVEL(Line, dx, dy);
	if (Data) {
		r_insert_entry(Layer->line_tree, (pcb_box_t *) Line, 0);
		ClearFromPolygon(Data, PCB_TYPE_LINE, Layer, Line);
	}
	return Line;
}

static void *MyMoveArcLowLevel(pcb_data_t * Data, pcb_layer_t * Layer, pcb_arc_t * Arc, Coord dx, Coord dy)
{
	if (Data) {
		RestoreToPolygon(Data, PCB_TYPE_ARC, Layer, Arc);
		r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
	}
	MOVE_ARC_LOWLEVEL(Arc, dx, dy);
	if (Data) {
		r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc, 0);
		ClearFromPolygon(Data, PCB_TYPE_ARC, Layer, Arc);
	}
	return Arc;
}

static void *MyMovePolygonLowLevel(pcb_data_t * Data, pcb_layer_t * Layer, pcb_polygon_t * Polygon, Coord dx, Coord dy)
{
	if (Data) {
		r_delete_entry(Layer->polygon_tree, (pcb_box_t *) Polygon);
	}
	/* move.c actually only moves points, note no Data/Layer args */
	MovePolygonLowLevel(Polygon, dx, dy);
	if (Data) {
		r_insert_entry(Layer->polygon_tree, (pcb_box_t *) Polygon, 0);
		InitClip(Data, Layer, Polygon);
	}
	return Polygon;
}

static void *MyMoveTextLowLevel(pcb_layer_t * Layer, pcb_text_t * Text, Coord dx, Coord dy)
{
	if (Layer)
		r_delete_entry(Layer->text_tree, (pcb_box_t *) Text);
	MOVE_TEXT_LOWLEVEL(Text, dx, dy);
	if (Layer)
		r_insert_entry(Layer->text_tree, (pcb_box_t *) Text, 0);
	return Text;
}

/*!
 * \brief Move everything.
 *
 * Call our own 'MyMove*LowLevel' where they don't exist in move.c.
 * This gets very slow if there are large polygons present, since every
 * element move re-clears the poly, followed by the polys moving and
 * re-clearing everything again.
 */
static void MoveAll(Coord dx, Coord dy)
{
	ELEMENT_LOOP(PCB->Data);
	{
		MoveElementLowLevel(PCB->Data, element, dx, dy);
		AddObjectToMoveUndoList(PCB_TYPE_ELEMENT, NULL, NULL, element, dx, dy);
	}
	END_LOOP;
	VIA_LOOP(PCB->Data);
	{
		MyMoveViaLowLevel(PCB->Data, via, dx, dy);
		AddObjectToMoveUndoList(PCB_TYPE_VIA, NULL, NULL, via, dx, dy);
	}
	END_LOOP;
	ALLLINE_LOOP(PCB->Data);
	{
		MyMoveLineLowLevel(PCB->Data, layer, line, dx, dy);
		AddObjectToMoveUndoList(PCB_TYPE_LINE, NULL, NULL, line, dx, dy);
	}
	ENDALL_LOOP;
	ALLARC_LOOP(PCB->Data);
	{
		MyMoveArcLowLevel(PCB->Data, layer, arc, dx, dy);
		AddObjectToMoveUndoList(PCB_TYPE_ARC, NULL, NULL, arc, dx, dy);
	}
	ENDALL_LOOP;
	ALLTEXT_LOOP(PCB->Data);
	{
		MyMoveTextLowLevel(layer, text, dx, dy);
		AddObjectToMoveUndoList(PCB_TYPE_TEXT, NULL, NULL, text, dx, dy);
	}
	ENDALL_LOOP;
	ALLPOLYGON_LOOP(PCB->Data);
	{
		/*
		 * XXX MovePolygonLowLevel does not mean "no gui" like
		 * XXX MoveElementLowLevel, it doesn't even handle layer
		 * XXX tree activity.
		 */
		MyMovePolygonLowLevel(PCB->Data, layer, polygon, dx, dy);
		AddObjectToMoveUndoList(PCB_TYPE_POLYGON, NULL, NULL, polygon, dx, dy);
	}
	ENDALL_LOOP;
}

static int autocrop(int argc, const char **argv, Coord x, Coord y)
{
	Coord dx, dy, pad;
	pcb_box_t *box;

	box = GetDataBoundingBox(PCB->Data);	/* handy! */
	if (!box || (box->X1 == box->X2 || box->Y1 == box->Y2)) {
		/* board would become degenerate */
		return 0;
	}
	/*
	 * Now X1/Y1 are the distance to move the left/top edge
	 * (actually moving all components to the left/up) such that
	 * the exact edge of the leftmost/topmost component would touch
	 * the edge.  Reduce the move by the edge relief requirement XXX
	 * and expand the board by the same amount.
	 */
	pad = PCB->minWid * 5;				/* XXX real edge clearance */
	dx = -box->X1 + pad;
	dy = -box->Y1 + pad;
	box->X2 += pad;
	box->Y2 += pad;
	/*
	 * Round move to keep components grid-aligned, then translate the
	 * upper coordinates into the new space.
	 */
	dx -= dx % (long) PCB->Grid;
	dy -= dy % (long) PCB->Grid;
	box->X2 += dx;
	box->Y2 += dy;
	/*
	 * Avoid touching any data if there's nothing to do.
	 */
	if (dx == 0 && dy == 0 && PCB->MaxWidth == box->X2 && PCB->MaxHeight == box->Y2) {
		return 0;
	}
	/* Resize -- XXX cannot be undone */
	PCB->MaxWidth = box->X2;
	PCB->MaxHeight = box->Y2;
	MoveAll(dx, dy);
	IncrementUndoSerialNumber();
	Redraw();
	SetChangedFlag(1);
	return 0;
}

static HID_Action autocrop_action_list[] = {
	{"autocrop", NULL, autocrop, NULL, NULL}
};

char *autocrop_cookie = "autocrop plugin";

REGISTER_ACTIONS(autocrop_action_list, autocrop_cookie)

static void hid_autocrop_uninit(void)
{
	hid_remove_actions_by_cookie(autocrop_cookie);
}

#include "dolists.h"
pcb_uninit_t hid_autocrop_init()
{
	REGISTER_ACTIONS(autocrop_action_list, autocrop_cookie);
	return hid_autocrop_uninit;
}
