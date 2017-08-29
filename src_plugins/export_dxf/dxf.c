 /*
  *                            COPYRIGHT
  *
  *  PCB, interactive printed circuit board design
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
  *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
  *
  */

#include "config.h"
#include "conf_core.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "math_helper.h"
#include "board.h"
#include "data.h"
#include "error.h"
#include "layer.h"
#include "plugins.h"

#include "hid.h"
#include "hid_nogui.h"
#include "hid_draw_helpers.h"

#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_color.h"
#include "hid_helper.h"
#include "hid_flags.h"


static pcb_hid_t dxf_hid;

const char *dxf_cookie = "dxf HID";

typedef struct {
	FILE *f;
	unsigned long handle;
	lht_doc_t *temp;
} dxf_ctx_t;

#include "dxf_draw.c"

dxf_ctx_t dxf_ctx;

typedef struct hid_gc_s {
	pcb_hid_t *me_pointer;
	pcb_cap_style_t cap;
	int width;
	char *color;
	int drill;
	unsigned warned_elliptical:1;
} hid_gc_s;


pcb_hid_attribute_t dxf_attribute_list[] = {
	/* other HIDs expect this to be first.  */

/* %start-doc options "93 DXF Options"
@ftable @code
@item --outfile <string>
Name of the file to be exported to. Can contain a path.
@end ftable
%end-doc
*/
	{"outfile", "Graphics output file",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_dxffile 0
};

#define NUM_OPTIONS (sizeof(dxf_attribute_list)/sizeof(dxf_attribute_list[0]))

#define TRX(x)
#define TRY(y) \
do { \
	y = PCB->MaxHeight - y; \
} while(0)


PCB_REGISTER_ATTRIBUTES(dxf_attribute_list, dxf_cookie)

static pcb_hid_attr_val_t dxf_values[NUM_OPTIONS];

static pcb_hid_attribute_t *dxf_get_export_options(int *n)
{
	static char *last_made_filename = 0;
	const char *suffix = ".dxf";

	if (PCB)
		pcb_derive_default_filename(PCB->Filename, &dxf_attribute_list[HA_dxffile], suffix, &last_made_filename);

	if (n)
		*n = NUM_OPTIONS;
	return dxf_attribute_list;
}

void dxf_hid_export_to_file(dxf_ctx_t *ctx, pcb_hid_attr_val_t * options)
{
	static int saved_layer_stack[PCB_MAX_LAYER];
	pcb_hid_expose_ctx_t hectx;

	hectx.view.X1 = 0;
	hectx.view.Y1 = 0;
	hectx.view.X2 = PCB->MaxWidth;
	hectx.view.Y2 = PCB->MaxHeight;

	memcpy(saved_layer_stack, pcb_layer_stack, sizeof(pcb_layer_stack));

	conf_force_set_bool(conf_core.editor.thin_draw, 0);
	conf_force_set_bool(conf_core.editor.thin_draw_poly, 0);
/*		conf_force_set_bool(conf_core.editor.check_planes, 0);*/
	conf_force_set_bool(conf_core.editor.show_solder_side, 0);

	pcb_hid_expose_all(&dxf_hid, &hectx);

	conf_update(NULL, -1); /* restore forced sets */
}

int insert_hdr(FILE *f, const char *prefix, char *name, lht_err_t *err)
{
	if (strcmp(name, "extmin") == 0)
		fprintf(f, "10\n0\n20\n0\n30\n0\n");
	else if (strcmp(name, "extmax") == 0)
		pcb_fprintf(f, "10\n%mm\n20\n0\n30\n%mm\n", PCB->MaxWidth, PCB->MaxHeight);
	else {
		pcb_message(PCB_MSG_ERROR, "Invalid header insertion: '%s'\n", name);
		return -1;
	}

	return 0;
}

int insert_ftr(FILE *f, const char *prefix, char *name, lht_err_t *err)
{
	pcb_message(PCB_MSG_ERROR, "Invalid footer insertion: '%s'\n", name);
	return -1;
}


static void dxf_do_export(pcb_hid_attr_val_t * options)
{
	const char *filename;
	int save_ons[PCB_MAX_LAYER + 2];
	int i;
	const char *fn = "dxf_templ.lht";
	char *errmsg;
	lht_err_t err;

	if (!options) {
		dxf_get_export_options(0);
		for (i = 0; i < NUM_OPTIONS; i++)
			dxf_values[i] = dxf_attribute_list[i].default_val;
		options = dxf_values;
	}

	filename = options[HA_dxffile].str_value;
	if (!filename)
		filename = "pcb.dxf";

	dxf_ctx.f = fopen(filename, "wb");
	if (!dxf_ctx.f) {
		perror(filename);
		return;
	}

	dxf_ctx.temp = lht_dom_load(fn, &errmsg);
	if (dxf_ctx.temp == NULL) {
		pcb_message(PCB_MSG_ERROR, "Can't open dxf template: %s\n", fn);
		fclose(dxf_ctx.f);
		return;
	}

	dxf_ctx.handle = 100;
	if (lht_temp_exec(dxf_ctx.f, "", dxf_ctx.temp, "header", insert_hdr, &err) != 0)
		pcb_message(PCB_MSG_ERROR, "Can't render dxf template header\n");

	pcb_hid_save_and_show_layer_ons(save_ons);

	dxf_hid_export_to_file(&dxf_ctx, options);

	pcb_hid_restore_layer_ons(save_ons);

	if (lht_temp_exec(dxf_ctx.f, "", dxf_ctx.temp, "footer", insert_ftr, &err) != 0)
		pcb_message(PCB_MSG_ERROR, "Can't render dxf template header\n");

	fclose(dxf_ctx.f);
}

static void dxf_parse_arguments(int *argc, char ***argv)
{
	pcb_hid_register_attributes(dxf_attribute_list, sizeof(dxf_attribute_list) / sizeof(dxf_attribute_list[0]), dxf_cookie, 0);
	pcb_hid_parse_command_line(argc, argv);
}

static int dxf_set_layer_group(pcb_layergrp_id_t group, pcb_layer_id_t layer, unsigned int flags, int is_empty)
{
	if (flags & PCB_LYT_UI)
		return 0;

	if (flags & PCB_LYT_INVIS)
		return 0;

	if (flags & PCB_LYT_OUTLINE)
		return 1;

	if (flags & PCB_LYT_PDRILL)
		return 1;

	if (flags & PCB_LYT_UDRILL)
		return 1;

	return 0;
}


static pcb_hid_gc_t dxf_make_gc(void)
{
	pcb_hid_gc_t rv = (pcb_hid_gc_t) calloc(sizeof(hid_gc_s), 1);
	rv->me_pointer = &dxf_hid;
	return rv;
}

static void dxf_destroy_gc(pcb_hid_gc_t gc)
{
	free(gc);
}

static void dxf_use_mask(pcb_mask_op_t use_it)
{
}

static void dxf_set_drawing_mode(pcb_composite_op_t op, pcb_bool direct, const pcb_box_t *screen)
{
	if (direct)
		return;

	switch(op) {
		case PCB_HID_COMP_RESET:
			break;

		case PCB_HID_COMP_POSITIVE:
		case PCB_HID_COMP_NEGATIVE:
			break;

		case PCB_HID_COMP_FLUSH:
			break;
	}
}

static void dxf_set_color(pcb_hid_gc_t gc, const char *name)
{
}

static void dxf_set_line_cap(pcb_hid_gc_t gc, pcb_cap_style_t style)
{
	gc->cap = style;
}

static void dxf_set_line_width(pcb_hid_gc_t gc, pcb_coord_t width)
{
	gc->width = width;
}


static void dxf_set_draw_xor(pcb_hid_gc_t gc, int xor_)
{
	;
}

#define fix_rect_coords() \
	if (x1 > x2) {\
		pcb_coord_t t = x1; \
		x1 = x2; \
		x2 = t; \
	} \
	if (y1 > y2) { \
		pcb_coord_t t = y1; \
		y1 = y2; \
		y2 = t; \
	}

static void dxf_draw_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	TRX(x1); TRY(y1); TRX(x2); TRY(y2);
	fix_rect_coords();
}

