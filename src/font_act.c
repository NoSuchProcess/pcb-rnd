/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#include "config.h"
#include <stdlib.h>
#include "board.h"
#include "hid.h"
#include "error.h"
#include "font.h"
#include "action_helper.h"


static const char pcb_acts_load_font_from[] = "LoadFontFrom()";
static const char pcb_acth_load_font_from[] = "Load PCB font from a file";

int pcb_act_load_font_from(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname, *sid;
	static char *default_file = NULL;
	pcb_font_id_t fid;

	fname = (argc > 0) ? argv[0] : NULL;
	sid = (argc > 1) ? argv[1] : NULL;

	if (sid != NULL) {
		char *end;
		fid = strtol(sid, &end, 10);
		if (*end != '\0') {
			pcb_message(PCB_MSG_ERROR, "LoadFontFrom(): when second argument is present, it must be an integer\n");
			return 1;
		}
		if (pcb_font(PCB, fid, 0) != NULL) {
			pcb_message(PCB_MSG_ERROR, "LoadFontFrom(): font ID %d is already taken\n", fid);
			return 1;
		}
	}
	else {
#warning TODO: allocate a new
	}

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect("Load PCB font file...",
																"Picks a PCB font file to load.\n",
																default_file, ".font", "pcnfont", HID_FILESELECT_READ);
		if (fname == NULL)
			PCB_ACT_FAIL(load_font_from);
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	abort();
}


pcb_hid_action_t font_action_list[] = {
	{"LoadFontFrom", 0, pcb_act_load_font_from,
		pcb_acth_load_font_from, pcb_acts_load_font_from}
};

PCB_REGISTER_ACTIONS(font_action_list, NULL)
