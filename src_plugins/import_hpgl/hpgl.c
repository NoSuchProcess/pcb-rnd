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
#include <libuhpgl/libuhpgl.h>
#include <libuhpgl/parse.h>

#include "board.h"
#include "conf_core.h"
#include "data.h"
#include "buffer.h"
#include "error.h"
#include "pcb-printf.h"
#include "compat_misc.h"
#include "safe_fs.h"

#include "actions.h"
#include "plugins.h"
#include "hid.h"

static const char *hpgl_cookie = "hpgl importer";

#define HPGL2CRD_D(crd)   (PCB_MM_TO_COORD((double)crd*0.025))
#define HPGL2CRD_X(crd)   (PCB_MM_TO_COORD((double)crd*0.025))
#define HPGL2CRD_Y(crd)   (PCB_MM_TO_COORD((double)crd*(-0.025)))

static pcb_layer_t *get_pen_layer(pcb_data_t *data, int pen)
{
	int n, old_len;
	pen = pen % PCB_MAX_LAYER;

	if (pen >= data->LayerN) {
		old_len = data->LayerN;
		data->LayerN = pen+1;
/*		data->Layer = realloc(data->Layer, sizeof(pcb_layer_t) * data->LayerN);*/
		for(n = old_len; n < data->LayerN; n++) {
			memset(&data->Layer[n], 0, sizeof(pcb_layer_t));
			pcb_layer_real2bound(&data->Layer[n], &PCB->Data->Layer[n], 0);
			free((char *)data->Layer[n].name);
			data->Layer[n].name = pcb_strdup_printf("hpgl_pen_%d", n);
			data->Layer[n].parent.data = data;
			data->Layer[n].parent_type = PCB_PARENT_DATA;
			data->Layer[n].type = PCB_OBJ_LAYER;
		}
	}

	return &data->Layer[pen];
}

static int load_line(uhpgl_ctx_t *ctx, uhpgl_line_t *line)
{
	pcb_data_t *data = (pcb_data_t *)ctx->user_data;
	pcb_layer_t *layer = get_pen_layer(data, line->pen);
	pcb_line_new(layer,
		HPGL2CRD_X(line->p1.x), HPGL2CRD_Y(line->p1.y), HPGL2CRD_X(line->p2.x), HPGL2CRD_Y(line->p2.y), 
		conf_core.design.line_thickness, 2 * conf_core.design.clearance,
		pcb_flag_make((conf_core.editor.clear_line ? PCB_FLAG_CLEARLINE : 0)));
	return 0;
}

static int load_arc(uhpgl_ctx_t *ctx, uhpgl_arc_t *arc)
{
	pcb_data_t *data = (pcb_data_t *)ctx->user_data;
	pcb_layer_t *layer = get_pen_layer(data, arc->pen);

	pcb_arc_new(layer,
		HPGL2CRD_X(arc->center.x), HPGL2CRD_Y(arc->center.y),
		HPGL2CRD_D(arc->r), HPGL2CRD_D(arc->r),
		arc->starta+180, arc->deltaa,
		conf_core.design.line_thickness, 2 * conf_core.design.clearance,
		pcb_flag_make((conf_core.editor.clear_line ? PCB_FLAG_CLEARLINE : 0)), pcb_true);

	return 0;
}

static int load_poly(uhpgl_ctx_t *ctx, uhpgl_poly_t *poly)
{
/*	pcb_data_t *data = (pcb_data_t *)ctx->user_data;*/
	pcb_message(PCB_MSG_ERROR, "HPGL: polygons are not yet supported\n");
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

	f = pcb_fopen(&PCB->hidlib, fname, "r");
	if (f == NULL) {
		pcb_message(PCB_MSG_ERROR, "Error opening HP-GL %s for read\n", fname);
		return 1;
	}

	pcb_buffer_clear(PCB, PCB_PASTEBUFFER);
	ctx.user_data = PCB_PASTEBUFFER->Data;
	PCB_PASTEBUFFER->Data->LayerN = 0;

	if ((uhpgl_parse_open(&ctx) == 0) && (uhpgl_parse_file(&ctx, f) == 0) && (uhpgl_parse_close(&ctx) == 0)) {
		fclose(f);
		if (PCB_PASTEBUFFER->Data->LayerN == 0) {
			pcb_message(PCB_MSG_ERROR, "Error loading HP-GL: could not load any object from %s\n", fname);
			return 0;
		}
		pcb_actionl("mode", "pastebuffer", NULL);
		return 0;
	}

	fclose(f);
	pcb_message(PCB_MSG_ERROR, "Error loading HP-GL at %s:%d.%d: %s\n", fname, ctx.error.line, ctx.error.col, ctx.error.msg);

	return 1;
}

static const char pcb_acts_LoadHpglFrom[] = "LoadHpglFrom(filename)";
static const char pcb_acth_LoadHpglFrom[] = "Loads the specified hpgl plot file to the current buffer";
fgw_error_t pcb_act_LoadHpglFrom(fgw_arg_t *res, int argc, fgw_arg_t *argv)
{
	const char *fname = NULL;
	static char *default_file = NULL;

	PCB_ACT_MAY_CONVARG(1, FGW_STR, LoadHpglFrom, fname = argv[1].val.str);

	if (!fname || !*fname) {
		fname = pcb_gui->fileselect(pcb_gui, "Load HP-GL file...",
																"Picks a HP-GL plot file to load.\n",
																default_file, ".hpgl", NULL, "hpgl", PCB_HID_FSD_READ, NULL);
		if (fname == NULL)
			return 0; /* cancel */
		if (default_file != NULL) {
			free(default_file);
			default_file = NULL;
		}
	}

	PCB_ACT_IRES(0);
	return hpgl_load(fname);
}

pcb_action_t hpgl_action_list[] = {
	{"LoadHpglFrom", pcb_act_LoadHpglFrom, pcb_acth_LoadHpglFrom, pcb_acts_LoadHpglFrom}
};

PCB_REGISTER_ACTIONS(hpgl_action_list, hpgl_cookie)

int pplg_check_ver_import_hpgl(int ver_needed) { return 0; }

void pplg_uninit_import_hpgl(void)
{
	pcb_remove_actions_by_cookie(hpgl_cookie);
}

#include "dolists.h"
int pplg_init_import_hpgl(void)
{
	PCB_API_CHK_VER;
	PCB_REGISTER_ACTIONS(hpgl_action_list, hpgl_cookie)
	return 0;
}
