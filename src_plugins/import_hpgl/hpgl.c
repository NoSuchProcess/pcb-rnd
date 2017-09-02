/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *
 *  HP-GL import HID
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libuhpgl/libuhpgl.h>
#include <libuhpgl/parse.h>

#include "board.h"
#include "conf_core.h"
#include "data.h"
#include "buffer.h"
#include "error.h"
#include "pcb-printf.h"
#include "compat_misc.h"

#include "action_helper.h"
#include "hid_actions.h"
#include "plugins.h"
#include "hid.h"

static const char *hpgl_cookie = "hpgl importer";

#define HPGL2CRD(crd)   (PCB_MM_TO_COORD((double)crd*0.025))

static int load_line(uhpgl_ctx_t *ctx, uhpgl_line_t *line)
{
	pcb_data_t *data = (pcb_data_t *)ctx->user_data;
	pcb_layer_t *layer = &data->Layer[line->pen % data->LayerN];
	pcb_line_new(layer,
		HPGL2CRD(line->p1.x), HPGL2CRD(line->p1.y), HPGL2CRD(line->p2.x), HPGL2CRD(line->p2.y), 
		conf_core.design.line_thickness, 2 * conf_core.design.clearance,
		pcb_flag_make((conf_core.editor.clear_line ? PCB_FLAG_CLEARLINE : 0)));
	return 0;
}

static int load_arc(uhpgl_ctx_t *ctx, uhpgl_arc_t *arc)
{
	pcb_data_t *data = (pcb_data_t *)ctx->user_data;

	return 0;
}

static int load_poly(uhpgl_ctx_t *ctx, uhpgl_poly_t *poly)
{
	pcb_data_t *data = (pcb_data_t *)ctx->user_data;

	return 0;
}


static int hpgl_load(const char *fname)
{
	FILE *f;
	uhpgl_ctx_t ctx;

	memset(&ctx, 0, sizeof(ctx));

	ctx.conf.line  = load_line;
	ctx.conf.arc   = load_arc;
	ctx.conf.poly  = load_poly;

	f = fopen(fname, "r");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Error opening HP-GL %s for read\n", fname);
		return 1;
	}

	pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
	ctx.user_data = PCB_PASTEBUFFER->Data;
	PCB_PASTEBUFFER->Data->LayerN = PCB->Data->LayerN;
	pcb_data_bind_board_layers(PCB, PCB_PASTEBUFFER->Data, 0);

	if ((uhpgl_parse_open(&ctx) == 0) && (uhpgl_parse_file(&ctx, f) == 0) && (uhpgl_parse_close(&ctx) == 0)) {
		fclose(f);
		return 0;
	}

	fclose(f);
	pcb_message(PCB_MSG_ERROR, "Error loading HP-GL at %s:%d.%d: %s\n", fname, ctx.error.line, ctx.error.col, ctx.error.msg);

	return 1;
}

static const char pcb_acts_LoadHpglFrom[] = "LoadHpglFrom(filename)";
static const char pcb_acth_LoadHpglFrom[] = "Loads the specified hpgl plot file to the current buffer";
int pcb_act_LoadHpglFrom(int argc, const char **argv, pcb_coord_t x, pcb_coord_t y)
{
	const char *fname = NULL;
	static char *default_file = NULL;

	fname = argc ? argv[0] : 0;

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect("Load HP-GL file...",
																"Picks a HP-GL plot file to load.\n",
																default_file, ".hpgl", "hpgl", HID_FILESELECT_READ);
		if (fname == NULL)
			PCB_ACT_FAIL(LoadHpglFrom);
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	return hpgl_load(fname);
}

pcb_hid_action_t hpgl_action_list[] = {
	{"LoadHpglFrom", 0, pcb_act_LoadHpglFrom, pcb_acth_LoadHpglFrom, pcb_acts_LoadHpglFrom}
};

PCB_REGISTER_ACTIONS(hpgl_action_list, hpgl_cookie)

int pplg_check_ver_import_hpgl(int ver_needed) { return 0; }

void pplg_uninit_import_hpgl(void)
{
	pcb_hid_remove_actions_by_cookie(hpgl_cookie);
}

#include "dolists.h"
int pplg_init_import_hpgl(void)
{
	PCB_REGISTER_ACTIONS(hpgl_action_list, hpgl_cookie)
	return 0;
}
