/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 * 
 *  This module, io_kicad_legacy, was written and is Copyright (C) 2016 by Tibor Palinkas
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
#include "plugins.h"
#include "plug_io.h"
#include "write.h"

static pcb_plug_io_t io_kicad_legacy;

int io_kicad_legacy_fmt(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, int wr, const char *fmt)
{
	if (strcmp(ctx->description, fmt) == 0)
		return 200;

	if ((strcmp(fmt, "kicadl") != 0) ||
		((typ & (~(PCB_IOT_FOOTPRINT | PCB_IOT_BUFFER | PCB_IOT_PCB))) != 0))
		return 0;
	if (wr)
		return 100;
	return 0; /* no read support yet */
}

static void hid_io_kicad_legacy_uninit(void)
{
	/* Runs once when the plugin is unloaded. TODO: free plugin-globals here. */
}

pcb_uninit_t hid_io_kicad_legacy_init(void)
{

	/* register the IO hook */
	io_kicad_legacy.plugin_data = NULL;
	io_kicad_legacy.fmt_support_prio = io_kicad_legacy_fmt;
	io_kicad_legacy.parse_pcb = NULL;
	io_kicad_legacy.parse_element = NULL;
	io_kicad_legacy.parse_font = NULL;
	io_kicad_legacy.write_buffer = io_kicad_legacy_write_buffer;
	io_kicad_legacy.write_element = io_kicad_legacy_write_element;
	io_kicad_legacy.write_pcb = io_kicad_legacy_write_pcb;
	io_kicad_legacy.default_fmt = "kicadl";
	io_kicad_legacy.description = "Kicad, legacy format";
	io_kicad_legacy.save_preference_prio = 90;

	PCB_HOOK_REGISTER(pcb_plug_io_t, plug_io_chain, &io_kicad_legacy);

	/* TODO: Alloc plugin-globals here. */

	return hid_io_kicad_legacy_uninit;
}

