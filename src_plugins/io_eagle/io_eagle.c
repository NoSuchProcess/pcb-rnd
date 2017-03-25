/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
 * 
 *  This module, io_eagle, was written and is Copyright (C) 2016 by Tibor Palinkas
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
#include "hid.h"
#include "plug_io.h"
#include "read.h"

static pcb_plug_io_t io_eagle;
static const char *eagle_cookie = "eagle plugin";

int io_eagle_fmt(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, int wr, const char *fmt)
{
	if (strcmp(ctx->description, fmt) == 0)
		return 200;

	if ((strcmp(fmt, "eagle") != 0) ||
		((typ & (~(PCB_IOT_FOOTPRINT | PCB_IOT_BUFFER | PCB_IOT_PCB))) != 0))
		return 0;

	return 100;
}

static void hid_io_eagle_uninit(void)
{
	/* Runs once when the plugin is unloaded. TODO: free plugin-globals here. */
	pcb_hid_remove_actions_by_cookie(eagle_cookie);
}

#include "dolists.h"

pcb_uninit_t hid_io_eagle_init(void)
{

	/* register the IO hook */
	io_eagle.plugin_data = NULL;
/*	io_eagle.fmt_support_prio = io_eagle_fmt;
	io_eagle.test_parse_pcb = io_eagle_test_parse_pcb;
	io_eagle.parse_pcb = io_eagle_read_pcb;
	io_eagle.parse_element = NULL;
	io_eagle.parse_font = NULL;
	io_eagle.write_buffer = io_eagle_write_buffer;
	io_eagle.write_element = io_eagle_write_element;
	io_eagle.write_pcb = io_eagle_write_pcb;*/
	io_eagle.default_fmt = "eagle";
	io_eagle.description = "eagle xml";
	io_eagle.save_preference_prio = 40;
	io_eagle.default_extension = ".eagle_pcb";
	io_eagle.fp_extension = ".eagle_mod";
	io_eagle.mime_type = "application/x-eagle-pcb";

	PCB_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_eagle);

	return hid_io_eagle_uninit;
}

