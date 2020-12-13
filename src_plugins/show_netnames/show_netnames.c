/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2020 Tibor 'Igor2' Palinkas
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
#include <librnd/core/actions.h>

#include "board.h"
#include "event.h"
#include <librnd/core/hid_menu.h>
#include "show_netnames_conf.h"

#include "menu_internal.c"
#include "conf_internal.c"

const char *pcb_show_netnames_cookie = "show_netnames plugin";
#define SHOW_NETNAMES_CONF_FN "show_netnames.conf"
conf_show_netnames_t conf_show_netnames;

static void show_netnames_invalidate(void)
{
	rnd_trace("show_netnames: invalidate\n");
}

static void show_netnames_brd_chg(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if (conf_show_netnames.plugins.show_netnames.enable)
		show_netnames_invalidate();
}

static void show_netnames_render(rnd_hidlib_t *hidlib, void *user_data, int argc, rnd_event_arg_t argv[])
{
	if (!conf_show_netnames.plugins.show_netnames.enable)
		return;
	rnd_trace("show_netnames: render\n");
}

int pplg_check_ver_show_netnames(int ver_needed) { return 0; }

void pplg_uninit_show_netnames(void)
{
	show_netnames_invalidate();

	rnd_conf_unreg_file(SHOW_NETNAMES_CONF_FN, show_netnames_conf_internal);
	rnd_hid_menu_unload(rnd_gui, pcb_show_netnames_cookie);
	rnd_event_unbind_allcookie(pcb_show_netnames_cookie);
	rnd_remove_actions_by_cookie(pcb_show_netnames_cookie);
	rnd_conf_unreg_fields("plugins/show_netnames/");
}

int pplg_init_show_netnames(void)
{
	RND_API_CHK_VER;

	rnd_conf_reg_file(SHOW_NETNAMES_CONF_FN, show_netnames_conf_internal);

#define conf_reg(field,isarray,type_name,cpath,cname,desc,flags) \
	rnd_conf_reg_field(conf_show_netnames, field,isarray,type_name,cpath,cname,desc,flags);
#include "show_netnames_conf_fields.h"

	rnd_event_bind(PCB_EVENT_BOARD_EDITED, show_netnames_brd_chg, NULL, pcb_show_netnames_cookie);
	rnd_event_bind(RND_EVENT_BOARD_CHANGED, show_netnames_brd_chg, NULL, pcb_show_netnames_cookie);
	rnd_event_bind(RND_EVENT_GUI_DRAW_OVERLAY_XOR, show_netnames_render, NULL, pcb_show_netnames_cookie);

	rnd_hid_menu_load(rnd_gui, NULL, pcb_show_netnames_cookie, 150, NULL, 0, show_netnames_menu, "plugin: show_netnames");
	return 0;
}
