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
#include "set.h"
#include "misc.h"
#include "find.h"
#include "remove.h"
#include "funchash_core.h"
#include "compat_nls.h"
#include "obj_rat.h"

#include "rats.h"
#include "draw.h"

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

static int ActionAddRats(int argc, const char **argv, Coord x, Coord y)
{
	const char *function = ACTION_ARG(0);
	RatTypePtr shorty;
	float len, small;

	if (function) {
		if (conf_core.temp.rat_warn)
			ClearWarnings();
		switch (funchash_get(function, NULL)) {
		case F_AllRats:
			if (AddAllRats(pcb_false, NULL))
				SetChangedFlag(pcb_true);
			break;
		case F_SelectedRats:
		case F_Selected:
			if (AddAllRats(pcb_true, NULL))
				SetChangedFlag(pcb_true);
			break;
		case F_Close:
			small = SQUARE(MAX_COORD);
			shorty = NULL;
			RAT_LOOP(PCB->Data);
			{
				if (TEST_FLAG(PCB_FLAG_SELECTED, line))
					continue;
				len = SQUARE(line->Point1.X - line->Point2.X) + SQUARE(line->Point1.Y - line->Point2.Y);
				if (len < small) {
					small = len;
					shorty = line;
				}
			}
			END_LOOP;
			if (shorty) {
				AddObjectToFlagUndoList(PCB_TYPE_RATLINE, shorty, shorty, shorty);
				SET_FLAG(PCB_FLAG_SELECTED, shorty);
				DrawRat(shorty);
				Draw();
				CenterDisplay((shorty->Point2.X + shorty->Point1.X) / 2, (shorty->Point2.Y + shorty->Point1.Y) / 2);
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

static int ActionConnection(int argc, const char **argv, Coord x, Coord y)
{
	const char *function = ACTION_ARG(0);
	if (function) {
		switch (funchash_get(function, NULL)) {
		case F_Find:
			{
				gui->get_coords(_("Click on a connection"), &x, &y);
				LookupConnection(x, y, pcb_true, 1, PCB_FLAG_FOUND);
				break;
			}

		case F_ResetLinesAndPolygons:
			if (ResetFoundLinesAndPolygons(pcb_true)) {
				IncrementUndoSerialNumber();
				Draw();
			}
			break;

		case F_ResetPinsViasAndPads:
			if (ResetFoundPinsViasAndPads(pcb_true)) {
				IncrementUndoSerialNumber();
				Draw();
			}
			break;

		case F_Reset:
			if (ResetConnections(pcb_true)) {
				IncrementUndoSerialNumber();
				Draw();
			}
			break;
		}
		return 0;
	}

	AFAIL(connection);
}

/* --------------------------------------------------------------------------- */

static const char deleterats_syntax[] = "DeleteRats(AllRats|Selected|SelectedRats)";

static const char deleterats_help[] = "Delete rat lines.";

/* %start-doc actions DeleteRats

%end-doc */

static int ActionDeleteRats(int argc, const char **argv, Coord x, Coord y)
{
	const char *function = ACTION_ARG(0);
	if (function) {
		if (conf_core.temp.rat_warn)
			ClearWarnings();
		switch (funchash_get(function, NULL)) {
		case F_AllRats:
			if (DeleteRats(pcb_false))
				SetChangedFlag(pcb_true);
			break;
		case F_SelectedRats:
		case F_Selected:
			if (DeleteRats(pcb_true))
				SetChangedFlag(pcb_true);
			break;
		}
	}
	return 0;
}


HID_Action rats_action_list[] = {
	{"AddRats", 0, ActionAddRats,
	 addrats_help, addrats_syntax}
	,
	{"Connection", 0, ActionConnection,
	 connection_help, connection_syntax}
	,
	{"DeleteRats", 0, ActionDeleteRats,
	 deleterats_help, deleterats_syntax}
};

REGISTER_ACTIONS(rats_action_list, NULL)
