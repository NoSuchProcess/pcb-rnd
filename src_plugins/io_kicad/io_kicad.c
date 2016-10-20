/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 * 
 *  This module, io_kicad, was written and is Copyright (C) 2016 by Tibor Palinkas
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
#include "plug_io.h"
#include "write.h"
#include "read.h"

static plug_io_t io_kicad;

int io_kicad_fmt(plug_io_t *ctx, plug_iot_t typ, int wr, const char *fmt)
{
	if (strcmp(ctx->description, fmt) == 0)
		return 200;

	if ((strcmp(fmt, "kicad") != 0) ||
		((typ & (~(PCB_IOT_FOOTPRINT | PCB_IOT_BUFFER | PCB_IOT_PCB))) != 0))
		return 0;
	if (wr)
		return 100;
	return 0; /* no read support yet */
}

static void hid_io_kicad_uninit(void)
{
	/* Runs once when the plugin is unloaded. TODO: free plugin-globals here. */
}

pcb_uninit_t hid_io_kicad_init(void)
{

	/* register the IO hook */
	io_kicad.plugin_data = NULL;
	io_kicad.fmt_support_prio = io_kicad_fmt;
	io_kicad.parse_pcb = io_kicad_read_pcb;
	io_kicad.parse_element = NULL;
	io_kicad.parse_font = NULL;
	io_kicad.write_buffer = io_kicad_write_buffer;
	io_kicad.write_element = io_kicad_write_element;
	io_kicad.write_pcb = io_kicad_write_pcb;
	io_kicad.default_fmt = "kicad";
	io_kicad.description = "Kicad, s-expression";
	io_kicad.save_preference_prio = 92;

	HOOK_REGISTER(plug_io_t, plug_io_chain, &io_kicad);

	/* TODO: Alloc plugin-globals here. */

	return hid_io_kicad_uninit;
}

