/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design - export to emsim-rnd (tEDAx)
 *  Copyright (C) 2024 Tibor 'Igor2' Palinkas
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
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <librnd/core/math_helper.h>
#include "board.h"
#include "data.h"
#include "find.h"
#include "obj_term.h"
#include <librnd/core/error.h>
#include "layer.h"
#include "hid_cam.h"
#include "src_plugins/lib_polyhelp/topoly.h"
#include <librnd/core/misc_util.h>
#include <librnd/core/compat_misc.h>
#include <librnd/core/plugins.h>
#include <librnd/core/safe_fs.h>

#include <librnd/hid/hid.h>
#include <librnd/hid/hid_nogui.h>

#include <librnd/hid/hid_init.h>
#include <librnd/hid/hid_attrib.h>

#include "export_emsim.h"

static rnd_hid_t exp_emsim_hid;

const char *exp_emsim_cookie = "exp_emsim HID";

/* global states during export */
static gds_t fn_gds;
static FILE *f;


#define TRX(x)  ((double)RND_COORD_TO_MM(x))
#define TRY(y)  ((double)RND_COORD_TO_MM(y))

static const rnd_export_opt_t exp_emsim_attribute_list[] = {
	/* other HIDs expect this to be first.  */

	{"outfile", "Output file name",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_exp_emsimfile 0

	{"lumped", "colon separated list of ports and lumped elements",
	 RND_HATT_STRING, 0, 0, {0, 0, 0}, 0},
#define HA_exp_emsim_lumped 1
};

#define NUM_OPTIONS (sizeof(exp_emsim_attribute_list)/sizeof(exp_emsim_attribute_list[0]))

static rnd_hid_attr_val_t exp_emsim_values[NUM_OPTIONS];

static const rnd_export_opt_t *exp_emsim_get_export_options(rnd_hid_t *hid, int *n, rnd_design_t *dsg, void *appspec)
{
	const char *suffix = ".emsim";
	const char *val = exp_emsim_values[HA_exp_emsimfile].str;

	if ((dsg != NULL) && ((val == NULL) || (*val == '\0')))
		pcb_derive_default_filename(dsg->loadname, &exp_emsim_values[HA_exp_emsimfile], suffix);

	if (n)
		*n = NUM_OPTIONS;

	return exp_emsim_attribute_list;
}

/* exports a single island, with holes */
RND_INLINE void emsim_export_polyarea_to_file(pcb_board_t *pcb, FILE *f, rnd_polyline_t *contour)
{

}

/* exports all islands of pa */
RND_INLINE void emsim_export_polyarea_to_file(pcb_board_t *pcb, FILE *f, rnd_polyarea_t *start)
{
	rnd_polyarea_t *pa = start;

	do {
		emsim_export_polylines_to_file(pcb, f, pa->contour);
	} while((pa = pa->f) != start);
}

void emsim_export_to_file(pcb_board_t *pcb, emsim_env_t *dst, FILE *f)
{
	emsim_lumped_t *n;
	pcb_find_t fctx = {0};
	pcb_any_obj_t *from;
	long lid, l = 0;
	vtp0_t *poly_per_layer;
	pcb_topoly_solid_opts_t opts;

	opts.per_grp = 1;
	opts.typ_mask = PCB_LYT_COPPER;

	/* find objects from ports and mark them with df */
	pcb_find_from_obj(&fctx, pcb->Data, NULL);
	for(n = dst->head; n != NULL; n = n->next) {
		switch(n->type) {
			case PORT:
				from = pcb_term_find_name(pcb, pcb->Data, PCB_LYT_COPPER, n->data.port.refdes, n->data.port.term, NULL, NULL);
				if (from != NULL)
					l += pcb_find_from_obj_next(&fctx, pcb->Data, from);
				else
					rnd_message(RND_MSG_ERROR, "emsim_export_to_file: failed to find terminal %s-%s\n", n->data.port.refdes, n->data.port.term);
			case RESISTOR:
			case VSRC:
				break;
		}
	}

	rnd_trace("df-marked %ld objs mark=%ld\n", l, (long)fctx.mark);

	poly_per_layer = pcb_topoly_solids_in(pcb, fctx.mark, &opts);

	rnd_trace("poly-per-layer: %ld layers\n", poly_per_layer->used);
	for(lid = 0; lid < poly_per_layer->used; lid++) {
		if (poly_per_layer->array[lid] != NULL) {
			rnd_trace(" [%ld] poly\n", lid);
		}
	}

	pcb_find_free(&fctx);

}

static void exp_emsim_hid_export_to_file(rnd_design_t *dsg, rnd_hid_attr_val_t *options)
{
	TODO("process list of lumped");
	TODO("call emsim_export_to_file((pcb_board_t *)dsg, env, f)");
}

static void exp_emsim_do_export(rnd_hid_t *hid, rnd_design_t *design, rnd_hid_attr_val_t *options, void *appspec)
{
	const char *filename;

	if (!options) {
		exp_emsim_get_export_options(hid, 0, design, appspec);
		options = exp_emsim_values;
	}

	filename = options[HA_exp_emsimfile].str;
	if (!filename)
		filename = "board.emsim";

	if (f != NULL) {
		fclose(f);
		f = NULL;
	}

	f = rnd_fopen_askovr(&PCB->hidlib, filename, "wb", NULL);
	if (!f) {
		perror(filename);
		return;
	}

	exp_emsim_hid_export_to_file(design, options);

	if (f != NULL) {
		fclose(f);
		f = NULL;
	}
	gds_uninit(&fn_gds);
}

static int exp_emsim_parse_arguments(rnd_hid_t *hid, int *argc, char ***argv)
{
	rnd_export_register_opts2(hid, exp_emsim_attribute_list, sizeof(exp_emsim_attribute_list) / sizeof(exp_emsim_attribute_list[0]), exp_emsim_cookie, 0);
	return rnd_hid_parse_command_line(argc, argv);
}

static int exp_emsim_usage(rnd_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\nexp_emsim exporter command line arguments:\n\n");
	rnd_hid_usage(exp_emsim_attribute_list, sizeof(exp_emsim_attribute_list) / sizeof(exp_emsim_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x exp_emsim [exp_emsim options] foo.rp\n\n");
	return 0;
}

/*** memory management: lumped components and env ***/

void emsim_lumped_link(emsim_env_t *ctx, emsim_lumped_t *lump)
{
	lump->next = NULL;
	if (ctx->tail != NULL) {
		ctx->tail->next = lump;
		ctx->tail = lump;
	}
	else
		ctx->head = ctx->tail = lump;
}


/*** plugin glue ***/

int pplg_check_ver_export_emsim(int ver_needed) { return 0; }

void pplg_uninit_export_emsim(void)
{
	rnd_export_remove_opts_by_cookie(exp_emsim_cookie);
	rnd_hid_remove_hid(&exp_emsim_hid);
}

int pplg_init_export_emsim(void)
{
	RND_API_CHK_VER;

	memset(&exp_emsim_hid, 0, sizeof(rnd_hid_t));

	rnd_hid_nogui_init(&exp_emsim_hid);

	exp_emsim_hid.struct_size = sizeof(rnd_hid_t);
	exp_emsim_hid.name = "emsim";
	exp_emsim_hid.description = "export board for electromagnetic simulation with emsim-rnd";
	exp_emsim_hid.exporter = 1;

	exp_emsim_hid.get_export_options = exp_emsim_get_export_options;
	exp_emsim_hid.do_export = exp_emsim_do_export;
	exp_emsim_hid.parse_arguments = exp_emsim_parse_arguments;
	exp_emsim_hid.argument_array = exp_emsim_values;

	exp_emsim_hid.usage = exp_emsim_usage;

	rnd_hid_register_hid(&exp_emsim_hid);
	rnd_hid_load_defaults(&exp_emsim_hid, exp_emsim_attribute_list, NUM_OPTIONS);

	return 0;
}
