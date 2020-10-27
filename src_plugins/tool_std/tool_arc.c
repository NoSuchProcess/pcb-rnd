/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 1994,1995,1996 Thomas Nau
 *  Copyright (C) 1997, 1998, 1999, 2000, 2001 Harry Eaton
 *  Copyright (C) 2017,2019 Tibor 'Igor2' Palinkas
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
#include <librnd/core/hidlib_conf.h>

#include "board.h"
#include "crosshair.h"
#include "data.h"
#include "draw.h"
#include "draw_wireframe.h"
#include "search.h"
#include <librnd/core/tool.h>
#include "undo.h"

#include "obj_arc_draw.h"


void pcb_tool_arc_init(void)
{
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_false);
	if (rnd_tool_prev_id == pcb_crosshair.tool_line && pcb_crosshair.AttachedLine.State != PCB_CH_STATE_FIRST) {
		pcb_crosshair.AttachedLine.State = PCB_CH_STATE_FIRST;
		pcb_crosshair.AttachedBox.State = PCB_CH_STATE_SECOND;
		pcb_crosshair.AttachedBox.Point1.X = pcb_crosshair.AttachedBox.Point2.X = pcb_crosshair.AttachedLine.Point1.X;
		pcb_crosshair.AttachedBox.Point1.Y = pcb_crosshair.AttachedBox.Point2.Y = pcb_crosshair.AttachedLine.Point1.Y;
		rnd_tool_adjust_attached(NULL);
	}
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_true);
}

void pcb_tool_arc_uninit(void)
{
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_false);
	pcb_added_lines = 0;
	if (rnd_tool_next_id != pcb_crosshair.tool_line) {
		pcb_crosshair.AttachedBox.State = PCB_CH_STATE_FIRST;
		if (!pcb_marked.user_placed)
			pcb_crosshair_set_local_ref(0, 0, rnd_false);
	}
	rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_true);
}

void pcb_tool_arc_notify_mode(rnd_hidlib_t *hl)
{
	pcb_board_t *pcb = (pcb_board_t *)hl;

	switch (pcb_crosshair.AttachedBox.State) {
	case PCB_CH_STATE_FIRST:
		pcb_crosshair.AttachedBox.Point1.X = pcb_crosshair.AttachedBox.Point2.X = hl->tool_x;
		pcb_crosshair.AttachedBox.Point1.Y = pcb_crosshair.AttachedBox.Point2.Y = hl->tool_y;
		pcb_crosshair.AttachedBox.State = PCB_CH_STATE_SECOND;
		break;

	case PCB_CH_STATE_SECOND:
	case PCB_CH_STATE_THIRD:
		{
			pcb_arc_t *arc;
			rnd_coord_t wx, wy;
			rnd_angle_t sa, dir;
			wx = hl->tool_x - pcb_crosshair.AttachedBox.Point1.X;
			wy = hl->tool_y - pcb_crosshair.AttachedBox.Point1.Y;
rnd_trace("arc: %mm %mm wh %mm %mm\n", hl->tool_x, hl->tool_y, wx, wy);
			if (RND_XOR(pcb_crosshair.AttachedBox.otherway, coord_abs(wy) > coord_abs(wx))) {
				pcb_crosshair.AttachedBox.Point2.X = pcb_crosshair.AttachedBox.Point1.X + coord_abs(wy) * RND_SGNZ(wx);
				sa = (wx >= 0) ? 0 : 180;
				dir = (RND_SGNZ(wx) == RND_SGNZ(wy)) ? 90 : -90;
			}
			else {
				pcb_crosshair.AttachedBox.Point2.Y = pcb_crosshair.AttachedBox.Point1.Y + coord_abs(wx) * RND_SGNZ(wy);
				sa = (wy >= 0) ? -90 : 90;
				dir = (RND_SGNZ(wx) == RND_SGNZ(wy)) ? -90 : 90;
				wy = wx;
			}
			if (coord_abs(wy) > 0 && (arc = pcb_arc_new(pcb_loose_subc_layer(pcb, PCB_CURRLAYER(pcb), rnd_true),
																										pcb_crosshair.AttachedBox.Point2.X,
																										pcb_crosshair.AttachedBox.Point2.Y,
																										coord_abs(wy),
																										coord_abs(wy),
																										sa,
																										dir,
																										conf_core.design.line_thickness,
																										2 * conf_core.design.clearance,
																										pcb_flag_make(conf_core.editor.clear_line ? PCB_FLAG_CLEARLINE : 0), rnd_true))) {
				pcb_obj_add_attribs((pcb_any_obj_t *)arc, pcb->pen_attr, NULL);
				pcb_arc_get_end(arc, 1, &pcb_crosshair.AttachedBox.Point2.X, &pcb_crosshair.AttachedBox.Point2.Y);
				pcb_crosshair.AttachedBox.Point1.X = pcb_crosshair.AttachedBox.Point2.X;
				pcb_crosshair.AttachedBox.Point1.Y = pcb_crosshair.AttachedBox.Point2.Y;
				pcb_undo_add_obj_to_create(PCB_OBJ_ARC, PCB_CURRLAYER(pcb), arc, arc);
				pcb_undo_inc_serial();
				pcb_added_lines++;
				pcb_arc_invalidate_draw(PCB_CURRLAYER(pcb), arc);
				pcb_subc_as_board_update(pcb);
				pcb_draw();
				pcb_crosshair.AttachedBox.State = PCB_CH_STATE_THIRD;
			}
			break;
		}
	}
}

