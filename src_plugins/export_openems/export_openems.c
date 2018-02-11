/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2018 Tibor 'Igor2' Palinkas
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
 *    lead developer: email to pcb-rnd (at) igor2.repo.hu
 *    mailing list: pcb-rnd (at) list.repo.hu (send "subscribe")
 */

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "board.h"
#include "plugins.h"
#include "safe_fs.h"

#include "hid.h"
#include "hid_nogui.h"
#include "hid_helper.h"
#include "hid_flags.h"
#include "hid_attrib.h"
#include "hid_draw_helpers.h"

static pcb_hid_t openems_hid;

const char *openems_cookie = "openems HID";


typedef struct hid_gc_s {
	pcb_hid_t *me_pointer;
	pcb_cap_style_t cap;
	int width;
} hid_gc_s;

typedef struct {
	FILE *f;
	pcb_board_t *pcb;
	int lg_pcb2ems[PCB_MAX_LAYERGRP]; /* indexed by gid, gives 0 or the ems-side layer ID */
	int lg_ems2pcb[PCB_MAX_LAYERGRP]; /* indexed by the ems-side layer ID, gives -1 or a gid */
} wctx_t;

static FILE *f = NULL;

pcb_hid_attribute_t openems_attribute_list[] = {
	{"outfile", "Graphics output file",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_openemsfile 0

};

#define NUM_OPTIONS (sizeof(openems_attribute_list)/sizeof(openems_attribute_list[0]))

PCB_REGISTER_ATTRIBUTES(openems_attribute_list, openems_cookie)

static pcb_hid_attr_val_t openems_values[NUM_OPTIONS];

static pcb_hid_attribute_t *openems_get_export_options(int *n)
{
	static char *last_made_filename = 0;
	const char *suffix = ".m";

	if (PCB)
		pcb_derive_default_filename(PCB->Filename, &openems_attribute_list[HA_openemsfile], suffix, &last_made_filename);

	if (n)
		*n = NUM_OPTIONS;
	return openems_attribute_list;
}

static void openems_write_layers(wctx_t *ctx)
{
	pcb_layergrp_id_t gid;
	int next = 1;

	for(gid = 0; gid < PCB_MAX_LAYERGRP; gid++)
		ctx->lg_ems2pcb[gid] = -1;

	fprintf(ctx->f, "%%%%%% Layer mapping\n");

	/* linear map of copper and substrate layers */
	for(gid = 0; gid < ctx->pcb->LayerGroups.len; gid++) {
		pcb_layergrp_t *grp = &ctx->pcb->LayerGroups.grp[gid];
		int iscop = (grp->type & PCB_LYT_COPPER);

		if (!(iscop) && !(grp->type & PCB_LYT_SUBSTRATE))
			continue;
		ctx->lg_ems2pcb[next] = gid;
		ctx->lg_ems2pcb[gid] = next;
		fprintf(ctx->f, "layers(%d).number = %d;\n", next, next); /* type index really */
		fprintf(ctx->f, "layers(%d).name = '%s';\n", next, (grp->name == NULL ? "anon" : grp->name));
		fprintf(ctx->f, "layers(%d).clearn = 0;\n", next);
		fprintf(ctx->f, "layer_types(%d).name = '%s_%d';\n", next, iscop ? "COPPER" : "SUBSTRATE", next);
		fprintf(ctx->f, "layer_types(%d).subtype = %d;\n", next, iscop ? 2 : 3);

#warning TODO: get layer properties from attributes or global exporter options
/*
layer_types(1).thickness = 1.03556;
layer_types(1).conductivity = 56*10^6; 
layer_types(1).epsilon;
layer_types(1).mue;
layer_types(1).kappa;
layer_types(1).sigma;
*/

		next++;
		fprintf(ctx->f, "\n");
	}
	fprintf(ctx->f, "\n");
}

void openems_hid_export_to_file(FILE *the_file, pcb_hid_attr_val_t *options)
{
	pcb_hid_expose_ctx_t ctx;
	wctx_t wctx;

	memset(&wctx, 0, sizeof(wctx));
	wctx.f = the_file;
	wctx.pcb = PCB;

	ctx.view.X1 = 0;
	ctx.view.Y1 = 0;
	ctx.view.X2 = PCB->MaxWidth;
	ctx.view.Y2 = PCB->MaxHeight;

	f = the_file;

	conf_force_set_bool(conf_core.editor.thin_draw, 0);
	conf_force_set_bool(conf_core.editor.thin_draw_poly, 0);
/*		conf_force_set_bool(conf_core.editor.check_planes, 0);*/
	conf_force_set_bool(conf_core.editor.show_solder_side, 0);

	openems_write_layers(&wctx);

	pcb_hid_expose_all(&openems_hid, &ctx);

	conf_update(NULL, -1); /* restore forced sets */
}

static void openems_do_export(pcb_hid_attr_val_t * options)
{
	const char *filename;
	int save_ons[PCB_MAX_LAYER + 2];
	int i;

	if (!options) {
		openems_get_export_options(0);
		for (i = 0; i < NUM_OPTIONS; i++)
			openems_values[i] = openems_attribute_list[i].default_val;
		options = openems_values;
	}

	filename = options[HA_openemsfile].str_value;
	if (!filename)
		filename = "pcb.m";

	f = pcb_fopen(filename, "wb");
	if (!f) {
		perror(filename);
		return;
	}

	pcb_hid_save_and_show_layer_ons(save_ons);

	openems_hid_export_to_file(f, options);

	pcb_hid_restore_layer_ons(save_ons);

	fclose(f);
	f = NULL;
}

static void openems_parse_arguments(int *argc, char ***argv)
{
	pcb_hid_register_attributes(openems_attribute_list, sizeof(openems_attribute_list) / sizeof(openems_attribute_list[0]), openems_cookie, 0);
	pcb_hid_parse_command_line(argc, argv);
}

static int openems_set_layer_group(pcb_layergrp_id_t group, pcb_layer_id_t layer, unsigned int flags, int is_empty)
{
	if (flags & PCB_LYT_COPPER) { /* export copper layers only */
		return 1;
	}
	return 0;
}


static pcb_hid_gc_t openems_make_gc(void)
{
	pcb_hid_gc_t rv = (pcb_hid_gc_t) calloc(sizeof(hid_gc_s), 1);
	rv->me_pointer = &openems_hid;
	return rv;
}

static void openems_destroy_gc(pcb_hid_gc_t gc)
{
	free(gc);
}

static void openems_set_drawing_mode(pcb_composite_op_t op, pcb_bool direct, const pcb_box_t *screen)
{
	switch(op) {
		case PCB_HID_COMP_RESET:
			break;
		case PCB_HID_COMP_POSITIVE:
			break;
		case PCB_HID_COMP_NEGATIVE:
			pcb_message(PCB_MSG_ERROR, "Can't draw composite layer, especially not on copper\n");
			break;
		case PCB_HID_COMP_FLUSH:
			break;
	}
}

static void openems_set_color(pcb_hid_gc_t gc, const char *name)
{
}

static void openems_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
	gc->cap = style;
}

