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
#include "change.h"
#include "compat_nls.h"
#include "data.h"
#include "draw.h"
#include "tool.h"
#include "undo.h"

#include "obj_pinvia_draw.h"


void pcb_tool_via_notify_mode(void)
{
	pcb_pin_t *via;

	if (!PCB->ViaOn) {
		pcb_message(PCB_MSG_WARNING, _("You must turn via visibility on before\n" "you can place vias\n"));
		return;
	}
	if ((via = pcb_via_new(PCB->Data, Note.X, Note.Y,
													conf_core.design.via_thickness, 2 * conf_core.design.clearance,
													0, conf_core.design.via_drilling_hole, NULL, pcb_no_flags())) != NULL) {
		pcb_obj_add_attribs(via, PCB->pen_attr);
		pcb_undo_add_obj_to_create(PCB_TYPE_VIA, via, via, via);
		if (pcb_gui->shift_is_pressed())
			pcb_chg_obj_thermal(PCB_TYPE_VIA, via, via, via, PCB->ThermStyle);
		pcb_undo_inc_serial();
		pcb_via_invalidate_draw(via);
		pcb_draw();
	}
}
