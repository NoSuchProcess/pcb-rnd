/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
 *  Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "tool.h"
#include "undo.h"

#include "obj_poly_draw.h"


void pcb_tool_rectangle_uninit(void)
{
	pcb_notify_crosshair_change(pcb_false);
	pcb_crosshair.AttachedBox.State = PCB_CH_STATE_FIRST;
	pcb_notify_crosshair_change(pcb_true);
}

void pcb_tool_rectangle_notify_mode(void)
{
	/* do update of position */
	pcb_tool_notify_block();

	/* create rectangle if both corners are determined
	 * and width, height are != 0
	 */
	if (pcb_crosshair.AttachedBox.State == PCB_CH_STATE_THIRD &&
			pcb_crosshair.AttachedBox.Point1.X != pcb_crosshair.AttachedBox.Point2.X &&
			pcb_crosshair.AttachedBox.Point1.Y != pcb_crosshair.AttachedBox.Point2.Y) {
		pcb_poly_t *polygon;
		pcb_layer_t *layer = pcb_loose_subc_layer(PCB, CURRENT, pcb_true);

		int flags = PCB_FLAG_CLEARPOLY;
		if (conf_core.editor.full_poly)
			flags |= PCB_FLAG_FULLPOLY;
		if (conf_core.editor.clear_polypoly)
			flags |= PCB_FLAG_CLEARPOLYPOLY;
		if ((polygon = pcb_poly_new_from_rectangle(layer,
																								 pcb_crosshair.AttachedBox.Point1.X,
																								 pcb_crosshair.AttachedBox.Point1.Y,
																								 pcb_crosshair.AttachedBox.Point2.X,
																								 pcb_crosshair.AttachedBox.Point2.Y,
																								 2 * conf_core.design.clearance,
																								 pcb_flag_make(flags))) != NULL) {
			pcb_obj_add_attribs(polygon, PCB->pen_attr);
			pcb_undo_add_obj_to_create(PCB_OBJ_POLY, layer, polygon, polygon);
			pcb_undo_inc_serial();
			pcb_poly_invalidate_draw(layer, polygon);
			pcb_draw();
		}

		/* reset state to 'first corner' */
		pcb_crosshair.AttachedBox.State = PCB_CH_STATE_FIRST;
		pcb_subc_as_board_update(PCB);
	}
}

void pcb_tool_rectangle_adjust_attached_objects(void)
{
	switch (pcb_crosshair.AttachedBox.State) {
	case PCB_CH_STATE_SECOND:						/* one corner is selected */
		{
			/* update coordinates */
			pcb_crosshair.AttachedBox.Point2.X = pcb_crosshair.X;
			pcb_crosshair.AttachedBox.Point2.Y = pcb_crosshair.Y;
			break;
		}
	}
}

pcb_bool pcb_tool_rectangle_anydo_act(void)
{
	/* don't allow undo in the middle of an operation */
	if (pcb_crosshair.AttachedBox.State != PCB_CH_STATE_FIRST)
		return pcb_false;
	return pcb_true;
}

/* XPM */
static const char *rect_icon[] = {
/* columns rows colors chars-per-pixel */
"21 21 3 1",
"  c black",
". c #6EA5D7",
"o c None",
/* pixels */
"ooooooooooooooooooooo",
"oo..................o",
"oo..................o",
"oo..................o",
"oo..................o",
"oo..................o",
"oo..................o",
"oo..................o",
"oo..................o",
"oo..................o",
"ooooooooooooooooooooo",
"ooooooooooooooooooooo",
"o   oo    oo  oo     ",
"o oo o oooo oo ooo oo",
"o oo o oooo oooooo oo",
"o   oo oooo oooooo oo",
"o o oo   oo oooooo oo",
"o oo o oooo oooooo oo",
"o oo o oooo oo ooo oo",
"o oo o    oo  oooo oo",
"ooooooooooooooooooooo"
};


pcb_tool_t pcb_tool_rectangle = {
	"rectangle", NULL, 100, rect_icon, PCB_TOOL_CURSOR_NAMED("ul_angle"), 0,
	NULL,
	pcb_tool_rectangle_uninit,
	pcb_tool_rectangle_notify_mode,
	NULL,
	pcb_tool_rectangle_adjust_attached_objects,
	NULL,
	pcb_tool_rectangle_anydo_act,
	pcb_tool_rectangle_anydo_act,
	
	pcb_false
};