static void openems_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
{
	gc->width = width;
}


static void openems_set_draw_xor(pcb_hid_gc_t gc, int xor_)
{
	;
}

static void openems_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
}

static void openems_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
}

static void openems_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
}

static void openems_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t start_angle, pcb_angle_t delta_angle)
{
}

static void openems_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
}

static void openems_fill_polygon_offs(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y, pcb_coord_t dx, pcb_coord_t dy)
{
}

static void openems_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t *x, pcb_coord_t *y)
{
	openems_fill_polygon_offs(gc, n_coords, x, y, 0, 0);
}

static void openems_calibrate(double xval, double yval)
{
	pcb_message(PCB_MSG_ERROR, "openems_calibrate() not implemented");
	return;
}

static void openems_set_crosshair(int x, int y, int a)
{
}

static int openems_usage(const char *topic)
{
	fprintf(stderr, "\nopenems exporter command line arguments:\n\n");
	pcb_hid_usage(openems_attribute_list, sizeof(openems_attribute_list) / sizeof(openems_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x openems [openems options] foo.pcb\n\n");
	return 0;
}

#include "dolists.h"

int pplg_check_ver_export_openems(int ver_needed) { return 0; }

void pplg_uninit_export_openems(void)
{
	pcb_hid_remove_attributes_by_cookie(openems_cookie);
}

int pplg_init_export_openems(void)
{
	memset(&openems_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&openems_hid);
	pcb_dhlp_draw_helpers_init(&openems_hid);

	openems_hid.struct_size = sizeof(pcb_hid_t);
	openems_hid.name = "openems";
	openems_hid.description = "OpenEMS exporter";
	openems_hid.exporter = 1;
	openems_hid.holes_after = 1;

	openems_hid.get_export_options = openems_get_export_options;
	openems_hid.do_export = openems_do_export;
	openems_hid.parse_arguments = openems_parse_arguments;
	openems_hid.set_layer_group = openems_set_layer_group;
	openems_hid.make_gc = openems_make_gc;
	openems_hid.destroy_gc = openems_destroy_gc;
	openems_hid.set_drawing_mode = openems_set_drawing_mode;
	openems_hid.set_color = openems_set_color;
	openems_hid.set_line_cap = openems_set_line_cap;
	openems_hid.set_line_width = openems_set_line_width;
	openems_hid.set_draw_xor = openems_set_draw_xor;
	openems_hid.draw_line = openems_draw_line;
	openems_hid.draw_arc = openems_draw_arc;
	openems_hid.draw_rect = openems_draw_rect;
	openems_hid.fill_circle = openems_fill_circle;
	openems_hid.fill_polygon = openems_fill_polygon;
	openems_hid.fill_polygon_offs = openems_fill_polygon_offs;
	openems_hid.fill_rect = openems_fill_rect;
	openems_hid.calibrate = openems_calibrate;
	openems_hid.set_crosshair = openems_set_crosshair;

	openems_hid.usage = openems_usage;

	pcb_hid_register_hid(&openems_hid);

	return 0;
}
