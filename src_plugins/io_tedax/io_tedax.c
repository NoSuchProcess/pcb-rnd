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
#include "conf_core.h"
#include "buffer.h"
#include <librnd/core/plugins.h>
#include <librnd/core/hid.h>
#include <librnd/core/actions.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/safe_fs.h>
#include <librnd/core/hid_menu.h>
#include "plug_io.h"
#include "stackup.h"
#include "tlayer.h"
#include "tboard.h"
#include "trouter.h"
#include "tdrc.h"
#include "tdrc_query.h"
#include "tetest.h"
#include "tnetlist.h"
#include "trouter.h"

#include "menu_internal.c"

static const char *tedax_cookie = "tEDAx IO";
static pcb_plug_io_t io_tedax;

int io_tedax_fmt(pcb_plug_io_t *ctx, pcb_plug_iot_t typ, int wr, const char *fmt)
{
	if (strcmp(ctx->description, fmt) == 0)
		return 200;

	if ((rnd_strcasecmp(fmt, "tedax") != 0) ||
		((typ & (~(PCB_IOT_PCB | PCB_IOT_FOOTPRINT | PCB_IOT_BUFFER))) != 0))
		return 0;

	return 100;
}


static const char pcb_acts_Savetedax[] = 
	"SaveTedax(netlist|board-footprints|stackup|layer|board|drc|etest, filename)\n"
	"SaveTedax(drc_query, filename, [rule_name])"
	"SaveTedax(route_req, filename, [confkey=value, confkey=value, ...])";
static const char pcb_acth_Savetedax[] = "Saves the specific type of data in a tEDAx file.";
static fgw_error_t pcb_act_Savetedax(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL, *type, *id = NULL;

	RND_ACT_CONVARG(1, FGW_STR, Savetedax, type = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, Savetedax, fname = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, Savetedax, id = argv[3].val.str);

	if (rnd_strcasecmp(type, "netlist") == 0) {
		RND_ACT_IRES(tedax_net_save(PCB, NULL, fname));
		return 0;
	}

	if (rnd_strcasecmp(type, "board-footprints") == 0) {
		RND_ACT_IRES(tedax_fp_save(PCB->Data, fname, -1));
		return 0;
	}

	if (rnd_strcasecmp(type, "stackup") == 0) {
		RND_ACT_IRES(tedax_stackup_save(PCB, PCB->hidlib.name, fname));
		return 0;
	}

	if (rnd_strcasecmp(type, "layer") == 0) {
		RND_ACT_IRES(tedax_layer_save(PCB, pcb_layer_get_group_(PCB_CURRLAYER(PCB)), NULL, fname));
		return 0;
	}

	if (rnd_strcasecmp(type, "board") == 0) {
		RND_ACT_IRES(tedax_board_save(PCB, fname));
		return 0;
	}

	if (rnd_strcasecmp(type, "drc") == 0) {
		RND_ACT_IRES(tedax_drc_save(PCB, NULL, fname));
		return 0;
	}

	if (rnd_strcasecmp(type, "drc_query") == 0) {
		RND_ACT_IRES(tedax_drc_query_save(PCB, id, fname));
		return 0;
	}

	if (rnd_strcasecmp(type, "etest") == 0) {
		RND_ACT_IRES(tedax_etest_save(PCB, NULL, fname));
		return 0;
	}

	if (rnd_strcasecmp(type, "route_req") == 0) {
		RND_ACT_IRES(tedax_route_req_save(PCB, fname, argc-3, argv+3));
		return 0;
	}

	RND_ACT_FAIL(Savetedax);
	RND_ACT_IRES(1);
	return 0;
}

