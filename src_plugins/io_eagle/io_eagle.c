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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include "plugins.h"
#include "hid.h"
#include "plug_io.h"
#include "read.h"
#include "read_dru.h"
#include "actions.h"

static pcb_plug_io_t io_eagle_xml, io_eagle_bin, io_eagle_dru;
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

int pplg_check_ver_io_eagle(int ver_needed) { return 0; }

void pplg_uninit_io_eagle(void)
{
	/* Runs once when the plugin is unloaded. TODO: free plugin-globals here. */
	pcb_remove_actions_by_cookie(eagle_cookie);
	PCB_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_eagle_xml);
	PCB_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_eagle_bin);
	PCB_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_eagle_dru);
}

#include "dolists.h"

int pplg_init_io_eagle(void)
{
	PCB_API_CHK_VER;

	/* register the IO hook */
	io_eagle_xml.plugin_data = NULL;
	io_eagle_xml.fmt_support_prio = io_eagle_fmt;
	io_eagle_xml.test_parse = io_eagle_test_parse_xml;
	io_eagle_xml.parse_pcb = io_eagle_read_pcb_xml;
/*	io_eagle_xml.parse_footprint = NULL;
	io_eagle_xml.parse_font = NULL;
	io_eagle_xml.write_buffer = io_eagle_write_buffer;
	io_eagle_xml.write_footprint = io_eagle_write_element;
	io_eagle_xml.write_pcb = io_eagle_write_pcb;*/
	io_eagle_xml.default_fmt = "eagle";
	io_eagle_xml.description = "eagle xml";
	io_eagle_xml.save_preference_prio = 40;
	io_eagle_xml.default_extension = ".eagle_pcb";
	io_eagle_xml.fp_extension = ".eagle_mod";
	io_eagle_xml.mime_type = "application/x-eagle-pcb";

	PCB_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_eagle_xml);

	/* register the IO hook */
	io_eagle_bin.plugin_data = NULL;
	io_eagle_bin.fmt_support_prio = io_eagle_fmt;
	io_eagle_bin.test_parse = io_eagle_test_parse_bin;
	io_eagle_bin.parse_pcb = io_eagle_read_pcb_bin;
/*	io_eagle_bin.parse_footprint = NULL;
	io_eagle_bin.parse_font = NULL;
	io_eagle_bin.write_buffer = io_eagle_write_buffer;
	io_eagle_bin.write_footprint = io_eagle_write_element;
	io_eagle_bin.write_pcb = io_eagle_write_pcb;*/
	io_eagle_bin.default_fmt = "eagle";
	io_eagle_bin.description = "eagle bin";
	io_eagle_bin.save_preference_prio = 30;
	io_eagle_bin.default_extension = ".brd";
	io_eagle_bin.fp_extension = ".???";
	io_eagle_bin.mime_type = "application/x-eagle-pcb";

	PCB_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_eagle_bin);

	/* register the IO hook */
	io_eagle_dru.plugin_data = NULL;
	io_eagle_dru.fmt_support_prio = io_eagle_fmt;
	io_eagle_dru.test_parse = io_eagle_test_parse_dru;
	io_eagle_dru.parse_pcb = io_eagle_read_pcb_dru;
	io_eagle_dru.parse_footprint = NULL;
	io_eagle_dru.parse_font = NULL;
	io_eagle_dru.write_buffer = NULL;
	io_eagle_dru.write_footprint = NULL;
	io_eagle_dru.write_pcb = /*io_eagle_write_pcb_dru*/ NULL;
	io_eagle_dru.default_fmt = "eagle";
	io_eagle_dru.description = "eagle dru";
	io_eagle_dru.save_preference_prio = 0;
	io_eagle_dru.default_extension = ".dru";
	io_eagle_dru.fp_extension = ".dru";
	io_eagle_dru.mime_type = "application/x-eagle-dru";

	PCB_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_eagle_dru);

	return 0;
}

