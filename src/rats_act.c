/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  Contact addresses for paper mail and Email:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"
#include "conf_core.h"

#include "board.h"
#include "data.h"
#include "action_helper.h"
#include "error.h"
#include "undo.h"
#include "find.h"
#include "remove.h"
#include "funchash_core.h"
#include "compat_nls.h"
#include "obj_rat.h"

#include "rats.h"
#include "draw.h"

#include "obj_rat_draw.h"

/* --------------------------------------------------------------------------- */

static const char addrats_syntax[] = "AddRats(AllRats|SelectedRats|Close)";

static const char addrats_help[] = "Add one or more rat lines to the board.";

/* %start-doc actions AddRats

@table @code

@item AllRats
Create rat lines for all loaded nets that aren't already connected on
with copper.

@item SelectedRats
Similarly, but only add rat lines for nets connected to selected pins
and pads.

@item Close
Selects the shortest unselected rat on the board.

@end table

%end-doc */

static int ActionAddRats(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	pcb_rat_t *shorty;
	float len, small;

	if (function) {
		if (conf_core.temp.rat_warn)
			pcb_clear_warnings();
		switch (pcb_funchash_get(function, NULL)) {
		case F_AllRats:
			if (pcb_rat_add_all(pcb_false, NULL))
				pcb_board_set_changed_flag(pcb_true);
			break;
		case F_SelectedRats:
		case F_Selected:
			if (pcb_rat_add_all(pcb_true, NULL))
				pcb_board_set_changed_flag(pcb_true);
			break;
		case F_Close:
			small = PCB_SQUARE(PCB_MAX_COORD);
			shorty = NULL;
			PCB_RAT_LOOP(PCB->Data);
			{
				if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, line))
					continue;
				len = PCB_SQUARE(line->Point1.X - line->Point2.X) + PCB_SQUARE(line->Point1.Y - line->Point2.Y);
				if (len < small) {
					small = len;
					shorty = line;
				}
			}
			END_LOOP;
			if (shorty) {
				pcb_undo_add_obj_to_flag(PCB_TYPE_RATLINE, shorty, shorty, shorty);
				PCB_FLAG_SET(PCB_FLAG_SELECTED, shorty);
				DrawRat(shorty);
				pcb_draw();
				pcb_center_display((shorty->Point2.X + shorty->Point1.X) / 2, (shorty->Point2.Y + shorty->Point1.Y) / 2);
			}
			break;
		}
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char connection_syntax[] = "Connection(Find|ResetLinesAndPolygons|ResetPinsAndVias|Reset)";

static const char connection_help[] = "Searches connections of the object at the cursor position.";

/* %start-doc actions Connection

Connections found with this action will be highlighted in the
``connected-color'' color and will have the ``found'' flag set.

@table @code

@item Find
The net under the cursor is ``found''.

@item ResetLinesAndPolygons
Any ``found'' lines and polygons are marked ``not found''.

@item ResetPinsAndVias
Any ``found'' pins and vias are marked ``not found''.

@item Reset
All ``found'' objects are marked ``not found''.

@end table

%end-doc */

static int ActionConnection(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_Find:
			{
				pcb_gui->get_coords(_("Click on a connection"), &x, &y);
				pcb_lookup_conn(x, y, pcb_true, 1, PCB_FLAG_FOUND);
				break;
			}

		case F_ResetLinesAndPolygons:
			if (pcb_reset_found_lines_polys(pcb_true)) {
				pcb_undo_inc_serial();
				pcb_draw();
			}
			break;

		case F_ResetPinsViasAndPads:
			if (pcb_reset_found_pins_vias_pads(pcb_true)) {
				pcb_undo_inc_serial();
				pcb_draw();
			}
			break;

		case F_Reset:
			if (pcb_reset_conns(pcb_true)) {
				pcb_undo_inc_serial();
				pcb_draw();
			}
			break;
		}
		return 0;
	}

	PCB_AFAIL(connection);
}

/* --------------------------------------------------------------------------- */

static const char deleterats_syntax[] = "DeleteRats(AllRats|Selected|SelectedRats)";

static const char deleterats_help[] = "Delete rat lines.";

/* %start-doc actions DeleteRats

%end-doc */

static int ActionDeleteRats(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (function) {
		if (conf_core.temp.rat_warn)
			pcb_clear_warnings();
		switch (pcb_funchash_get(function, NULL)) {
		case F_AllRats:
			if (pcb_rats_destroy(pcb_false))
				pcb_board_set_changed_flag(pcb_true);
			break;
		case F_SelectedRats:
		case F_Selected:
			if (pcb_rats_destroy(pcb_true))
				pcb_board_set_changed_flag(pcb_true);
			break;
		}
	}
	return 0;
}


pcb_hid_action_t rats_action_list[] = {
	{"AddRats", 0, ActionAddRats,
	 addrats_help, addrats_syntax}
	,
	{"Connection", 0, ActionConnection,
	 connection_help, connection_syntax}
	,
	{"DeleteRats", 0, ActionDeleteRats,
	 deleterats_help, deleterats_syntax}
};

PCB_REGISTER_ACTIONS(rats_action_list, NULL)