static void dxf_fill_rect(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	TRX(x1); TRY(y1); TRX(x2); TRY(y2);
	fix_rect_coords();
}

static void dxf_draw_line(pcb_hid_gc_t gc, pcb_coord_t x1, pcb_coord_t y1, pcb_coord_t x2, pcb_coord_t y2)
{
	TRX(x1); TRY(y1); TRX(x2); TRY(y2);
}

static void dxf_draw_arc(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t width, pcb_coord_t height, pcb_angle_t start_angle, pcb_angle_t delta_angle)
{
	TRX(cx); TRY(cy);
}

static void dxf_fill_circle(pcb_hid_gc_t gc, pcb_coord_t cx, pcb_coord_t cy, pcb_coord_t radius)
{
	TRX(cx); TRY(cy);
}

static void dxf_fill_polygon(pcb_hid_gc_t gc, int n_coords, pcb_coord_t * x, pcb_coord_t * y)
{
}

static void dxf_calibrate(double xval, double yval)
{
	pcb_message(PCB_MSG_ERROR, "dxf_calibrate() not implemented");
	return;
}

static void dxf_set_crosshair(int x, int y, int a)
{
}

static int dxf_usage(const char *topic)
{
	fprintf(stderr, "\ndxf exporter command line arguments:\n\n");
	pcb_hid_usage(dxf_attribute_list, sizeof(dxf_attribute_list) / sizeof(dxf_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x dxf [dxf options] foo.pcb\n\n");
	return 0;
}

#include "dolists.h"

int pplg_check_ver_export_dxf(int ver_needed) { return 0; }

void pplg_uninit_export_dxf(void)
{
	pcb_hid_remove_attributes_by_cookie(dxf_cookie);
}

int pplg_init_export_dxf(void)
{
	memset(&dxf_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&dxf_hid);
	pcb_dhlp_draw_helpers_init(&dxf_hid);

	dxf_hid.struct_size = sizeof(pcb_hid_t);
	dxf_hid.name = "dxf";
	dxf_hid.description = "Drawing eXchange Format exporter";
	dxf_hid.exporter = 1;
	dxf_hid.holes_after = 1;

	dxf_hid.get_export_options = dxf_get_export_options;
	dxf_hid.do_export = dxf_do_export;
	dxf_hid.parse_arguments = dxf_parse_arguments;
	dxf_hid.set_layer_group = dxf_set_layer_group;
	dxf_hid.make_gc = dxf_make_gc;
	dxf_hid.destroy_gc = dxf_destroy_gc;
	dxf_hid.use_mask = dxf_use_mask;
	dxf_hid.set_drawing_mode = dxf_set_drawing_mode;
	dxf_hid.set_color = dxf_set_color;
	dxf_hid.set_line_cap = dxf_set_line_cap;
	dxf_hid.set_line_width = dxf_set_line_width;
	dxf_hid.set_draw_xor = dxf_set_draw_xor;
	dxf_hid.draw_line = dxf_draw_line;
	dxf_hid.draw_arc = dxf_draw_arc;
	dxf_hid.draw_rect = dxf_draw_rect;
	dxf_hid.fill_circle = dxf_fill_circle;
	dxf_hid.fill_polygon = dxf_fill_polygon;
	dxf_hid.fill_rect = dxf_fill_rect;
	dxf_hid.calibrate = dxf_calibrate;
	dxf_hid.set_crosshair = dxf_set_crosshair;

	dxf_hid.usage = dxf_usage;

	pcb_hid_register_hid(&dxf_hid);

	return 0;
}
