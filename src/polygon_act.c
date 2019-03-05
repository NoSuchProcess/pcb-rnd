/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 *
 *
 *  Old contact info:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */
#include "config.h"
#include "conf_core.h"

#include "board.h"
#include "data.h"
#include "undo.h"
#include "funchash_core.h"

#include "polygon.h"
#include "draw.h"
#include "search.h"
#include "crosshair.h"
#include "compat_nls.h"
#include "tool.h"
#include "actions.h"

#include "obj_poly.h"

static const char pcb_acts_MorphPolygon[] = "pcb_poly_morph(Object|Selected)";
static const char pcb_acth_MorphPolygon[] = "Converts dead polygon islands into separate polygons.";
/* DOC: morphpolygon.html */
static fgw_error_t pcb_act_MorphPolygon(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;

	PCB_ACT_CONVARG(1, FGW_KEYWORD, MorphPolygon, op = fgw_keyword(&argv[1]));

	switch(op) {
		case F_Object:
			{
				pcb_coord_t x, y;
				int type;
				void *ptr1, *ptr2, *ptr3;

				pcb_hid_get_coords("Select an Object", &x, &y, 0);
				if ((type = pcb_search_screen(x, y, PCB_OBJ_POLY, &ptr1, &ptr2, &ptr3)) != PCB_OBJ_VOID) {
					pcb_poly_morph((pcb_layer_t *) ptr1, (pcb_poly_t *) ptr3);
					pcb_draw();
					pcb_undo_inc_serial();
				}
				break;
			}
		case F_Selected:
		case F_SelectedObjects:
			PCB_POLY_ALL_LOOP(PCB->Data);
			{
				if (PCB_FLAG_TEST(PCB_FLAG_SELECTED, polygon))
					pcb_poly_morph(layer, polygon);
			}
			PCB_ENDALL_LOOP;
			pcb_draw();
			pcb_undo_inc_serial();
			break;
	}
	PCB_ACT_IRES(0);
	return 0;
}

static const char pcb_acts_Polygon[] = "Polygon(Close|CloseHole|PreviousPoint)";
static const char pcb_acth_Polygon[] = "Some polygon related stuff.";
/* DOC: polygon.html */
static fgw_error_t pcb_act_Polygon(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	int op;
	
	PCB_ACT_CONVARG(1, FGW_KEYWORD, Polygon, op = fgw_keyword(&argv[1]));

	if ((argc > 1) && ((conf_core.editor.mode == PCB_MODE_POLYGON) || (conf_core.editor.mode == PCB_MODE_POLYGON_HOLE))) {
		pcb_notify_crosshair_change(pcb_false);
		switch(op) {
			/* close open polygon if possible */
		case F_Close:
			pcb_polygon_close_poly();
			break;
			
			/* close open polygon hole if possible */
		case F_CloseHole:
			pcb_polygon_close_hole();
			break;
		
			/* go back to the previous point */
		case F_PreviousPoint:
			pcb_polygon_go_to_prev_point();
			break;
		}
		pcb_notify_crosshair_change(pcb_true);
	}
	PCB_ACT_IRES(0);
	return 0;
}


pcb_action_t polygon_action_list[] = {
	{"MorphPolygon", pcb_act_MorphPolygon, pcb_acth_MorphPolygon, pcb_acts_MorphPolygon},
	{"Polygon", pcb_act_Polygon, pcb_acth_Polygon, pcb_acts_Polygon}
};

PCB_REGISTER_ACTIONS(polygon_action_list, NULL)
