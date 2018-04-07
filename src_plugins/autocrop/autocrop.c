/*
 * Autocrop plug-in for PCB.
 * Reduce the board dimensions to just enclose the objects.
 *
 * Copyright (C) 2007 Ben Jackson <ben@ben.com> based on teardrops.c by
 * Copyright (C) 2006 DJ Delorie <dj@delorie.com>
 *
 * Licensed under the terms of the GNU General Public
 * License, version 2 or later.
 *
 * Ported to pcb-rnd by Tibor 'Igor2' Palinkas in 2016.
 * Copyright (C) 2016..2018 Tibor 'Igor2' Palinkas
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
#include "conf_core.h"
#include "data.h"
#include "hid.h"
#include "rtree.h"
#include "undo.h"
#include "move.h"
#include "draw.h"
#include "polygon.h"
#include "plugins.h"
#include "box.h"
#include "hid_actions.h"

static const char autocrop_description[] =
        "Autocrops the board dimensions to extants + margin";

static const char autocrop_syntax[] =
        "autocrop()";


static void *MyMoveLineLowLevel(pcb_data_t * Data, pcb_layer_t * Layer, pcb_line_t * Line, pcb_coord_t dx, pcb_coord_t dy)
{
	pcb_line_move(Line, dx, dy);
	return Line;
}

static void *MyMoveArcLowLevel(pcb_data_t * Data, pcb_layer_t * Layer, pcb_arc_t * Arc, pcb_coord_t dx, pcb_coord_t dy)
{
	if (Data)
		pcb_r_delete_entry(Layer->arc_tree, (pcb_box_t *) Arc);
	pcb_arc_move(Arc, dx, dy);
	if (Data)
		pcb_r_insert_entry(Layer->arc_tree, (pcb_box_t *) Arc);
	return Arc;
}

static void *Mypcb_poly_move(pcb_data_t * Data, pcb_layer_t * Layer, pcb_poly_t * Polygon, pcb_coord_t dx, pcb_coord_t dy)
{
	if (Data)
		pcb_r_delete_entry(Layer->polygon_tree, (pcb_box_t *) Polygon);
	/* move.c actually only moves points, note no Data/Layer args */
	pcb_poly_move(Polygon, dx, dy);
	if (Data)
		pcb_r_insert_entry(Layer->polygon_tree, (pcb_box_t *) Polygon);
	return Polygon;
}

static void *MyMoveTextLowLevel(pcb_layer_t * Layer, pcb_text_t * Text, pcb_coord_t dx, pcb_coord_t dy)
{
	if (Layer)
		pcb_r_delete_entry(Layer->text_tree, (pcb_box_t *) Text);
	pcb_text_move(Text, dx, dy);
	if (Layer)
		pcb_r_insert_entry(Layer->text_tree, (pcb_box_t *) Text);
	return Text;
}

/* Move everything.
 *
 * Call our own 'MyMove*LowLevel' where they don't exist in move.c.
 * This gets very slow if there are large polygons present, since every
 * object move re-clears the poly, followed by the polys moving and
 * re-clearing everything again.
 */
static void MoveAll(pcb_coord_t dx, pcb_coord_t dy)
{
	PCB_SUBC_LOOP(PCB->Data);
	{
		pcb_subc_move(subc, dx, dy, pcb_true);
		pcb_undo_add_obj_to_move(PCB_OBJ_SUBC, NULL, NULL, subc, dx, dy);
	}
	PCB_END_LOOP;
	PCB_PADSTACK_LOOP(PCB->Data);
	{
		pcb_pstk_move(padstack, dx, dy, pcb_true);
		pcb_undo_add_obj_to_move(PCB_OBJ_PSTK, NULL, NULL, padstack, dx, dy);
	}
	PCB_END_LOOP;
	PCB_LINE_ALL_LOOP(PCB->Data);
	{
		MyMoveLineLowLevel(PCB->Data, layer, line, dx, dy);
		pcb_undo_add_obj_to_move(PCB_OBJ_LINE, NULL, NULL, line, dx, dy);
	}
	PCB_ENDALL_LOOP;
	PCB_ARC_ALL_LOOP(PCB->Data);
	{
		MyMoveArcLowLevel(PCB->Data, layer, arc, dx, dy);
		pcb_undo_add_obj_to_move(PCB_OBJ_ARC, NULL, NULL, arc, dx, dy);
	}
	PCB_ENDALL_LOOP;
	PCB_TEXT_ALL_LOOP(PCB->Data);
	{
		MyMoveTextLowLevel(layer, text, dx, dy);
		pcb_undo_add_obj_to_move(PCB_OBJ_TEXT, NULL, NULL, text, dx, dy);
	}
	PCB_ENDALL_LOOP;
	PCB_POLY_ALL_LOOP(PCB->Data);
	{
		/*
		 * XXX MovePolygonLowLevel does not mean "no gui" like
		 * XXX MoveElementLowLevel, it doesn't even handle layer
		 * XXX tree activity.
		 */
		Mypcb_poly_move(PCB->Data, layer, polygon, dx, dy);
		pcb_undo_add_obj_to_move(PCB_OBJ_POLY, NULL, NULL, polygon, dx, dy);
	}
	PCB_ENDALL_LOOP;
}

static int autocrop(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	pcb_coord_t dx, dy, pad;
	pcb_box_t tmp, *box;

	box = pcb_data_bbox(&tmp, PCB->Data, pcb_false);	/* handy! */
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
	pad = conf_core.design.min_wid * 5;				/* XXX real edge clearance */
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
	MoveAll(dx, dy);
	pcb_board_resize(box->X2, box->Y2);
	pcb_data_clip_all(PCB->Data, pcb_true);
	pcb_undo_inc_serial();
	pcb_redraw();
	pcb_board_set_changed_flag(1);
	return 0;
}

static pcb_hid_action_t autocrop_action_list[] = {
	{"autocrop", NULL, autocrop, autocrop_description , autocrop_syntax}
};

char *autocrop_cookie = "autocrop plugin";

PCB_REGISTER_ACTIONS(autocrop_action_list, autocrop_cookie)

int pplg_check_ver_autocrop(int ver_needed) { return 0; }

void pplg_uninit_autocrop(void)
{
	pcb_hid_remove_actions_by_cookie(autocrop_cookie);
}

#include "dolists.h"
int pplg_init_autocrop(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(autocrop_action_list, autocrop_cookie);
	return 0;
}