void pcb_tool_arc_adjust_attached_objects(rnd_hidlib_t *hl)
{
	pcb_crosshair.AttachedBox.otherway = rnd_gui->shift_is_pressed(rnd_gui);
}

void pcb_tool_arc_draw_attached(rnd_hidlib_t *hl)
{
	pcb_board_t *pcb = (pcb_board_t *)hl;

	if (pcb_crosshair.AttachedBox.State != PCB_CH_STATE_FIRST) {
		pcb_xordraw_attached_arc(conf_core.design.line_thickness);
		if (conf_core.editor.show_drc) {
			rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.drc);
			pcb_xordraw_attached_arc(conf_core.design.line_thickness + 2 * (conf_core.design.clearance + 1));
			rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.attached);
		}
	}
	else {
		/* Draw a circle (0 length line) to show where the arc will start when placed */
		if(PCB_CURRLAYER(pcb))
			rnd_render->set_color(pcb_crosshair.GC, &PCB_CURRLAYER(pcb)->meta.real.color);

		pcb_draw_wireframe_line(pcb_crosshair.GC,
			pcb_crosshair.X, pcb_crosshair.Y,
			pcb_crosshair.X, pcb_crosshair.Y,
			conf_core.design.line_thickness, 0);

		if(conf_core.editor.show_drc) {
			rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.drc);
			pcb_draw_wireframe_line(pcb_crosshair.GC,
				pcb_crosshair.X, pcb_crosshair.Y,
				pcb_crosshair.X, pcb_crosshair.Y,
				conf_core.design.line_thickness + (2 * conf_core.design.clearance), 0);
			rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.attached);
		}
	}

}

rnd_bool pcb_tool_arc_undo_act(rnd_hidlib_t *hl)
{
	if (pcb_crosshair.AttachedBox.State == PCB_CH_STATE_SECOND) {
		pcb_crosshair.AttachedBox.State = PCB_CH_STATE_FIRST;
		rnd_hid_notify_crosshair_change(&PCB->hidlib, rnd_true);
		return rnd_false;
	}
	if (pcb_crosshair.AttachedBox.State == PCB_CH_STATE_THIRD) {
		void *ptr1, *ptr2, *ptr3;
		/* guaranteed to succeed */
		pcb_search_obj_by_location(PCB_OBJ_ARC, &ptr1, &ptr2, &ptr3,
													 pcb_crosshair.AttachedBox.Point1.X, pcb_crosshair.AttachedBox.Point1.Y, 0);
		pcb_arc_get_end((pcb_arc_t *) ptr2, 0, &pcb_crosshair.AttachedBox.Point2.X, &pcb_crosshair.AttachedBox.Point2.Y);
		pcb_crosshair.AttachedBox.Point1.X = pcb_crosshair.AttachedBox.Point2.X;
		pcb_crosshair.AttachedBox.Point1.Y = pcb_crosshair.AttachedBox.Point2.Y;
		rnd_tool_adjust_attached(hl);
		if (--pcb_added_lines == 0)
			pcb_crosshair.AttachedBox.State = PCB_CH_STATE_SECOND;
	}
	return rnd_true;
}

void pcb_tool_arc_escape(rnd_hidlib_t *hl)
{
	if (pcb_crosshair.AttachedBox.State == PCB_CH_STATE_FIRST)
		rnd_tool_select_by_name(hl, "arrow");
	else
		rnd_tool_select_by_name(hl, "arc");
}

/* XPM */
static const char *arc_icon[] = {
/* columns rows colors chars-per-pixel */
"21 21 3 1",
"  c #000000",
". c #6EA5D7",
"o c None",
/* pixels */
"ooooo.ooooooooooooooo",
"ooooo.ooooooooooooooo",
"ooooo.ooooooooooooooo",
"ooooo.ooooooooooooooo",
"oooooo.oooooooooooooo",
"oooooo.oooooooooooooo",
"ooooooo.ooooooooooooo",
"ooooooo..oooooooooooo",
"oooooooo..ooooooooooo",
"oooooooooo..ooooooooo",
"oooooooooooo....ooooo",
"ooooooooooooooooooooo",
"ooo oooo    oooo   oo",
"oo o ooo ooo oo ooo o",
"oo o ooo ooo oo ooo o",
"o ooo oo    ooo ooooo",
"o     oo  ooooo ooooo",
"o ooo oo o oooo ooooo",
"o ooo oo oo ooo ooo o",
"o ooo oo ooo ooo   oo",
"ooooooooooooooooooooo"
};

rnd_tool_t pcb_tool_arc = {
	"arc", NULL, NULL, 100, arc_icon, RND_TOOL_CURSOR_NAMED("question_arrow"), 0,
	pcb_tool_arc_init,
	pcb_tool_arc_uninit,
	pcb_tool_arc_notify_mode,
	NULL,
	pcb_tool_arc_adjust_attached_objects,
	pcb_tool_arc_draw_attached,
	pcb_tool_arc_undo_act,
	NULL,
	pcb_tool_arc_escape,
	
	0
};
