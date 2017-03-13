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
#include "conf_core.h"
#include "plug_io.h"


static const char pcb_acts_load_font_from[] = "LoadFontFrom([file, id])";
static const char pcb_acth_load_font_from[] = "Load PCB font from a file";

int pcb_act_load_font_from(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname, *sid;
	static char *default_file = NULL;
	pcb_font_id_t fid;
	pcb_font_t *fnt;
	int res;

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
	else
		fid = -1; /* auto-allocate a new ID */

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect("Load PCB font file...",
																"Picks a PCB font file to load.\n",
																default_file, ".font", "pcbfont", HID_FILESELECT_READ);
		if (fname == NULL)
			PCB_ACT_FAIL(load_font_from);
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	fnt = pcb_new_font(&PCB->fontkit, fid, NULL);
	if (fnt == NULL) {
		pcb_message(PCB_MSG_ERROR, "LoadFontFrom(): unable to allocate font\n");
		return 1;
	}
	
	res = pcb_parse_font(fnt, fname);
	if (res != 0) {
		pcb_message(PCB_MSG_ERROR, "LoadFontFrom(): failed to load font from %s\n", fname);
		pcb_del_font(&PCB->fontkit, fnt->id);
		return 1;
	}

	pcb_message(PCB_MSG_INFO, "LoadFontFrom(): new font (ID %d) successfully loaded from file %s\n", fnt->id, fname);
	return 0;
}


static const char pcb_acts_save_font_to[] = "SaveFontTo([file, id])";
static const char pcb_acth_save_font_to[] = "Save PCB font to a file";

int pcb_act_save_font_to(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname, *sid;
	static char *default_file = NULL;
	pcb_font_id_t fid;
	pcb_font_t *fnt;
	int res;

	fname = (argc > 0) ? argv[0] : NULL;
	sid = (argc > 1) ? argv[1] : NULL;

	if (sid != NULL) {
		char *end;
		fid = strtol(sid, &end, 10);
		if (*end != '\0') {
			pcb_message(PCB_MSG_ERROR, "SaveFontTo(): when second argument is present, it must be an integer\n");
			return 1;
		}
		if (pcb_font(PCB, fid, 0) == NULL) {
			pcb_message(PCB_MSG_ERROR, "SaveFontTo(): can not fetch font ID %d\n", fid);
			return 1;
		}
	}
	else
		fid = conf_core.design.text_font_id;

	fnt = pcb_font(PCB, fid, 0);
	if (fnt == NULL) {
		pcb_message(PCB_MSG_ERROR, "SaveFontTo(): failed to fetch font %d\n", fid);
		return 1;
	}

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect("Load PCB font file...",
																"Picks a PCB font file to load.\n",
																default_file, ".font", "pcbfont", HID_FILESELECT_MAY_NOT_EXIST);
		if (fname == NULL)
			PCB_ACT_FAIL(save_font_to);
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	res = pcb_write_font(fnt, fname, "lihata");
	if (res != 0) {
		pcb_message(PCB_MSG_ERROR, "SaveFontTo(): failed to save font to %s\n", fname);
		return 1;
	}

	return 0;
}


pcb_hid_action_t font_action_list[] = {
	{"LoadFontFrom", 0, pcb_act_load_font_from,
		pcb_acth_load_font_from, pcb_acts_load_font_from},
	{"SaveFontTo", 0, pcb_act_save_font_to,
		pcb_acth_save_font_to, pcb_acts_save_font_to}
};

PCB_REGISTER_ACTIONS(font_action_list, NULL)
