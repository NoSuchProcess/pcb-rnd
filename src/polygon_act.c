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
#include "global.h"
#include "data.h"
#include "action.h"
#include "change.h"
#include "error.h"
#include "undo.h"

#include "polygon.h"
#include "draw.h"
#include "search.h"
#include "crosshair.h"

/* --------------------------------------------------------------------------- */

static const char morphpolygon_syntax[] = "MorphPolygon(Object|Selected)";

static const char morphpolygon_help[] = "Converts dead polygon islands into separate polygons.";

/* %start-doc actions MorphPolygon 

If a polygon is divided into unconnected "islands", you can use
this command to convert the otherwise disappeared islands into
separate polygons. Be sure the cursor is over a portion of the
polygon that remains visible. Very small islands that may flake
off are automatically deleted.

%end-doc */

static int ActionMorphPolygon(int argc, char **argv, Coord x, Coord y)
{
	char *function = ARG(0);
	if (function) {
		switch (GetFunctionID(function)) {
		case F_Object:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;

				gui->get_coords(_("Select an Object"), &x, &y);
				if ((type = SearchScreen(x, y, POLYGON_TYPE, &ptr1, &ptr2, &ptr3)) != NO_TYPE) {
					MorphPolygon((LayerType *) ptr1, (PolygonType *) ptr3);
					Draw();
					IncrementUndoSerialNumber();
				}
				break;
			}
		case F_Selected:
		case F_SelectedObjects:
			ALLPOLYGON_LOOP(PCB->Data);
			{
				if (TEST_FLAG(SELECTEDFLAG, polygon))
					MorphPolygon(layer, polygon);
			}
			ENDALL_LOOP;
			Draw();
			IncrementUndoSerialNumber();
			break;
		}
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char polygon_syntax[] = "Polygon(Close|PreviousPoint)";

static const char polygon_help[] = "Some polygon related stuff.";

/* %start-doc actions Polygon

Polygons need a special action routine to make life easier.

@table @code

@item Close
Creates the final segment of the polygon.  This may fail if clipping
to 45 degree lines is switched on, in which case a warning is issued.

@item PreviousPoint
Resets the newly entered corner to the previous one. The Undo action
will call Polygon(PreviousPoint) when appropriate to do so.

@end table

%end-doc */

static int ActionPolygon(int argc, char **argv, Coord x, Coord y)
{
	char *function = ARG(0);
	if (function && Settings.Mode == POLYGON_MODE) {
		notify_crosshair_change(false);
		switch (GetFunctionID(function)) {
			/* close open polygon if possible */
		case F_Close:
			ClosePolygon();
			break;

			/* go back to the previous point */
		case F_PreviousPoint:
			GoToPreviousPoint();
			break;
		}
		notify_crosshair_change(true);
	}
	return 0;
}


HID_Action polygon_action_list[] = {
	{"MorphPolygon", 0, ActionMorphPolygon,
	 morphpolygon_help, morphpolygon_syntax}
	,
	{"Polygon", 0, ActionPolygon,
	 polygon_help, polygon_syntax}
};

REGISTER_ACTIONS(polygon_action_list)
