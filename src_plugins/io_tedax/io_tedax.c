/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  tedax IO plugin - plugin coordination
 *  pcb-rnd Copyright (C) 2017 Tibor 'Igor2' Palinkas
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
#include <string.h>

#include "footprint.h"

#include "board.h"
#include "netlist.h"
#include "conf_core.h"
#include "buffer.h"
#include "plugins.h"
#include "hid.h"
#include "actions.h"
#include "compat_misc.h"
#include "plug_io.h"
#include "stackup.h"
#include "tlayer.h"
#include "tboard.h"
#include "tdrc.h"


static const char *tedax_cookie = "tEDAx IO";
static pcb_plug_io_t io_tedax;

int io_tedax_fmt(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, int wr, const char *fmt)
{
	if (strcmp(ctx->description, fmt) == 0)
		return 200;

	if ((pcb_strcasecmp(fmt, "tedax") != 0) ||
		((typ & (~(PCB_IOT_PCB | PCB_IOT_FOOTPRINT | PCB_IOT_BUFFER))) != 0))
		return 0;

	return 100;
}


static const char pcb_acts_Savetedax[] = "SaveTedax(netlist|board-footprints|stackup|layer|board|drc, filename)";
static const char pcb_acth_Savetedax[] = "Saves the specific type of data in a tEDAx file.";
static fgw_error_t pcb_act_Savetedax(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL, *type;

	PCB_ACT_CONVARG(1, FGW_STR, Savetedax, type = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, Savetedax, fname = argv[2].val.str);

	if (pcb_strcasecmp(type, "netlist") == 0) {
		PCB_ACT_IRES(tedax_net_save(PCB, NULL, fname));
		return 0;
	}

	if (pcb_strcasecmp(type, "board-footprints") == 0) {
		PCB_ACT_IRES(tedax_fp_save(PCB->Data, fname));
		return 0;
	}

	if (pcb_strcasecmp(type, "stackup") == 0) {
		PCB_ACT_IRES(tedax_stackup_save(PCB, PCB->Name, fname));
		return 0;
	}

	if (pcb_strcasecmp(type, "layer") == 0) {
		PCB_ACT_IRES(tedax_layer_save(PCB, pcb_layer_get_group_(CURRENT), NULL, fname));
		return 0;
	}

	if (pcb_strcasecmp(type, "board") == 0) {
		PCB_ACT_IRES(tedax_board_save(PCB, fname));
		return 0;
	}

	if (pcb_strcasecmp(type, "drc") == 0) {
		PCB_ACT_IRES(tedax_drc_save(PCB, NULL, fname));
		return 0;
	}

	PCB_ACT_FAIL(Savetedax);
	PCB_ACT_IRES(1);
	return 0;
}

#define gen_load(type, fname) \
do { \
	static char *default_file = NULL; \
	if ((fname == NULL) || (*fname == '\0')) { \
		fname = pcb_gui->fileselect("Load tedax " #type " file...", \
																"Picks a tedax " #type " file to load.\n", \
																default_file, ".tdx", "tedax-" #type, HID_FILESELECT_READ); \
		if (fname == NULL) \
			return 1; \
		if (default_file != NULL) { \
			free(default_file); \
			default_file = NULL; \
		} \
	} \
} while(0)

static const char pcb_acts_LoadtedaxFrom[] = "LoadTedaxFrom(netlist|board|footprint|stackup|layer, filename, [block_id, [silent]])";
static const char pcb_acth_LoadtedaxFrom[] = "Loads the specified block from a tedax file.";
static fgw_error_t pcb_act_LoadtedaxFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL, *type, *id = NULL, *silents = NULL;
	int silent;

	PCB_ACT_CONVARG(1, FGW_STR, LoadtedaxFrom, type = argv[1].val.str);
	PCB_ACT_MAY_CONVARG(2, FGW_STR, LoadtedaxFrom, fname = argv[2].val.str);
	PCB_ACT_MAY_CONVARG(3, FGW_STR, LoadtedaxFrom, id = argv[3].val.str);
	PCB_ACT_MAY_CONVARG(4, FGW_STR, LoadtedaxFrom, silents = argv[4].val.str);

	silent = (silents != NULL) && (pcb_strcasecmp(silents, "silent") == 0);

	if (pcb_strcasecmp(type, "netlist") == 0) {
		gen_load(netlist, fname);
		PCB_ACT_IRES(tedax_net_load(fname, 1, id, silent));
		return 0;
	}
	if (pcb_strcasecmp(type, "board") == 0) {
		gen_load(board, fname);
		PCB_ACT_IRES(tedax_board_load(PCB, fname, id, silent));
		return 0;
	}
	if (pcb_strcasecmp(type, "footprint") == 0) {
		gen_load(footprint, fname);
		PCB_ACT_IRES(tedax_fp_load(PCB_PASTEBUFFER->Data, fname, 0, id, silent));
		return 0;
	}
	if (pcb_strcasecmp(type, "stackup") == 0) {
		gen_load(stackup, fname);
		PCB_ACT_IRES(tedax_stackup_load(PCB, fname, id, silent));
		return 0;
	}
	if (pcb_strcasecmp(type, "layer") == 0) {
		gen_load(layer, fname);
		PCB_ACT_IRES(tedax_layers_load(PCB_PASTEBUFFER->Data, fname, id, silent));
		return 0;
	}
	if (pcb_strcasecmp(type, "drc") == 0) {
		gen_load(drc, fname);
		PCB_ACT_IRES(tedax_drc_load(PCB, fname, id, silent));
		return 0;
	}
	PCB_ACT_FAIL(LoadtedaxFrom);
}

