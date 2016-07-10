/*
 *                            COPYRIGHT
 *
 *  PCB, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 * 
 *  This module, io_pcb, was written and is Copyright (C) 2016 by Tibor Palinkas
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"
#include "global.h"
#include "plugins.h"
#include "parse_l.h"
#include "file.h"

static plug_io_t io_pcb;

int io_pcb_fmt(int wr, const char *fmt)
{
	if (strcmp(fmt, "pcb") != 0)
		return 0;
	return 100; /* read-write */
}

extern void yylex_destroy(void);
static void hid_io_pcb_uninit(void)
{
	yylex_destroy();
	HOOK_UNREGISTER(plug_io_t, plug_io_chain, &io_pcb);
}

pcb_uninit_t hid_io_pcb_init(void)
{

	/* register the IO hook */
	io_pcb.plugin_data = NULL;
	io_pcb.fmt_support_prio = io_pcb_fmt;
	io_pcb.parse_pcb = io_pcb_ParsePCB;
	io_pcb.parse_element = io_pcb_ParseElement;
	io_pcb.parse_font = io_pcb_ParseFont;
	io_pcb.write_buffer = io_pcb_WriteBuffer;
	io_pcb.write_element = io_pcb_WriteElementData;
	io_pcb.write_pcb = io_pcb_WritePCB;
	HOOK_REGISTER(plug_io_t, plug_io_chain, &io_pcb);

	return hid_io_pcb_uninit;
}

