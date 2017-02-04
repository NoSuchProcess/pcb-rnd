/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  Specctra .dsn import HID
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <gensexpr/gsxl.h>

#include "board.h"
#include "data.h"

#include "action_helper.h"
#include "hid.h"
#include "plugins.h"

static const char *dsn_cookie = "dsn importer";

static const char load_dsn_syntax[] = "LoadDsnFrom(filename)";

static const char load_dsn_help[] = "Loads the specified routed dsn file.";

int pcb_act_LoadDsnFrom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname = NULL;
	pcb_coord_t clear;
	FILE *f;
	gsxl_dom_t dom;
	int c, seek_quote = 1;
	gsx_parse_res_t res;

	fname = argc ? argv[0] : 0;

	if ((fname == NULL) || (*fname == '\0')) {
		fname = pcb_gui->fileselect(
			"Load a routed dsn file...",
			"Select dsn file to load.\nThe file could be generated using the tool downloaded from freeroute.net\n",
			NULL, /* default file name */
			".dsn", "dsn", HID_FILESELECT_READ);
		if (fname == NULL)
			PCB_AFAIL(load_dsn);
	}

	/* load and parse the file into a dom tree */
	f = fopen(fname, "r");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "import_dsn: can't open %s for read\n", fname);
		return 1;
	}
	gsxl_init(&dom, gsxl_node_t);
	dom.parse.line_comment_char = '#';
	do {
		c = fgetc(f);
		res = gsxl_parse_char(&dom, c);

		/* Workaround: skip the unbalanced '"' in string_quote */
		if (seek_quote && (dom.parse.used == 12) && (strncmp(dom.parse.atom, "string_quote", dom.parse.used) == 0)) {
			do {
				c = fgetc(f);
			} while((c != ')') && (c != EOF));
			res = gsxl_parse_char(&dom, ')');
			seek_quote = 0;
		}
	} while(res == GSX_RES_NEXT);
	fclose(f);

	if (res != GSX_RES_EOE) {
		pcb_message(PCB_MSG_ERROR, "import_dsn: parse error in %s at %d:%d\n", fname, dom.parse.line, dom.parse.col);
		gsxl_uninit(&dom);
		return 1;
	}
	gsxl_compact_tree(&dom);


	/* parse the tree */
	clear = PCB->RouteStyle.array[0].Clearance * 2;
	

	gsxl_uninit(&dom);
	return 0;
}

pcb_hid_action_t dsn_action_list[] = {
	{"LoadDsnFrom", 0, pcb_act_LoadDsnFrom, load_dsn_help, load_dsn_syntax}
};

PCB_REGISTER_ACTIONS(dsn_action_list, dsn_cookie)

static void hid_dsn_uninit()
{

}

#include "dolists.h"
pcb_uninit_t hid_import_dsn_init()
{
	PCB_REGISTER_ACTIONS(dsn_action_list, dsn_cookie)
	return hid_dsn_uninit;
}

