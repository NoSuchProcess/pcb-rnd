/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  MUCS unixplot import HID
 *  pcb-rnd Copyright (C) 2017 Erich Heinzle
 *  Based on up2pcb.cc, a simple unixplot file to pcb syntax converter
 *  Copyright (C) 2001 Luis Claudio Gamboa Lopes
 *  And loosely based on dsn.c
 *  Copyright (C) 2008, 2011 Josh Jordan, Dan McMahill, and Jared Casper
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

/* This plugin imports unixplot format line and via data into pcb-rnd */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "board.h"
#include "data.h"
#include <librnd/core/error.h>
#include <librnd/core/rnd_printf.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/safe_fs.h>
#include <librnd/hid/hid_menu.h>
#include <librnd/core/actions.h>
#include <librnd/core/plugins.h>
#include "layer.h"
#include "conf_core.h"

#include "menu_internal.c"

static const char *mucs_cookie = "mucs importer";

static const char pcb_acts_LoadMucsFrom[] = "LoadMucsFrom(filename)";
static const char pcb_acth_LoadMucsFrom[] = "Loads the specified mucs routing file.";
fgw_error_t pcb_act_LoadMucsFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL;
	static char *default_file = NULL;
	FILE *fi;
	int c, c2;
	rnd_coord_t x1, y1, x2, y2, r;

	RND_ACT_MAY_CONVARG(1, FGW_STR, LoadMucsFrom, fname = argv[1].val.str);

	if (!(pcb_layer_flags(PCB, PCB_CURRLID(PCB)) & PCB_LYT_COPPER)) {
		rnd_message(RND_MSG_ERROR, "The currently active layer is not a copper layer.\n");
		RND_ACT_IRES(1);
		return 0;
	}

	if (!fname || !*fname) {
		fname = rnd_hid_fileselect(rnd_gui, "Load mucs routing session Resource File...",
																"Picks a mucs session resource file to load.\n"
																	"This file could be generated by mucs-pcb\n",
																default_file, ".l1", NULL, "unixplot", RND_HID_FSD_READ, NULL);
		if (fname == NULL) {
			RND_ACT_IRES(1);
			return 0;
		}
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}

		if (fname && *fname)
			default_file = rnd_strdup(fname);
	}

	fi = rnd_fopen(&PCB->hidlib, fname, "r");
	if (!fi) {
		rnd_message(RND_MSG_ERROR, "Can't load mucs unixplot file %s for read\n", fname);
		RND_ACT_IRES(1);
		return 0;
	}

	while ((c = getc(fi)) != EOF) {
/*		rnd_trace("Char: %d \n", c); */
		switch (c) {
			case 's':
				x1 = 100 * (getc(fi) + (getc(fi) * 256));
				y1 = 100 * (getc(fi) + (getc(fi) * 256));
				x2 = 100 * (getc(fi) + (getc(fi) * 256));
				y2 = 100 * (getc(fi) + (getc(fi) * 256));
				rnd_trace("s--%i %i %i %i ???\n", x1, y1, x2, y2);
				break;
			case 'l':
				x1 = (getc(fi) + (getc(fi) * 256));
				y1 = (getc(fi) + (getc(fi) * 256));
				x2 = (getc(fi) + (getc(fi) * 256));
				y2 = (getc(fi) + (getc(fi) * 256));
				rnd_trace("Line(%d %d %d %d 20 \" \")\n", x1, y1, x2, y2);
				/* consider a bounds checking function to censor absurd coord sizes */
				pcb_line_new(PCB_CURRLAYER(PCB), RND_MIL_TO_COORD(x1), RND_MIL_TO_COORD(y1), RND_MIL_TO_COORD(x2), RND_MIL_TO_COORD(y2), RND_MIL_TO_COORD(10), RND_MIL_TO_COORD(10), pcb_flag_make(PCB_FLAG_AUTO));
				break;
			case 'c':
				x1 = (getc(fi) + (getc(fi) * 256));
				y1 = (getc(fi) + (getc(fi) * 256));
				r = (getc(fi) + (getc(fi) * 256));
				rnd_trace("Via(%d %d 60 25 \"\" \" \")\n", x1, y1);
				pcb_pstk_new(PCB->Data, -1, conf_core.design.via_proto,
					RND_MIL_TO_COORD(x1), RND_MIL_TO_COORD(y1), conf_core.design.clearance, pcb_flag_make(PCB_FLAG_CLEARLINE | PCB_FLAG_AUTO));
				break;
			case 'n':
				x1 = (getc(fi) + (getc(fi) * 256));
				y1 = (getc(fi) + (getc(fi) * 256));
				rnd_trace("Line(%d %d %d %d 20 \" \")\n", x1, y1, x2, y2);
				pcb_line_new(PCB_CURRLAYER(PCB), RND_MIL_TO_COORD(x1), RND_MIL_TO_COORD(y1), RND_MIL_TO_COORD(x2), RND_MIL_TO_COORD(y2), RND_MIL_TO_COORD(10), RND_MIL_TO_COORD(10), pcb_flag_make(PCB_FLAG_AUTO));
				x2 = x1;
				y2 = y1;
				break;
			case 'a':
				x1 = 100 * ((getc(fi) * 256) + getc(fi));
				y1 = 100 * ((getc(fi) * 256) + getc(fi));
				x2 = 100 * ((getc(fi) * 256) + getc(fi));
				y2 = 100 * ((getc(fi) * 256) + getc(fi));
				r = 100 * ((getc(fi) * 256) + getc(fi));
				rnd_trace("a--stroke newpath\n%d %d %d %d %d arc\n", x1, y1, x2, y2, r);
				break;
			case 'e':
				break;
			case 't':
				do {
					c2 = getc(fi);
				} while (c2 != '\0' && c2 != EOF);
				break;
		}
	}
	fclose(fi);
	RND_ACT_IRES(0);
	return 0;
}

rnd_action_t mucs_action_list[] = {
	{"LoadMucsFrom", pcb_act_LoadMucsFrom, pcb_acth_LoadMucsFrom, pcb_acts_LoadMucsFrom}
};

int pplg_check_ver_import_mucs(int ver_needed) { return 0; }

void pplg_uninit_import_mucs(void)
{
	rnd_remove_actions_by_cookie(mucs_cookie);
	rnd_hid_menu_unload(rnd_gui, mucs_cookie);
}

int pplg_init_import_mucs(void)
{
	RND_API_CHK_VER;
	RND_REGISTER_ACTIONS(mucs_action_list, mucs_cookie)
	rnd_hid_menu_load(rnd_gui, NULL, mucs_cookie, 125, NULL, 0, mucs_menu, "plugin: import_mucs");
	return 0;
}
