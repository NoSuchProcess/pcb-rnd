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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include <librnd/core/plugins.h>
#include <librnd/core/hid_menu.h>
#include "plug_io.h"
#include "write.h"
#include "read.h"
#include "read_net.h"
#include <librnd/core/actions.h>

#include "menu_internal.c"

static pcb_plug_io_t io_kicad;
static const char *kicad_cookie = "kicad plugin";

int io_kicad_fmt(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, int wr, const char *fmt)
{
	if (strcmp(ctx->description, fmt) == 0)
		return 200;

	if ((strcmp(fmt, "kicad") != 0) ||
		((typ & (~(PCB_IOT_FOOTPRINT | PCB_IOT_BUFFER | PCB_IOT_PCB))) != 0))
		return 0;

	return 100;
}

rnd_action_t eeschema_action_list[] = {
	{"LoadEeschemaFrom", pcb_act_LoadeeschemaFrom, pcb_acth_LoadeeschemaFrom, pcb_acts_LoadeeschemaFrom}
};

int pplg_check_ver_io_kicad(int ver_needed) { return 0; }

void pplg_uninit_io_kicad(void)
{
	/* Runs once when the plugin is unloaded. TODO: free plugin-globals here. */
	rnd_remove_actions_by_cookie(kicad_cookie);
	RND_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_kicad);
	pcb_eeschema_uninit();
	rnd_hid_menu_unload(rnd_gui, kicad_cookie);
}

int pplg_init_io_kicad(void)
{
	RND_API_CHK_VER;

	/* register the IO hook */
	io_kicad.plugin_data = NULL;
	io_kicad.fmt_support_prio = io_kicad_fmt;
	io_kicad.test_parse = io_kicad_test_parse;
	io_kicad.parse_pcb = io_kicad_read_pcb;
	io_kicad.parse_footprint = io_kicad_parse_module;
	io_kicad.map_footprint = io_kicad_map_footprint;
	io_kicad.parse_font = NULL;
	io_kicad.write_buffer = NULL;
	io_kicad.write_subcs_head = io_kicad_write_subcs_head;
	io_kicad.write_subcs_subc = io_kicad_write_subcs_subc;
	io_kicad.write_subcs_tail = io_kicad_write_subcs_tail;
	io_kicad.write_pcb = io_kicad_write_pcb;
	io_kicad.default_fmt = "kicad";
	io_kicad.description = "Kicad, s-expression";
	io_kicad.save_preference_prio = 80;
	io_kicad.default_extension = ".kicad_pcb";
	io_kicad.fp_extension = ".kicad_mod";
	io_kicad.mime_type = "application/x-kicad-pcb";


	RND_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_kicad);

	RND_REGISTER_ACTIONS(eeschema_action_list, kicad_cookie);

	pcb_eeschema_init();

	rnd_hid_menu_load(rnd_gui, NULL, kicad_cookie, 190, NULL, 0, kicad_menu, "plugin: io_kicad");

	/* TODO: Alloc plugin-globals here. */

	return 0;
}

