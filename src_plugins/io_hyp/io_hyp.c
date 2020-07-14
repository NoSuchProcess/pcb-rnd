/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  (this file is based on PCB, interactive printed circuit board design)
 *
 *  hyperlynx .hyp importer, plugin entry
 *  Copyright (C) 2016 Tibor 'Igor2' Palinkas
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

#include <stdlib.h>
#include <string.h>

#include <librnd/core/hid.h>
#include <librnd/core/hid_nogui.h>
#include <librnd/core/actions.h>
#include <librnd/core/hid_init.h>
#include <librnd/core/hid_attrib.h>
#include <librnd/core/hid_menu.h>
#include <librnd/core/plugins.h>
#include "event.h"
#include "plug_io.h"
#include "parser.h"
#include "board.h"
#include "write.h"
#include "obj_rat.h"

#include "menu_internal.c"

static const char *hyp_cookie = "hyp importer";


static pcb_plug_io_t io_hyp;

int io_hyp_fmt(pcb_plug_io_t * ctx, pcb_plug_iot_t typ, int wr, const char *fmt)
{
	if (wr && (typ & PCB_IOT_FOOTPRINT)) /* no footprint write */
		return 0;

	if (strcmp(ctx->description, fmt) == 0)
		return 200;

	if ((strcmp(fmt, "hyp") != 0) || ((typ & (~(PCB_IOT_PCB))) != 0))
		return 0;

	return 70;
}



static const char pcb_acts_LoadhypFrom[] = "LoadhypFrom(filename[, \"debug\"]...)";

static const char pcb_acth_LoadhypFrom[] = "Loads the specified Hyperlynx file.";

fgw_error_t pcb_act_LoadhypFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL;
	int debug = 0;
	rnd_bool_t retval;

	RND_ACT_MAY_CONVARG(1, FGW_STR, LoadhypFrom, fname = argv[1].val.str);

	if ((fname == NULL) || (*fname == '\0')) {
		fname = rnd_gui->fileselect(rnd_gui,
			"Load .hyp file...", "Picks a hyperlynx file to load.\n", "default.hyp",
			".hyp", NULL, "hyp", RND_HID_FSD_READ, NULL);
	}

	if (fname == NULL) {
		RND_ACT_IRES(1);
		return 0;
	}

TODO(": rewrite this - this is very unpcb-rnd")
#if 0
	int i = 0;
	/* 
	 * debug level.
	 * one "debug" argument: hyperlynx logging.
	 * two "debug" arguments: hyperlynx and bison logging.
	 * three "debug" arguments: hyperlynx, bison and flex logging.
	 */

	for (i = 0; i < argc; i++)
		debug += (strcmp(argv[i], "debug") == 0);
#endif

	if (debug > 0)
		rnd_message(RND_MSG_INFO, "Importing Hyperlynx file '%s', debug level %d\n", fname, debug);

	rnd_hid_busy(PCB, 1);
	retval = hyp_parse(PCB->Data, fname, debug);
	rnd_hid_busy(PCB, 0);

	/* notify GUI */
	rnd_event(&PCB->hidlib, PCB_EVENT_LAYERS_CHANGED, NULL);
	rnd_event(&PCB->hidlib, RND_EVENT_BOARD_CHANGED, NULL);

	RND_ACT_IRES(retval);
	return 0;
}

rnd_action_t hyp_action_list[] = {
	{"LoadhypFrom", pcb_act_LoadhypFrom, pcb_acth_LoadhypFrom, pcb_acts_LoadhypFrom}
};

/* cheap, partial read of the file to determine if it is worth running the real parser */
int io_hyp_test_parse(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, const char *Filename, FILE * f)
{
	char line[1024];
	int found = 0, lineno = 0;

	if (typ != PCB_IOT_PCB)
		return 0; /* support only boards for now */

	/* look for {VERSION and {BOARD in the first 32 lines, not assuming indentation */
	while (fgets(line, sizeof(line), f) != NULL) {
		if ((found == 0) && (strstr(line, "{VERSION=")))
			found = 1;
		if ((found == 1) && (strstr(line, "{BOARD")))
			return 1;
		lineno++;
		if (lineno > 32)
			break;
	}
	return 0;
}

int io_hyp_read_pcb(pcb_plug_io_t * ctx, pcb_board_t * pcb, const char *Filename, rnd_conf_role_t settings_dest)
{
	int res = hyp_parse(pcb->Data, Filename, 0);
	pcb_layer_auto_fixup(pcb);
	pcb_layer_colors_from_conf(pcb, 1);
	pcb_rat_all_anchor_guess(pcb->Data);
	return res;
}

int pplg_check_ver_io_hyp(int ver_needed)
{
	return 0;
}

void pplg_uninit_io_hyp(void)
{
	rnd_remove_actions_by_cookie(hyp_cookie);
	RND_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_hyp);
	rnd_hid_menu_unload(rnd_gui, hyp_cookie);
}

int pplg_init_io_hyp(void)
{
	RND_API_CHK_VER;

	/* register the IO hook */
	io_hyp.plugin_data = NULL;
	io_hyp.fmt_support_prio = io_hyp_fmt;
	io_hyp.test_parse = io_hyp_test_parse;
	io_hyp.parse_pcb = io_hyp_read_pcb;
/*	io_hyp.parse_footprint = NULL;
	io_hyp.map_footprint = NULL;
	io_hyp.parse_font = NULL;
	io_hyp.write_buffer = io_hyp_write_buffer;*/
	io_hyp.write_pcb = io_hyp_write_pcb;
	io_hyp.default_fmt = "hyp";
	io_hyp.description = "hyperlynx";
	io_hyp.save_preference_prio = 30;
	io_hyp.default_extension = ".hyp";
TODO(": look these up")
	io_hyp.fp_extension = ".hyp_mod";
	io_hyp.mime_type = "application/x-hyp-pcb";

	RND_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_hyp);


	RND_REGISTER_ACTIONS(hyp_action_list, hyp_cookie)

	rnd_hid_menu_load(rnd_gui, NULL, hyp_cookie, 130, NULL, 0, hyp_menu, "plugin: io_hyp");

	return 0;
}

/* not truncated */
