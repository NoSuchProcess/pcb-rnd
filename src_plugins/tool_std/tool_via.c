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
#include <librnd/core/hid_inlines.h>
#include "data.h"
#include "draw.h"
#include <librnd/core/tool.h>
#include "tool_thermal.h"
#include "undo.h"

#include "obj_pstk_draw.h"

TODO("padstack: remove this when via is removed and the padstack is created from style directly")
#include "src_plugins/lib_compat_help/pstk_compat.h"

void pcb_tool_via_notify_mode(rnd_hidlib_t *hl)
{
	pcb_board_t *pcb = (pcb_board_t *)hl;

	if (!pcb->pstk_on) {
		rnd_message(RND_MSG_WARNING, "You must turn via visibility on before\nyou can place vias\n");
		return;
	}

	if (conf_core.design.via_drilling_hole >= conf_core.design.via_thickness) {
		rnd_message(RND_MSG_ERROR, "Can't place via: invalid via geometry (hole too large for via size)\n");
		return;
	}

TODO("pstk #21: do not work in comp mode, use a pstk proto - scconfig also has TODO #21, fix it there too")
	{
		pcb_pstk_t *ps = pcb_pstk_new_compat_via(pcb->Data, -1, hl->tool_x, hl->tool_y,
			conf_core.design.via_drilling_hole, conf_core.design.via_thickness, conf_core.design.clearance,
			0, PCB_PSTK_COMPAT_ROUND, rnd_true);
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
}

static void xor_draw_fake_via(rnd_coord_t x, rnd_coord_t y, rnd_coord_t dia, rnd_coord_t clearance)
{
	rnd_coord_t r = (dia/2)+clearance;
	rnd_render->draw_arc(pcb_crosshair.GC, x, y, r, r, 0, 360);
}


void pcb_tool_via_draw_attached(rnd_hidlib_t *hl)
{
TODO("pstk: replace this when route style has a prototype")
	xor_draw_fake_via(pcb_crosshair.X, pcb_crosshair.Y, conf_core.design.via_thickness, 0);
	if (conf_core.editor.show_drc) {
		/* XXX: Naughty cheat - use the mask to draw DRC clearance! */
		rnd_render->set_color(pcb_crosshair.GC, &conf_core.appearance.color.drc);
		xor_draw_fake_via(pcb_crosshair.X, pcb_crosshair.Y, conf_core.design.via_thickness, conf_core.design.clearance);
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
