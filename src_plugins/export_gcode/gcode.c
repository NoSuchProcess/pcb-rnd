/*
 *                            COPYRIGHT
 *
 *  pcb-rnd, interactive printed circuit board design
 *  Copyright (C) 2019 Tibor 'Igor2' Palinkas
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
#include "plugins.h"
#include "actions.h"
#include "board.h"
#include "safe_fs.h"
#include "funchash_core.h"
#include "layer.h"

#include "hid.h"
#include "hid_nogui.h"

#include "hid_init.h"
#include "hid_attrib.h"
#include "hid_cam.h"

#include "../src_plugins/millpath/toolpath.h"

const char *pcb_export_gcode_cookie = "export_gcode plugin";

static pcb_hid_t gcode_hid;

typedef struct {
	pcb_cam_t cam;
	pcb_board_t *pcb;
	FILE *f; /* output file */
	int passes; /* number of millings for a thru-cut - calculated for the header */
	pcb_layergrp_t *grp; /* layer group being exported */
	long drawn_objs;
} gcode_t;

static gcode_t gctx;

static const char def_layer_script[] = "setup_negative; trace_contour; fix_overcuts";
static const char def_mech_script[]  = "setup_positive; trace_contour; fix_overcuts";

pcb_export_opt_t gcode_attribute_list[] = {
	{"outfile", "file name prefix for non-cam",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_outfile 0

	{"dias", "tool diameters",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_template 1

	{"layer-script", "rendering script for layer graphics",
	 PCB_HATT_STRING, 0, 0, {0, def_layer_script, 0}, 0, 0},
#define HA_layer_script 2

	{"mech-script", "rendering script for boundary/mech/drill",
	 PCB_HATT_STRING, 0, 0, {0, def_mech_script, 0}, 0, 0},
#define HA_mech_script 3

	{"mill-depth", "Milling depth on layers",
	 PCB_HATT_COORD, PCB_MM_TO_COORD(-10), PCB_MM_TO_COORD(100), {0, 0, 0, PCB_MM_TO_COORD(-0.05)}, 0},
#define HA_layerdepth 4

	{"total-cut-depth", "Total milling depth when cutting through the board (if 0, use pcb thickness)",
	 PCB_HATT_COORD, PCB_MM_TO_COORD(-10), PCB_MM_TO_COORD(100), {0, 0, 0, PCB_MM_TO_COORD(-1.6)}, 0},
#define HA_totalcutdepth 5

	{"cut-depth", "Milling depth increment in each pass",
	 PCB_HATT_COORD, PCB_MM_TO_COORD(-10), PCB_MM_TO_COORD(100), {0, 0, 0, PCB_MM_TO_COORD(0.5)}, 0},
#define HA_cutdepth 6

	{"safe-Z", "Safe Z (above the board) for traverse move",
	 PCB_HATT_COORD, PCB_MM_TO_COORD(-10), PCB_MM_TO_COORD(100), {0, 0, 0, PCB_MM_TO_COORD(0.5)}, 0},
#define HA_safeZ 7

	{"cam", "CAM instruction",
	 PCB_HATT_STRING, 0, 0, {0, 0, 0}, 0, 0},
#define HA_cam 8

};

#define NUM_OPTIONS (sizeof(gcode_attribute_list)/sizeof(gcode_attribute_list[0]))

static pcb_hid_attr_val_t gcode_values[NUM_OPTIONS];

static pcb_export_opt_t *gcode_get_export_options(pcb_hid_t *hid, int *n)
{
	if (n)
		*n = NUM_OPTIONS;
	return gcode_attribute_list;
}

#define TX(x) (x)
#define TY_MIRROR(y) (gctx.pcb->hidlib.size_y - (y))
#define TY_NORMAL(y) (y)
#define TY(y) (gctx.grp->ltype & PCB_LYT_BOTTOM ? TY_MIRROR(y) : TY_NORMAL(y))

static void gcode_print_lines_(pcb_line_t *from, pcb_line_t *to, int passes, int depth)
{
	pcb_line_t *l;

	gctx.drawn_objs++;
	pcb_fprintf(gctx.f, "G0 Z#100\nG0 X%mm Y%mm\n", TX(from->Point1.X), TY(from->Point1.Y));

	if (passes > 1)
		fprintf(gctx.f, "(new path)\n");
	for(;;) {
		/* mill from..to (forward) */
		fprintf(gctx.f, "G0 Z#%d\n", depth++);
		for(l = from; l != to; l = l->link.next)
			pcb_fprintf(gctx.f, "G1 X%mm Y%mm\n", TX(l->Point2.X), TY(l->Point2.Y));
		pcb_fprintf(gctx.f, "G1 X%mm Y%mm\n", TX(to->Point2.X), TY(to->Point2.Y));
		if (--passes <= 0)
			break;

		/* if we are back where we normally start, just go on to the next loop... */
		if ((to->Point2.X == from->Point1.X) && (to->Point2.Y == from->Point1.Y)) {
			fprintf(gctx.f, "(continue-loop one step deeper)\n");
			continue;
		}

		/* ... else turn back to do it backward */
		fprintf(gctx.f, "(back-loop one step deeper)\n");
		/* mill to..from (backward) */
		fprintf(gctx.f, "G0 Z#%d\n", depth++);
		for(l = to; l != from; l = l->link.prev)
			pcb_fprintf(gctx.f, "G1 X%mm Y%mm\n", TX(l->Point1.X), TY(l->Point1.Y));
		pcb_fprintf(gctx.f, "G1 X%mm Y%mm\n", TX(from->Point1.X), TY(from->Point1.Y));
		if (--passes <= 0)
			break;
	}

}


#define thru_start_depth 102

static void gcode_print_header(void)
{
	pcb_coord_t step = gcode_values[HA_cutdepth].crd;
	pcb_coord_t total = gcode_values[HA_totalcutdepth].crd;
	pcb_coord_t at = gcode_values[HA_layerdepth].crd;

	pcb_fprintf(gctx.f, "#100=%mm  (safe Z for travels above the board)\n", gcode_values[HA_safeZ].crd);
	pcb_fprintf(gctx.f, "#101=%mm  (cutting depth for layers)\n", gcode_values[HA_layerdepth].crd);


	if (step > 0)
		step = -step;
	else if (step == 0) {
		pcb_message(PCB_MSG_ERROR, "export_gcode: cut increment not configured - not exporting thru-cut layer\n");
		return;
	}


	if (total == 0) {
		total = pcb_board_thickness(gctx.pcb, "gcode", PCB_BRDTHICK_PRINT_ERROR);
		if (total == 0) {
			pcb_message(PCB_MSG_ERROR, "export_gcode: can't determine board thickness - not exporting thru-cut layer\n");
			return;
		}
	}

	for(gctx.passes = 0, at += step; at > total; gctx.passes++, at += step)
		pcb_fprintf(gctx.f, "#%d=%mm  (%s cutting depth for thru-cuts)\n", thru_start_depth+gctx.passes, at, gctx.passes == 0 ? "first" : "next");
	pcb_fprintf(gctx.f, "#%d=%mm  (last cutting depth for thru-cuts)\n", thru_start_depth+gctx.passes, total);
	gctx.passes++;

	pcb_fprintf(gctx.f,
		"G17 " /* X-Y plane */
		"G21 " /* mm */
		"G90 " /* absolute coords */
		"G64 " /* best speed path */
		"M03 " /* spindle on, CW */
		"S3000 " /* spindle speed */
		"M07 " /* mist coolant on */
		"F1 " /* feed rate */
		"\n");
}

static void gcode_print_footer(void)
{
	pcb_fprintf(gctx.f,
		"G0 Z#100\n" /* remove the tool from the board, just in case */
		"M05 " /* stop spindle */
		"M09 " /* coolant off */
		"M02\n" /* end */
	);
}


static void gcode_print_lines(pcb_tlp_session_t *tctx, pcb_layergrp_t *grp, int thru)
{
	pcb_line_t *from = NULL, *to, *last_to = NULL;
	gdl_iterator_t it;
	pcb_coord_t lastx = PCB_MAX_COORD, lasty = PCB_MAX_COORD;
	int start_depth, passes;

	if (tctx->res_path->Line.lst.length == 0) {
		pcb_fprintf(gctx.f, "(empty layer group: %s)\n", grp->name);
		return;
	}

	if (thru) {
		passes = gctx.passes;
		start_depth = thru_start_depth;
	}
	else {
		passes = 1;
		start_depth = 101;
	}

	from = linelist_first(&tctx->res_path->Line);
	lastx = TX(from->Point2.X);
	lasty = TY(from->Point2.Y);
	linelist_foreach(&tctx->res_path->Line, &it, to) {
		pcb_coord_t x1 = TX(to->Point1.X), y1 = TY(to->Point1.Y), x2 = TX(to->Point2.X), y2 = TY(to->Point2.Y);
		if ((lastx != x1) && (lasty != y1)) {
			if (to->link.prev == NULL)
				gcode_print_lines_(from, from, passes, start_depth); /* corner case: first line is a stand-alone segment */
			else
				gcode_print_lines_(from, to->link.prev, passes, start_depth);
			from = to;
		}
		lastx = x2;
		lasty = y2;
		last_to = to;
	}
	gcode_print_lines_(from, last_to, passes, start_depth);

}

static int gcode_export_layer_group(pcb_layergrp_id_t group, const char *purpose, int purpi, pcb_layer_id_t layer, unsigned int flags, pcb_xform_t **xform)
{
	int script_ha, thru;
	const char *script;
	pcb_layergrp_t *grp = &gctx.pcb->LayerGroups.grp[group];
	static pcb_tlp_session_t tctx;
	static pcb_coord_t tool_dias[] = {
		PCB_MM_TO_COORD(0.2),
		PCB_MM_TO_COORD(3)
	};
	static pcb_tlp_tools_t tools = { sizeof(tool_dias)/sizeof(tool_dias[0]), tool_dias};


	if (flags & PCB_LYT_UI)
		return 0;

	pcb_cam_set_layer_group(&gctx.cam, group, purpose, purpi, flags, xform);

	if (!gctx.cam.active) {
		/* in direct export do only mechanical and copper layer groups */
		if (!(flags & PCB_LYT_COPPER) && !(flags & PCB_LYT_BOUNDARY) && !(flags & PCB_LYT_MECH))
			return 0;
	}

	if (!gctx.cam.active) {
		const char *base = gcode_values[HA_outfile].str;
		gds_t fn;

		if (base == NULL) base = gctx.pcb->hidlib.filename;
		if (base == NULL) base = "unknown";

		gds_init(&fn);
		gds_append_str(&fn, base);
		gds_append(&fn, '.');
		pcb_layer_to_file_name_append(&fn, layer, flags, purpose, purpi, PCB_FNS_pcb_rnd);
		gds_append_str(&fn, ".cnc");

		gctx.f = pcb_fopen_askovr(&gctx.pcb->hidlib, fn.array, "w", NULL);
		if (gctx.f != NULL)
			gcode_print_header();

		gds_uninit(&fn);
	}

	if (gctx.f == NULL)
		return 0;

	if (PCB_LAYER_IS_ROUTE(flags, purpi) || PCB_LAYER_IS_DRILL(flags, purpi)) {
		script_ha = HA_mech_script;
		script = def_mech_script;
		thru = 1;
	}
	else {
		script_ha = HA_layer_script;
		script = def_layer_script;
		thru = 0;
	}

	if (gcode_values[script_ha].str != NULL)
		script = gcode_values[script_ha].str;

	memset(&tctx, 0, sizeof(tctx));
	tctx.edge_clearance = PCB_MM_TO_COORD(0.05);
	tctx.tools = &tools;
	pcb_tlp_mill_script(gctx.pcb, &tctx, grp, script);

	gcode_print_lines(&tctx, grp, thru);

	if (!gctx.cam.active) {
		gcode_print_footer();
		fclose(gctx.f);
	}
	return 0;
}

static void gcode_do_export(pcb_hid_t *hid, pcb_hid_attr_val_t *options)
{
	int i;
	pcb_layergrp_id_t gid;
	pcb_xform_t xform;
	pcb_board_t *pcb = PCB;

	if (!options) {
		gcode_get_export_options(hid, 0);
		for (i = 0; i < NUM_OPTIONS; i++)
			gcode_values[i] = gcode_attribute_list[i].default_val;
		options = gcode_values;
	}

	gctx.pcb = pcb;
	gctx.f = NULL;
	gctx.drawn_objs = 0;
	pcb_cam_begin(pcb, &gctx.cam, &xform, options[HA_cam].str, gcode_attribute_list, NUM_OPTIONS, options);


	if (gctx.cam.active) {
		gctx.f = pcb_fopen_askovr(&pcb->hidlib, gctx.cam.fn, "w", NULL);
		if (gctx.f != NULL)
			gcode_print_header();
	}

	if (!gctx.cam.active || (gctx.f != NULL)) {
		for(gid = 0; gid < pcb->LayerGroups.len; gid++) {
			pcb_layergrp_t *grp = &pcb->LayerGroups.grp[gid];
			pcb_xform_t *xf = &xform;
			gctx.grp = grp;
			gcode_export_layer_group(gid, grp->purpose, grp->purpi, grp->lid[0], grp->ltype, &xf);
			gctx.grp = NULL;
		}

		if (gctx.cam.active) {
			gcode_print_footer();
			fclose(gctx.f);
		}
	}

	if (pcb_cam_end(&gctx.cam) == 0)
		if (!gctx.cam.okempty_group)
			pcb_message(PCB_MSG_ERROR, "gcode cam export for '%s' failed to produce any content (layer group missing)\n", options[HA_cam].str);
	else if (gctx.drawn_objs == 0) {
		if (!gctx.cam.okempty_content)
			pcb_message(PCB_MSG_ERROR, "gcode cam export for '%s' failed to produce any content (no objects)\n", options[HA_cam].str);
	}
}

static int gcode_parse_arguments(pcb_hid_t *hid, int *argc, char ***argv)
{
	pcb_export_register_opts(gcode_attribute_list, sizeof(gcode_attribute_list) / sizeof(gcode_attribute_list[0]), pcb_export_gcode_cookie, 0);
	return pcb_hid_parse_command_line(argc, argv);
}


static int gcode_usage(pcb_hid_t *hid, const char *topic)
{
	fprintf(stderr, "\ngcode exporter command line arguments:\n\n");
	pcb_hid_usage(gcode_attribute_list, sizeof(gcode_attribute_list) / sizeof(gcode_attribute_list[0]));
	fprintf(stderr, "\nUsage: pcb-rnd [generic_options] -x gcode [gcode options] foo.pcb\n\n");
	return 0;
}



int pplg_check_ver_export_gcode(int ver_needed) { return 0; }

void pplg_uninit_export_gcode(void)
{
}


int pplg_init_export_gcode(void)
{
	PCB_API_CHK_VER;

	memset(&gcode_hid, 0, sizeof(pcb_hid_t));

	pcb_hid_nogui_init(&gcode_hid);

	gcode_hid.struct_size = sizeof(pcb_hid_t);
	gcode_hid.name = "gcode";
	gcode_hid.description = "router g-code for removing copper, drilling and routing board outline";
	gcode_hid.exporter = 1;

	gcode_hid.get_export_options = gcode_get_export_options;
	gcode_hid.do_export = gcode_do_export;
	gcode_hid.parse_arguments = gcode_parse_arguments;

	gcode_hid.usage = gcode_usage;

	pcb_hid_register_hid(&gcode_hid);


	return 0;
}