pcb_action_t tedax_action_list[] = {
	{"LoadTedaxFrom", pcb_act_LoadtedaxFrom, pcb_acth_LoadtedaxFrom, pcb_acts_LoadtedaxFrom},
	{"SaveTedax", pcb_act_Savetedax, pcb_acth_Savetedax, pcb_acts_Savetedax}
};

PCB_REGISTER_ACTIONS(tedax_action_list, tedax_cookie)

static int io_tedax_parse_element(pcb_plug_io_t *ctx, pcb_data_t *Ptr, const char *name)
{
	return tedax_fp_load(Ptr, name, 0, NULL, 0);
}

static int io_tedax_write_element(pcb_plug_io_t *ctx, FILE *f, pcb_data_t *dt)
{
	return tedax_fp_fsave(dt, f);
}

static int io_tedax_write_buffer(pcb_plug_io_t *ctx, FILE *f, pcb_buffer_t *buff, pcb_bool elem_only)
{
	return tedax_fp_fsave(buff->Data, f);
}

static int io_tedax_test_parse(pcb_plug_io_t *plug_ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
{
	char line[515], *s;
	int n;
	for(n = 0; n < 32; n++) {
		s = fgets(line, sizeof(line), f);
		if (s == NULL)
			return 0;
		while(isspace(*s)) s++;
		if (*s == '#')
			continue;
		if (strncmp(s, "tEDAx", 5) == 0) {
			s += 5;
			while(isspace(*s)) s++;
			if ((s[0] == 'v') && (s[1] == '1'))
				return 1; /* support version 1 only */
		}
	}
	return 0;
}

int io_tedax_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, conf_role_t settings_dest)
{
	int res;

	Ptr->is_footprint = 1;

	res = tedax_fp_load(Ptr->Data, Filename, 0, NULL, 0);
	if (res == 0) {
		pcb_subc_t *sc = Ptr->Data->subc.lst.first;

		pcb_layergrp_upgrade_to_pstk(Ptr);
		pcb_layer_create_all_for_recipe(Ptr, sc->data->Layer, sc->data->LayerN);
		pcb_subc_rebind(Ptr, sc);
		pcb_data_clip_polys(sc->data);
	}
	return res;
}


int pplg_check_ver_io_tedax(int ver_needed) { return 0; }

void pplg_uninit_io_tedax(void)
{
	pcb_remove_actions_by_cookie(tedax_cookie);
	PCB_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_tedax);
}

#include "dolists.h"
int pplg_init_io_tedax(void)
{
	PCB_API_CHK_VER;

	/* register the IO hook */
	io_tedax.plugin_data = NULL;
	io_tedax.fmt_support_prio = io_tedax_fmt;
	io_tedax.test_parse = io_tedax_test_parse;
	io_tedax.parse_pcb = io_tedax_parse_pcb;
	io_tedax.parse_footprint = io_tedax_parse_element;
	io_tedax.parse_font = NULL;
	io_tedax.write_buffer = io_tedax_write_buffer;
	io_tedax.write_footprint = io_tedax_write_element;
	io_tedax.write_pcb = NULL;
	io_tedax.default_fmt = "tEDAx";
	io_tedax.description = "Trivial EDA eXchange format";
	io_tedax.save_preference_prio = 95;
	io_tedax.default_extension = ".tdx";
	io_tedax.fp_extension = ".tdx";
	io_tedax.mime_type = "application/tEDAx";

	PCB_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_tedax);

	PCB_REGISTER_ACTIONS(tedax_action_list, tedax_cookie)
	return 0;
}
