/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
 *
 *  This module, dialogs, was written and is Copyright (C) 2017 by Tibor Palinkas
 *  this module is also subject to the GNU GPL as described below
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include "conf_core.h"

#include <librnd/core/plugins.h>
#include "tool.h"

#include "tool_arc.h"
#include "tool_arrow.h"
#include "tool_buffer.h"
#include "tool_copy.h"
#include "tool_insert.h"
#include "tool_line.h"
#include "tool_lock.h"
#include "tool_move.h"
#include "tool_poly.h"
#include "tool_polyhole.h"
#include "tool_rectangle.h"
#include "tool_remove.h"
#include "tool_rotate.h"
#include "tool_text.h"
#include "tool_thermal.h"
#include "tool_via.h"

static const char pcb_tool_std_cookie[] = "tool_std";

int pplg_check_ver_tool_std(int ver_needed) { return 0; }

void pplg_uninit_tool_std(void)
{
	pcb_tool_unreg_by_cookie(pcb_tool_std_cookie);
}

int pplg_init_tool_std(void)
{
	PCB_API_CHK_VER;

	pcb_tool_reg(&pcb_tool_arc, pcb_tool_std_cookie);
	pcb_tool_reg(&pcb_tool_arrow, pcb_tool_std_cookie);
	pcb_tool_reg(&pcb_tool_buffer, pcb_tool_std_cookie);
	pcb_tool_reg(&pcb_tool_copy, pcb_tool_std_cookie);
	pcb_tool_reg(&pcb_tool_insert, pcb_tool_std_cookie);
	pcb_tool_reg(&pcb_tool_line, pcb_tool_std_cookie);
	pcb_tool_reg(&pcb_tool_lock, pcb_tool_std_cookie);
	pcb_tool_reg(&pcb_tool_move, pcb_tool_std_cookie);
	pcb_tool_reg(&pcb_tool_poly, pcb_tool_std_cookie);
	pcb_tool_reg(&pcb_tool_polyhole, pcb_tool_std_cookie);
	pcb_tool_reg(&pcb_tool_rectangle, pcb_tool_std_cookie);
	pcb_tool_reg(&pcb_tool_remove, pcb_tool_std_cookie);
	pcb_tool_reg(&pcb_tool_rotate, pcb_tool_std_cookie);
	pcb_tool_reg(&pcb_tool_text, pcb_tool_std_cookie);
	pcb_tool_reg(&pcb_tool_thermal, pcb_tool_std_cookie);
	pcb_tool_reg(&pcb_tool_via, pcb_tool_std_cookie);

	return 0;
}
