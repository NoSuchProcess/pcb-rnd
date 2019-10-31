/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  ttf glyph import
 *  pcb-rnd Copyright (C) 2018 Tibor 'Igor2' Palinkas
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

#include <stdio.h>
#include <stdlib.h>

#include "board.h"
#include "data.h"

#include "actions.h"
#include "plugins.h"
#include "hid.h"

#include "ttf_load.h"

static const char *ttf_cookie = "ttf importer";

static const char pcb_acts_LoadTtfGlyphs[] = "LoadTtfGlyphs(filename, srcglyps, [dstchars])";
static const char pcb_acth_LoadTtfGlyphs[] = "Loads glyphs from an outline ttf in the specified source range, optionally remapping them to dstchars range in the pcb-rnd font";
fgw_error_t pcb_act_LoadTtfGlyphs(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	PCB_ACT_IRES(-1);
	return 0;
}

pcb_action_t ttf_action_list[] = {
	{"LoadTtfGlyphs", pcb_act_LoadTtfGlyphs, pcb_acth_LoadTtfGlyphs, pcb_acts_LoadTtfGlyphs}
};

int pplg_check_ver_import_ttf(int ver_needed) { return 0; }

void pplg_uninit_import_ttf(void)
{
	pcb_remove_actions_by_cookie(ttf_cookie);
}

int pplg_init_import_ttf(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(ttf_action_list, ttf_cookie)
	return 0;
}
