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
#include "undo.h"
#include "funchash_core.h"

#include "polygon.h"
#include "draw.h"
#include "search.h"
#include "crosshair.h"
#include "compat_nls.h"

#include "obj_poly.h"

/* --------------------------------------------------------------------------- */

static const char morphpcb_polygon_syntax[] = "MorphPolygon(Object|Selected)";

static const char morphpolygon_help[] = "Converts dead polygon islands into separate polygons.";

/* %start-doc actions MorphPolygon

If a polygon is divided into unconnected "islands", you can use
this command to convert the otherwise disappeared islands into
separate polygons. Be sure the cursor is over a portion of the
polygon that remains visible. Very small islands that may flake
off are automatically deleted.

%end-doc */

static int ActionMorphPolygon(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (function) {
		switch (pcb_funchash_get(function, NULL)) {
		case F_Object:
			{
				int type;
				void *ptr1, *ptr2, *ptr3;

				gui->get_coords(_("Select an Object"), &x, &y);
				if ((type = SearchScreen(x, y, PCB_TYPE_POLYGON, &ptr1, &ptr2, &ptr3)) != PCB_TYPE_NONE) {
					MorphPolygon((pcb_layer_t *) ptr1, (pcb_polygon_t *) ptr3);
					pcb_draw();
					IncrementUndoSerialNumber();
				}
				break;
			}
		case F_Selected:
		case F_SelectedObjects:
			ALLPOLYGON_LOOP(PCB->Data);
			{
				if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, polygon))
					MorphPolygon(layer, polygon);
			}
			ENDALL_LOOP;
			pcb_draw();
			IncrementUndoSerialNumber();
			break;
		}
	}
	return 0;
}

/* --------------------------------------------------------------------------- */

static const char pcb_polygon_syntax[] = "Polygon(Close|PreviousPoint)";

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

static int ActionPolygon(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *function = PCB_ACTION_ARG(0);
	if (function && conf_core.editor.mode == PCB_MODE_POLYGON) {
		pcb_notify_crosshair_change(pcb_false);
		switch (pcb_funchash_get(function, NULL)) {
			/* close open polygon if possible */
		case F_Close:
			ClosePolygon();
			break;

			/* go back to the previous point */
		case F_PreviousPoint:
			GoToPreviousPoint();
			break;
		}
		pcb_notify_crosshair_change(pcb_true);
	}
	return 0;
}


pcb_hid_action_t polygon_action_list[] = {
	{"MorphPolygon", 0, ActionMorphPolygon,
	 morphpolygon_help, morphpcb_polygon_syntax}
	,
	{"Polygon", 0, ActionPolygon,
	 polygon_help, pcb_polygon_syntax}
};

REGISTER_ACTIONS(polygon_action_list, NULL)
