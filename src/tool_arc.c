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
 *  Contact addresses for paper mail and Email:
 *  Harry Eaton, 6697 Buttonhole Ct, Columbia, MD 21044, USA
 *  haceaton@aplcomm.jhuapl.edu
 *
 */

#include "config.h"
#include "conf_core.h"

#include "action_helper.h"
#include "board.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "undo.h"

#include "obj_arc_draw.h"


void pcb_tool_arc_notify_mode(void)
{
	switch (pcb_crosshair.AttachedBox.State) {
	case PCB_CH_STATE_FIRST:
		pcb_crosshair.AttachedBox.Point1.X = pcb_crosshair.AttachedBox.Point2.X = Note.X;
		pcb_crosshair.AttachedBox.Point1.Y = pcb_crosshair.AttachedBox.Point2.Y = Note.Y;
		pcb_crosshair.AttachedBox.State = PCB_CH_STATE_SECOND;
		break;

	case PCB_CH_STATE_SECOND:
	case PCB_CH_STATE_THIRD:
		{
			pcb_arc_t *arc;
			pcb_coord_t wx, wy;
			pcb_angle_t sa, dir;

			wx = Note.X - pcb_crosshair.AttachedBox.Point1.X;
			wy = Note.Y - pcb_crosshair.AttachedBox.Point1.Y;
			if (PCB_XOR(pcb_crosshair.AttachedBox.otherway, coord_abs(wy) > coord_abs(wx))) {
				pcb_crosshair.AttachedBox.Point2.X = pcb_crosshair.AttachedBox.Point1.X + coord_abs(wy) * PCB_SGNZ(wx);
				sa = (wx >= 0) ? 0 : 180;
				dir = (PCB_SGNZ(wx) == PCB_SGNZ(wy)) ? 90 : -90;
			}
			else {
				pcb_crosshair.AttachedBox.Point2.Y = pcb_crosshair.AttachedBox.Point1.Y + coord_abs(wx) * PCB_SGNZ(wy);
				sa = (wy >= 0) ? -90 : 90;
				dir = (PCB_SGNZ(wx) == PCB_SGNZ(wy)) ? -90 : 90;
				wy = wx;
			}
			if (coord_abs(wy) > 0 && (arc = pcb_arc_new(CURRENT,
																										pcb_crosshair.AttachedBox.Point2.X,
																										pcb_crosshair.AttachedBox.Point2.Y,
																										coord_abs(wy),
																										coord_abs(wy),
																										sa,
																										dir,
																										conf_core.design.line_thickness,
																										2 * conf_core.design.clearance,
																										pcb_flag_make(conf_core.editor.clear_line ? PCB_FLAG_CLEARLINE : 0)))) {
				pcb_obj_add_attribs(arc, PCB->pen_attr);
				pcb_arc_get_end(arc, 1, &pcb_crosshair.AttachedBox.Point2.X, &pcb_crosshair.AttachedBox.Point2.Y);
				pcb_crosshair.AttachedBox.Point1.X = pcb_crosshair.AttachedBox.Point2.X;
				pcb_crosshair.AttachedBox.Point1.Y = pcb_crosshair.AttachedBox.Point2.Y;
				pcb_undo_add_obj_to_create(PCB_TYPE_ARC, CURRENT, arc, arc);
				pcb_undo_inc_serial();
				pcb_added_lines++;
				pcb_arc_invalidate_draw(CURRENT, arc);
				pcb_draw();
				pcb_crosshair.AttachedBox.State = PCB_CH_STATE_THIRD;
			}
			break;
		}
	}
}