#define gen_load(type, fname) \
do { \
	static char *default_file = NULL; \
	if ((fname == NULL) || (*fname == '\0')) { \
		fname = rnd_gui->fileselect(rnd_gui, "Load tedax " #type " file...", \
																"Picks a tedax " #type " file to load.\n", \
																default_file, ".tdx", NULL, "tedax-" #type, RND_HID_FSD_READ, NULL); \
		if (fname == NULL) \
			return 1; \
		if (default_file != NULL) { \
			free(default_file); \
			default_file = NULL; \
		} \
	} \
} while(0)

static const char pcb_acts_LoadtedaxFrom[] = "LoadTedaxFrom(netlist|board|footprint|stackup|layer|drc|drc_query|route_res, filename, [block_id, [silent, [src]]])";
static const char pcb_acth_LoadtedaxFrom[] = "Loads the specified block from a tedax file.";
static fgw_error_t pcb_act_LoadtedaxFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL, *type, *id = NULL, *silents = NULL, *src = NULL;
	int silent;

	RND_ACT_CONVARG(1, FGW_STR, LoadtedaxFrom, type = argv[1].val.str);
	RND_ACT_MAY_CONVARG(2, FGW_STR, LoadtedaxFrom, fname = argv[2].val.str);
	RND_ACT_MAY_CONVARG(3, FGW_STR, LoadtedaxFrom, id = argv[3].val.str);
	RND_ACT_MAY_CONVARG(4, FGW_STR, LoadtedaxFrom, silents = argv[4].val.str);
	RND_ACT_MAY_CONVARG(5, FGW_STR, LoadtedaxFrom, src = argv[5].val.str);

	if ((id != NULL) && (*id == '\0'))
		id = NULL;

	silent = (silents != NULL) && (rnd_strcasecmp(silents, "silent") == 0);

	if (rnd_strcasecmp(type, "netlist") == 0) {
		gen_load(netlist, fname);
		RND_ACT_IRES(tedax_net_load(fname, 1, id, silent));
		return 0;
	}
	if (rnd_strcasecmp(type, "route_res") == 0) {
		gen_load(route_res, fname);
		RND_ACT_IRES(tedax_route_res_load(fname, id, silent));
		return 0;
	}
	if (rnd_strcasecmp(type, "route_conf_keys") == 0) { /* intentionally undocumented - for internal use, returns a pointer */
		gen_load(route_conf_keys, fname);
		res->type = FGW_PTR;
		res->val.ptr_void = tedax_route_conf_keys_load(fname, id, silent);
		return 0;
	}
	if (rnd_strcasecmp(type, "board") == 0) {
		gen_load(board, fname);
		RND_ACT_IRES(tedax_board_load(PCB, fname, id, silent));
		return 0;
	}
	if (rnd_strcasecmp(type, "footprint") == 0) {
		gen_load(footprint, fname);
		RND_ACT_IRES(tedax_fp_load(PCB_PASTEBUFFER->Data, fname, 0, id, silent, 0));
		return 0;
	}
	if (rnd_strcasecmp(type, "stackup") == 0) {
		gen_load(stackup, fname);
		RND_ACT_IRES(tedax_stackup_load(PCB, fname, id, silent));
		return 0;
	}
	if (rnd_strcasecmp(type, "layer") == 0) {
		gen_load(layer, fname);
		RND_ACT_IRES(tedax_layers_load(PCB_PASTEBUFFER->Data, fname, id, silent));
		return 0;
	}
	if (rnd_strcasecmp(type, "drc") == 0) {
		gen_load(drc, fname);
		RND_ACT_IRES(tedax_drc_load(PCB, fname, id, silent));
		return 0;
	}
	if (rnd_strcasecmp(type, "drc_query") == 0) {
		gen_load(drc_query, fname);
		RND_ACT_IRES(tedax_drc_query_load(PCB, fname, id, src, silent));
		return 0;
	}
	RND_ACT_FAIL(LoadtedaxFrom);
}

int pcb_io_tedax_test_parse(pcb_plug_io_t *plug_ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f);

static const char pcb_acts_TedaxTestParse[] = "TedaxTestParse(filename|FILE*)";
static const char pcb_acth_TedaxTestParse[] = "Returns 1 if the file looks like tEDAx (0 if not)";
static fgw_error_t pcb_act_TedaxTestParse(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	if (argc < 2)
		return -1;
	if ((argv[1].type & FGW_STR) == FGW_STR) {
		FILE *f = rnd_fopen(RND_ACT_HIDLIB, argv[1].val.str, "r");
		if (f == NULL) {
			RND_ACT_IRES(0);
			return 0;
		}
		RND_ACT_IRES(pcb_io_tedax_test_parse(&io_tedax, 0, NULL, f));
		fclose(f);
		return 0;
	}
	else if (((argv[1].type & FGW_PTR) == FGW_PTR)) {
		if (!fgw_ptr_in_domain(&rnd_fgw, &argv[1], RND_PTR_DOMAIN_FILE_PTR))
			return FGW_ERR_PTR_DOMAIN;
		RND_ACT_IRES(pcb_io_tedax_test_parse(&io_tedax, 0, NULL, argv[1].val.ptr_void));
		return 0;
	}
	return FGW_ERR_ARG_CONV;
}

