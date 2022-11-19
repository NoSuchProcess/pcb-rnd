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
#include "change.h"
#include <librnd/hid/hid_inlines.h>
#include "data.h"
#include "draw.h"
#include <librnd/hid/tool.h>
#include "tool_thermal.h"
#include "undo.h"

#include "obj_pstk_draw.h"

void pcb_tool_via_notify_mode(rnd_hidlib_t *hl)
{
	pcb_board_t *pcb = (pcb_board_t *)hl;
	pcb_pstk_t *ps;

	if (!pcb->pstk_on) {
		rnd_message(RND_MSG_WARNING, "You must turn via visibility on before\nyou can place vias\n");
		return;
	}

	ps = pcb_pstk_new(pcb->Data, -1, conf_core.design.via_proto,
		hl->tool_x, hl->tool_y, conf_core.design.clearance, pcb_flag_make(PCB_FLAG_CLEARLINE));

	if (ps == NULL)
		return;

	pcb_obj_add_attribs((pcb_any_obj_t *)ps, pcb->pen_attr, NULL);
	pcb_undo_add_obj_to_create(PCB_OBJ_PSTK, ps, ps, ps);

	if (rnd_gui->shift_is_pressed(rnd_gui))
		pcb_tool_thermal_on_obj((pcb_any_obj_t *)ps, PCB_CURRLID(pcb));

	pcb_undo_inc_serial();
	pcb_pstk_invalidate_draw(ps);
	pcb_draw();
}


void pcb_tool_via_draw_attached(rnd_hidlib_t *hl)
{
		static pcb_pstk_t ps; /* initialized to all-zero */

		ps.parent.data = PCB->Data;
		ps.parent_type = PCB_PARENT_DATA;
		ps.proto = conf_core.design.via_proto;
		ps.protoi = -1;
		ps.x = pcb_crosshair.X;
		ps.y = pcb_crosshair.Y;
		ps.Clearance = conf_core.design.clearance;
		pcb_pstk_thindraw(NULL, pcb_crosshair.GC, &ps);

		if (conf_core.editor.show_drc) {
			static rnd_xform_t xform;
			static pcb_draw_info_t info;
			info.xform = &xform;
			xform.bloat = conf_core.design.clearance*2;
			rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.drc);
			pcb_pstk_thindraw(&info, pcb_crosshair.GC, &ps);
			rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.attached);
		}
}

/* XPM */
static const char *via_icon[] = {
/* columns rows colors chars-per-pixel */
"21 21 3 1",
"  c #000000",
". c #7A8584",
"o c None",
/* pixels */
"ooooooooooooooooooooo",
"ooooooooo...ooooooooo",
"oooooooo.....oooooooo",
"ooooooo..ooo..ooooooo",
"oooooo..ooooo..oooooo",
"oooooo..ooooo..oooooo",
"oooooo..ooooo..oooooo",
"ooooooo..ooo..ooooooo",
"oooooooo.....oooooooo",
"ooooooooo...ooooooooo",
"ooooooooooooooooooooo",
"ooooooooooooooooooooo",
"ooooooooooooooooooooo",
"ooo ooo o   ooo ooooo",
"ooo ooo oo ooo o oooo",
"ooo ooo oo oo ooo ooo",
"oooo o ooo oo ooo ooo",
"oooo o ooo oo     ooo",
"oooo o ooo oo ooo ooo",
"ooooo ooo   o ooo ooo",
"ooooooooooooooooooooo"
};

rnd_tool_t pcb_tool_via = {
	"via", NULL, NULL, 100, via_icon, RND_TOOL_CURSOR_NAMED(NULL), 0,
	NULL,
	NULL,
	pcb_tool_via_notify_mode,
	NULL,
	NULL,
	pcb_tool_via_draw_attached,
	NULL,
	NULL,
	NULL, /* escape */
	
	0
};
