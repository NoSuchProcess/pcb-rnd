/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include "tool.h"

#include "error.h"


static void default_tool_reg(void);
static void default_tool_unreg(void);

void pcb_tool_init(void)
{
	vtp0_init(&pcb_tools);
	default_tool_reg(); /* temporary */
}

void pcb_tool_uninit(void)
{
	default_tool_unreg(); /* temporary */
	while(vtp0_len(&pcb_tools) != 0) {
		const pcb_tool_t *tool = pcb_tool_get(0);
		pcb_message(PCB_MSG_WARNING, "Unregistered tool: %s of %s; check your plugins, fix them to unregister their tools!\n", tool->name, tool->cookie);
		pcb_tool_unreg_by_cookie(tool->cookie);
	}
	vtp0_uninit(&pcb_tools);
}

int pcb_tool_reg(const pcb_tool_t *tool)
{
	if (pcb_tool_lookup(tool->name) != PCB_TOOLID_INVALID) /* don't register two tools with the same name */
		return -1;
	vtp0_append(&pcb_tools, (void *)tool);
	return 0;
}

void pcb_tool_unreg_by_cookie(const char *cookie)
{
	pcb_toolid_t n;
	for(n = 0; n < vtp0_len(&pcb_tools); n++) {
		const pcb_tool_t *tool = pcb_tool_get(0);
		if (tool->cookie == cookie) {
			vtp0_remove(&pcb_tools, n, 1);
			n--;
		}
	}
}

pcb_toolid_t pcb_tool_lookup(const char *name)
{
	pcb_toolid_t n;
	for(n = 0; n < vtp0_len(&pcb_tools); n++) {
		const pcb_tool_t *tool = (const pcb_tool_t *)pcb_tools.array[n];
		if (strcmp(tool->name, name) == 0)
			return n;
	}
	return PCB_TOOLID_INVALID;
}



#warning tool TODO: move this out to a tool plugin

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

const char *cookie = "default tools";

static void default_tool_reg(void)
{
	
}

static void default_tool_unreg(void)
{
	pcb_tool_unreg_by_cookie(cookie);
}