rnd_action_t tedax_action_list[] = {
	{"LoadTedaxFrom", pcb_act_LoadtedaxFrom, pcb_acth_LoadtedaxFrom, pcb_acts_LoadtedaxFrom},
	{"SaveTedax", pcb_act_Savetedax, pcb_acth_Savetedax, pcb_acts_Savetedax},
	{"TedaxTestParse", pcb_act_TedaxTestParse, pcb_acth_TedaxTestParse, pcb_acts_TedaxTestParse}
};

static int io_tedax_parse_footprint(pcb_plug_io_t *ctx, pcb_data_t *Ptr, const char *name, const char *subfpname)
{
	return tedax_fp_load(Ptr, name, 0, NULL, 0, 1);
}

int io_tedax_fp_write_subcs_head(pcb_plug_io_t *ctx, void **udata, FILE *f, int lib, long num_subcs)
{
	fprintf(f, "tEDAx v1\n");
	return 0;
}

int io_tedax_fp_write_subcs_subc(pcb_plug_io_t *ctx, void **udata, FILE *f, pcb_subc_t *subc)
{
	return tedax_fp_fsave_subc(subc, f);
}

int io_tedax_fp_write_subcs_tail(pcb_plug_io_t *ctx, void **udata, FILE *f)
{
	return 0;
}

int pcb_io_tedax_test_parse(pcb_plug_io_t *plug_ctx, pcb_plug_iot_t typ, const char *Filename, FILE *f)
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

int io_tedax_parse_pcb(pcb_plug_io_t *ctx, pcb_board_t *Ptr, const char *Filename, rnd_conf_role_t settings_dest)
{
	int res;

	Ptr->is_footprint = 1;

	res = tedax_fp_load(Ptr->Data, Filename, 0, NULL, 0, 0);
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
	rnd_remove_actions_by_cookie(tedax_cookie);
	tedax_etest_uninit();
	RND_HOOK_UNREGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_tedax);
	pcb_tedax_net_uninit();
	rnd_hid_menu_unload(rnd_gui, tedax_cookie);
}

int pplg_init_io_tedax(void)
{
	RND_API_CHK_VER;

	/* register the IO hook */
	io_tedax.plugin_data = NULL;
	io_tedax.fmt_support_prio = io_tedax_fmt;
	io_tedax.test_parse = pcb_io_tedax_test_parse;
	io_tedax.parse_pcb = io_tedax_parse_pcb;
	io_tedax.parse_footprint = io_tedax_parse_footprint;
	io_tedax.map_footprint = tedax_fp_map;
	io_tedax.parse_font = NULL;
	io_tedax.write_buffer = NULL;
	io_tedax.write_subcs_head = io_tedax_fp_write_subcs_head;
	io_tedax.write_subcs_subc = io_tedax_fp_write_subcs_subc;
	io_tedax.write_subcs_tail = io_tedax_fp_write_subcs_tail;
	io_tedax.write_pcb = NULL;
	io_tedax.default_fmt = "tEDAx";
	io_tedax.description = "Trivial EDA eXchange format";
	io_tedax.save_preference_prio = 95;
	io_tedax.default_extension = ".tdx";
	io_tedax.fp_extension = ".tdx";
	io_tedax.mime_type = "application/tEDAx";

	RND_HOOK_REGISTER(pcb_plug_io_t, pcb_plug_io_chain, &io_tedax);

	tedax_etest_init();

	RND_REGISTER_ACTIONS(tedax_action_list, tedax_cookie)
	pcb_tedax_net_init();
	rnd_hid_menu_load(rnd_gui, NULL, tedax_cookie, 195, NULL, 0, tedax_menu, "plugin: io_tedax");
	return 0;
}
