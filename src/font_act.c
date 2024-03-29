/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2017,2023 Tibor 'Igor2' Palinkas
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
 *  Contact:
 *    Project page: http://repo.hu/projects/pcb-rnd
 *    lead developer: http://repo.hu/projects/pcb-rnd/contact.html
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */
#include "config.h"
#include <stdlib.h>
#include "board.h"
#include <librnd/core/actions.h>
#include <librnd/core/error.h>
#include <librnd/hid/hid_dad.h>
#include "font.h"
#include "conf_core.h"
#include "plug_io.h"
#include "event.h"

#define PCB (do not use PCB directly)

static const char pcb_acts_load_font_from[] = "LoadFontFrom([file, id])";
static const char pcb_acth_load_font_from[] = "Load PCB font from a file";

fgw_error_t pcb_act_load_font_from(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL, *sid = NULL;
	static char *default_file = NULL;
	rnd_font_id_t fid, dst_fid = -1;
	rnd_font_t *fnt;
	int r;

	RND_ACT_MAY_CONVARG(1, FGW_STR, load_font_from, fname = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, load_font_from, sid = argv[2].val.str);

	if (sid != NULL) {
		char *end;
		fid = strtol(sid, &end, 10);
		if (*end != '\0') {
			rnd_message(RND_MSG_ERROR, "LoadFontFrom(): when second argument is present, it must be an integer\n");
			return 1;
		}
		if (pcb_font(PCB_ACT_BOARD, fid, 0) != NULL) { /* font already exists */
			dst_fid = fid;
			fid = -1; /* allocate a new id, which will be later moved to dst_fid */
		}
	}
	else
		fid = -1; /* auto-allocate a new ID */

	if (!fname || !*fname) {
		static const char *flt_lht[]   = {"*.lht", NULL};
		static const char *flt_pcb[]   = {"*.font", "*.pcb_font", NULL};
		static const char *flt_fonts[] = {"*.lht", "*.font", "*.pcb_font", NULL};
		static const char *flt_any[] = {"*", "*.*", NULL};
		const rnd_hid_fsd_filter_t flt[] = {
			{"any font",            NULL, flt_fonts},
			{"lihata pcb-rnd font", NULL, flt_lht},
			{"gEDA pcb font",       NULL, flt_pcb},
			{"all",                 NULL, flt_any},
			{NULL, NULL,NULL}
		};
		fname = rnd_hid_fileselect(rnd_gui,
			"Load PCB font file...", "Picks a PCB font file to load.\n",
			default_file, ".font", flt, "pcbfont", RND_HID_FSD_READ, NULL);
		if (fname == NULL)
			return 0; /* cancel */
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	fnt = pcb_new_font(&PCB_ACT_BOARD->fontkit, fid, NULL);
	if (fnt == NULL) {
		rnd_message(RND_MSG_ERROR, "LoadFontFrom(): unable to allocate font\n");
		return 1;
	}
	
	r = pcb_parse_font(fnt, fname);
	rnd_event(RND_ACT_DESIGN, PCB_EVENT_FONT_CHANGED, "i", fnt->id);
	if (r != 0) {
		rnd_message(RND_MSG_ERROR, "LoadFontFrom(): failed to load font from %s\n", fname);
		pcb_del_font(&PCB_ACT_BOARD->fontkit, fnt->id);
		return 1;
	}
	
	if (dst_fid != -1) {
		pcb_move_font(&PCB_ACT_BOARD->fontkit, fnt->id, dst_fid);
	}
	
	fid = dst_fid == 0 ? 0 : fnt->id;
	rnd_message(RND_MSG_INFO, "LoadFontFrom(): new font (ID %d) successfully loaded from file %s\n", fid, fname);
	RND_ACT_IRES(0);
	return 0;
}

static int font_fmt_selected; /* this is not reentrant but the dialog is modal */

static void font_fmt_change_cb(void *hid_ctx, void *caller_data, rnd_hid_attribute_t *attr)
{
	font_fmt_selected = attr->val.lng;
}

static const char pcb_acts_save_font_to[] = "SaveFontTo([file, id, fmt])";
static const char pcb_acth_save_font_to[] = "Save PCB font to a file";

fgw_error_t pcb_act_save_font_to(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fmts[] = {"lihata font v2", "lihata font v1", NULL};
	const char *fname = NULL, *sid = NULL, *fmt;
	static char *default_file = NULL;
	rnd_font_id_t fid;
	rnd_font_t *fnt;
	int r;

	/* pick default font */
	fmt = fmts[0];

	RND_ACT_MAY_CONVARG(1, FGW_STR, load_font_from, fname = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, load_font_from, sid = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, load_font_from, fmt = argv[3].val.str);

	if (sid != NULL) {
		char *end;
		fid = strtol(sid, &end, 10);
		if (*end != '\0') {
			rnd_message(RND_MSG_ERROR, "SaveFontTo(): when second argument is present, it must be an integer\n");
			return 1;
		}
		if (pcb_font(PCB_ACT_BOARD, fid, 0) == NULL) {
			rnd_message(RND_MSG_ERROR, "SaveFontTo(): can not fetch font ID %d\n", fid);
			return 1;
		}
	}
	else
		fid = conf_core.design.text_font_id;

	fnt = pcb_font(PCB_ACT_BOARD, fid, 0);
	if (fnt == NULL) {
		rnd_message(RND_MSG_ERROR, "SaveFontTo(): failed to fetch font %d\n", fid);
		return 1;
	}

	if (!fname || !*fname) {
		rnd_hid_dad_subdialog_t fmtsub = {0}, *fmtsubp = NULL;

		font_fmt_selected = 0;

		if (RND_HAVE_GUI_ATTR_DLG) {
			RND_DAD_ENUM(fmtsub.dlg, fmts);
				RND_DAD_CHANGE_CB(fmtsub.dlg, font_fmt_change_cb);
			fmtsubp = &fmtsub;
		}

		fname = rnd_hid_fileselect(rnd_gui, "Save PCB font file...",
																"Picks a PCB font file to save.\n",
																default_file, ".font", NULL, "pcbfont", 0, fmtsubp);
		if (fname == NULL)
			RND_ACT_FAIL(save_font_to);
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
		if (RND_HAVE_GUI_ATTR_DLG) {
			if ((font_fmt_selected >= 0) && (font_fmt_selected < sizeof(fmts)/sizeof(fmts[0]))) {
				fmt = fmts[font_fmt_selected];
			}
		}
	}

	r = pcb_write_font(fnt, fname, fmt);
	if (r != 0) {
		rnd_message(RND_MSG_ERROR, "SaveFontTo(): failed to save font to %s\n", fname);
		return 1;
	}

	RND_ACT_IRES(0);
	return 0;
}


static rnd_action_t font_action_list[] = {
	{"LoadFontFrom", pcb_act_load_font_from, pcb_acth_load_font_from, pcb_acts_load_font_from},
	{"SaveFontTo", pcb_act_save_font_to, pcb_acth_save_font_to, pcb_acts_save_font_to}
};

void pcb_font_act_init2(void)
{
	RND_REGISTER_ACTIONS(font_action_list, NULL);
}
